// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>
#include <eo/maps.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

// Runs f, which must panic, and returns the panic message.
template<typename F>
auto panic_message(F f) -> std::string {
  try {
    f();
  } catch (const Panic& p) {
    return p.message();
  }
  throw std::logic_error("expected a panic");
}

struct Key {
  int64_t X{};
  std::string S;

  auto operator==(const Key&) const -> bool = default;
};

struct FloatKey {
  double F{};

  auto operator==(const FloatKey&) const -> bool = default;
};

void test_nil_map_reads_as_empty() {
  map<std::string, int64_t> m;
  check(m == nullptr, "the zero value should be the nil map");
  check(eo::len(m) == 0, "a nil map should have no entries");
  check(eo::map_get(m, "a"s) == 0, "a nil map should read the zero value");
  check(!eo::map_get_ok(m, "a"s).second, "a nil map should not have a key");
  eo::delete_(m, "a"s);
  eo::clear(m);
  auto count = 0;
  for ([[maybe_unused]] auto _eo_range_key : eo::range(m)) {
    ++count;
  }
  check(count == 0, "a nil map should range over nothing");
  check(fmt::Sprint(m) == "map[]", "a nil map should print as map[]");
}

void test_nil_map_write_panics() {
  map<std::string, int64_t> m;
  check(panic_message([&] { eo::map_entry(m, "a"s) = 1; }) == "assignment to entry in nil map",
    "assigning to a nil map should panic with Go's message");
  check(panic_message([&] { ++eo::map_entry(m, "a"s); }) == "assignment to entry in nil map",
    "incrementing an entry of a nil map should panic");
  try {
    eo::map_entry(m, "a"s) = 1;
  } catch (const Panic& p) {
    check(fmt::Sprintf("%T"s, p.value()) == "runtime.plainError", "the panic value should be a runtime.plainError");
  }
}

void test_copies_share_the_map() {
  auto m = eo::make_map<std::string, int64_t>();
  auto alias = m;
  eo::map_entry(alias, "k"s) = 3;
  check(eo::map_get(m, "k"s) == 3, "a copy of a map should share its entries");
  check(m != nullptr && eo::make_map<int64_t, int64_t>(int64_t{-1}) != nullptr, "make should give a non-nil map");
  check(map<int64_t, int64_t>::of({}) != nullptr, "an empty literal should not be nil");
}

void test_missing_key_read_does_not_insert() {
  auto m = map<std::string, int64_t>::of({{"a"s, 1}});
  check(eo::map_get(m, "b"s) == 0, "a missing key should read the zero value");
  check(eo::len(m) == 1, "reading a missing key should not insert it");
  auto [v, ok] = eo::map_get_ok(m, "a"s);
  check(v == 1 && ok, "comma-ok should find a present key");
  auto [w, found] = eo::map_get_ok(m, "b"s);
  check(w == 0 && !found, "comma-ok should report a missing key");
}

void test_entry_inserts_and_updates() {
  auto m = eo::make_map<std::string, int64_t>(8);
  eo::map_entry(m, "n"s) += 5;
  ++eo::map_entry(m, "n"s);
  eo::map_entry(m, "z"s)++;
  check(eo::map_get(m, "n"s) == 6 && eo::map_get(m, "z"s) == 1, "op-assignment should start from the zero value");
  // C++ evaluates the right side first, as Go reads len(m) before the assignment inserts.
  eo::map_entry(m, "len"s) = eo::len(m);
  check(eo::map_get(m, "len"s) == 2, "the right side should be evaluated before the entry is inserted");
  eo::delete_(m, "n"s);
  eo::delete_(m, "absent"s);
  check(eo::len(m) == 2 && !eo::map_get_ok(m, "n"s).second, "delete should remove only the key");
  eo::clear(m);
  check(eo::len(m) == 0 && m != nullptr, "clear should empty the map and keep it non-nil");
}

void test_literal_later_key_wins() {
  auto m = map<int64_t, std::string>::of({{1, "a"s}, {2, "b"s}, {1, "c"s}});
  check(eo::len(m) == 2 && eo::map_get(m, 1) == "c", "a later duplicate key should replace an earlier one");
}

