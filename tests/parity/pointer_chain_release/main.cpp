// Provisional: struct types, composite literals, and Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct Node {
  ptr<Node> next;
  int v{};
};

func<> eo_main() {
  ptr<Node> head;
  for (auto i = 0; i < 1000000; i++) {
    head = New<Node>(Node{.next = head, .v = i});
  }
  fmt::Println(head->v, head->next->v);
  head = nullptr;
  fmt::Println("freed"s);
  co_return;
}
