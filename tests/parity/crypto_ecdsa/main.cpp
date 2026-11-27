#include <eo/crypto/ecdsa.h>
#include <eo/crypto/elliptic.h>
#include <eo/crypto/rand.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/io.h>
#include <eo/math/big.h>
#include <eo/reflect.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;
using namespace std::string_literals;

struct toyCurve;
struct countReader;
struct failReader;

// toyCurve is not a real curve: its base point multiple k*G is (k mod P, 2k mod P), which is enough
// to see that GenerateKey derives the public key from D.
struct toyCurve {
  static constexpr std::string_view _eo_name = "main.toyCurve";

  ptr<elliptic::CurveParams> params{};
  int64_t calls{};

  auto operator==(const toyCurve&) const -> bool = default;

  static auto Params(ptr<toyCurve> c) -> ptr<elliptic::CurveParams>;
  static auto IsOnCurve(ptr<toyCurve> c, ptr<big::Int> x, ptr<big::Int> y) -> bool;
  static auto Add(ptr<toyCurve> c, ptr<big::Int> x1, ptr<big::Int> y1, ptr<big::Int> x2, ptr<big::Int> y2)
    -> std::tuple<ptr<big::Int>, ptr<big::Int>>;
  static auto Double(ptr<toyCurve> c, ptr<big::Int> x1, ptr<big::Int> y1) -> std::tuple<ptr<big::Int>, ptr<big::Int>>;
  static auto ScalarMult(
    ptr<toyCurve> c, ptr<big::Int> x1, ptr<big::Int> y1, slice<uint8_t> k) -> std::tuple<ptr<big::Int>, ptr<big::Int>>;
  static auto ScalarBaseMult(ptr<toyCurve> c, slice<uint8_t> k) -> std::tuple<ptr<big::Int>, ptr<big::Int>>;

  using _eo_methods = eo::methods<eo::method<&toyCurve::Params, "Params">,
    eo::method<&toyCurve::IsOnCurve, "IsOnCurve">,
    eo::method<&toyCurve::Add, "Add">,
    eo::method<&toyCurve::Double, "Double">,
    eo::method<&toyCurve::ScalarMult, "ScalarMult">,
    eo::method<&toyCurve::ScalarBaseMult, "ScalarBaseMult">>;
};

// countReader returns bytes 1, 2, 3, ... and counts the calls of Read.
struct countReader {
  static constexpr std::string_view _eo_name = "main.countReader";

  uint8_t next{};
  int64_t reads{};

  auto operator==(const countReader&) const -> bool = default;

  static auto Read(ptr<countReader> r, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&countReader::Read, "Read">>;
};

// failReader fails every Read.
struct failReader {
  static constexpr std::string_view _eo_name = "main.failReader";

  auto operator==(const failReader&) const -> bool = default;

  static auto Read(failReader _eo_receiver, slice<uint8_t> p) -> std::tuple<int64_t, error>;

  using _eo_methods = eo::methods<eo::method<&failReader::Read, "Read">>;
};

auto toyCurve::Params(ptr<toyCurve> c) -> ptr<elliptic::CurveParams> {
  return c->params;
}

auto toyCurve::IsOnCurve(ptr<toyCurve> c, ptr<big::Int> x, ptr<big::Int> y) -> bool {
  auto twice = big::Int::Lsh(New<big::Int>(), x, 1);
  return big::Int::Cmp(big::Int::Mod(twice, twice, c->params->P), y) == 0;
}

auto toyCurve::Add(ptr<toyCurve> c, ptr<big::Int> x1, ptr<big::Int> y1, ptr<big::Int> x2, ptr<big::Int> y2)
  -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  auto x = big::Int::Add(New<big::Int>(), x1, x2);
  auto y = big::Int::Add(New<big::Int>(), y1, y2);
  auto _eo_call_0 = big::Int::Mod(x, x, c->params->P);
  auto _eo_call_1 = big::Int::Mod(y, y, c->params->P);
  return {_eo_call_0, _eo_call_1};
}

auto toyCurve::Double(ptr<toyCurve> c, ptr<big::Int> x1, ptr<big::Int> y1) -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  return toyCurve::Add(c, x1, y1, x1, y1);
}

