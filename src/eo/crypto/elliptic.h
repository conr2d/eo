// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/math/big.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's crypto/elliptic: the Curve interface and the CurveParams struct of Go 1.26's elliptic.go and
// params.go, for curves that a program implements, such as go-ethereum's secp256k1. The NIST curves
// P224, P256, P384, and P521, the functions Marshal, Unmarshal, MarshalCompressed,
// UnmarshalCompressed, and GenerateKey, and the generic arithmetic of CurveParams, which Go
// deprecates, are not mapped: a *CurveParams has only its Params method, so it does not implement
// Curve.

namespace eo::elliptic {

struct CurveParams;
struct Curve;

// Go's elliptic.CurveParams: the parameters of a short-form Weierstrass curve with a = -3.
struct CurveParams {
  static constexpr std::string_view _eo_name = "elliptic.CurveParams";

  ptr<big::Int> P{};
  ptr<big::Int> N{};
  ptr<big::Int> B{};
  ptr<big::Int> Gx{}, Gy{};
  int64_t BitSize{};
  std::string Name{};

  auto operator==(const CurveParams&) const -> bool = default;

  static auto Params(ptr<CurveParams> curve) -> ptr<CurveParams> {
    return curve;
  }

  using _eo_methods = eo::methods<eo::method<&CurveParams::Params, "Params">>;
};

// Go's elliptic.Curve.
struct Curve : eo::interface<Curve> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "elliptic.Curve";

  auto Params() const -> ptr<CurveParams>;
  auto IsOnCurve(ptr<big::Int> x, ptr<big::Int> y) const -> bool;
  auto Add(ptr<big::Int> x1,
    ptr<big::Int> y1,
    ptr<big::Int> x2,
    ptr<big::Int> y2) const -> std::tuple<ptr<big::Int>, ptr<big::Int>>;
  auto Double(ptr<big::Int> x1, ptr<big::Int> y1) const -> std::tuple<ptr<big::Int>, ptr<big::Int>>;
  auto ScalarMult(
    ptr<big::Int> x1, ptr<big::Int> y1, slice<uint8_t> k) const -> std::tuple<ptr<big::Int>, ptr<big::Int>>;
  auto ScalarBaseMult(slice<uint8_t> k) const -> std::tuple<ptr<big::Int>, ptr<big::Int>>;

  using _eo_methods = eo::methods<eo::method<&Curve::Params, "Params">,
    eo::method<&Curve::IsOnCurve, "IsOnCurve">,
    eo::method<&Curve::Add, "Add">,
    eo::method<&Curve::Double, "Double">,
    eo::method<&Curve::ScalarMult, "ScalarMult">,
    eo::method<&Curve::ScalarBaseMult, "ScalarBaseMult">>;
};

inline auto Curve::Params() const -> ptr<CurveParams> {
  return _eo_call<&Curve::Params>();
}
inline auto Curve::IsOnCurve(ptr<big::Int> x, ptr<big::Int> y) const -> bool {
  return _eo_call<&Curve::IsOnCurve>(std::move(x), std::move(y));
}
inline auto Curve::Add(ptr<big::Int> x1,
  ptr<big::Int> y1,
  ptr<big::Int> x2,
  ptr<big::Int> y2) const -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  return _eo_call<&Curve::Add>(std::move(x1), std::move(y1), std::move(x2), std::move(y2));
}
inline auto Curve::Double(ptr<big::Int> x1, ptr<big::Int> y1) const -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  return _eo_call<&Curve::Double>(std::move(x1), std::move(y1));
}
inline auto Curve::ScalarMult(
  ptr<big::Int> x1, ptr<big::Int> y1, slice<uint8_t> k) const -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  return _eo_call<&Curve::ScalarMult>(std::move(x1), std::move(y1), std::move(k));
}
inline auto Curve::ScalarBaseMult(slice<uint8_t> k) const -> std::tuple<ptr<big::Int>, ptr<big::Int>> {
  return _eo_call<&Curve::ScalarBaseMult>(std::move(k));
}

} // namespace eo::elliptic
