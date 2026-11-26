// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/io.h>
#include <eo/slices.h>
#include <eo/strconv.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

// Go's encoding/base64, ported from Go 1.26's base64.go. Encoding is written as a translated type:
// its methods are static members whose first parameter is the receiver, a ptr<Encoding> except for
// WithPadding and Strict, whose receivers are values. base64.StdEncoding.EncodeToString(b) is
// base64::Encoding::EncodeToString(base64::StdEncoding, b).

namespace eo::base64 {

inline constexpr int32_t StdPadding = '=';
inline constexpr int32_t NoPadding = -1;

namespace detail {

  inline const std::string decodeMapInitialize(256, '\xff');
  inline constexpr uint8_t invalidIndex = 0xff;

  // Go's internal/byteorder.BEPutUint32 and BEPutUint64, which check the last index first.
  inline void bePutUint32(slice<uint8_t> b, uint32_t v) {
    (void)eo::at(b, 3);
    eo::at(b, 0) = static_cast<uint8_t>(v >> 24);
    eo::at(b, 1) = static_cast<uint8_t>(v >> 16);
    eo::at(b, 2) = static_cast<uint8_t>(v >> 8);
    eo::at(b, 3) = static_cast<uint8_t>(v);
  }

  inline void bePutUint64(slice<uint8_t> b, uint64_t v) {
    (void)eo::at(b, 7);
    for (int64_t i = 0; i < 8; i++) {
      eo::at(b, i) = static_cast<uint8_t>(v >> (56 - 8 * i));
    }
  }

  inline auto decodedLen(int64_t n, int32_t padChar) -> int64_t {
    if (padChar == NoPadding) {
      return n / 4 * 3 + n % 4 * 6 / 8;
    }
    return n / 4 * 3;
  }

  inline auto assemble32(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4) -> std::tuple<uint32_t, bool> {
    if ((n1 | n2 | n3 | n4) == 0xff) {
      return {0, false};
    }
    return {uint32_t{n1} << 26 | uint32_t{n2} << 20 | uint32_t{n3} << 14 | uint32_t{n4} << 8, true};
  }

  inline auto assemble64(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4, uint8_t n5, uint8_t n6, uint8_t n7, uint8_t n8)
    -> std::tuple<uint64_t, bool> {
    if ((n1 | n2 | n3 | n4 | n5 | n6 | n7 | n8) == 0xff) {
      return {0, false};
    }
    return {uint64_t{n1} << 58 | uint64_t{n2} << 52 | uint64_t{n3} << 46 | uint64_t{n4} << 40 | uint64_t{n5} << 34 |
        uint64_t{n6} << 28 | uint64_t{n7} << 22 | uint64_t{n8} << 16,
      true};
  }

} // namespace detail

// Go's base64.CorruptInputError, a defined type over int64.
struct CorruptInputError : eo::defined<CorruptInputError, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "base64.CorruptInputError";

  static auto Error(CorruptInputError e) -> std::string {
    return "illegal base64 data at input byte " + strconv::FormatInt(static_cast<int64_t>(e), 10);
  }

  using _eo_methods = eo::methods<eo::method<&CorruptInputError::Error, "Error">>;
};

// Go's base64.Encoding.
struct Encoding {
  static constexpr std::string_view _eo_name = "base64.Encoding";

  std::array<uint8_t, 64> encode{};
  std::array<uint8_t, 256> decodeMap{};
  int32_t padChar{};
  bool strict{};

  auto operator==(const Encoding&) const -> bool = default;

  static auto WithPadding(Encoding _eo_param_enc, int32_t padding) -> ptr<Encoding> {
    auto enc = New<Encoding>(std::move(_eo_param_enc));
    if (padding < NoPadding || padding == '\r' || padding == '\n' || padding > 0xff) {
      panic(std::string{"invalid padding"});
    } else if (padding != NoPadding && eo::at(enc->decodeMap, static_cast<uint8_t>(padding)) != detail::invalidIndex) {
      panic(std::string{"padding contained in alphabet"});
    }
    enc->padChar = padding;
    return enc;
  }

