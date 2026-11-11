// Provisional: struct types, composite literals, and Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct Node {
  ptr<int> p;
  ptr<Node> next;
};

func<> eo_main() {
  ptr<Node> head;
  for (auto i = New<int>(0); *i < 8; i = New<int>(*i), (*i)++) {
    if (*i % 3 == 1) {
      (*i)++;
      continue;
    }
    head = New<Node>(Node{.p = i, .next = head});
    (*i)++;
  }
  for (auto n = head; n != nullptr; n = n->next) {
    fmt::Println(*n->p);
    *n->p += 100;
  }
  for (auto n = head; n != nullptr; n = n->next) {
    fmt::Println(*n->p);
  }
  co_return;
}
