// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/invoke.h>
#include <eo/sync.h>
#include <eo/time.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>

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
// EOMAXPROCS=1 (the eo.mutex_one_worker test).
void test_lock_does_not_block_the_worker() {
  auto order = eo::invoke([]() -> eo::func<int> {
    auto mu = std::make_shared<eo::sync::Mutex>();
    auto ch = eo::make_chan<int>();
    auto done = eo::make_chan<int>();
    co_await mu->Lock();
    eo::go([mu, done]() -> eo::func<> {
      co_await mu->Lock();
      mu->Unlock();
      co_await (done << 2);
    });
    eo::go([ch]() -> eo::func<> { co_await (ch << 1); });
    auto first = co_await *ch;
    mu->Unlock();
    auto second = co_await *done;
    co_return first * 10 + second;
  });

  check(order == 12, "Mutex waiter ran before the holder unlocked");
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

// A waiting writer keeps new readers out, and gets the lock when the last reader leaves. Readers
// that arrived while it waited get the lock when it unlocks.
void test_rwmutex_waiting_writer_blocks_readers() {
  auto order = eo::invoke([]() -> eo::func<int> {
    auto rw = std::make_shared<eo::sync::RWMutex>();
    auto events = eo::make_chan<int>(4);
    co_await rw->RLock();
    eo::go([rw, events]() -> eo::func<> {
      co_await rw->Lock();
      co_await (events << 1);
      rw->Unlock();
    });
    // TryRLock fails only once the writer is waiting.
    while (rw->TryRLock()) {
      rw->RUnlock();
      co_await eo::time::Sleep(std::chrono::microseconds{100});
    }
    eo::go([rw, events]() -> eo::func<> {
      co_await rw->RLock();
      co_await (events << 2);
      rw->RUnlock();
    });
    co_await eo::time::Sleep(std::chrono::milliseconds{5});
    rw->RUnlock();
    auto first = co_await *events;
    auto second = co_await *events;
    co_return first * 10 + second;
  });

  check(order == 12, "a reader got the RWMutex ahead of a waiting writer");
}

int main() {
  test_try_lock();
  test_mutual_exclusion();
  test_lock_does_not_block_the_worker();
  test_rwmutex_shares_read_locks();
  test_rwmutex_waiting_writer_blocks_readers();
  return 0;
}
