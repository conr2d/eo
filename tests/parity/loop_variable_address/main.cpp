#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

struct Node;

struct Node {
  ptr<int64_t> p{};
  ptr<Node> next{};

  auto operator==(const Node&) const -> bool = default;
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
  for (auto i = New<int64_t>(0); *i < 8; i = New<int64_t>(*i), (*i)++) {
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
  for (auto i = New<int64_t>(0), j = New<int64_t>(10); *i < *j; i = New<int64_t>(*i), j = New<int64_t>(*j)) {
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
