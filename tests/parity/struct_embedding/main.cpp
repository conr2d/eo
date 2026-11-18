#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Base;
struct Counter;
struct Item;
struct Wrapper;

struct Base {
  static constexpr std::string_view _eo_name = "main.Base";

  int64_t ID{};
  std::string Name{};

  auto operator==(const Base&) const -> bool = default;

  static auto Describe(Base b) -> std::string;
  static void Rename(ptr<Base> b, std::string name);

  using _eo_methods = eo::methods<eo::method<&Base::Describe, "Describe">, eo::method<&Base::Rename, "Rename">>;
};

struct Counter {
  static constexpr std::string_view _eo_name = "main.Counter";

  int64_t N{};

  auto operator==(const Counter&) const -> bool = default;

  static auto Inc(ptr<Counter> c) -> int64_t;
  static auto Get(Counter c) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Counter::Inc, "Inc">, eo::method<&Counter::Get, "Get">>;
};

// Item embeds a Base by value and a Counter through a pointer. Its own Name hides Base.Name.
struct Item {
  static constexpr std::string_view _eo_name = "main.Item";

  ::Base Base{};
  ptr<::Counter> Counter{};
  std::string Name{};

  auto operator==(const Item&) const -> bool = default;

  static auto Describe(Item _eo_receiver) -> std::string;
  static auto Get(Item _eo_receiver) -> int64_t;
  static auto Inc(Item _eo_receiver) -> int64_t;
  static void Rename(ptr<Item> _eo_receiver, std::string name);

  using _eo_methods = eo::methods<eo::method<&Item::Describe, "Describe">,
    eo::method<&Item::Get, "Get">,
    eo::method<&Item::Inc, "Inc">,
    eo::method<&Item::Rename, "Rename">>;
};

// Wrapper embeds an Item, so Base's fields and methods are promoted two levels.
struct Wrapper {
  static constexpr std::string_view _eo_name = "main.Wrapper";

  ::Item Item{};
  int64_t Extra{};

  auto operator==(const Wrapper&) const -> bool = default;

  static auto Describe(Wrapper _eo_receiver) -> std::string;
  static auto Get(Wrapper _eo_receiver) -> int64_t;
  static auto Inc(Wrapper _eo_receiver) -> int64_t;
  static void Rename(ptr<Wrapper> _eo_receiver, std::string name);

  using _eo_methods = eo::methods<eo::method<&Wrapper::Describe, "Describe">,
    eo::method<&Wrapper::Get, "Get">,
    eo::method<&Wrapper::Inc, "Inc">,
    eo::method<&Wrapper::Rename, "Rename">>;
};

auto Item::Describe(Item _eo_receiver) -> std::string {
  return ::Base::Describe(_eo_receiver.Base);
}

auto Item::Get(Item _eo_receiver) -> int64_t {
  return ::Counter::Get(*_eo_receiver.Counter);
}

auto Item::Inc(Item _eo_receiver) -> int64_t {
  return ::Counter::Inc(_eo_receiver.Counter);
}

void Item::Rename(ptr<Item> _eo_receiver, std::string name) {
  return ::Base::Rename(ptr(_eo_receiver, &_eo_receiver->Base), name);
}

auto Wrapper::Describe(Wrapper _eo_receiver) -> std::string {
  return ::Base::Describe(_eo_receiver.Item.Base);
}

auto Wrapper::Get(Wrapper _eo_receiver) -> int64_t {
  return ::Counter::Get(*_eo_receiver.Item.Counter);
}

auto Wrapper::Inc(Wrapper _eo_receiver) -> int64_t {
  return ::Counter::Inc(_eo_receiver.Item.Counter);
}

void Wrapper::Rename(ptr<Wrapper> _eo_receiver, std::string name) {
  return ::Base::Rename(ptr(_eo_receiver, &_eo_receiver->Item.Base), name);
}

auto Base::Describe(Base b) -> std::string {
  return "base "s + b.Name;
}

void Base::Rename(ptr<Base> b, std::string name) {
  b->Name = name;
}

auto Counter::Inc(ptr<Counter> c) -> int64_t {
  c->N++;
  return c->N;
}

auto Counter::Get(Counter c) -> int64_t {
  return c.N;
}

func<> eo_main() {
  eo_defer_scope;
  auto it = Item{.Base = Base{.ID = 1, .Name = "base"s}, .Counter = New<Counter>(Counter{}), .Name = "item"s};
  fmt::Println(it.Base.ID, it.Name, it.Base.Name, it.Counter->N);

  it.Base.ID = 2;
  Base::Rename(ref(it.Base), "renamed"s);
  fmt::Println(Base::Describe(it.Base), it.Base.ID, it.Name);

  auto first = Counter::Inc(it.Counter);
  auto second = Counter::Inc(it.Counter);
  fmt::Println(first, second, Counter::Get(*it.Counter), it.Counter->N);

  auto shared = it;
  shared.Base.ID = 3;
  fmt::Println(Counter::Inc(shared.Counter));
  fmt::Println(it.Counter->N, it.Base.ID, shared.Base.ID);

  auto w = Wrapper{.Item = it, .Extra = 7};
  w.Item.Base.ID = 4;
  Base::Rename(ref(w.Item.Base), "wrapped"s);
  fmt::Println(Base::Describe(w.Item.Base),
    w.Item.Base.ID,
    w.Item.Base.ID,
    it.Base.ID,
    Counter::Inc(w.Item.Counter),
    w.Extra);

  auto p = ref(w);
  p->Item.Base.ID = 5;
  Base::Rename(ptr(p, &p->Item.Base), "via pointer"s);
  fmt::Println(Base::Describe(p->Item.Base), p->Item.Base.ID, Counter::Get(*p->Item.Counter));

  fmt::Println(Base{.ID = 6, .Name = "printed"s});
  fmt::Println(Wrapper{.Item = Item{.Name = "x"s}, .Extra = 1});

  try {
    eo_defer([&] { fmt::Println("recovered:"s, recover()); });
    auto empty = Item{.Name = "empty"s};
    fmt::Println(Counter::Get(*empty.Counter));
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
