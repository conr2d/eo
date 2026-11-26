#include <eo/fmt.h>
#include <eo/sync.h>
#include <eo/sync/atomic.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct node;
struct stats;

struct node {
  static constexpr std::string_view _eo_name = "main.node";

  std::string name{};

  auto operator==(const node&) const -> bool = default;
};

struct stats {
  static constexpr std::string_view _eo_name = "main.stats";

  atomic::Int64 hits{};
  atomic::Bool ready{};
  atomic::Pointer<node> head{};
};

void attempt(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println(name, "panic:"s, r);
      }
    });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

void typed() {
  atomic::Int32 i32{};
  atomic::Uint32 u32{};
  atomic::Uint64 u64{};
  atomic::Uintptr up{};
  i32.Store(2147483647);
  auto _eo_call_0 = i32.Add(1);
  auto _eo_call_1 = i32.Load();
  fmt::Println(_eo_call_0, _eo_call_1);
  auto _eo_call_2 = i32.Swap(5);
  auto _eo_call_3 = i32.CompareAndSwap(4, 9);
  auto _eo_call_4 = i32.CompareAndSwap(5, 9);
  auto _eo_call_5 = i32.Load();
  fmt::Println(_eo_call_2, _eo_call_3, _eo_call_4, _eo_call_5);
  auto _eo_call_6 = i32.And(12);
  auto _eo_call_7 = i32.Or(6);
  auto _eo_call_8 = i32.Load();
  fmt::Println(_eo_call_6, _eo_call_7, _eo_call_8);
  auto _eo_call_9 = u32.Add(uint32_t{4294967295u});
  auto _eo_call_10 = u64.Add(uint64_t{9223372036854775808u});
  auto _eo_call_11 = u64.Add(uint64_t{9223372036854775808u});
  auto _eo_call_12 = up.Add(7);
  fmt::Println(_eo_call_9, _eo_call_10, _eo_call_11, _eo_call_12);
  auto _eo_call_13 = u32.Or(0xf0);
  auto _eo_call_14 = u32.And(0x3c);
  auto _eo_call_15 = u32.Load();
  auto _eo_call_16 = u64.Swap(3);
  auto _eo_call_17 = up.CompareAndSwap(7, 8);
  auto _eo_call_18 = up.Load();
  fmt::Println(_eo_call_13, _eo_call_14, _eo_call_15, _eo_call_16, _eo_call_17, _eo_call_18);

  atomic::Bool b{};
  auto _eo_call_19 = b.Load();
  auto _eo_call_20 = b.Swap(true);
  auto _eo_call_21 = b.CompareAndSwap(false, true);
  auto _eo_call_22 = b.CompareAndSwap(true, false);
  auto _eo_call_23 = b.Load();
  fmt::Println(_eo_call_19, _eo_call_20, _eo_call_21, _eo_call_22, _eo_call_23);

  auto p = New<atomic::Pointer<node>>();
  auto a = New<node>(node{"a"s});
  auto c = New<node>(node{"c"s});
  auto _eo_call_24 = p->Load();
  auto _eo_call_25 = p->CompareAndSwap(nullptr, a);
  auto _eo_call_26 = p->Load();
  fmt::Println(_eo_call_24 == nullptr, _eo_call_25, _eo_call_26->name);
  auto _eo_call_27 = p->CompareAndSwap(New<node>(node{"a"s}), c);
  auto _eo_call_28 = p->Swap(c);
  auto _eo_call_29 = p->Load();
  fmt::Println(_eo_call_27, _eo_call_28->name, _eo_call_29->name);
  p->Store(nullptr);
  fmt::Println(p->Load() == nullptr);
}