auto toyCurve::ScalarMult(
  ptr<toyCurve> c, ptr<big::Int> x1, ptr<big::Int> y1, slice<uint8_t> k) -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  auto s = big::Int::SetBytes(New<big::Int>(), k);
  auto x = big::Int::Mul(New<big::Int>(), x1, s);
  auto y = big::Int::Mul(New<big::Int>(), y1, s);
  auto _eo_call_0 = big::Int::Mod(x, x, c->params->P);
  auto _eo_call_1 = big::Int::Mod(y, y, c->params->P);
  return {_eo_call_0, _eo_call_1};
}

auto toyCurve::ScalarBaseMult(ptr<toyCurve> c, slice<uint8_t> k) -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  c->calls++;
  return toyCurve::ScalarMult(c, c->params->Gx, c->params->Gy, k);
}

auto countReader::Read(ptr<countReader> r, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  r->reads++;
  for (auto i : eo::range(eo::len(p))) {
    r->next++;
    eo::at(p, i) = r->next;
  }
  return {eo::len(p), nullptr};
}

auto failReader::Read(failReader, slice<uint8_t> p) -> std::tuple<int64_t, error> {
  return {0, errors::New("no entropy"s)};
}

auto newToyCurve(int64_t bits) -> ptr<toyCurve> {
  auto p = big::Int::Lsh(New<big::Int>(), big::NewInt(1), static_cast<uint64_t>(bits));
  big::Int::Sub(p, p, big::NewInt(1));
  auto n = big::Int::Sub(New<big::Int>(), p, big::NewInt(2));
  auto _eo_call_0 = big::NewInt(7);
  auto _eo_call_1 = big::NewInt(1);
  auto _eo_call_2 = big::NewInt(2);
  return New<toyCurve>(toyCurve{.params = New<elliptic::CurveParams>(elliptic::CurveParams{
                                  .P = p,
                                  .N = n,
                                  .B = _eo_call_0,
                                  .Gx = _eo_call_1,
                                  .Gy = _eo_call_2,
                                  .BitSize = bits,
                                  .Name = "toy"s,
                                })});
}

void randomBytes() {
  fmt::Printf("%T\n"s, rand::Reader);
  auto b = eo::make_slice<uint8_t>(64);
  auto [n, err] = rand::Read(b);
  fmt::Println(n, err);
  auto zero = int64_t{0};
  for (auto [_eo_range_key, x] : eo::range(b)) {
    if (x == 0) {
      zero++;
    }
  }
  // 64 random bytes have more than 16 zeros with negligible probability.
  fmt::Println(zero <= 16);

  std::tie(n, err) = rand::Read(nullptr);
  fmt::Println(n, err);

  auto q = New<std::array<uint8_t, 16>>();
  std::tie(n, err) = io::ReadFull(rand::Reader, eo::subslice(q, 0));
  fmt::Println(n, err, *q != std::array<uint8_t, 16>{});

  // A replaced Reader is read by Read.
  auto saved = rand::Reader;
  auto cr = New<countReader>(countReader{});
  rand::Reader = cr;
  b = eo::make_slice<uint8_t>(5);
  std::tie(n, err) = rand::Read(b);
  fmt::Println(n, err, b, cr->reads);
  rand::Reader = saved;
}

void curveParams() {
  auto c = newToyCurve(61);
  auto p = toyCurve::Params(c);
  fmt::Println(elliptic::CurveParams::Params(p) == p, p->Name, p->BitSize, p->P, p->N, p->B, p->Gx, p->Gy);

  elliptic::Curve curve = c;
  auto [x, y] = curve.ScalarBaseMult(slice<uint8_t>::of({3}));
  auto _eo_call_0 = curve.IsOnCurve(x, y);
  auto _eo_call_1 = curve.IsOnCurve(y, x);
  fmt::Println(x, y, _eo_call_0, _eo_call_1);
  std::tie(x, y) = curve.Double(x, y);
  fmt::Println(x, y);
  auto _eo_call_2 = big::NewInt(1);
  auto _eo_call_3 = big::NewInt(2);
  std::tie(x, y) = curve.Add(x, y, _eo_call_2, _eo_call_3);
  fmt::Println(x, y);
  std::tie(x, y) = curve.ScalarMult(x, y, slice<uint8_t>::of({1, 0}));
  fmt::Println(x, y);
}

