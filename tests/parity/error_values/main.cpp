#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct NotFound;
struct Temporary;

struct NotFound {
  static constexpr std::string_view _eo_name = "main.NotFound";

  std::string Key;

  auto operator==(const NotFound&) const -> bool = default;

  static auto Error(ptr<NotFound> e) -> std::string;

  using _eo_methods = eo::methods<eo::method<&NotFound::Error, "Error">>;
};

struct Temporary {
  static constexpr std::string_view _eo_name = "main.Temporary";

  int64_t Code{};

  auto operator==(const Temporary&) const -> bool = default;

  static auto Error(Temporary t) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Temporary::Error, "Error">>;
};

auto NotFound::Error(ptr<NotFound> e) -> std::string {
  return "not found: "s + e->Key;
}

auto Temporary::Error(Temporary t) -> std::string {
  return fmt::Sprintf("temporary %d"s, t.Code);
}

auto lookup(std::string key) -> std::tuple<int64_t, error> {
  if (key == ""s) {
    return {0, New<NotFound>(NotFound{.Key = "<empty>"s})};
  }
  if (key == "busy"s) {
    return {0, Temporary{.Code = 503}};
  }
  return {eo::len(key), nullptr};
}

auto load(std::string key) -> std::tuple<int64_t, error> {
  auto [n, err] = lookup(key);
  if (err != nullptr) {
    return {0, fmt::Errorf("load %q: %w"s, key, err)};
  }
  return {n, nullptr};
}

auto typedNil() -> error {
  ptr<NotFound> p;
  return p;
}

func<> eo_main() {
  auto a = errors::New("same"s);
  auto b = errors::New("same"s);
  fmt::Println(a == b, a == a, a.Error() == b.Error());

  auto [n, err] = load("abc"s);
  fmt::Println(n, err == nullptr, err);

  err = std::get<1>(load(""s));
  fmt::Println(err);
  auto nf = New<ptr<NotFound>>();
  auto found = errors::As(err, nf);
  fmt::Println(found, (*nf)->Key, errors::Unwrap(err) == error(*nf));

  err = std::get<1>(load("busy"s));
  auto tmp = New<Temporary>();
  found = errors::As(err, tmp);
  fmt::Println(found, tmp->Code);
  fmt::Println(errors::Is(err, Temporary{.Code = 503}), errors::Is(err, Temporary{.Code = 500}));

  auto sentinel = errors::New("sentinel"s);
  auto wrapped = fmt::Errorf("layer 2: %w"s, fmt::Errorf("layer 1: %w"s, sentinel));
  fmt::Println(wrapped);
  fmt::Println(errors::Is(wrapped, sentinel), errors::Is(wrapped, errors::New("sentinel"s)));
  fmt::Println(errors::Unwrap(errors::Unwrap(wrapped)) == sentinel, errors::Unwrap(sentinel) == nullptr);

  auto joined = errors::Join(slice<error>::of({sentinel, nullptr, New<NotFound>(NotFound{.Key = "k"s})}));
  found = errors::As(joined, nf);
  fmt::Println(errors::Is(joined, sentinel), found, (*nf)->Key);
  fmt::Printf("%q\n"s, joined.Error());
  fmt::Println(errors::Join(slice<error>::of({nullptr, nullptr})) == nullptr);

  auto multi = fmt::Errorf("%w + %w"s, sentinel, Temporary{.Code = 1});
  fmt::Println(multi,
    errors::Is(multi, sentinel),
    errors::Is(multi, Temporary{.Code = 1}),
    errors::Unwrap(multi) == nullptr);

  err = typedNil();
  fmt::Println(err == nullptr, err);
  fmt::Printf("%T %v\n"s, err, err);

  error none{};
  fmt::Println(none == nullptr, none);
  fmt::Printf("%v|%T\n"s, none, none);
  fmt::Println(fmt::Errorf("plain %d"s, int64_t{7}), fmt::Errorf("bad %w"s, int64_t{7}), fmt::Errorf("nil %w"s, none));
  co_return;
}