void test_struct_and_pointer_keys() {
  auto m = map<Key, bool>::of(
    {{Key{.X = 2, .S = "b"s}, true}, {Key{.X = 1, .S = "z"s}, false}, {Key{.X = 1, .S = "a"s}, true}});
  check(eo::map_get(m, Key{.X = 1, .S = "a"s}), "a struct key should be found by value");
  check(fmt::Sprint(m) == "map[{1 a}:true {1 z}:false {2 b}:true]", "struct keys should print field by field");
  auto a = New<int64_t>(1);
  auto b = New<int64_t>(1);
  auto p = map<ptr<int64_t>, int64_t>::of({{a, 1}, {b, 2}});
  check(eo::len(p) == 2 && eo::map_get(p, a) == 1 && eo::map_get(p, b) == 2, "pointer keys should compare by address");
  auto e = map<empty_struct, int64_t>::of({{empty_struct{}, 1}, {empty_struct{}, 2}});
  check(eo::len(e) == 1 && eo::map_get(e, empty_struct{}) == 2, "struct{} should be one key");
}

void test_float_keys() {
  auto nan = std::numeric_limits<double>::quiet_NaN();
  auto m = eo::make_map<double, int64_t>();
  eo::map_entry(m, nan) = 1;
  eo::map_entry(m, nan) = 1;
  eo::map_entry(m, 0.0) = 3;
  eo::map_entry(m, -0.0) = 4;
  eo::map_entry(m, -std::numeric_limits<double>::infinity()) = 5;
  check(eo::len(m) == 4, "each NaN key should be a new entry, and -0 should equal 0");
  check(!eo::map_get_ok(m, nan).second, "a NaN key should never be found");
  eo::delete_(m, nan);
  check(eo::len(m) == 4, "deleting a NaN key should do nothing");
  check(fmt::Sprint(m) == "map[NaN:1 NaN:1 -Inf:5 -0:4]", "assigning -0 should store the new key, as Go does");
  auto count = 0;
  for ([[maybe_unused]] auto _eo_range_key : eo::range(m)) {
    ++count;
  }
  check(count == 4, "range should produce the NaN entries");
  auto s = map<FloatKey, int64_t>::of({{FloatKey{.F = 0.0}, 1}});
  eo::map_entry(s, FloatKey{.F = -0.0}) = 2;
  check(fmt::Sprint(s) == "map[{-0}:2]", "a struct key with a float field should store the new key");
  eo::clear(m);
  check(eo::len(m) == 0, "clear should remove NaN entries");
}

void test_range_sees_deletes_and_updates() {
  auto m = map<int64_t, int64_t>::of({{1, 10}, {2, 20}, {3, 30}, {4, 40}});
  auto seen = 0;
  for (auto [k, v] : eo::range(m)) {
    ++seen;
    // Deletes every other entry and updates none, so exactly one iteration happens.
    for (auto key : {int64_t{1}, int64_t{2}, int64_t{3}, int64_t{4}}) {
      if (key != k) {
        eo::delete_(m, key);
      }
    }
    check(v == k * 10, "range should produce the entry's value");
  }
  check(seen == 1 && eo::len(m) == 1, "range should not produce an entry deleted before it is reached");

  auto n = map<int64_t, int64_t>::of({{1, 0}, {2, 0}});
  auto sum = int64_t{0};
  for (auto [k, v] : eo::range(n)) {
    sum += v;
    eo::map_entry(n, 3 - k) = 100;
  }
  check(sum == 100, "range should produce the value an entry has when it is reached");

  auto c = map<int64_t, int64_t>::of({{1, 0}, {2, 0}, {3, 0}});
  seen = 0;
  for ([[maybe_unused]] auto _eo_range_key : eo::range(c)) {
    ++seen;
    eo::clear(c);
  }
  check(seen == 1, "range should stop after clear");

  auto g = map<int64_t, int64_t>::of({{1, 0}});
  seen = 0;
  for ([[maybe_unused]] auto _eo_range_key : eo::range(g)) {
    ++seen;
    for (auto i : eo::range(int64_t{100})) {
      eo::map_entry(g, i + 10) = i;
    }
  }
  check(seen >= 1 && eo::len(g) == 101, "inserting during range should keep the loop valid");
}

