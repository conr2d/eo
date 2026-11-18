#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Outer;
struct Inner;
struct Parent;
struct Child;

struct Inner {
  static constexpr std::string_view _eo_name = "main.Inner";

  int64_t A{}, B{};

  auto operator==(const Inner&) const -> bool = default;
};

// Outer holds an Inner by value before Inner is declared.
struct Outer {
  static constexpr std::string_view _eo_name = "main.Outer";

  std::string Name{};
  Inner In{};
  ptr<int64_t> Count{};
  slice<int64_t> Items{};

  auto operator==(const Outer&) const -> bool = default;
};

// Parent and Child refer to each other through pointers.
struct Parent {
  static constexpr std::string_view _eo_name = "main.Parent";

  std::string Name{};
  ptr<Child> First{};

  auto operator==(const Parent&) const -> bool = default;
};

struct Child {
  static constexpr std::string_view _eo_name = "main.Child";

  std::string Name{};
  ptr<::Parent> Parent{};
  ptr<Child> Next{};

  auto operator==(const Child&) const -> bool = default;
};

auto bump(Outer o) -> Outer {
  o.Name += "!"s;
  o.In.A++;
  (*o.Count)++;
  eo::at(o.Items, 0)++;
  return o;
}

func<> eo_main() {
  eo_defer_scope;
  auto n = New<int64_t>(1);
  auto a = Outer{.Name = "a"s, .In = Inner{1, 2}, .Count = n, .Items = slice<int64_t>::of({10, 20})};
  auto b = a;
  b.Name = "b"s;
  b.In.B = 9;
  fmt::Println(a.Name, a.In, b.Name, b.In);

  auto c = bump(a);
  fmt::Println(a.Name, a.In, *a.Count, a.Items);
  fmt::Println(c.Name, c.In, *c.Count, c.Items);
  fmt::Println(a.Count == c.Count, *n);

  auto x = Inner{1, 2};
  auto y = Inner{.A = 1, .B = 2};
  auto z = Inner{2, 1};
  fmt::Println(x == y, x != y, x == z, x != z);

  auto p = New<Parent>(Parent{.Name = "p"s});
  auto c1 = New<Child>(Child{.Name = "c1"s, .Parent = p});
  auto c2 = New<Child>(Child{.Name = "c2"s, .Parent = p, .Next = c1});
  p->First = c2;
  for (auto c = p->First; c != nullptr; c = c->Next) {
    fmt::Println(c->Name, c->Parent->Name, c->Parent == p);
  }
  try {
    eo_defer([&] {
      p->First = nullptr; // Eo never frees a reference cycle (#80).
    });

    auto k1 = Child{.Name = "k"s, .Parent = p};
    auto k2 = Child{.Name = "k"s, .Parent = p};
    auto k3 = Child{.Name = "k"s, .Parent = New<Parent>(Parent{.Name = "p"s})};
    fmt::Println(k1 == k2, k1 == k3);

    auto arr = slice<Inner>::of({Inner{1, 1}, Inner{2, 2}});
    auto first = eo::at(arr, 0);
    first.A = 100;
    eo::at(arr, 1).B = 200;
    fmt::Println(first, arr);
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  co_return;
}