  static auto Strict(Encoding _eo_param_enc) -> ptr<Encoding> {
    auto enc = New<Encoding>(std::move(_eo_param_enc));
    enc->strict = true;
    return enc;
  }

  static void Encode(ptr<Encoding> enc, slice<uint8_t> dst, slice<uint8_t> src) {
    if (eo::len(src) == 0) {
      return;
    }
    (void)enc->encode;

    int64_t di = 0;
    int64_t si = 0;
    auto n = (eo::len(src) / 3) * 3;
    while (si < n) {
      auto val =
        uint64_t{eo::at(src, si + 0)} << 16 | uint64_t{eo::at(src, si + 1)} << 8 | uint64_t{eo::at(src, si + 2)};

      eo::at(dst, di + 0) = eo::at(enc->encode, val >> 18 & 0x3F);
      eo::at(dst, di + 1) = eo::at(enc->encode, val >> 12 & 0x3F);
      eo::at(dst, di + 2) = eo::at(enc->encode, val >> 6 & 0x3F);
      eo::at(dst, di + 3) = eo::at(enc->encode, val & 0x3F);

      si += 3;
      di += 4;
    }

    auto remain = eo::len(src) - si;
    if (remain == 0) {
      return;
    }
    auto val = uint64_t{eo::at(src, si + 0)} << 16;
    if (remain == 2) {
      val |= uint64_t{eo::at(src, si + 1)} << 8;
    }

    eo::at(dst, di + 0) = eo::at(enc->encode, val >> 18 & 0x3F);
    eo::at(dst, di + 1) = eo::at(enc->encode, val >> 12 & 0x3F);

    switch (remain) {
    case 2:
      eo::at(dst, di + 2) = eo::at(enc->encode, val >> 6 & 0x3F);
      if (enc->padChar != NoPadding) {
        eo::at(dst, di + 3) = static_cast<uint8_t>(enc->padChar);
      }
      break;
    case 1:
      if (enc->padChar != NoPadding) {
        eo::at(dst, di + 2) = static_cast<uint8_t>(enc->padChar);
        eo::at(dst, di + 3) = static_cast<uint8_t>(enc->padChar);
      }
      break;
    }
  }

  static auto AppendEncode(ptr<Encoding> enc, slice<uint8_t> dst, slice<uint8_t> src) -> slice<uint8_t> {
    auto n = Encoding::EncodedLen(enc, eo::len(src));
    dst = slices::Grow(dst, n);
    Encoding::Encode(enc, eo::subslice(eo::subslice(dst, eo::len(dst)), 0, n), src);
    return eo::subslice(dst, 0, eo::len(dst) + n);
  }

  static auto EncodeToString(ptr<Encoding> enc, slice<uint8_t> src) -> std::string {
    auto buf = eo::make_slice<uint8_t>(Encoding::EncodedLen(enc, eo::len(src)));
    Encoding::Encode(enc, buf, src);
    return eo::string_from_bytes(buf);
  }

  static auto EncodedLen(ptr<Encoding> enc, int64_t n) -> int64_t {
    if (enc->padChar == NoPadding) {
      return n / 3 * 4 + (n % 3 * 8 + 5) / 6;
    }
    return (n + 2) / 3 * 4;
  }

