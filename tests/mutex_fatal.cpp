// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// Unlocking an unlocked mutex is a fatal error: the process exits with status 2 at once, and the
// catch handler below, which stands in for a deferred recover, never runs.
#include <eo/sync.h>

#include <cstdio>
#include <string_view>

int main(int argc, char** argv) {
  if (argc != 2) {
    return 1;
  }
  auto which = std::string_view{argv[1]};
  try {
    if (which == "mutex") {
      eo::sync::Mutex mu;
      mu.Unlock();
    } else if (which == "rwmutex") {
      eo::sync::RWMutex rw;
      rw.Unlock();
    } else if (which == "rwmutex_reader") {
      eo::sync::RWMutex rw;
      rw.RUnlock();
    } else if (which == "rwmutex_writer_while_reader") {
      eo::sync::RWMutex rw;
      if (rw.TryRLock()) {
        rw.Unlock();
      }
    } else if (which == "rwmutex_reader_while_writer") {
      eo::sync::RWMutex rw;
      if (rw.TryLock()) {
        rw.RUnlock();
      }
    }
  } catch (...) {
    std::puts("caught");
  }
  std::puts("returned");
  return 0;
}
