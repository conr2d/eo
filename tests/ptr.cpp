// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>
#include <eo/fmt.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

struct Point {
  int64_t x;
  int64_t y;
};

// Counts live instances, to see when an owned target is destroyed.
struct Tracked {
  static inline int live = 0;
  int64_t n{};
  Point p{};
  Tracked() {
    live++;
  }
  Tracked(const Tracked& other): n(other.n), p(other.p) {
    live++;
  }
  ~Tracked() {
    live--;
  }
};

// Returns the recovered panic message, or "" when f returns normally.
template<typename F>
auto recovered_message(F f) -> std::string {
  std::string message;
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        message = fmt::format("{}", r);
      }
    });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
  return message;
}

void test_nil_is_the_zero_value() {
  ptr<int64_t> p;
  ptr<int64_t> q = nullptr;
  check(p == nullptr, "a default pointer is not nil");
  check(q == nullptr, "nullptr does not make a nil pointer");
  check(p == q, "two nil pointers are not equal");
  check(!(New<int64_t>() == nullptr), "New returned nil");
}

void test_new_is_zeroed_and_shared_by_copies() {
  auto p = New<Point>();
  check(p->x == 0 && p->y == 0, "New<T>() is not the zero value");
  auto q = p;
  q->x = 3;
  (*q).y = 4;
  check(p->x == 3 && p->y == 4, "a copy does not share the target");
  check(p == q, "copies are not equal");
  check(!(New<Point>() == New<Point>()), "two New targets compare equal");
  auto n = New<int64_t>(7);
  check(*n == 7, "New<T>(v) does not hold v");
}

void test_nil_dereference_panics() {
  const auto want = std::string{"runtime error: invalid memory address or nil pointer dereference"};
  ptr<Point> p;
  check(recovered_message([&] { p->x = 1; }) == want, "-> on nil did not panic with Go's runtime error");
  check(recovered_message([&] { (void)*p; }) == want, "* on nil did not panic with Go's runtime error");
  check(recovered_message([&] { (void)ptr(p, &p->y); }) == want, "&p.f on nil did not panic");
  check(recovered_message([] { (void)*New<Point>(); }).empty(), "a non-nil dereference panicked");
}

void test_field_pointer_keeps_the_target_alive() {
  ptr<int64_t> f;
  {
    auto t = New<Tracked>();
    f = ptr(t, &t->p.y);
    *f = 9;
    check(t->p.y == 9, "the field pointer does not refer to the field");
  }
  check(Tracked::live == 1, "the target died while a field pointer still refers to it");
  check(*f == 9, "the field lost its value");
  f = nullptr;
  check(Tracked::live == 0, "the target outlived every pointer to it");
}

void test_ref_borrows_without_owning() {
  {
    Tracked local;
    auto r = ref(local);
    auto s = r;
    s->n = 5;
    check(local.n == 5, "ref does not refer to the variable");
    check(r == ref(local), "two refs of one variable are not equal");
    check(ptr(r, &r->n) == ref(local.n), "&x.f through a ref and ref(x.f) differ");
    check(Tracked::live == 1, "ref made a copy");
  }
  check(Tracked::live == 0, "ref kept a borrowed variable alive");
}

struct Node {
  static inline int live = 0;
  ptr<Node> next;
  Node() {
    live++;
  }
  Node(const Node&) = delete;
  ~Node() {
    live--;
  }
};

// Freeing a list releases each node from the previous node's destructor. Doing that recursively
// would overflow the stack long before a million nodes.
void test_long_chain_is_freed_without_recursion() {
  ptr<Node> head;
  for (auto i = 0; i < 1'000'000; i++) {
    auto n = New<Node>();
    n->next = head;
    head = n;
  }
  check(Node::live == 1'000'000, "the list was not built");
  head = nullptr;
  check(Node::live == 0, "dropping the head did not free the list");
  {
    ptr<Node> scoped;
    for (auto i = 0; i < 1'000'000; i++) {
      auto n = New<Node>();
      n->next = std::move(scoped);
      scoped = std::move(n);
    }
  }
  check(Node::live == 0, "a list leaving scope was not freed");
}

void test_pointers_hash_by_address() {
  auto a = New<int64_t>();
  auto b = New<int64_t>();
  auto set = std::unordered_set<ptr<int64_t>>{a, b, a};
  check(set.size() == 2, "equal pointers hash differently");
}

int main() {
  test_nil_is_the_zero_value();
  test_new_is_zeroed_and_shared_by_copies();
  test_nil_dereference_panics();
  test_field_pointer_keeps_the_target_alive();
  test_ref_borrows_without_owning();
  test_long_chain_is_freed_without_recursion();
  test_pointers_hash_by_address();
}