  static auto decodeQuantum(
    ptr<Encoding> enc, slice<uint8_t> dst, slice<uint8_t> src, int64_t si) -> std::tuple<int64_t, int64_t, error> {
    error err{};
    std::array<uint8_t, 4> dbuf{};
    int64_t dlen = 4;

    (void)enc->decodeMap;

    for (int64_t j = 0; j < eo::len(dbuf); j++) {
      if (eo::len(src) == si) {
        if (j == 0) {
          return {si, 0, nullptr};
        } else if (j == 1 || enc->padChar != NoPadding) {
          return {si, 0, CorruptInputError(si - j)};
        }
        dlen = j;
        break;
      }
      auto in = eo::at(src, si);
      si++;

      auto out = eo::at(enc->decodeMap, in);
      if (out != 0xff) {
        eo::at(dbuf, j) = out;
        continue;
      }

      if (in == '\n' || in == '\r') {
        j--;
        continue;
      }

      if (static_cast<int32_t>(in) != enc->padChar) {
        return {si, 0, CorruptInputError(si - 1)};
      }

      switch (j) {
      case 0:
      case 1:
        return {si, 0, CorruptInputError(si - 1)};
      case 2:
        while (si < eo::len(src) && (eo::at(src, si) == '\n' || eo::at(src, si) == '\r')) {
          si++;
        }
        if (si == eo::len(src)) {
          return {si, 0, CorruptInputError(eo::len(src))};
        }
        if (static_cast<int32_t>(eo::at(src, si)) != enc->padChar) {
          return {si, 0, CorruptInputError(si - 1)};
        }

        si++;
      }

      while (si < eo::len(src) && (eo::at(src, si) == '\n' || eo::at(src, si) == '\r')) {
        si++;
      }
      if (si < eo::len(src)) {
        err = CorruptInputError(si);
      }
      dlen = j;
      break;
    }

    auto val = uint64_t{eo::at(dbuf, 0)} << 18 | uint64_t{eo::at(dbuf, 1)} << 12 | uint64_t{eo::at(dbuf, 2)} << 6 |
      uint64_t{eo::at(dbuf, 3)};
    eo::at(dbuf, 2) = static_cast<uint8_t>(val >> 0);
    eo::at(dbuf, 1) = static_cast<uint8_t>(val >> 8);
    eo::at(dbuf, 0) = static_cast<uint8_t>(val >> 16);
    switch (dlen) {
    case 4:
      eo::at(dst, 2) = eo::at(dbuf, 2);
      eo::at(dbuf, 2) = 0;
      [[fallthrough]];
    case 3:
      eo::at(dst, 1) = eo::at(dbuf, 1);
      if (enc->strict && eo::at(dbuf, 2) != 0) {
        return {si, 0, CorruptInputError(si - 1)};
      }
      eo::at(dbuf, 1) = 0;
      [[fallthrough]];
    case 2:
      eo::at(dst, 0) = eo::at(dbuf, 0);
      if (enc->strict && (eo::at(dbuf, 1) != 0 || eo::at(dbuf, 2) != 0)) {
        return {si, 0, CorruptInputError(si - 2)};
      }
    }

    return {si, dlen - 1, err};
  }

  static auto AppendDecode(
    ptr<Encoding> enc, slice<uint8_t> dst, slice<uint8_t> src) -> std::tuple<slice<uint8_t>, error> {
    auto n = eo::len(src);
    while (n > 0 && static_cast<int32_t>(eo::at(src, n - 1)) == enc->padChar) {
      n--;
    }
    n = detail::decodedLen(n, NoPadding);

    dst = slices::Grow(dst, n);
    auto [m, err] = Encoding::Decode(enc, eo::subslice(eo::subslice(dst, eo::len(dst)), 0, n), src);
    return {eo::subslice(dst, 0, eo::len(dst) + m), err};
  }

  static auto DecodeString(ptr<Encoding> enc, std::string s) -> std::tuple<slice<uint8_t>, error> {
    auto dbuf = eo::make_slice<uint8_t>(Encoding::DecodedLen(enc, eo::len(s)));
    auto [n, err] = Encoding::Decode(enc, dbuf, eo::bytes_from_string(s));
    return {eo::subslice(dbuf, 0, n), err};
  }

