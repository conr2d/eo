#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  auto jobs = eo::make_chan<int64_t>(3);
  co_await (jobs << 1);
  co_await (jobs << 2);
  co_await (jobs << 3);
  jobs.close();

  auto wg = New<sync::WaitGroup>();
  auto results = eo::make_chan<int64_t>(3);
  for (auto _eo_range_chan = jobs;;) {
    auto [j, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        co_await (results << j * 10);
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  results.close();

  auto total = int64_t{0};
  for (auto _eo_range_chan = results;;) {
    auto [r, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    total += r;
  }
  fmt::Println("total"s, total);

  slice<ptr<int64_t>> ptrs;
  auto nums = eo::make_chan<int64_t>(2);
  co_await (nums << 4);
  co_await (nums << 5);
  nums.close();
  for (auto _eo_range_chan = nums;;) {
    auto [_eo_range_key, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    auto n = New<int64_t>(_eo_range_key);
    ptrs = eo::append(ptrs, {n});
  }
  fmt::Println(*eo::at(ptrs, 0), *eo::at(ptrs, 1));
}
