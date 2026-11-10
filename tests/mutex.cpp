// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/invoke.h>
#include <eo/sync.h>
#include <eo/time.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>

// Lets a test wait until goroutines are queued in a lock, so that handoff order is deterministic.
struct eo::sync::detail::probe {
  static auto waiters(Mutex& mu) -> std::size_t {
    auto lock = std::lock_guard{mu.mutex};
    return mu.waiters.size();
  }

  static auto writers(RWMutex& rw) -> std::size_t {
    auto lock = std::lock_guard{rw.mutex};
    return rw.writers.size();
  }

  static auto readers(RWMutex& rw) -> std::size_t {
    auto lock = std::lock_guard{rw.mutex};
    return rw.waiting_readers.size();
  }
};

using probe = eo::sync::detail::probe;

template<typename Count>
auto until_queued(Count count, std::size_t n) -> eo::func<> {
  while (count() < n) {
    co_await eo::time::Sleep(std::chrono::microseconds{100});
  }
}

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_try_lock() {
  eo::sync::Mutex mu;
  check(mu.TryLock(), "TryLock failed on an unlocked Mutex");
  check(!mu.TryLock(), "TryLock succeeded on a locked Mutex");
  mu.Unlock();
  check(mu.TryLock(), "TryLock failed after Unlock");
  mu.Unlock();
}

// Every increment happens under the lock, and the goroutines suspend while they hold it, so a lost
// update or a second holder shows up in the count.
void test_mutual_exclusion() {
  auto count = eo::invoke([]() -> eo::func<int> {
    struct state {
      eo::sync::Mutex mu;
      eo::sync::WaitGroup wg;
      int count = 0;
      std::atomic<int> holders = 0;
      std::atomic<bool> overlapped = false;
    };
    auto s = std::make_shared<state>();
    for (auto i = 0; i < 8; i++) {
      s->wg.Add(1);
      eo::go([s]() -> eo::func<> {
        for (auto j = 0; j < 100; j++) {
          co_await s->mu.Lock();
          if (++s->holders != 1) {
            s->overlapped = true;
          }
          auto seen = s->count;
          if (j % 10 == 0) {
            co_await eo::time::Sleep(std::chrono::microseconds{50});
          }
          s->count = seen + 1;
          --s->holders;
          s->mu.Unlock();
        }
        s->wg.Done();
      });
    }
    co_await s->wg.Wait();
    check(!s->overlapped, "two goroutines held the Mutex at once");
    co_return s->count;
  });

  check(count == 800, "Mutex lost an update");
}

// The holder suspends on a channel while it holds the lock, and a second goroutine waits in Lock.
// The waiter must not occupy the worker, or the sender, and then the holder, never run with
// EOMAXPROCS=1 (the eo.mutex_one_worker test). A deadlock fails through the test timeout.
void test_lock_does_not_block_the_worker() {
  auto waited = eo::invoke([]() -> eo::func<bool> {
    auto mu = std::make_shared<eo::sync::Mutex>();
    auto unlocked = std::make_shared<std::atomic<bool>>(false);
    auto ch = eo::make_chan<int>();
    auto done = eo::make_chan<bool>();
    co_await mu->Lock();
    eo::go([mu, unlocked, done]() -> eo::func<> {
      co_await mu->Lock();
      auto after_unlock = unlocked->load();
      mu->Unlock();
      co_await (done << after_unlock);
    });
    eo::go([ch]() -> eo::func<> { co_await (ch << 1); });
    co_await *ch;
    co_await until_queued([mu] { return probe::waiters(*mu); }, 1);
    *unlocked = true;
    mu->Unlock();
    co_return co_await *done;
  });

  check(waited, "Mutex waiter got the lock before the holder unlocked");
}

// Unlock hands the lock to the longest-waiting goroutine: TryLock cannot take it in between, and
// the waiters get it in the order they arrived.
void test_unlock_hands_off_in_order() {
  auto order = eo::invoke([]() -> eo::func<int> {
    auto mu = std::make_shared<eo::sync::Mutex>();
    auto events = eo::make_chan<int>(2);
    co_await mu->Lock();
    for (auto id = 1; id <= 2; id++) {
      eo::go([mu, events, id]() -> eo::func<> {
        co_await mu->Lock();
        co_await (events << id);
        mu->Unlock();
      });
      co_await until_queued([mu] { return probe::waiters(*mu); }, static_cast<std::size_t>(id));
    }
    mu->Unlock();
    check(!mu->TryLock(), "TryLock took a Mutex that Unlock handed to a waiter");
    auto first = co_await *events;
    auto second = co_await *events;
    co_return first * 10 + second;
  });

  check(order == 12, "Mutex waiters got the lock out of order");
}