  static auto Decode(ptr<Encoding> enc, slice<uint8_t> dst, slice<uint8_t> src) -> std::tuple<int64_t, error> {
    int64_t n{};
    error err{};
    if (eo::len(src) == 0) {
      return {0, nullptr};
    }

    (void)enc->decodeMap;

    int64_t si = 0;
    while (strconv::IntSize >= 64 && eo::len(src) - si >= 8 && eo::len(dst) - n >= 8) {
      auto src2 = eo::subslice(src, si, si + 8);
      if (auto [dn, ok] = detail::assemble64(eo::at(enc->decodeMap, eo::at(src2, 0)),
            eo::at(enc->decodeMap, eo::at(src2, 1)),
            eo::at(enc->decodeMap, eo::at(src2, 2)),
            eo::at(enc->decodeMap, eo::at(src2, 3)),
            eo::at(enc->decodeMap, eo::at(src2, 4)),
            eo::at(enc->decodeMap, eo::at(src2, 5)),
            eo::at(enc->decodeMap, eo::at(src2, 6)),
            eo::at(enc->decodeMap, eo::at(src2, 7)));
          ok) {
        detail::bePutUint64(eo::subslice(dst, n), dn);
        n += 6;
        si += 8;
      } else {
        int64_t ninc{};
        std::tie(si, ninc, err) = Encoding::decodeQuantum(enc, eo::subslice(dst, n), src, si);
        n += ninc;
        if (err != nullptr) {
          return {n, err};
        }
      }
    }

    while (eo::len(src) - si >= 4 && eo::len(dst) - n >= 4) {
      auto src2 = eo::subslice(src, si, si + 4);
      if (auto [dn, ok] = detail::assemble32(eo::at(enc->decodeMap, eo::at(src2, 0)),
            eo::at(enc->decodeMap, eo::at(src2, 1)),
            eo::at(enc->decodeMap, eo::at(src2, 2)),
            eo::at(enc->decodeMap, eo::at(src2, 3)));
          ok) {
        detail::bePutUint32(eo::subslice(dst, n), dn);
        n += 3;
        si += 4;
      } else {
        int64_t ninc{};
        std::tie(si, ninc, err) = Encoding::decodeQuantum(enc, eo::subslice(dst, n), src, si);
        n += ninc;
        if (err != nullptr) {
          return {n, err};
        }
      }
    }

    while (si < eo::len(src)) {
      int64_t ninc{};
      std::tie(si, ninc, err) = Encoding::decodeQuantum(enc, eo::subslice(dst, n), src, si);
      n += ninc;
      if (err != nullptr) {
        return {n, err};
      }
    }
    return {n, err};
  }

  static auto DecodedLen(ptr<Encoding> enc, int64_t n) -> int64_t {
    return detail::decodedLen(n, enc->padChar);
  }

  using _eo_methods = eo::methods<eo::method<&Encoding::WithPadding, "WithPadding">,
    eo::method<&Encoding::Strict, "Strict">,
    eo::method<&Encoding::Encode, "Encode">,
    eo::method<&Encoding::AppendEncode, "AppendEncode">,
    eo::method<&Encoding::EncodeToString, "EncodeToString">,
    eo::method<&Encoding::EncodedLen, "EncodedLen">,
    eo::method<&Encoding::decodeQuantum, "decodeQuantum">,
    eo::method<&Encoding::AppendDecode, "AppendDecode">,
    eo::method<&Encoding::DecodeString, "DecodeString">,
    eo::method<&Encoding::Decode, "Decode">,
    eo::method<&Encoding::DecodedLen, "DecodedLen">>;
};

inline auto NewEncoding(std::string encoder) -> ptr<Encoding> {
  if (eo::len(encoder) != 64) {
    panic(std::string{"encoding alphabet is not 64-bytes long"});
  }

  auto e = New<Encoding>();
  e->padChar = StdPadding;
  eo::copy(eo::subslice(ptr(e, &e->encode), 0), encoder);
  eo::copy(eo::subslice(ptr(e, &e->decodeMap), 0), detail::decodeMapInitialize);

  for (int64_t i = 0; i < eo::len(encoder); i++) {
    if (eo::at(encoder, i) == '\n' || eo::at(encoder, i) == '\r') {
      panic(std::string{"encoding alphabet contains newline character"});
    } else if (eo::at(e->decodeMap, eo::at(encoder, i)) != detail::invalidIndex) {
      panic(std::string{"encoding alphabet includes duplicate symbols"});
    }
    eo::at(e->decodeMap, eo::at(encoder, i)) = static_cast<uint8_t>(i);
  }
  return e;
}

