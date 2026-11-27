// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/crypto/elliptic.h>
#include <eo/crypto/rand.h>
#include <eo/errors.h>
#include <eo/io.h>
#include <eo/math/big.h>
#include <eo/struct.h>

#include <cstdint>
#include <string_view>
#include <tuple>

// Go's crypto/ecdsa: the PublicKey and PrivateKey structs and GenerateKey of Go 1.26's ecdsa.go and
// ecdsa_legacy.go, for curves that a program implements. The NIST curves, which Go generates and
// signs with crypto/internal/fips140/ecdsa, are not mapped, nor are the methods of the keys, Sign,
// SignASN1, Verify, VerifyASN1, and the parsing and encoding functions. The keys have no method
// table, so the methods of the curve that they embed are not promoted to them in an interface
// conversion; a program calls them through the field, as priv.PublicKey.Curve.Params().

namespace eo::ecdsa {

// Go's ecdsa.PublicKey.
struct PublicKey {
  static constexpr std::string_view _eo_name = "ecdsa.PublicKey";

  eo::elliptic::Curve Curve{};
  ptr<big::Int> X{}, Y{};

  static constexpr eo::field _eo_fields[] = {{.embedded = true}, {}, {}};

  auto operator==(const PublicKey&) const -> bool = default;
};

// Go's ecdsa.PrivateKey.
struct PrivateKey {
  static constexpr std::string_view _eo_name = "ecdsa.PrivateKey";

  eo::ecdsa::PublicKey PublicKey{};
  ptr<big::Int> D{};

  static constexpr eo::field _eo_fields[] = {{.embedded = true}, {}};

  auto operator==(const PrivateKey&) const -> bool = default;
};

namespace detail {

  // Go's randFieldElement: a random scalar in [1, N-1], read from rand with rejection sampling.
  inline auto randFieldElement(const elliptic::Curve& c, const io::Reader& rand) -> std::tuple<ptr<big::Int>, error> {
    ptr<big::Int> k{};
    error err{};
    for (;;) {
      auto N = c.Params()->N;
      auto b = eo::make_slice<uint8_t>((big::Int::BitLen(N) + 7) / 8);
      std::tie(std::ignore, err) = io::ReadFull(rand, b);
      if (err != nullptr) {
        return {k, err};
      }
      if (auto excess = eo::len(b) * 8 - big::Int::BitLen(N); excess > 0) {
        eo::at(b, 0) >>= static_cast<uint64_t>(excess);
      }
      k = big::Int::SetBytes(New<big::Int>(), b);
      if (big::Int::Sign(k) != 0 && big::Int::Cmp(k, N) < 0) {
        return {k, err};
      }
    }
  }

  // Go's generateLegacy, which GenerateKey uses for a curve that is not a NIST curve.
  inline auto generateLegacy(const elliptic::Curve& c, const io::Reader& rand) -> std::tuple<ptr<PrivateKey>, error> {
    auto [k, err] = randFieldElement(c, rand);
    if (err != nullptr) {
      return {nullptr, err};
    }

    auto priv = New<PrivateKey>();
    priv->PublicKey.Curve = c;
    priv->D = k;
    std::tie(priv->PublicKey.X, priv->PublicKey.Y) = c.ScalarBaseMult(big::Int::Bytes(k));
    return {priv, nullptr};
  }

} // namespace detail

// Go's ecdsa.GenerateKey. As in Go 1.26 with its default GODEBUG, r is ignored and the key comes
// from crypto/rand's default reader.
inline auto GenerateKey(elliptic::Curve c, io::Reader r) -> std::tuple<ptr<PrivateKey>, error> {
  r = rand::detail::CustomReader(r);
  return detail::generateLegacy(c, r);
}

} // namespace eo::ecdsa
