// Provisional: struct types, composite literals, and Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct Node {
  ptr<int> p;
  ptr<Node> next;
};

void show(ptr<Node> head) {
  for (auto n = head; n != nullptr; n = n->next) {
    fmt::Println(*n->p);
    *n->p += 100;
  }
  for (auto n = head; n != nullptr; n = n->next) {
    fmt::Println(*n->p);
  }
}

void steps() {
  ptr<Node> head;
  for (auto i = New<int>(0); *i < 8; i = New<int>(*i), (*i)++) {
    head = New<Node>(Node{.p = i, .next = head});
    if (*i % 3 == 1) {
      (*i)++;
      continue;
    }
    (*i)++;
  }
  show(head);
}

void pairs() {
  ptr<Node> head;
  for (auto i = New<int>(0), j = New<int>(10); *i < *j; i = New<int>(*i), j = New<int>(*j)) {
    head = New<Node>(Node{.p = i, .next = head});
    head = New<Node>(Node{.p = j, .next = head});
    *i += 3;
    *j -= 2;
  }
  show(head);
}

func<> eo_main() {
  steps();
  pairs();
  co_return;
}