inline ptr<Encoding> StdEncoding = NewEncoding("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
inline ptr<Encoding> URLEncoding = NewEncoding("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");
inline ptr<Encoding> RawStdEncoding = Encoding::WithPadding(*StdEncoding, NoPadding);
inline ptr<Encoding> RawURLEncoding = Encoding::WithPadding(*URLEncoding, NoPadding);

namespace detail {

  struct encoder {
    static constexpr std::string_view _eo_name = "base64.encoder";

    error err{};
    ptr<Encoding> enc{};
    io::Writer w{};
    std::array<uint8_t, 3> buf{};
    int64_t nbuf{};
    std::array<uint8_t, 1024> out{};

    auto operator==(const encoder&) const -> bool = default;

    static auto Write(ptr<encoder> e, slice<uint8_t> p) -> std::tuple<int64_t, error> {
      int64_t n{};
      error err{};
      if (e->err != nullptr) {
        return {0, e->err};
      }

      if (e->nbuf > 0) {
        int64_t i{};
        for (i = 0; i < eo::len(p) && e->nbuf < 3; i++) {
          eo::at(e->buf, e->nbuf) = eo::at(p, i);
          e->nbuf++;
        }
        n += i;
        p = eo::subslice(p, i);
        if (e->nbuf < 3) {
          return {n, err};
        }
        Encoding::Encode(e->enc, eo::subslice(ptr(e, &e->out), 0), eo::subslice(ptr(e, &e->buf), 0));
        if (std::tie(std::ignore, e->err) = e->w.Write(eo::subslice(ptr(e, &e->out), 0, 4)); e->err != nullptr) {
          return {n, e->err};
        }
        e->nbuf = 0;
      }

      while (eo::len(p) >= 3) {
        auto nn = eo::len(e->out) / 4 * 3;
        if (nn > eo::len(p)) {
          nn = eo::len(p);
          nn -= nn % 3;
        }
        Encoding::Encode(e->enc, eo::subslice(ptr(e, &e->out), 0), eo::subslice(p, 0, nn));
        if (std::tie(std::ignore, e->err) = e->w.Write(eo::subslice(ptr(e, &e->out), 0, nn / 3 * 4));
            e->err != nullptr) {
          return {n, e->err};
        }
        n += nn;
        p = eo::subslice(p, nn);
      }

      eo::copy(eo::subslice(ptr(e, &e->buf), 0), p);
      e->nbuf = eo::len(p);
      n += eo::len(p);
      return {n, err};
    }

    static auto Close(ptr<encoder> e) -> error {
      if (e->err == nullptr && e->nbuf > 0) {
        Encoding::Encode(e->enc, eo::subslice(ptr(e, &e->out), 0), eo::subslice(ptr(e, &e->buf), 0, e->nbuf));
        std::tie(std::ignore, e->err) =
          e->w.Write(eo::subslice(ptr(e, &e->out), 0, Encoding::EncodedLen(e->enc, e->nbuf)));
        e->nbuf = 0;
      }
      return e->err;
    }

    using _eo_methods = eo::methods<eo::method<&encoder::Write, "Write">, eo::method<&encoder::Close, "Close">>;
  };

  struct decoder {
    static constexpr std::string_view _eo_name = "base64.decoder";

    error err{};
    error readErr{};
    ptr<Encoding> enc{};
    io::Reader r{};
    std::array<uint8_t, 1024> buf{};
    int64_t nbuf{};
    slice<uint8_t> out{};
    // Go's [1024 / 4 * 3]byte. out is a slice of it, so it is a separate array: a slice of an array
    // field would make the decoder own itself, a cycle that is never freed.
    slice<uint8_t> outbuf{};

    auto operator==(const decoder&) const -> bool = default;

    static auto Read(ptr<decoder> d, slice<uint8_t> p) -> std::tuple<int64_t, error> {
      int64_t n{};
      if (eo::len(d->out) > 0) {
        n = eo::copy(p, d->out);
        d->out = eo::subslice(d->out, n);
        return {n, nullptr};
      }

      if (d->err != nullptr) {
        return {0, d->err};
      }

      while (d->nbuf < 4 && d->readErr == nullptr) {
        auto nn = eo::len(p) / 3 * 4;
        if (nn < 4) {
          nn = 4;
        }
        if (nn > eo::len(d->buf)) {
          nn = eo::len(d->buf);
        }
        std::tie(nn, d->readErr) = d->r.Read(eo::subslice(ptr(d, &d->buf), d->nbuf, nn));
        d->nbuf += nn;
      }

      if (d->nbuf < 4) {
        if (d->enc->padChar == NoPadding && d->nbuf > 0) {
          int64_t nw{};
          std::tie(nw, d->err) = Encoding::Decode(d->enc, d->outbuf, eo::subslice(ptr(d, &d->buf), 0, d->nbuf));
          d->nbuf = 0;
          d->out = eo::subslice(d->outbuf, 0, nw);
          n = eo::copy(p, d->out);
          d->out = eo::subslice(d->out, n);
          if (n > 0 || (eo::len(p) == 0 && eo::len(d->out) > 0)) {
            return {n, nullptr};
          }
          if (d->err != nullptr) {
            return {0, d->err};
          }
        }
        d->err = d->readErr;
        if (d->err == io::EOF_ && d->nbuf > 0) {
          d->err = io::ErrUnexpectedEOF;
        }
        return {0, d->err};
      }

      auto nr = d->nbuf / 4 * 4;
      auto nw = d->nbuf / 4 * 3;
      if (nw > eo::len(p)) {
        std::tie(nw, d->err) = Encoding::Decode(d->enc, d->outbuf, eo::subslice(ptr(d, &d->buf), 0, nr));
        d->out = eo::subslice(d->outbuf, 0, nw);
        n = eo::copy(p, d->out);
        d->out = eo::subslice(d->out, n);
      } else {
        std::tie(n, d->err) = Encoding::Decode(d->enc, p, eo::subslice(ptr(d, &d->buf), 0, nr));
      }
      d->nbuf -= nr;
      eo::copy(eo::subslice(ptr(d, &d->buf), 0, d->nbuf), eo::subslice(ptr(d, &d->buf), nr));
      return {n, d->err};
    }

    using _eo_methods = eo::methods<eo::method<&decoder::Read, "Read">>;
  };

  struct newlineFilteringReader {
    static constexpr std::string_view _eo_name = "base64.newlineFilteringReader";

    io::Reader wrapped{};

    auto operator==(const newlineFilteringReader&) const -> bool = default;

    static auto Read(ptr<newlineFilteringReader> r, slice<uint8_t> p) -> std::tuple<int64_t, error> {
      auto [n, err] = r->wrapped.Read(p);
      while (n > 0) {
        int64_t offset = 0;
        for (auto [i, b] : eo::range(eo::subslice(p, 0, n))) {
          if (b != '\r' && b != '\n') {
            if (i != offset) {
              eo::at(p, offset) = b;
            }
            offset++;
          }
        }
        if (offset > 0) {
          return {offset, err};
        }
        std::tie(n, err) = r->wrapped.Read(p);
      }
      return {n, err};
    }

    using _eo_methods = eo::methods<eo::method<&newlineFilteringReader::Read, "Read">>;
  };

} // namespace detail

inline auto NewEncoder(ptr<Encoding> enc, io::Writer w) -> io::WriteCloser {
  return io::WriteCloser(New<detail::encoder>(detail::encoder{.enc = enc, .w = w}));
}

inline auto NewDecoder(ptr<Encoding> enc, io::Reader r) -> io::Reader {
  return io::Reader(New<detail::decoder>(detail::decoder{.enc = enc,
    .r = io::Reader(New<detail::newlineFilteringReader>(detail::newlineFilteringReader{r})),
    .outbuf = eo::make_slice<uint8_t>(1024 / 4 * 3)}));
}

} // namespace eo::base64
