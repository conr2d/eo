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
  auto _eo_call_0 = a.Error();
  auto _eo_call_1 = b.Error();
  fmt::Println(a == b, a == a, _eo_call_0 == _eo_call_1);

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
  auto _eo_call_2 = errors::Is(err, Temporary{.Code = 503});
  auto _eo_call_3 = errors::Is(err, Temporary{.Code = 500});
  fmt::Println(_eo_call_2, _eo_call_3);

  auto sentinel = errors::New("sentinel"s);
  auto wrapped = fmt::Errorf("layer 2: %w"s, fmt::Errorf("layer 1: %w"s, sentinel));
  fmt::Println(wrapped);
  auto _eo_call_4 = errors::Is(wrapped, sentinel);
  auto _eo_call_5 = errors::New("sentinel"s);
  auto _eo_call_6 = errors::Is(wrapped, _eo_call_5);
  fmt::Println(_eo_call_4, _eo_call_6);
  auto _eo_call_7 = errors::Unwrap(wrapped);
  auto _eo_call_8 = errors::Unwrap(_eo_call_7);
  auto _eo_call_9 = errors::Unwrap(sentinel);
  fmt::Println(_eo_call_8 == sentinel, _eo_call_9 == nullptr);

  auto joined = errors::Join(slice<error>::of({sentinel, nullptr, New<NotFound>(NotFound{.Key = "k"s})}));
  found = errors::As(joined, nf);
  fmt::Println(errors::Is(joined, sentinel), found, (*nf)->Key);
  fmt::Printf("%q\n"s, joined.Error());
  fmt::Println(errors::Join(slice<error>::of({nullptr, nullptr})) == nullptr);

  auto multi = fmt::Errorf("%w + %w"s, sentinel, Temporary{.Code = 1});
  auto _eo_call_10 = errors::Is(multi, sentinel);
  auto _eo_call_11 = errors::Is(multi, Temporary{.Code = 1});
  auto _eo_call_12 = errors::Unwrap(multi);
  fmt::Println(multi, _eo_call_10, _eo_call_11, _eo_call_12 == nullptr);

  err = typedNil();
  fmt::Println(err == nullptr, err);
  fmt::Printf("%T %v\n"s, err, err);

  error none{};
  fmt::Println(none == nullptr, none);
  fmt::Printf("%v|%T\n"s, none, none);
  auto _eo_call_13 = fmt::Errorf("plain %d"s, int64_t{7});
  auto _eo_call_14 = fmt::Errorf("bad %w"s, int64_t{7});
  auto _eo_call_15 = fmt::Errorf("nil %w"s, none);
  fmt::Println(_eo_call_13, _eo_call_14, _eo_call_15);
  co_return;
}