void keys() {
  for (auto [_eo_range_key, bits] : eo::range(slice<int64_t>::of({5, 8, 61, 256}))) {
    auto c = newToyCurve(bits);
    auto cr = New<countReader>(countReader{});
    auto [priv, err] = ecdsa::GenerateKey(c, cr);
    if (err != nullptr) {
      fmt::Println(err);
      continue;
    }
    auto d = priv->D;
    auto inRange = big::Int::Sign(d) > 0 && big::Int::Cmp(d, c->params->N) < 0;
    auto [wantX, wantY] = toyCurve::ScalarMult(c, c->params->Gx, c->params->Gy, big::Int::Bytes(d));
    auto _eo_call_0 = big::Int::Cmp(priv->PublicKey.X, wantX) == 0;
    auto _eo_call_1 = big::Int::Cmp(priv->PublicKey.Y, wantY) == 0;
    fmt::Println(bits, inRange, _eo_call_0, _eo_call_1);
    // The reader passed in is ignored, and ScalarBaseMult is called once, before the check above.
    fmt::Println(cr->reads, c->calls);
    auto _eo_call_2 = priv->PublicKey.Curve == elliptic::Curve(c);
    auto _eo_call_3 = priv->PublicKey.Curve.Params()->Name;
    auto _eo_call_4 = priv->PublicKey.Curve.Params()->BitSize;
    fmt::Println(_eo_call_2, _eo_call_3, _eo_call_4);
    fmt::Println(priv->PublicKey.Curve.IsOnCurve(priv->PublicKey.X, priv->PublicKey.Y));
  }

  // A failing reader is ignored too.
  auto _eo_call_5 = newToyCurve(16);
  auto [priv, err] = ecdsa::GenerateKey(_eo_call_5, failReader{});
  fmt::Println(priv != nullptr, err);

  // So is a replaced Reader.
  auto saved = rand::Reader;
  auto cr = New<countReader>(countReader{});
  rand::Reader = cr;
  auto _eo_call_6 = newToyCurve(16);
  std::tie(priv, err) = ecdsa::GenerateKey(_eo_call_6, rand::Reader);
  fmt::Println(priv != nullptr, err, cr->reads);
  rand::Reader = saved;

  // Two keys differ.
  auto c = newToyCurve(256);
  auto a = std::get<0>(ecdsa::GenerateKey(c, rand::Reader));
  auto b = std::get<0>(ecdsa::GenerateKey(c, rand::Reader));
  fmt::Println(big::Int::Cmp(a->D, b->D) != 0);

  auto pub = a->PublicKey;
  auto same = ecdsa::PublicKey{.Curve = c, .X = a->PublicKey.X, .Y = a->PublicKey.Y};
  auto other = ecdsa::PublicKey{.Curve = c, .X = b->PublicKey.X, .Y = b->PublicKey.Y};
  fmt::Println(pub == same, pub == other);
  fmt::Println(*a == *a, *a == *b);
  ecdsa::PrivateKey zero{};
  fmt::Println(zero.PublicKey.Curve == nullptr, zero.PublicKey.X == nullptr, zero.D == nullptr);
}

void types() {
  auto _eo_call_0 = reflect::TypeOf(ecdsa::PublicKey{});
  auto _eo_call_1 = reflect::TypeOf(ecdsa::PrivateKey{});
  auto _eo_call_2 = reflect::TypeOf(elliptic::CurveParams{});
  for (auto [_eo_range_key, t] : eo::range(slice<reflect::Type>::of({_eo_call_0, _eo_call_1, _eo_call_2}))) {
    auto _eo_call_3 = t.Kind();
    auto _eo_call_4 = t.NumField();
    fmt::Println(t, _eo_call_3, _eo_call_4);
    for (auto i = int64_t{0}; i < t.NumField(); i++) {
      auto f = t.Field(i);
      fmt::Println(" "s, f.Name, f.Type, f.Anonymous);
    }
  }
}

func<> eo_main() {
  randomBytes();
  curveParams();
  keys();
  types();
  co_return;
}
