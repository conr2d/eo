// Provisional: package-level variables and channel types and operations are not frozen mappings (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

using namespace eo;

sync::RWMutex rw;

func<> reader(chan<std::string> got, chan<bool> done) {
  co_await rw.RLock();
  co_await (got << "second reader in"s);
  rw.RUnlock();
  co_await (done << true);
}

func<> eo_main() {
  auto got = make_chan<std::string>();
  auto done = make_chan<bool>();
  co_await rw.RLock();
  chan<std::string> _eo_go_arg_0_0 = got;
  chan<bool> _eo_go_arg_0_1 = done;
  go([=]() mutable { return reader(_eo_go_arg_0_0, _eo_go_arg_0_1); });
  fmt::Println(co_await *got);
  fmt::Println("TryLock while reading:"s, rw.TryLock());
  co_await *done;
  rw.RUnlock();
  fmt::Println("TryLock after RUnlock:"s, rw.TryLock());
  fmt::Println("TryRLock while writing:"s, rw.TryRLock());
  rw.Unlock();
  fmt::Println("TryRLock after Unlock:"s, rw.TryRLock());
  rw.RUnlock();
}
