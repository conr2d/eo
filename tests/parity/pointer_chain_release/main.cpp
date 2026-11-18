#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

struct Node;

struct Node {
  ptr<Node> next{};
  int64_t v{};

  auto operator==(const Node&) const -> bool = default;
};

func<> eo_main() {
  ptr<Node> head;
  for (auto i = int64_t{0}; i < 1000000; i++) {
    head = New<Node>(Node{.next = head, .v = i});
  }
  fmt::Println(head->v, head->next->v);
  head = nullptr;
  fmt::Println("freed"s);
  co_return;
}