void functions() {
  int64_t n{};
  uint32_t m{};
  auto _eo_call_0 = atomic::AddInt64(ref(n), 5);
  auto _eo_call_1 = atomic::AddInt64(ref(n), -7);
  auto _eo_call_2 = atomic::LoadInt64(ref(n));
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
  atomic::StoreInt64(ref(n), 40);
  auto _eo_call_3 = atomic::SwapInt64(ref(n), 41);
  auto _eo_call_4 = atomic::CompareAndSwapInt64(ref(n), 41, 42);
  fmt::Println(_eo_call_3, _eo_call_4, n);
  auto _eo_call_5 = atomic::AddUint32(ref(m), uint32_t{4294967295u});
  auto _eo_call_6 = atomic::OrUint32(ref(m), 1);
  auto _eo_call_7 = atomic::AndUint32(ref(m), 3);
  fmt::Println(_eo_call_5, _eo_call_6, _eo_call_7, m);
  int32_t i32{};
  uint64_t u64{};
  uintptr_t up{};
  auto _eo_call_8 = atomic::AddInt32(ref(i32), -3);
  auto _eo_call_9 = atomic::OrInt32(ref(i32), 8);
  auto _eo_call_10 = atomic::AndInt32(ref(i32), 14);
  auto _eo_call_11 = atomic::SwapInt32(ref(i32), 1);
  fmt::Println(_eo_call_8, _eo_call_9, _eo_call_10, _eo_call_11);
  auto _eo_call_12 = atomic::CompareAndSwapInt32(ref(i32), 2, 3);
  auto _eo_call_13 = atomic::CompareAndSwapInt32(ref(i32), 1, 3);
  auto _eo_call_14 = atomic::LoadInt32(ref(i32));
  fmt::Println(_eo_call_12, _eo_call_13, _eo_call_14);
  atomic::StoreUint64(ref(u64), uint64_t{9223372036854775808u});
  auto _eo_call_15 = atomic::AddUint64(ref(u64), uint64_t{9223372036854775808u});
  auto _eo_call_16 = atomic::OrUint64(ref(u64), 6);
  auto _eo_call_17 = atomic::AndUint64(ref(u64), 3);
  auto _eo_call_18 = atomic::SwapUint64(ref(u64), 9);
  fmt::Println(_eo_call_15, _eo_call_16, _eo_call_17, _eo_call_18);
  auto _eo_call_19 = atomic::CompareAndSwapUint64(ref(u64), 9, 10);
  auto _eo_call_20 = atomic::LoadUint64(ref(u64));
  auto _eo_call_21 = atomic::AndInt64(ref(n), 3);
  auto _eo_call_22 = atomic::OrInt64(ref(n), 4);
  fmt::Println(_eo_call_19, _eo_call_20, _eo_call_21, _eo_call_22, n);
  atomic::StoreUintptr(ref(up), 5);
  auto _eo_call_23 = atomic::AddUintptr(ref(up), 2);
  auto _eo_call_24 = atomic::SwapUintptr(ref(up), 3);
  auto _eo_call_25 = atomic::CompareAndSwapUintptr(ref(up), 3, 4);
  auto _eo_call_26 = atomic::LoadUintptr(ref(up));
  fmt::Println(_eo_call_23, _eo_call_24, _eo_call_25, _eo_call_26);
  auto _eo_call_27 = atomic::OrUintptr(ref(up), 1);
  auto _eo_call_28 = atomic::AndUintptr(ref(up), 1);
  auto _eo_call_29 = atomic::LoadUint32(ref(m));
  auto _eo_call_30 = atomic::SwapUint32(ref(m), 7);
  auto _eo_call_31 = atomic::CompareAndSwapUint32(ref(m), 7, 8);
  fmt::Println(_eo_call_27, _eo_call_28, _eo_call_29, _eo_call_30, _eo_call_31, m);
  atomic::StoreInt32(ref(i32), 6);
  atomic::StoreUint32(ref(m), 6);
  fmt::Println(i32, m);
  ptr<int32_t> none{};
  attempt("AddInt32"s, [=]() mutable { atomic::AddInt32(none, 1); });
}

void values() {
  auto v = New<atomic::Value>();
  fmt::Println(v->Load() == nullptr);
  attempt("Store nil"s, [=]() mutable { v->Store(nullptr); });
  auto _eo_call_0 = v->CompareAndSwap(int64_t{1}, int64_t{2});
  auto _eo_call_1 = v->CompareAndSwap(nullptr, int64_t{1});
  auto _eo_call_2 = v->Load();
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);
  v->Store(int64_t{3});
  attempt("Store string"s, [=]() mutable { v->Store("x"s); });
  attempt("Swap nil"s, [=]() mutable { v->Swap(nullptr); });
  attempt("Swap string"s, [=]() mutable { v->Swap("x"s); });
  auto _eo_call_3 = v->Swap(int64_t{4});
  auto _eo_call_4 = v->Load();
  fmt::Println(_eo_call_3, _eo_call_4);
  auto _eo_call_5 = v->CompareAndSwap(int64_t{3}, int64_t{5});
  auto _eo_call_6 = v->CompareAndSwap(int64_t{4}, int64_t{5});
  auto _eo_call_7 = v->Load();
  fmt::Println(_eo_call_5, _eo_call_6, _eo_call_7);
  attempt("CompareAndSwap nil"s, [=]() mutable { v->CompareAndSwap(int64_t{5}, nullptr); });
  attempt("CompareAndSwap mixed"s, [=]() mutable { v->CompareAndSwap("a"s, int64_t{6}); });
  attempt("CompareAndSwap string"s, [=]() mutable { v->CompareAndSwap(nullptr, "b"s); });

  auto w = New<atomic::Value>();
  w->Store(map<std::string, int64_t>::of({{"a"s, 1}}));
  auto cur = eo::type_assert<map<std::string, int64_t>>(w->Load());
  fmt::Println(eo::map_get(cur, "a"s), eo::len(cur));
  attempt("CompareAndSwap map"s,
    [=]() mutable { w->CompareAndSwap(map<std::string, int64_t>::of({}), map<std::string, int64_t>::of({})); });

  auto s = New<atomic::Value>();
  fmt::Println(s->Swap(slice<int64_t>::of({1})) == nullptr);
  fmt::Println(s->Load());
}

func<> goroutines() {
  auto s = New<stats>();
  auto count = New<int64_t>(0);
  auto wg = New<sync::WaitGroup>();
  for (auto g = int64_t{0}; g < 8; g++) {
    wg->Add(1);
    go([=]() mutable -> func<> {
      eo_defer_scope;
      try {
        auto _eo_defer_receiver_0 = wg;
        eo_defer([=] { _eo_defer_receiver_0->Done(); });
        for (auto i = int64_t{0}; i < 1000; i++) {
          s->hits.Add(1);
          atomic::AddInt64(count, 2);
        }
        s->head.Store(New<node>(node{"done"s}));
        s->ready.Store(true);
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
      co_return;
    });
  }
  co_await wg->Wait();
  auto _eo_call_0 = s->hits.Load();
  auto _eo_call_1 = atomic::LoadInt64(count);
  auto _eo_call_2 = s->ready.Load();
  auto _eo_call_3 = s->head.Load();
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3->name);
}

func<> eo_main() {
  typed();
  functions();
  values();
  co_await goroutines();
  co_return;
}