void test_rwmutex_shares_read_locks() {
  eo::sync::RWMutex rw;
  check(rw.TryRLock(), "TryRLock failed on an unlocked RWMutex");
  check(rw.TryRLock(), "second TryRLock failed while only readers hold the RWMutex");
  check(!rw.TryLock(), "TryLock succeeded while readers hold the RWMutex");
  rw.RUnlock();
  check(!rw.TryLock(), "TryLock succeeded while a reader holds the RWMutex");
  rw.RUnlock();
  check(rw.TryLock(), "TryLock failed after the last RUnlock");
  check(!rw.TryRLock(), "TryRLock succeeded while a writer holds the RWMutex");
  check(!rw.TryLock(), "TryLock succeeded while a writer holds the RWMutex");
  rw.Unlock();
  check(rw.TryRLock(), "TryRLock failed after Unlock");
  rw.RUnlock();
}

// A waiting writer keeps new readers out, and gets the lock when the last reader leaves.
void test_rwmutex_waiting_writer_blocks_readers() {
  auto order = eo::invoke([]() -> eo::func<int> {
    auto rw = std::make_shared<eo::sync::RWMutex>();
    auto events = eo::make_chan<int>(2);
    co_await rw->RLock();
    eo::go([rw, events]() -> eo::func<> {
      co_await rw->Lock();
      co_await (events << 1);
      rw->Unlock();
    });
    co_await until_queued([rw] { return probe::writers(*rw); }, 1);
    check(!rw->TryRLock(), "TryRLock succeeded while a writer was waiting");
    eo::go([rw, events]() -> eo::func<> {
      co_await rw->RLock();
      co_await (events << 2);
      rw->RUnlock();
    });
    co_await until_queued([rw] { return probe::readers(*rw); }, 1);
    rw->RUnlock();
    auto first = co_await *events;
    auto second = co_await *events;
    co_return first * 10 + second;
  });

  check(order == 12, "a reader got the RWMutex ahead of a waiting writer");
}

// When a writer unlocks, the readers waiting at that moment get the lock before the next writer,
// and waiting writers get it in the order they arrived.
void test_rwmutex_unlock_releases_readers_first() {
  auto released = eo::invoke([]() -> eo::func<bool> {
    auto rw = std::make_shared<eo::sync::RWMutex>();
    auto events = eo::make_chan<int>(4);
    co_await rw->Lock();
    for (auto id = 1; id <= 2; id++) {
      eo::go([rw, events, id]() -> eo::func<> {
        co_await rw->Lock();
        co_await (events << id);
        rw->Unlock();
      });
      co_await until_queued([rw] { return probe::writers(*rw); }, static_cast<std::size_t>(id));
    }
    for (auto id = 3; id <= 4; id++) {
      eo::go([rw, events, id]() -> eo::func<> {
        co_await rw->RLock();
        co_await (events << id);
        rw->RUnlock();
      });
      co_await until_queued([rw] { return probe::readers(*rw); }, static_cast<std::size_t>(id - 2));
    }
    rw->Unlock();
    // The two readers may report in either order, but both before the first writer.
    auto a = co_await *events;
    auto b = co_await *events;
    auto c = co_await *events;
    auto d = co_await *events;
    co_return std::min(a, b) == 3 && std::max(a, b) == 4 && c == 1 && d == 2;
  });

  check(released, "RWMutex Unlock did not release the waiting readers before the writers in order");
}

int main() {
  test_try_lock();
  test_mutual_exclusion();
  test_lock_does_not_block_the_worker();
  test_unlock_hands_off_in_order();
  test_rwmutex_shares_read_locks();
  test_rwmutex_waiting_writer_blocks_readers();
  test_rwmutex_unlock_releases_readers_first();
  return 0;
}