void test_printing_and_type_names() {
  auto m = map<int8_t, uint8_t>::of({{3, 4}, {-1, 255}});
  check(fmt::Sprint(m) == "map[-1:255 3:4]", "integer keys should print in numeric order");
  check(fmt::Sprintf("%x"s, map<std::string, int64_t>::of({{"a"s, 10}})) == "map[61:a]",
    "a verb should apply to keys and values");
  check(fmt::Sprintf("%T"s, map<std::string, slice<int64_t>>{}) == "map[string][]int", "%T should name the map type");
  auto pm = New<map<std::string, int64_t>>(map<std::string, int64_t>::of({{"k"s, 1}}));
  check(fmt::Sprint(pm) == "&map[k:1]", "a pointer to a map should print as & and the map");
  check(fmt::Sprint(map<bool, std::string>::of({{true, "t"s}, {false, "f"s}})) == "map[false:f true:t]",
    "bool keys should print false first");
  any a = map<int64_t, std::string>{};
  check(panic_message([&] { [[maybe_unused]] auto equal = a == a; }) ==
      "runtime error: comparing uncomparable type map[int]string",
    "comparing maps in interfaces should panic");
}

void test_maps_package() {
  auto m = map<std::string, int64_t>::of({{"a"s, 1}, {"b"s, 2}});
  auto c = maps::Clone(m);
  eo::map_entry(c, "c"s) = 3;
  check(eo::len(m) == 2 && eo::len(c) == 3, "Clone should copy the entries into a new map");
  check(maps::Clone(map<std::string, int64_t>{}) == nullptr, "Clone of nil should be nil");
  maps::Copy(m, map<std::string, int64_t>::of({{"b"s, 20}, {"d"s, 4}}));
  check(eo::map_get(m, "b"s) == 20 && eo::len(m) == 3, "Copy should assign every entry of src");
  check(panic_message([&] { maps::Copy(map<std::string, int64_t>{}, m); }) == "assignment to entry in nil map",
    "Copy into nil should panic");
  maps::Copy(map<std::string, int64_t>{}, map<std::string, int64_t>{});
  maps::DeleteFunc(m, [](const std::string& k, int64_t) { return k != "a"; });
  check(eo::len(m) == 1 && eo::map_get(m, "a"s) == 1, "DeleteFunc should delete the matching entries");
  check(maps::Equal(m, map<std::string, int64_t>::of({{"a"s, 1}})), "Equal should compare keys and values");
  check(!maps::Equal(m, map<std::string, int64_t>::of({{"a"s, 2}})), "Equal should compare values");
  check(maps::Equal(map<std::string, int64_t>{}, map<std::string, int64_t>::of({})), "nil should equal an empty map");
  check(maps::EqualFunc(m,
          map<std::string, std::string>::of({{"a"s, "1"s}}),
          [](int64_t v, const std::string& s) { return fmt::Sprint(v) == s; }),
    "EqualFunc should compare values with the function");
  auto nan = std::numeric_limits<double>::quiet_NaN();
  auto f = map<double, int64_t>::of({{nan, 1}});
  check(!maps::Equal(f, f), "a map with a NaN key should not equal itself");
  maps::DeleteFunc(f, [](double, int64_t) { return true; });
  check(eo::len(maps::Clone(f)) == 1, "DeleteFunc should not delete a NaN key, and Clone should keep it");
}

int main() {
  test_nil_map_reads_as_empty();
  test_nil_map_write_panics();
  test_copies_share_the_map();
  test_missing_key_read_does_not_insert();
  test_entry_inserts_and_updates();
  test_literal_later_key_wins();
  test_struct_and_pointer_keys();
  test_float_keys();
  test_range_sees_deletes_and_updates();
  test_printing_and_type_names();
  test_maps_package();
}
