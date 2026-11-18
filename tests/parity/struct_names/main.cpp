// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/sync.h>

#include <cstdint>

using namespace eo;

struct Config;
struct Hash;
struct Node;
struct point;

struct Config {
  int64_t Depth{};

  auto operator==(const Config&) const -> bool = default;
};

struct Hash {
  int64_t Lo{}, Hi{};

  auto operator==(const Hash&) const -> bool = default;
};

// Node has a field named after its type and a method named after another type.
struct Node {
  ::Config Config{};
  int64_t Value{};

  auto operator==(const Node&) const -> bool = default;

  static auto Hash(Node n) -> ::Hash;
  static auto Size(ptr<Node> n) -> int64_t;
  static void Reset(ptr<Node> n);
};

struct point {
  int64_t x{}, y{};

  auto operator==(const point&) const -> bool = default;
};

auto Size(int64_t n) -> int64_t {
  return n * 10;
}

// Hash is a method named after the type Hash, and Size calls the package function Size.
auto Node::Hash(Node n) -> ::Hash {
  return ::Hash{.Lo = n.Value, .Hi = n.Config.Depth};
}

auto Node::Size(ptr<Node> n) -> int64_t {
  return ::Size(n->Value) + n->Config.Depth;
}

void Node::Reset(ptr<Node> n) {
  *n = Node{.Config = ::Config{.Depth = 1}};
}

auto mk(int64_t x) -> point {
  return point{x, x + 1};
}

func<> eo_main() {
  auto n = Node{.Config = Config{.Depth = 2}, .Value = 3};
  fmt::Println(Node::Hash(n), Node::Size(ref(n)), n);
  Node::Reset(ref(n));
  fmt::Println(n, Node::Hash(n) == Hash{0, 1});

  // p is moved to heap while the variable point hides the type point.
  auto point = int64_t{1};
  auto p = New<::point>(mk(point));
  auto q = p;
  auto wg = New<sync::WaitGroup>();
  wg->Add(1);
  go([=]() mutable -> func<> {
    eo_defer_scope;
    try {
      auto _eo_defer_receiver_0 = wg;
      eo_defer([=] { _eo_defer_receiver_0->Done(); });
      q->x = 7;
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
    co_return;
  });
  co_await wg->Wait();
  fmt::Println(point, *p, *q);

  {
    auto _eo_shadow_mk = mk(4);
    fmt::Println(_eo_shadow_mk);
  }
  struct local {
    int64_t a{};

    auto operator==(const local&) const -> bool = default;
  };
  fmt::Println(local{.a = 5});
  co_return;
}
