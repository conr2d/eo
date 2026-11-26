// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/bytes.h>
#include <eo/cmp.h>
#include <eo/core.h>
#include <eo/encoding.h>
#include <eo/encoding/base64.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/io.h>
#include <eo/math.h>
#include <eo/reflect.h>
#include <eo/slices.h>
#include <eo/strconv.h>
#include <eo/strings.h>
#include <eo/sync.h>
#include <eo/unicode.h>
#include <eo/unicode/utf16.h>
#include <eo/unicode/utf8.h>

#include <array>
#include <cstdint>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

// Go's encoding/json, ported from Go 1.26's encoding/json without the jsonv2 experiment: decode.go,
// encode.go, fold.go, indent.go, scanner.go, stream.go, tables.go, and tags.go, in that package's
// order of declarations where C++ allows it. Exported types are translated types, whose methods are
// static members whose first parameter is the receiver: dec.Decode(&v) is
// json::Decoder::Decode(dec, v). The unexported types are internal, and their methods take a raw
// pointer to the receiver, as the scanner is a field of the decoder and of the decoder's state.
//
// The port differs from Go's source where Eo has no form for it:
//
// - The caches of encoders and of struct fields are typed maps under a mutex, as a function value
//   cannot be held in an interface, and so not in a sync.Map. They behave as Go's: a recursive type
//   finds the indirect encoder that typeEncoder stores before it builds the real one.
// - The pools of encode states and scanners are left out, as their reuse is not observable.
// - The cycle check, which Go makes after 1000 nested pointers, maps, or slices, keys its set by the
//   address that reflect::Value::Pointer reports, with the pointer's type for a pointer, as Go keys
//   it by the pointer as an interface value, and with the length for a slice.
// - The encoder's error, which Go raises as a panic of jsonError and recovers in marshal, is a C++
//   exception of its own type, which only marshal catches.
// - The methods named error are error_, as a member named error would hide the type.

namespace eo::json {

struct Number;
struct Marshaler;
struct Unmarshaler;

namespace detail {

  // tables.go: safeSet holds true for the ASCII characters that need no escape in a JSON string, and
  // htmlSafeSet also holds false for <, >, and &.
  inline constexpr auto safeSet = [] {
    auto set = std::array<bool, utf8::RuneSelf>{};
    for (auto c = 0x20; c < utf8::RuneSelf; c++) {
      set[c] = c != '"' && c != '\\';
    }
    return set;
  }();

  inline constexpr auto htmlSafeSet = [] {
    auto set = safeSet;
    set['<'] = false;
    set['>'] = false;
    set['&'] = false;
    return set;
  }();

  inline constexpr std::string_view hex = "0123456789abcdef";

  // tags.go.
  struct tagOptions : eo::defined<tagOptions, std::string> {
    using defined::defined;
    static constexpr std::string_view _eo_name = "json.tagOptions";

    static auto Contains(tagOptions o, std::string optionName) -> bool {
      if (eo::len(std::string{o}) == 0) {
        return false;
      }
      auto s = std::string{o};
      while (s != "") {
        auto name = std::string{};
        std::tie(name, s, std::ignore) = strings::Cut(s, ",");
        if (name == optionName) {
          return true;
        }
      }
      return false;
    }

    using _eo_methods = eo::methods<eo::method<&tagOptions::Contains, "Contains">>;
  };

  inline auto parseTag(std::string tag) -> std::tuple<std::string, tagOptions> {
    auto [name, opt, _] = strings::Cut(std::move(tag), ",");
    return {name, tagOptions{opt}};
  }

  // fold.go.
  inline auto foldRune(int32_t r) -> int32_t {
    for (;;) {
      auto r2 = unicode::SimpleFold(r);
      if (r2 <= r) {
        return r2;
      }
      r = r2;
    }
  }

  inline auto appendFoldedName(slice<uint8_t> out, slice<uint8_t> in) -> slice<uint8_t> {
    for (int64_t i = 0; i < eo::len(in);) {
      if (auto c = eo::at(in, i); c < utf8::RuneSelf) {
        if ('a' <= c && c <= 'z') {
          c -= 'a' - 'A';
        }
        out = eo::append(out, {c});
        i++;
        continue;
      }
      auto [r, n] = utf8::DecodeRune(eo::subslice(in, i));
      out = utf8::AppendRune(out, foldRune(r));
      i += n;
    }
    return out;
  }

  inline auto foldName(slice<uint8_t> in) -> slice<uint8_t> {
    return appendFoldedName(eo::make_slice<uint8_t>(0, 32), in);
  }

} // namespace detail

// scanner.go.

// Go's json.SyntaxError.
struct SyntaxError {
  static constexpr std::string_view _eo_name = "json.SyntaxError";

  std::string msg{};
  int64_t Offset{};

  auto operator==(const SyntaxError&) const -> bool = default;

  static auto Error(ptr<SyntaxError> e) -> std::string {
    return e->msg;
  }

  using _eo_methods = eo::methods<eo::method<&SyntaxError::Error, "Error">>;
};

namespace detail {

  struct scanner;

  using stepFunc = auto (*)(scanner*, uint8_t) -> int64_t;

  inline constexpr int64_t scanContinue = 0;
  inline constexpr int64_t scanBeginLiteral = 1;
  inline constexpr int64_t scanBeginObject = 2;
  inline constexpr int64_t scanObjectKey = 3;
  inline constexpr int64_t scanObjectValue = 4;
  inline constexpr int64_t scanEndObject = 5;
  inline constexpr int64_t scanBeginArray = 6;
  inline constexpr int64_t scanArrayValue = 7;
  inline constexpr int64_t scanEndArray = 8;
  inline constexpr int64_t scanSkipSpace = 9;
  inline constexpr int64_t scanEnd = 10;
  inline constexpr int64_t scanError = 11;

  inline constexpr int64_t parseObjectKey = 0;
  inline constexpr int64_t parseObjectValue = 1;
  inline constexpr int64_t parseArrayValue = 2;

  inline constexpr int64_t maxNestingDepth = 10000;

  auto stateBeginValueOrEmpty(scanner* s, uint8_t c) -> int64_t;
  auto stateBeginValue(scanner* s, uint8_t c) -> int64_t;
  auto stateBeginStringOrEmpty(scanner* s, uint8_t c) -> int64_t;
  auto stateBeginString(scanner* s, uint8_t c) -> int64_t;
  auto stateEndValue(scanner* s, uint8_t c) -> int64_t;
  auto stateEndTop(scanner* s, uint8_t c) -> int64_t;
  auto stateInString(scanner* s, uint8_t c) -> int64_t;
  auto stateInStringEsc(scanner* s, uint8_t c) -> int64_t;
  auto stateInStringEscU(scanner* s, uint8_t c) -> int64_t;
  auto stateInStringEscU1(scanner* s, uint8_t c) -> int64_t;
  auto stateInStringEscU12(scanner* s, uint8_t c) -> int64_t;
  auto stateInStringEscU123(scanner* s, uint8_t c) -> int64_t;
  auto stateNeg(scanner* s, uint8_t c) -> int64_t;
  auto state1(scanner* s, uint8_t c) -> int64_t;
  auto state0(scanner* s, uint8_t c) -> int64_t;
  auto stateDot(scanner* s, uint8_t c) -> int64_t;
  auto stateDot0(scanner* s, uint8_t c) -> int64_t;
  auto stateE(scanner* s, uint8_t c) -> int64_t;
  auto stateESign(scanner* s, uint8_t c) -> int64_t;
  auto stateE0(scanner* s, uint8_t c) -> int64_t;
  auto stateT(scanner* s, uint8_t c) -> int64_t;
  auto stateTr(scanner* s, uint8_t c) -> int64_t;
  auto stateTru(scanner* s, uint8_t c) -> int64_t;
  auto stateF(scanner* s, uint8_t c) -> int64_t;
  auto stateFa(scanner* s, uint8_t c) -> int64_t;
  auto stateFal(scanner* s, uint8_t c) -> int64_t;
  auto stateFals(scanner* s, uint8_t c) -> int64_t;
  auto stateN(scanner* s, uint8_t c) -> int64_t;
  auto stateNu(scanner* s, uint8_t c) -> int64_t;
  auto stateNul(scanner* s, uint8_t c) -> int64_t;
  auto stateError(scanner* s, uint8_t c) -> int64_t;
  auto quoteChar(uint8_t c) -> std::string;

  struct scanner {
    stepFunc step = nullptr;
    bool endTop{};
    slice<int64_t> parseState{};
    error err{};
    int64_t bytes{};

    static void reset(scanner* s) {
      s->step = stateBeginValue;
      s->parseState = eo::subslice(s->parseState, 0, 0);
      s->err = nullptr;
      s->endTop = false;
    }

    static auto eof(scanner* s) -> int64_t {
      if (s->err != nullptr) {
        return scanError;
      }
      if (s->endTop) {
        return scanEnd;
      }
      s->step(s, ' ');
      if (s->endTop) {
        return scanEnd;
      }
      if (s->err == nullptr) {
        s->err = New<SyntaxError>(SyntaxError{"unexpected end of JSON input", s->bytes});
      }
      return scanError;
    }

    static auto pushParseState(scanner* s, uint8_t c, int64_t newParseState, int64_t successState) -> int64_t {
      s->parseState = eo::append(s->parseState, {newParseState});
      if (eo::len(s->parseState) <= maxNestingDepth) {
        return successState;
      }
      return error_(s, c, "exceeded max depth");
    }

    static void popParseState(scanner* s) {
      auto n = eo::len(s->parseState) - 1;
      s->parseState = eo::subslice(s->parseState, 0, n);
      if (n == 0) {
        s->step = stateEndTop;
        s->endTop = true;
      } else {
        s->step = stateEndValue;
      }
    }

    static auto error_(scanner* s, uint8_t c, std::string context) -> int64_t {
      s->step = stateError;
      s->err = New<SyntaxError>(SyntaxError{"invalid character " + quoteChar(c) + " " + context, s->bytes});
      return scanError;
    }
  };

  inline auto newScanner() -> ptr<scanner> {
    auto scan = New<scanner>();
    scan->bytes = 0;
    scanner::reset(scan.get());
    return scan;
  }

  inline auto checkValid(slice<uint8_t> data, scanner* scan) -> error {
    scanner::reset(scan);
    for (auto c : data) {
      scan->bytes++;
      if (scan->step(scan, c) == scanError) {
        return scan->err;
      }
    }
    if (scanner::eof(scan) == scanError) {
      return scan->err;
    }
    return nullptr;
  }

  inline auto isSpace(uint8_t c) -> bool {
    return c <= ' ' && (c == ' ' || c == '\t' || c == '\r' || c == '\n');
  }

  inline auto isHex(uint8_t c) -> bool {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
  }

  inline auto stateBeginValueOrEmpty(scanner* s, uint8_t c) -> int64_t {
    if (isSpace(c)) {
      return scanSkipSpace;
    }
    if (c == ']') {
      return stateEndValue(s, c);
    }
    return stateBeginValue(s, c);
  }

  inline auto stateBeginValue(scanner* s, uint8_t c) -> int64_t {
    if (isSpace(c)) {
      return scanSkipSpace;
    }
    switch (c) {
    case '{':
      s->step = stateBeginStringOrEmpty;
      return scanner::pushParseState(s, c, parseObjectKey, scanBeginObject);
    case '[':
      s->step = stateBeginValueOrEmpty;
      return scanner::pushParseState(s, c, parseArrayValue, scanBeginArray);
    case '"':
      s->step = stateInString;
      return scanBeginLiteral;
    case '-':
      s->step = stateNeg;
      return scanBeginLiteral;
    case '0':
      s->step = state0;
      return scanBeginLiteral;
    case 't':
      s->step = stateT;
      return scanBeginLiteral;
    case 'f':
      s->step = stateF;
      return scanBeginLiteral;
    case 'n':
      s->step = stateN;
      return scanBeginLiteral;
    default:
      break;
    }
    if ('1' <= c && c <= '9') {
      s->step = state1;
      return scanBeginLiteral;
    }
    return scanner::error_(s, c, "looking for beginning of value");
  }

  inline auto stateBeginStringOrEmpty(scanner* s, uint8_t c) -> int64_t {
    if (isSpace(c)) {
      return scanSkipSpace;
    }
    if (c == '}') {
      auto n = eo::len(s->parseState);
      eo::at(s->parseState, n - 1) = parseObjectValue;
      return stateEndValue(s, c);
    }
    return stateBeginString(s, c);
  }

  inline auto stateBeginString(scanner* s, uint8_t c) -> int64_t {
    if (isSpace(c)) {
      return scanSkipSpace;
    }
    if (c == '"') {
      s->step = stateInString;
      return scanBeginLiteral;
    }
    return scanner::error_(s, c, "looking for beginning of object key string");
  }

  inline auto stateEndValue(scanner* s, uint8_t c) -> int64_t {
    auto n = eo::len(s->parseState);
    if (n == 0) {
      s->step = stateEndTop;
      s->endTop = true;
      return stateEndTop(s, c);
    }
    if (isSpace(c)) {
      s->step = stateEndValue;
      return scanSkipSpace;
    }
    auto ps = eo::at(s->parseState, n - 1);
    switch (ps) {
    case parseObjectKey:
      if (c == ':') {
        eo::at(s->parseState, n - 1) = parseObjectValue;
        s->step = stateBeginValue;
        return scanObjectKey;
      }
      return scanner::error_(s, c, "after object key");
    case parseObjectValue:
      if (c == ',') {
        eo::at(s->parseState, n - 1) = parseObjectKey;
        s->step = stateBeginString;
        return scanObjectValue;
      }
      if (c == '}') {
        scanner::popParseState(s);
        return scanEndObject;
      }
      return scanner::error_(s, c, "after object key:value pair");
    case parseArrayValue:
      if (c == ',') {
        s->step = stateBeginValue;
        return scanArrayValue;
      }
      if (c == ']') {
        scanner::popParseState(s);
        return scanEndArray;
      }
      return scanner::error_(s, c, "after array element");
    default:
      break;
    }
    return scanner::error_(s, c, "");
  }

  inline auto stateEndTop(scanner* s, uint8_t c) -> int64_t {
    if (!isSpace(c)) {
      scanner::error_(s, c, "after top-level value");
    }
    return scanEnd;
  }

  inline auto stateInString(scanner* s, uint8_t c) -> int64_t {
    if (c == '"') {
      s->step = stateEndValue;
      return scanContinue;
    }
    if (c == '\\') {
      s->step = stateInStringEsc;
      return scanContinue;
    }
    if (c < 0x20) {
      return scanner::error_(s, c, "in string literal");
    }
    return scanContinue;
  }

  inline auto stateInStringEsc(scanner* s, uint8_t c) -> int64_t {
    switch (c) {
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
    case '\\':
    case '/':
    case '"':
      s->step = stateInString;
      return scanContinue;
    case 'u':
      s->step = stateInStringEscU;
      return scanContinue;
    default:
      break;
    }
    return scanner::error_(s, c, "in string escape code");
  }

  inline auto stateInStringEscU(scanner* s, uint8_t c) -> int64_t {
    if (isHex(c)) {
      s->step = stateInStringEscU1;
      return scanContinue;
    }
    return scanner::error_(s, c, "in \\u hexadecimal character escape");
  }

  inline auto stateInStringEscU1(scanner* s, uint8_t c) -> int64_t {
    if (isHex(c)) {
      s->step = stateInStringEscU12;
      return scanContinue;
    }
    return scanner::error_(s, c, "in \\u hexadecimal character escape");
  }

  inline auto stateInStringEscU12(scanner* s, uint8_t c) -> int64_t {
    if (isHex(c)) {
      s->step = stateInStringEscU123;
      return scanContinue;
    }
    return scanner::error_(s, c, "in \\u hexadecimal character escape");
  }

  inline auto stateInStringEscU123(scanner* s, uint8_t c) -> int64_t {
    if (isHex(c)) {
      s->step = stateInString;
      return scanContinue;
    }
    return scanner::error_(s, c, "in \\u hexadecimal character escape");
  }

  inline auto stateNeg(scanner* s, uint8_t c) -> int64_t {
    if (c == '0') {
      s->step = state0;
      return scanContinue;
    }
    if ('1' <= c && c <= '9') {
      s->step = state1;
      return scanContinue;
    }
    return scanner::error_(s, c, "in numeric literal");
  }

  inline auto state1(scanner* s, uint8_t c) -> int64_t {
    if ('0' <= c && c <= '9') {
      s->step = state1;
      return scanContinue;
    }
    return state0(s, c);
  }

  inline auto state0(scanner* s, uint8_t c) -> int64_t {
    if (c == '.') {
      s->step = stateDot;
      return scanContinue;
    }
    if (c == 'e' || c == 'E') {
      s->step = stateE;
      return scanContinue;
    }
    return stateEndValue(s, c);
  }

  inline auto stateDot(scanner* s, uint8_t c) -> int64_t {
    if ('0' <= c && c <= '9') {
      s->step = stateDot0;
      return scanContinue;
    }
    return scanner::error_(s, c, "after decimal point in numeric literal");
  }

  inline auto stateDot0(scanner* s, uint8_t c) -> int64_t {
    if ('0' <= c && c <= '9') {
      return scanContinue;
    }
    if (c == 'e' || c == 'E') {
      s->step = stateE;
      return scanContinue;
    }
    return stateEndValue(s, c);
  }

  inline auto stateE(scanner* s, uint8_t c) -> int64_t {
    if (c == '+' || c == '-') {
      s->step = stateESign;
      return scanContinue;
    }
    return stateESign(s, c);
  }

  inline auto stateESign(scanner* s, uint8_t c) -> int64_t {
    if ('0' <= c && c <= '9') {
      s->step = stateE0;
      return scanContinue;
    }
    return scanner::error_(s, c, "in exponent of numeric literal");
  }

  inline auto stateE0(scanner* s, uint8_t c) -> int64_t {
    if ('0' <= c && c <= '9') {
      return scanContinue;
    }
    return stateEndValue(s, c);
  }

  // The states of the literals true, false, and null: each expects one byte and moves to the next.
  inline auto literalStep(scanner* s, uint8_t c, uint8_t want, stepFunc next, const char* context) -> int64_t {
    if (c == want) {
      s->step = next;
      return scanContinue;
    }
    return scanner::error_(s, c, context);
  }

  inline auto stateT(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'r', stateTr, "in literal true (expecting 'r')");
  }

  inline auto stateTr(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'u', stateTru, "in literal true (expecting 'u')");
  }

  inline auto stateTru(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'e', stateEndValue, "in literal true (expecting 'e')");
  }

  inline auto stateF(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'a', stateFa, "in literal false (expecting 'a')");
  }

  inline auto stateFa(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'l', stateFal, "in literal false (expecting 'l')");
  }

  inline auto stateFal(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 's', stateFals, "in literal false (expecting 's')");
  }

  inline auto stateFals(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'e', stateEndValue, "in literal false (expecting 'e')");
  }

  inline auto stateN(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'u', stateNu, "in literal null (expecting 'u')");
  }

  inline auto stateNu(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'l', stateNul, "in literal null (expecting 'l')");
  }

  inline auto stateNul(scanner* s, uint8_t c) -> int64_t {
    return literalStep(s, c, 'l', stateEndValue, "in literal null (expecting 'l')");
  }

  inline auto stateError(scanner*, uint8_t) -> int64_t {
    return scanError;
  }

  inline auto quoteChar(uint8_t c) -> std::string {
    if (c == '\'') {
      return R"('\'')";
    }
    if (c == '"') {
      return R"('"')";
    }
    auto s = strconv::Quote(eo::string_from_rune(int32_t{c}));
    return "'" + eo::substr(s, 1, eo::len(s) - 1) + "'";
  }

} // namespace detail

// Go's json.Valid.
inline auto Valid(slice<uint8_t> data) -> bool {
  auto scan = detail::newScanner();
  return detail::checkValid(data, scan.get()) == nullptr;
}

// indent.go.

namespace detail {

  inline auto appendHTMLEscape(slice<uint8_t> dst, slice<uint8_t> src) -> slice<uint8_t> {
    int64_t start = 0;
    for (int64_t i = 0; i < eo::len(src); i++) {
      auto c = eo::at(src, i);
      if (c == '<' || c == '>' || c == '&') {
        dst = eo::append_slice(dst, eo::subslice(src, start, i));
        dst = eo::append(dst, {'\\', 'u', '0', '0', uint8_t(hex[c >> 4]), uint8_t(hex[c & 0xF])});
        start = i + 1;
      }
      if (c == 0xE2 && i + 2 < eo::len(src) && eo::at(src, i + 1) == 0x80 && (eo::at(src, i + 2) & ~1) == 0xA8) {
        dst = eo::append_slice(dst, eo::subslice(src, start, i));
        dst = eo::append(dst, {'\\', 'u', '2', '0', '2', uint8_t(hex[eo::at(src, i + 2) & 0xF])});
        start = i + 3;
      }
    }
    return eo::append_slice(dst, eo::subslice(src, start));
  }

  inline auto appendCompact(slice<uint8_t> dst, slice<uint8_t> src, bool escape) -> std::tuple<slice<uint8_t>, error> {
    auto origLen = eo::len(dst);
    auto scan = newScanner();
    int64_t start = 0;
    for (int64_t i = 0; i < eo::len(src); i++) {
      auto c = eo::at(src, i);
      if (escape && (c == '<' || c == '>' || c == '&')) {
        if (start < i) {
          dst = eo::append_slice(dst, eo::subslice(src, start, i));
        }
        dst = eo::append(dst, {'\\', 'u', '0', '0', uint8_t(hex[c >> 4]), uint8_t(hex[c & 0xF])});
        start = i + 1;
      }
      if (escape && c == 0xE2 && i + 2 < eo::len(src) && eo::at(src, i + 1) == 0x80 &&
        (eo::at(src, i + 2) & ~1) == 0xA8) {
        if (start < i) {
          dst = eo::append_slice(dst, eo::subslice(src, start, i));
        }
        dst = eo::append(dst, {'\\', 'u', '2', '0', '2', uint8_t(hex[eo::at(src, i + 2) & 0xF])});
        start = i + 3;
      }
      auto v = scan->step(scan.get(), c);
      if (v >= scanSkipSpace) {
        if (v == scanError) {
          break;
        }
        if (start < i) {
          dst = eo::append_slice(dst, eo::subslice(src, start, i));
        }
        start = i + 1;
      }
    }
    if (scanner::eof(scan.get()) == scanError) {
      return {eo::subslice(dst, 0, origLen), scan->err};
    }
    if (start < eo::len(src)) {
      dst = eo::append_slice(dst, eo::subslice(src, start));
    }
    return {dst, nullptr};
  }

  inline auto appendNewline(
    slice<uint8_t> dst, const std::string& prefix, const std::string& indent, int64_t depth) -> slice<uint8_t> {
    dst = eo::append(dst, {'\n'});
    dst = eo::append_slice(dst, prefix);
    for (int64_t i = 0; i < depth; i++) {
      dst = eo::append_slice(dst, indent);
    }
    return dst;
  }

  inline constexpr int64_t indentGrowthFactor = 2;

  inline auto appendIndent(slice<uint8_t> dst,
    slice<uint8_t> src,
    std::string prefix,
    std::string indent) -> std::tuple<slice<uint8_t>, error> {
    auto origLen = eo::len(dst);
    auto scan = newScanner();
    auto needIndent = false;
    int64_t depth = 0;
    for (auto c : src) {
      scan->bytes++;
      auto v = scan->step(scan.get(), c);
      if (v == scanSkipSpace) {
        continue;
      }
      if (v == scanError) {
        break;
      }
      if (needIndent && v != scanEndObject && v != scanEndArray) {
        needIndent = false;
        depth++;
        dst = appendNewline(dst, prefix, indent, depth);
      }
      if (v == scanContinue) {
        dst = eo::append(dst, {c});
        continue;
      }
      switch (c) {
      case '{':
      case '[':
        needIndent = true;
        dst = eo::append(dst, {c});
        break;
      case ',':
        dst = eo::append(dst, {c});
        dst = appendNewline(dst, prefix, indent, depth);
        break;
      case ':':
        dst = eo::append(dst, {c, ' '});
        break;
      case '}':
      case ']':
        if (needIndent) {
          needIndent = false;
        } else {
          depth--;
          dst = appendNewline(dst, prefix, indent, depth);
        }
        dst = eo::append(dst, {c});
        break;
      default:
        dst = eo::append(dst, {c});
        break;
      }
    }
    if (scanner::eof(scan.get()) == scanError) {
      return {eo::subslice(dst, 0, origLen), scan->err};
    }
    return {dst, nullptr};
  }

} // namespace detail

// Go's json.HTMLEscape.
inline void HTMLEscape(ptr<bytes::Buffer> dst, slice<uint8_t> src) {
  bytes::Buffer::Grow(dst, eo::len(src));
  bytes::Buffer::Write(dst, detail::appendHTMLEscape(bytes::Buffer::AvailableBuffer(dst), src));
}

// Go's json.Compact.
inline auto Compact(ptr<bytes::Buffer> dst, slice<uint8_t> src) -> error {
  bytes::Buffer::Grow(dst, eo::len(src));
  auto b = bytes::Buffer::AvailableBuffer(dst);
  auto [out, err] = detail::appendCompact(b, src, false);
  bytes::Buffer::Write(dst, out);
  return err;
}

// Go's json.Indent.
inline auto Indent(ptr<bytes::Buffer> dst, slice<uint8_t> src, std::string prefix, std::string indent) -> error {
  bytes::Buffer::Grow(dst, detail::indentGrowthFactor * eo::len(src));
  auto b = bytes::Buffer::AvailableBuffer(dst);
  auto [out, err] = detail::appendIndent(b, src, std::move(prefix), std::move(indent));
  bytes::Buffer::Write(dst, out);
  return err;
}

// encode.go.

// Go's json.Number, from decode.go, which the encoder names too.
struct Number : eo::defined<Number, std::string> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "json.Number";

  static auto String(Number n) -> std::string {
    return std::string{n};
  }

  static auto Float64(Number n) -> std::tuple<double, error> {
    return strconv::ParseFloat(std::string{n}, 64);
  }

  static auto Int64(Number n) -> std::tuple<int64_t, error> {
    return strconv::ParseInt(std::string{n}, 10, 64);
  }

  using _eo_methods = eo::methods<eo::method<&Number::Float64, "Float64">,
    eo::method<&Number::Int64, "Int64">,
    eo::method<&Number::String, "String">>;
};

// Go's json.Marshaler.
struct Marshaler : eo::interface<Marshaler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "json.Marshaler";

  auto MarshalJSON() const -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&Marshaler::MarshalJSON, "MarshalJSON">>;
};

inline auto Marshaler::MarshalJSON() const -> std::tuple<slice<uint8_t>, error> {
  return _eo_call<&Marshaler::MarshalJSON>();
}

// Go's json.UnsupportedTypeError.
struct UnsupportedTypeError {
  static constexpr std::string_view _eo_name = "json.UnsupportedTypeError";

  reflect::Type Type{};

  auto operator==(const UnsupportedTypeError&) const -> bool = default;

  static auto Error(ptr<UnsupportedTypeError> e) -> std::string {
    return "json: unsupported type: " + e->Type.String();
  }

  using _eo_methods = eo::methods<eo::method<&UnsupportedTypeError::Error, "Error">>;
};

// Go's json.UnsupportedValueError.
struct UnsupportedValueError {
  static constexpr std::string_view _eo_name = "json.UnsupportedValueError";

  reflect::Value Value{};
  std::string Str{};

  static auto Error(ptr<UnsupportedValueError> e) -> std::string {
    return "json: unsupported value: " + e->Str;
  }

  using _eo_methods = eo::methods<eo::method<&UnsupportedValueError::Error, "Error">>;
};

// Go's json.InvalidUTF8Error.
struct InvalidUTF8Error {
  static constexpr std::string_view _eo_name = "json.InvalidUTF8Error";

  std::string S{};

  auto operator==(const InvalidUTF8Error&) const -> bool = default;

  static auto Error(ptr<InvalidUTF8Error> e) -> std::string {
    return "json: invalid UTF-8 in string: " + strconv::Quote(e->S);
  }

  using _eo_methods = eo::methods<eo::method<&InvalidUTF8Error::Error, "Error">>;
};

// Go's json.MarshalerError.
struct MarshalerError {
  static constexpr std::string_view _eo_name = "json.MarshalerError";

  reflect::Type Type{};
  error Err{};
  std::string sourceFunc{};

  auto operator==(const MarshalerError&) const -> bool = default;

  static auto Error(ptr<MarshalerError> e) -> std::string {
    auto srcFunc = e->sourceFunc;
    if (srcFunc == "") {
      srcFunc = "MarshalJSON";
    }
    return "json: error calling " + srcFunc + " for type " + e->Type.String() + ": " + e->Err.Error();
  }

  static auto Unwrap(ptr<MarshalerError> e) -> error {
    return e->Err;
  }

  using _eo_methods =
    eo::methods<eo::method<&MarshalerError::Error, "Error">, eo::method<&MarshalerError::Unwrap, "Unwrap">>;
};

namespace detail {

  inline auto isIntKind(reflect::Kind k) -> bool {
    return k == reflect::Int || k == reflect::Int8 || k == reflect::Int16 || k == reflect::Int32 || k == reflect::Int64;
  }

  inline auto isUintKind(reflect::Kind k) -> bool {
    return k == reflect::Uint || k == reflect::Uint8 || k == reflect::Uint16 || k == reflect::Uint32 ||
      k == reflect::Uint64 || k == reflect::Uintptr;
  }

  // The panic of jsonError, which marshal recovers.
  struct jsonError {
    error err;
  };

  struct encOpts {
    bool quoted{};
    bool escapeHTML{};
  };

  struct encodeState;

  using encoderFunc = fn<void(ptr<encodeState>, reflect::Value, encOpts)>;

  inline constexpr uint64_t startDetectingCyclesAfter = 1000;

  struct encodeState {
    bytes::Buffer Buffer{};

    uint64_t ptrLevel{};
    std::set<std::tuple<const void*, uintptr_t, int64_t>> ptrSeen{};

    // The methods of the embedded bytes.Buffer.
    static auto buffer(const ptr<encodeState>& e) -> ptr<bytes::Buffer> {
      return ptr<bytes::Buffer>(e, &e->Buffer);
    }

    static void WriteString(const ptr<encodeState>& e, std::string s) {
      bytes::Buffer::WriteString(buffer(e), std::move(s));
    }

    static void WriteByte(const ptr<encodeState>& e, uint8_t c) {
      bytes::Buffer::WriteByte(buffer(e), c);
    }

    static void Write(const ptr<encodeState>& e, slice<uint8_t> b) {
      bytes::Buffer::Write(buffer(e), b);
    }

    static void Grow(const ptr<encodeState>& e, int64_t n) {
      bytes::Buffer::Grow(buffer(e), n);
    }

    static auto AvailableBuffer(const ptr<encodeState>& e) -> slice<uint8_t> {
      return bytes::Buffer::AvailableBuffer(buffer(e));
    }

    static auto Bytes(const ptr<encodeState>& e) -> slice<uint8_t> {
      return bytes::Buffer::Bytes(buffer(e));
    }

    static auto marshal(const ptr<encodeState>& e, const any& v, encOpts opts) -> error;

    [[noreturn]] static void error_(const ptr<encodeState>&, error err) {
      throw jsonError{std::move(err)};
    }

    static void reflectValue(const ptr<encodeState>& e, reflect::Value v, encOpts opts);
  };

  // Removes a pointer, map, or slice from ptrSeen when its encoder returns, as Go's deferred delete
  // does.
  struct seenGuard {
    encodeState* e = nullptr;
    std::tuple<const void*, uintptr_t, int64_t> key{};

    seenGuard() = default;
    seenGuard(const seenGuard&) = delete;
    auto operator=(const seenGuard&) -> seenGuard& = delete;

    ~seenGuard() {
      if (e != nullptr) {
        e->ptrSeen.erase(key);
      }
    }

    void enter(const ptr<encodeState>& state, reflect::Value v, std::tuple<const void*, uintptr_t, int64_t> k) {
      if (state->ptrSeen.contains(k)) {
        encodeState::error_(state,
          New<UnsupportedValueError>(
            UnsupportedValueError{v, "encountered a cycle via " + reflect::Value::Type(v).String()}));
      }
      state->ptrSeen.insert(k);
      e = state.get();
      key = k;
    }
  };

  inline auto newEncodeState() -> ptr<encodeState> {
    return New<encodeState>();
  }

  inline auto encodeState::marshal(const ptr<encodeState>& e, const any& v, encOpts opts) -> error {
    try {
      reflectValue(e, reflect::ValueOf(v), opts);
    } catch (const jsonError& je) {
      return je.err;
    }
    return nullptr;
  }

  inline auto isEmptyValue(reflect::Value v) -> bool {
    auto k = reflect::Value::Kind(v);
    if (k == reflect::Array || k == reflect::Map || k == reflect::Slice || k == reflect::String) {
      return reflect::Value::Len(v) == 0;
    }
    if (k == reflect::Bool || isIntKind(k) || isUintKind(k) || k == reflect::Float32 || k == reflect::Float64 ||
      k == reflect::Interface || k == reflect::Pointer) {
      return reflect::Value::IsZero(v);
    }
    return false;
  }

  auto valueEncoder(reflect::Value v) -> encoderFunc;
  auto typeEncoder(reflect::Type t) -> encoderFunc;
  auto newTypeEncoder(reflect::Type t, bool allowAddr) -> encoderFunc;

  inline void encodeState::reflectValue(const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
    valueEncoder(v)(e, v, opts);
  }

  // A sync.Map keyed by reflect.Type, for values that an interface cannot hold.
  template<typename V>
  struct typeCache {
    std::mutex mu;
    std::unordered_map<const void*, V> m;

    auto Load(reflect::Type t) -> std::tuple<V, bool> {
      auto lock = std::lock_guard{mu};
      if (auto it = m.find(t._eo_key()); it != m.end()) {
        return {it->second, true};
      }
      return {V{}, false};
    }

    auto LoadOrStore(reflect::Type t, V value) -> std::tuple<V, bool> {
      auto lock = std::lock_guard{mu};
      auto [it, inserted] = m.try_emplace(t._eo_key(), std::move(value));
      return {it->second, !inserted};
    }

    void Store(reflect::Type t, V value) {
      auto lock = std::lock_guard{mu};
      m.insert_or_assign(t._eo_key(), std::move(value));
    }
  };

  inline typeCache<encoderFunc> encoderCache;

  void invalidValueEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);

  inline auto valueEncoder(reflect::Value v) -> encoderFunc {
    if (!reflect::Value::IsValid(v)) {
      return invalidValueEncoder;
    }
    return typeEncoder(reflect::Value::Type(v));
  }

  inline auto typeEncoder(reflect::Type t) -> encoderFunc {
    if (auto [fi, ok] = encoderCache.Load(t); ok) {
      return fi;
    }

    auto indirect = sync::OnceValue([t]() -> encoderFunc { return newTypeEncoder(t, true); });
    auto [fi, loaded] = encoderCache.LoadOrStore(t,
      encoderFunc([indirect](ptr<encodeState> e, reflect::Value v, encOpts opts) { indirect()(e, v, opts); }));
    if (loaded) {
      return fi;
    }

    auto f = indirect();
    encoderCache.Store(t, f);
    return f;
  }

  struct isZeroer : eo::interface<isZeroer> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "json.isZeroer";

    auto IsZero() const -> bool;

    using _eo_methods = eo::methods<eo::method<&isZeroer::IsZero, "IsZero">>;
  };

  inline auto isZeroer::IsZero() const -> bool {
    return _eo_call<&isZeroer::IsZero>();
  }

  inline const reflect::Type marshalerType = reflect::TypeFor<Marshaler>();
  inline const reflect::Type textMarshalerType = reflect::TypeFor<encoding::TextMarshaler>();
  inline const reflect::Type numberType = reflect::TypeFor<Number>();
  inline const reflect::Type isZeroerType = reflect::TypeFor<isZeroer>();

  void marshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void addrMarshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void textMarshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void addrTextMarshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void boolEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void intEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void uintEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void float32Encoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void float64Encoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void stringEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void interfaceEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void unsupportedTypeEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts);
  void encodeByteSlice(ptr<encodeState> e, reflect::Value v, encOpts opts);
  auto newStructEncoder(reflect::Type t) -> encoderFunc;
  auto newMapEncoder(reflect::Type t) -> encoderFunc;
  auto newSliceEncoder(reflect::Type t) -> encoderFunc;
  auto newArrayEncoder(reflect::Type t) -> encoderFunc;
  auto newPtrEncoder(reflect::Type t) -> encoderFunc;
  auto newCondAddrEncoder(encoderFunc canAddrEnc, encoderFunc elseEnc) -> encoderFunc;

  inline auto newTypeEncoder(reflect::Type t, bool allowAddr) -> encoderFunc {
    if (t.Kind() != reflect::Pointer && allowAddr && reflect::PointerTo(t).Implements(marshalerType)) {
      return newCondAddrEncoder(addrMarshalerEncoder, newTypeEncoder(t, false));
    }
    if (t.Implements(marshalerType)) {
      return marshalerEncoder;
    }
    if (t.Kind() != reflect::Pointer && allowAddr && reflect::PointerTo(t).Implements(textMarshalerType)) {
      return newCondAddrEncoder(addrTextMarshalerEncoder, newTypeEncoder(t, false));
    }
    if (t.Implements(textMarshalerType)) {
      return textMarshalerEncoder;
    }

    auto k = t.Kind();
    if (k == reflect::Bool) {
      return boolEncoder;
    }
    if (isIntKind(k)) {
      return intEncoder;
    }
    if (isUintKind(k)) {
      return uintEncoder;
    }
    if (k == reflect::Float32) {
      return float32Encoder;
    }
    if (k == reflect::Float64) {
      return float64Encoder;
    }
    if (k == reflect::String) {
      return stringEncoder;
    }
    if (k == reflect::Interface) {
      return interfaceEncoder;
    }
    if (k == reflect::Struct) {
      return newStructEncoder(t);
    }
    if (k == reflect::Map) {
      return newMapEncoder(t);
    }
    if (k == reflect::Slice) {
      return newSliceEncoder(t);
    }
    if (k == reflect::Array) {
      return newArrayEncoder(t);
    }
    if (k == reflect::Pointer) {
      return newPtrEncoder(t);
    }
    return unsupportedTypeEncoder;
  }

  inline void invalidValueEncoder(ptr<encodeState> e, reflect::Value, encOpts) {
    encodeState::WriteString(e, "null");
  }

  template<typename Bytes>
  auto appendString(slice<uint8_t> dst, const Bytes& src, bool escapeHTML) -> slice<uint8_t>;

  inline auto mayAppendQuote(slice<uint8_t> b, bool quoted) -> slice<uint8_t> {
    if (quoted) {
      b = eo::append(b, {'"'});
    }
    return b;
  }

  inline void marshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    if (reflect::Value::Kind(v) == reflect::Pointer && reflect::Value::IsNil(v)) {
      encodeState::WriteString(e, "null");
      return;
    }
    auto [m, ok] = reflect::TypeAssert<Marshaler>(v);
    if (!ok) {
      encodeState::WriteString(e, "null");
      return;
    }
    auto [b, err] = m.MarshalJSON();
    if (err == nullptr) {
      encodeState::Grow(e, eo::len(b));
      auto out = encodeState::AvailableBuffer(e);
      std::tie(out, err) = appendCompact(out, b, opts.escapeHTML);
      encodeState::Write(e, out);
    }
    if (err != nullptr) {
      encodeState::error_(e, New<MarshalerError>(MarshalerError{reflect::Value::Type(v), err, "MarshalJSON"}));
    }
  }

  inline void addrMarshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    auto va = reflect::Value::Addr(v);
    if (reflect::Value::IsNil(va)) {
      encodeState::WriteString(e, "null");
      return;
    }
    auto [m, _] = reflect::TypeAssert<Marshaler>(va);
    auto [b, err] = m.MarshalJSON();
    if (err == nullptr) {
      encodeState::Grow(e, eo::len(b));
      auto out = encodeState::AvailableBuffer(e);
      std::tie(out, err) = appendCompact(out, b, opts.escapeHTML);
      encodeState::Write(e, out);
    }
    if (err != nullptr) {
      encodeState::error_(e, New<MarshalerError>(MarshalerError{reflect::Value::Type(v), err, "MarshalJSON"}));
    }
  }

  inline void textMarshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    if (reflect::Value::Kind(v) == reflect::Pointer && reflect::Value::IsNil(v)) {
      encodeState::WriteString(e, "null");
      return;
    }
    auto [m, ok] = reflect::TypeAssert<encoding::TextMarshaler>(v);
    if (!ok) {
      encodeState::WriteString(e, "null");
      return;
    }
    auto [b, err] = m.MarshalText();
    if (err != nullptr) {
      encodeState::error_(e, New<MarshalerError>(MarshalerError{reflect::Value::Type(v), err, "MarshalText"}));
    }
    encodeState::Write(e, appendString(encodeState::AvailableBuffer(e), b, opts.escapeHTML));
  }

  inline void addrTextMarshalerEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    auto va = reflect::Value::Addr(v);
    if (reflect::Value::IsNil(va)) {
      encodeState::WriteString(e, "null");
      return;
    }
    auto [m, _] = reflect::TypeAssert<encoding::TextMarshaler>(va);
    auto [b, err] = m.MarshalText();
    if (err != nullptr) {
      encodeState::error_(e, New<MarshalerError>(MarshalerError{reflect::Value::Type(v), err, "MarshalText"}));
    }
    encodeState::Write(e, appendString(encodeState::AvailableBuffer(e), b, opts.escapeHTML));
  }

  inline void boolEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    auto b = encodeState::AvailableBuffer(e);
    b = mayAppendQuote(b, opts.quoted);
    b = strconv::AppendBool(b, reflect::Value::Bool(v));
    b = mayAppendQuote(b, opts.quoted);
    encodeState::Write(e, b);
  }

  inline void intEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    auto b = encodeState::AvailableBuffer(e);
    b = mayAppendQuote(b, opts.quoted);
    b = strconv::AppendInt(b, reflect::Value::Int(v), 10);
    b = mayAppendQuote(b, opts.quoted);
    encodeState::Write(e, b);
  }

  inline void uintEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    auto b = encodeState::AvailableBuffer(e);
    b = mayAppendQuote(b, opts.quoted);
    b = strconv::AppendUint(b, reflect::Value::Uint(v), 10);
    b = mayAppendQuote(b, opts.quoted);
    encodeState::Write(e, b);
  }

  // Go's floatEncoder, the number of bits, whose encode method is the encoder of its size.
  inline void floatEncode(int64_t bits, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
    auto f = reflect::Value::Float(v);
    if (math::IsInf(f, 0) || math::IsNaN(f)) {
      encodeState::error_(e,
        New<UnsupportedValueError>(UnsupportedValueError{v, strconv::FormatFloat(f, 'g', -1, bits)}));
    }

    auto b = encodeState::AvailableBuffer(e);
    b = mayAppendQuote(b, opts.quoted);
    auto abs = math::Abs(f);
    auto fmt_ = uint8_t{'f'};
    if (abs != 0) {
      if ((bits == 64 && (abs < 1e-6 || abs >= 1e21)) ||
        (bits == 32 && (static_cast<float>(abs) < 1e-6F || static_cast<float>(abs) >= 1e21F))) {
        fmt_ = 'e';
      }
    }
    b = strconv::AppendFloat(b, f, fmt_, -1, bits);
    if (fmt_ == 'e') {
      auto n = eo::len(b);
      if (n >= 4 && eo::at(b, n - 4) == 'e' && eo::at(b, n - 3) == '-' && eo::at(b, n - 2) == '0') {
        eo::at(b, n - 2) = eo::at(b, n - 1);
        b = eo::subslice(b, 0, n - 1);
      }
    }
    b = mayAppendQuote(b, opts.quoted);
    encodeState::Write(e, b);
  }

  inline void float32Encoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    floatEncode(32, e, std::move(v), opts);
  }

  inline void float64Encoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    floatEncode(64, e, std::move(v), opts);
  }

  inline auto isValidNumber(std::string_view s) -> bool {
    if (s.empty()) {
      return false;
    }

    if (s[0] == '-') {
      s = s.substr(1);
      if (s.empty()) {
        return false;
      }
    }

    if (s[0] == '0') {
      s = s.substr(1);
    } else if ('1' <= s[0] && s[0] <= '9') {
      s = s.substr(1);
      while (!s.empty() && '0' <= s[0] && s[0] <= '9') {
        s = s.substr(1);
      }
    } else {
      return false;
    }

    if (s.size() >= 2 && s[0] == '.' && '0' <= s[1] && s[1] <= '9') {
      s = s.substr(2);
      while (!s.empty() && '0' <= s[0] && s[0] <= '9') {
        s = s.substr(1);
      }
    }

    if (s.size() >= 2 && (s[0] == 'e' || s[0] == 'E')) {
      s = s.substr(1);
      if (s[0] == '+' || s[0] == '-') {
        s = s.substr(1);
        if (s.empty()) {
          return false;
        }
      }
      while (!s.empty() && '0' <= s[0] && s[0] <= '9') {
        s = s.substr(1);
      }
    }

    return s.empty();
  }

  inline void stringEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    if (reflect::Value::Type(v) == numberType) {
      auto numStr = reflect::Value::String(v);
      if (numStr == "") {
        numStr = "0";
      }
      if (!isValidNumber(numStr)) {
        encodeState::error_(e, fmt::Errorf("json: invalid number literal %q", numStr));
      }
      auto b = encodeState::AvailableBuffer(e);
      b = mayAppendQuote(b, opts.quoted);
      b = eo::append_slice(b, numStr);
      b = mayAppendQuote(b, opts.quoted);
      encodeState::Write(e, b);
      return;
    }
    if (opts.quoted) {
      auto b = appendString(slice<uint8_t>{}, reflect::Value::String(v), opts.escapeHTML);
      encodeState::Write(e, appendString(encodeState::AvailableBuffer(e), b, false));
    } else {
      encodeState::Write(e, appendString(encodeState::AvailableBuffer(e), reflect::Value::String(v), opts.escapeHTML));
    }
  }

  inline void interfaceEncoder(ptr<encodeState> e, reflect::Value v, encOpts opts) {
    if (reflect::Value::IsNil(v)) {
      encodeState::WriteString(e, "null");
      return;
    }
    encodeState::reflectValue(e, reflect::Value::Elem(v), opts);
  }

  inline void unsupportedTypeEncoder(ptr<encodeState> e, reflect::Value v, encOpts) {
    encodeState::error_(e, New<UnsupportedTypeError>(UnsupportedTypeError{reflect::Value::Type(v)}));
  }

  struct field {
    std::string name{};
    slice<uint8_t> nameBytes{};

    std::string nameNonEsc{};
    std::string nameEscHTML{};

    bool tag{};
    slice<int64_t> index{};
    reflect::Type typ{};
    bool omitEmpty{};
    bool omitZero{};
    fn<bool(reflect::Value)> isZero{};
    bool quoted{};

    encoderFunc encoder{};
  };

  struct structFields {
    slice<field> list{};
    map<std::string, ptr<field>> byExactName{};
    map<std::string, ptr<field>> byFoldedName{};
  };

  auto cachedTypeFields(reflect::Type t) -> structFields;

  struct structEncoder {
    structFields fields;

    static void encode(const structEncoder& se, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
      auto next = uint8_t{'{'};
      for (int64_t i = 0; i < eo::len(se.fields.list); i++) {
        auto& f = eo::at(se.fields.list, i);

        auto fv = v;
        auto skip = false;
        for (auto j : f.index) {
          if (reflect::Value::Kind(fv) == reflect::Pointer) {
            if (reflect::Value::IsNil(fv)) {
              skip = true;
              break;
            }
            fv = reflect::Value::Elem(fv);
          }
          fv = reflect::Value::Field(fv, j);
        }
        if (skip) {
          continue;
        }

        if ((f.omitEmpty && isEmptyValue(fv)) ||
          (f.omitZero &&
            ((f.isZero == nullptr && reflect::Value::IsZero(fv)) || (f.isZero != nullptr && f.isZero(fv))))) {
          continue;
        }
        encodeState::WriteByte(e, next);
        next = ',';
        if (opts.escapeHTML) {
          encodeState::WriteString(e, f.nameEscHTML);
        } else {
          encodeState::WriteString(e, f.nameNonEsc);
        }
        opts.quoted = f.quoted;
        f.encoder(e, fv, opts);
      }
      if (next == '{') {
        encodeState::WriteString(e, "{}");
      } else {
        encodeState::WriteByte(e, '}');
      }
    }
  };

  inline auto newStructEncoder(reflect::Type t) -> encoderFunc {
    auto se = structEncoder{cachedTypeFields(t)};
    return
      [se](ptr<encodeState> e, reflect::Value v, encOpts opts) { structEncoder::encode(se, e, std::move(v), opts); };
  }

  struct reflectWithString {
    reflect::Value v{};
    std::string ks{};
  };

  auto resolveKeyName(reflect::Value k) -> std::tuple<std::string, error>;

  struct mapEncoder {
    encoderFunc elemEnc;

    static void encode(const mapEncoder& me, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
      if (reflect::Value::IsNil(v)) {
        encodeState::WriteString(e, "null");
        return;
      }
      auto seen = seenGuard{};
      if (++e->ptrLevel > startDetectingCyclesAfter) {
        seen.enter(e, v, {nullptr, reflect::Value::Pointer(v), -1});
      }
      encodeState::WriteByte(e, '{');

      auto sv = eo::make_slice<reflectWithString>(reflect::Value::Len(v));
      auto mi = reflect::Value::MapRange(v);
      auto err = error{};
      for (int64_t i = 0; reflect::MapIter::Next(mi); i++) {
        if (std::tie(eo::at(sv, i).ks, err) = resolveKeyName(reflect::MapIter::Key(mi)); err != nullptr) {
          encodeState::error_(e,
            fmt::Errorf("json: encoding error for type %q: %q", reflect::Value::Type(v).String(), err.Error()));
        }
        eo::at(sv, i).v = reflect::MapIter::Value(mi);
      }
      slices::SortFunc(sv,
        [](const reflectWithString& i, const reflectWithString& j) { return strings::Compare(i.ks, j.ks); });

      for (int64_t i = 0; i < eo::len(sv); i++) {
        const auto& kv = eo::at(sv, i);
        if (i > 0) {
          encodeState::WriteByte(e, ',');
        }
        encodeState::Write(e, appendString(encodeState::AvailableBuffer(e), kv.ks, opts.escapeHTML));
        encodeState::WriteByte(e, ':');
        me.elemEnc(e, kv.v, opts);
      }
      encodeState::WriteByte(e, '}');
      e->ptrLevel--;
    }
  };

  inline auto newMapEncoder(reflect::Type t) -> encoderFunc {
    auto k = t.Key().Kind();
    if (!(k == reflect::String || isIntKind(k) || isUintKind(k))) {
      if (!t.Key().Implements(textMarshalerType)) {
        return unsupportedTypeEncoder;
      }
    }
    auto me = mapEncoder{typeEncoder(t.Elem())};
    return [me](ptr<encodeState> e, reflect::Value v, encOpts opts) { mapEncoder::encode(me, e, std::move(v), opts); };
  }

  inline void encodeByteSlice(ptr<encodeState> e, reflect::Value v, encOpts) {
    if (reflect::Value::IsNil(v)) {
      encodeState::WriteString(e, "null");
      return;
    }

    auto s = reflect::Value::Bytes(v);
    auto b = encodeState::AvailableBuffer(e);
    b = eo::append(b, {'"'});
    b = base64::Encoding::AppendEncode(base64::StdEncoding, b, s);
    b = eo::append(b, {'"'});
    encodeState::Write(e, b);
  }

  struct sliceEncoder {
    encoderFunc arrayEnc;

    static void encode(const sliceEncoder& se, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
      if (reflect::Value::IsNil(v)) {
        encodeState::WriteString(e, "null");
        return;
      }
      auto seen = seenGuard{};
      if (++e->ptrLevel > startDetectingCyclesAfter) {
        seen.enter(e, v, {nullptr, reflect::Value::Pointer(v), reflect::Value::Len(v)});
      }
      se.arrayEnc(e, v, opts);
      e->ptrLevel--;
    }
  };

  inline auto newSliceEncoder(reflect::Type t) -> encoderFunc {
    if (t.Elem().Kind() == reflect::Uint8) {
      auto p = reflect::PointerTo(t.Elem());
      if (!p.Implements(marshalerType) && !p.Implements(textMarshalerType)) {
        return encodeByteSlice;
      }
    }
    auto enc = sliceEncoder{newArrayEncoder(t)};
    return
      [enc](ptr<encodeState> e, reflect::Value v, encOpts opts) { sliceEncoder::encode(enc, e, std::move(v), opts); };
  }

  struct arrayEncoder {
    encoderFunc elemEnc;

    static void encode(const arrayEncoder& ae, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
      encodeState::WriteByte(e, '[');
      auto n = reflect::Value::Len(v);
      for (int64_t i = 0; i < n; i++) {
        if (i > 0) {
          encodeState::WriteByte(e, ',');
        }
        ae.elemEnc(e, reflect::Value::Index(v, i), opts);
      }
      encodeState::WriteByte(e, ']');
    }
  };

  inline auto newArrayEncoder(reflect::Type t) -> encoderFunc {
    auto enc = arrayEncoder{typeEncoder(t.Elem())};
    return
      [enc](ptr<encodeState> e, reflect::Value v, encOpts opts) { arrayEncoder::encode(enc, e, std::move(v), opts); };
  }

  struct ptrEncoder {
    encoderFunc elemEnc;

    static void encode(const ptrEncoder& pe, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
      if (reflect::Value::IsNil(v)) {
        encodeState::WriteString(e, "null");
        return;
      }
      auto seen = seenGuard{};
      if (++e->ptrLevel > startDetectingCyclesAfter) {
        seen.enter(e, v, {v._eo_typ, reflect::Value::Pointer(v), -1});
      }
      pe.elemEnc(e, reflect::Value::Elem(v), opts);
      e->ptrLevel--;
    }
  };

  inline auto newPtrEncoder(reflect::Type t) -> encoderFunc {
    auto enc = ptrEncoder{typeEncoder(t.Elem())};
    return
      [enc](ptr<encodeState> e, reflect::Value v, encOpts opts) { ptrEncoder::encode(enc, e, std::move(v), opts); };
  }

  struct condAddrEncoder {
    encoderFunc canAddrEnc;
    encoderFunc elseEnc;

    static void encode(const condAddrEncoder& ce, const ptr<encodeState>& e, reflect::Value v, encOpts opts) {
      if (reflect::Value::CanAddr(v)) {
        ce.canAddrEnc(e, std::move(v), opts);
      } else {
        ce.elseEnc(e, std::move(v), opts);
      }
    }
  };

  inline auto newCondAddrEncoder(encoderFunc canAddrEnc, encoderFunc elseEnc) -> encoderFunc {
    auto enc = condAddrEncoder{.canAddrEnc = std::move(canAddrEnc), .elseEnc = std::move(elseEnc)};
    return [enc](ptr<encodeState> e, reflect::Value v, encOpts opts) {
      condAddrEncoder::encode(enc, e, std::move(v), opts);
    };
  }

  inline auto isValidTag(const std::string& s) -> bool {
    if (s == "") {
      return false;
    }
    for (int64_t i = 0; i < eo::len(s);) {
      auto [c, size] = utf8::DecodeRuneInString(eo::substr(s, i));
      i += size;
      if (strings::ContainsRune("!#$%&()*+-./:;<=>?@[]^_{|}~ ", c)) {
        continue;
      }
      if (!unicode::IsLetter(c) && !unicode::IsDigit(c)) {
        return false;
      }
    }
    return true;
  }

  inline auto typeByIndex(reflect::Type t, const slice<int64_t>& index) -> reflect::Type {
    for (auto i : index) {
      if (t.Kind() == reflect::Pointer) {
        t = t.Elem();
      }
      t = t.Field(i).Type;
    }
    return t;
  }

  inline auto resolveKeyName(reflect::Value k) -> std::tuple<std::string, error> {
    if (reflect::Value::Kind(k) == reflect::String) {
      return {reflect::Value::String(k), nullptr};
    }
    if (auto [tm, ok] = reflect::TypeAssert<encoding::TextMarshaler>(k); ok) {
      if (reflect::Value::Kind(k) == reflect::Pointer && reflect::Value::IsNil(k)) {
        return {"", nullptr};
      }
      auto [buf, err] = tm.MarshalText();
      return {eo::string_from_bytes(buf), err};
    }
    auto kind = reflect::Value::Kind(k);
    if (isIntKind(kind)) {
      return {strconv::FormatInt(reflect::Value::Int(k), 10), nullptr};
    }
    if (isUintKind(kind)) {
      return {strconv::FormatUint(reflect::Value::Uint(k), 10), nullptr};
    }
    eo::panic(std::string{"unexpected map key type"});
  }

  template<typename Bytes>
  auto appendString(slice<uint8_t> dst, const Bytes& src, bool escapeHTML) -> slice<uint8_t> {
    auto sub = [&](int64_t low, int64_t high) {
      if constexpr (std::is_same_v<Bytes, std::string>) {
        return eo::substr(src, low, high);
      } else {
        return eo::subslice(src, low, high);
      }
    };
    dst = eo::append(dst, {'"'});
    int64_t start = 0;
    for (int64_t i = 0; i < eo::len(src);) {
      if (auto b = static_cast<uint8_t>(eo::at(src, i)); b < utf8::RuneSelf) {
        if (htmlSafeSet[b] || (!escapeHTML && safeSet[b])) {
          i++;
          continue;
        }
        dst = eo::append_slice(dst, sub(start, i));
        switch (b) {
        case '\\':
        case '"':
          dst = eo::append(dst, {'\\', b});
          break;
        case '\b':
          dst = eo::append(dst, {'\\', 'b'});
          break;
        case '\f':
          dst = eo::append(dst, {'\\', 'f'});
          break;
        case '\n':
          dst = eo::append(dst, {'\\', 'n'});
          break;
        case '\r':
          dst = eo::append(dst, {'\\', 'r'});
          break;
        case '\t':
          dst = eo::append(dst, {'\\', 't'});
          break;
        default:
          dst = eo::append(dst, {'\\', 'u', '0', '0', uint8_t(hex[b >> 4]), uint8_t(hex[b & 0xF])});
          break;
        }
        i++;
        start = i;
        continue;
      }
      auto n = std::min(eo::len(src) - i, utf8::UTFMax);
      auto chunk = sub(i, i + n);
      auto [c, size] = utf8::DecodeRuneInString(std::string{chunk.begin(), chunk.end()});
      if (c == utf8::RuneError && size == 1) {
        dst = eo::append_slice(dst, sub(start, i));
        dst = eo::append_slice(dst, std::string{"\\ufffd"});
        i += size;
        start = i;
        continue;
      }
      if (c == 0x2028 || c == 0x2029) {
        dst = eo::append_slice(dst, sub(start, i));
        dst = eo::append(dst, {'\\', 'u', '2', '0', '2', uint8_t(hex[c & 0xF])});
        i += size;
        start = i;
        continue;
      }
      i += size;
    }
    dst = eo::append_slice(dst, sub(start, eo::len(src)));
    dst = eo::append(dst, {'"'});
    return dst;
  }

  inline auto dominantField(slice<field> fields) -> std::tuple<field, bool> {
    if (eo::len(fields) > 1 && eo::len(eo::at(fields, 0).index) == eo::len(eo::at(fields, 1).index) &&
      eo::at(fields, 0).tag == eo::at(fields, 1).tag) {
      return {field{}, false};
    }
    return {eo::at(fields, 0), true};
  }

  inline auto typeFields(reflect::Type t) -> structFields {
    auto current = slice<field>::of({});
    auto next = slice<field>::of({field{.typ = t}});

    auto count = map<reflect::Type, int64_t>{};
    auto nextCount = map<reflect::Type, int64_t>{};

    auto visited = eo::make_map<reflect::Type, bool>();

    auto fields = slice<field>{};

    auto nameEscBuf = slice<uint8_t>{};

    while (eo::len(next) > 0) {
      std::tie(current, next) = std::tuple{next, eo::subslice(current, 0, 0)};
      std::tie(count, nextCount) = std::tuple{nextCount, eo::make_map<reflect::Type, int64_t>()};

      for (auto f : current) {
        if (eo::map_get(visited, f.typ)) {
          continue;
        }
        eo::map_entry(visited, f.typ) = true;

        for (int64_t i = 0; i < f.typ.NumField(); i++) {
          auto sf = f.typ.Field(i);
          if (sf.Anonymous) {
            auto ft = sf.Type;
            if (ft.Kind() == reflect::Pointer) {
              ft = ft.Elem();
            }
            if (!reflect::StructField::IsExported(sf) && ft.Kind() != reflect::Struct) {
              continue;
            }
          } else if (!reflect::StructField::IsExported(sf)) {
            continue;
          }
          auto tag = reflect::StructTag::Get(sf.Tag, "json");
          if (tag == "-") {
            continue;
          }
          auto [name, opts] = parseTag(tag);
          if (!isValidTag(name)) {
            name = "";
          }
          auto index = eo::make_slice<int64_t>(eo::len(f.index) + 1);
          eo::copy(index, f.index);
          eo::at(index, eo::len(f.index)) = i;

          auto ft = sf.Type;
          if (ft.Name() == "" && ft.Kind() == reflect::Pointer) {
            ft = ft.Elem();
          }

          auto quoted = false;
          if (tagOptions::Contains(opts, "string")) {
            auto k = ft.Kind();
            if (k == reflect::Bool || isIntKind(k) || isUintKind(k) || k == reflect::Float32 || k == reflect::Float64 ||
              k == reflect::String) {
              quoted = true;
            }
          }

          if (name != "" || !sf.Anonymous || ft.Kind() != reflect::Struct) {
            auto tagged = name != "";
            if (name == "") {
              name = sf.Name;
            }
            auto fld = field{
              .name = name,
              .tag = tagged,
              .index = index,
              .typ = ft,
              .omitEmpty = tagOptions::Contains(opts, "omitempty"),
              .omitZero = tagOptions::Contains(opts, "omitzero"),
              .quoted = quoted,
            };
            fld.nameBytes = eo::bytes_from_string(fld.name);

            nameEscBuf = appendHTMLEscape(eo::subslice(nameEscBuf, 0, 0), fld.nameBytes);
            fld.nameEscHTML = "\"" + eo::string_from_bytes(nameEscBuf) + "\":";
            fld.nameNonEsc = "\"" + fld.name + "\":";

            if (fld.omitZero) {
              auto st = sf.Type;
              if (st.Kind() == reflect::Interface && st.Implements(isZeroerType)) {
                fld.isZero = [](reflect::Value v) {
                  return reflect::Value::IsNil(v) ||
                    (reflect::Value::Kind(reflect::Value::Elem(v)) == reflect::Pointer &&
                      reflect::Value::IsNil(reflect::Value::Elem(v))) ||
                    eo::type_assert<isZeroer>(reflect::Value::Interface(v)).IsZero();
                };
              } else if (st.Kind() == reflect::Pointer && st.Implements(isZeroerType)) {
                fld.isZero = [](reflect::Value v) {
                  return reflect::Value::IsNil(v) || eo::type_assert<isZeroer>(reflect::Value::Interface(v)).IsZero();
                };
              } else if (st.Implements(isZeroerType)) {
                fld.isZero = [](reflect::Value v) {
                  return eo::type_assert<isZeroer>(reflect::Value::Interface(v)).IsZero();
                };
              } else if (reflect::PointerTo(st).Implements(isZeroerType)) {
                fld.isZero = [](reflect::Value v) {
                  if (!reflect::Value::CanAddr(v)) {
                    auto v2 = reflect::Value::Elem(reflect::New(reflect::Value::Type(v)));
                    reflect::Value::Set(v2, v);
                    v = v2;
                  }
                  return eo::type_assert<isZeroer>(reflect::Value::Interface(reflect::Value::Addr(v))).IsZero();
                };
              }
            }

            fields = eo::append(fields, {fld});
            if (eo::map_get(count, f.typ) > 1) {
              fields = eo::append(fields, {eo::at(fields, eo::len(fields) - 1)});
            }
            continue;
          }

          eo::map_entry(nextCount, ft)++;
          if (eo::map_get(nextCount, ft) == 1) {
            next = eo::append(next, {field{.name = ft.Name(), .index = index, .typ = ft}});
          }
        }
      }
    }

    slices::SortFunc(fields, [](const field& a, const field& b) -> int64_t {
      if (auto c = strings::Compare(a.name, b.name); c != 0) {
        return c;
      }
      if (auto c = cmp::Compare(eo::len(a.index), eo::len(b.index)); c != 0) {
        return c;
      }
      if (a.tag != b.tag) {
        if (a.tag) {
          return -1;
        }
        return +1;
      }
      return slices::Compare(a.index, b.index);
    });

    auto out = eo::subslice(fields, 0, 0);
    for (int64_t advance = 0, i = 0; i < eo::len(fields); i += advance) {
      auto fi = eo::at(fields, i);
      auto name = fi.name;
      for (advance = 1; i + advance < eo::len(fields); advance++) {
        const auto& fj = eo::at(fields, i + advance);
        if (fj.name != name) {
          break;
        }
      }
      if (advance == 1) {
        out = eo::append(out, {fi});
        continue;
      }
      auto [dominant, ok] = dominantField(eo::subslice(fields, i, i + advance));
      if (ok) {
        out = eo::append(out, {dominant});
      }
    }

    fields = out;
    slices::SortFunc(fields, [](const field& i, const field& j) { return slices::Compare(i.index, j.index); });

    for (int64_t i = 0; i < eo::len(fields); i++) {
      auto& f = eo::at(fields, i);
      f.encoder = typeEncoder(typeByIndex(t, f.index));
    }
    auto exactNameIndex = eo::make_map<std::string, ptr<field>>(eo::len(fields));
    auto foldedNameIndex = eo::make_map<std::string, ptr<field>>(eo::len(fields));
    for (int64_t i = 0; i < eo::len(fields); i++) {
      const auto& fld = eo::at(fields, i);
      eo::map_entry(exactNameIndex, fld.name) = ptr<field>(fields, fields.data() + i);
      if (auto ok = eo::map_get_ok(foldedNameIndex, eo::string_from_bytes(foldName(fld.nameBytes))).second; !ok) {
        eo::map_entry(foldedNameIndex, eo::string_from_bytes(foldName(fld.nameBytes))) =
          ptr<field>(fields, fields.data() + i);
      }
    }
    return structFields{fields, exactNameIndex, foldedNameIndex};
  }

  inline typeCache<structFields> fieldCache;

  inline auto cachedTypeFields(reflect::Type t) -> structFields {
    if (auto [f, ok] = fieldCache.Load(t); ok) {
      return f;
    }
    auto [f, _] = fieldCache.LoadOrStore(t, typeFields(t));
    return f;
  }

} // namespace detail

// Go's json.Marshal.
inline auto Marshal(const any& v) -> std::tuple<slice<uint8_t>, error> {
  auto e = detail::newEncodeState();

  auto err = detail::encodeState::marshal(e, v, detail::encOpts{.escapeHTML = true});
  if (err != nullptr) {
    return {nullptr, err};
  }
  auto buf = eo::append_slice(slice<uint8_t>{}, detail::encodeState::Bytes(e));

  return {buf, nullptr};
}

// Go's json.MarshalIndent.
inline auto MarshalIndent(const any& v, std::string prefix, std::string indent) -> std::tuple<slice<uint8_t>, error> {
  auto [b, err] = Marshal(v);
  if (err != nullptr) {
    return {nullptr, err};
  }
  auto b2 = eo::make_slice<uint8_t>(0, detail::indentGrowthFactor * eo::len(b));
  std::tie(b2, err) = detail::appendIndent(b2, b, std::move(prefix), std::move(indent));
  if (err != nullptr) {
    return {nullptr, err};
  }
  return {b2, nullptr};
}

// decode.go.

// Go's json.Unmarshaler.
struct Unmarshaler : eo::interface<Unmarshaler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "json.Unmarshaler";

  auto UnmarshalJSON(slice<uint8_t> data) const -> error;

  using _eo_methods = eo::methods<eo::method<&Unmarshaler::UnmarshalJSON, "UnmarshalJSON">>;
};

inline auto Unmarshaler::UnmarshalJSON(slice<uint8_t> data) const -> error {
  return _eo_call<&Unmarshaler::UnmarshalJSON>(std::move(data));
}

// Go's json.UnmarshalTypeError.
struct UnmarshalTypeError {
  static constexpr std::string_view _eo_name = "json.UnmarshalTypeError";

  std::string Value{};
  reflect::Type Type{};
  int64_t Offset{};
  std::string Struct{};
  std::string Field{};

  auto operator==(const UnmarshalTypeError&) const -> bool = default;

  static auto Error(ptr<UnmarshalTypeError> e) -> std::string {
    if (e->Struct != "" || e->Field != "") {
      return "json: cannot unmarshal " + e->Value + " into Go struct field " + e->Struct + "." + e->Field +
        " of type " + e->Type.String();
    }
    return "json: cannot unmarshal " + e->Value + " into Go value of type " + e->Type.String();
  }

  using _eo_methods = eo::methods<eo::method<&UnmarshalTypeError::Error, "Error">>;
};

// Go's json.UnmarshalFieldError, which no function returns any longer.
struct UnmarshalFieldError {
  static constexpr std::string_view _eo_name = "json.UnmarshalFieldError";

  std::string Key{};
  reflect::Type Type{};
  reflect::StructField Field{};

  static auto Error(ptr<UnmarshalFieldError> e) -> std::string {
    return "json: cannot unmarshal object key " + strconv::Quote(e->Key) + " into unexported field " + e->Field.Name +
      " of type " + e->Type.String();
  }

  using _eo_methods = eo::methods<eo::method<&UnmarshalFieldError::Error, "Error">>;
};

// Go's json.InvalidUnmarshalError.
struct InvalidUnmarshalError {
  static constexpr std::string_view _eo_name = "json.InvalidUnmarshalError";

  reflect::Type Type{};

  auto operator==(const InvalidUnmarshalError&) const -> bool = default;

  static auto Error(ptr<InvalidUnmarshalError> e) -> std::string {
    if (e->Type == nullptr) {
      return "json: Unmarshal(nil)";
    }

    if (e->Type.Kind() != reflect::Pointer) {
      return "json: Unmarshal(non-pointer " + e->Type.String() + ")";
    }
    return "json: Unmarshal(nil " + e->Type.String() + ")";
  }

  using _eo_methods = eo::methods<eo::method<&InvalidUnmarshalError::Error, "Error">>;
};

namespace detail {

  struct errorContext {
    reflect::Type Struct{};
    slice<std::string> FieldStack{};
  };

  inline constexpr std::string_view phasePanicMsg = "JSON decoder out of sync - data changing underfoot?";

  [[noreturn]] inline void panicPhase() {
    eo::panic(std::string{phasePanicMsg});
  }

  struct unquotedValue {
    static constexpr std::string_view _eo_name = "json.unquotedValue";

    auto operator==(const unquotedValue&) const -> bool = default;
  };

  inline const slice<uint8_t> nullLiteral = eo::bytes_from_string("null");
  inline const reflect::Type textUnmarshalerType = reflect::TypeFor<encoding::TextUnmarshaler>();

  auto unquote(slice<uint8_t> s) -> std::tuple<std::string, bool>;
  auto unquoteBytes(slice<uint8_t> s) -> std::tuple<slice<uint8_t>, bool>;

  // fmt.Errorf("json: invalid use of ,string struct tag, trying to unmarshal %q into %v", item, t).
  inline auto invalidStringTag(slice<uint8_t> item, reflect::Type t) -> error {
    return fmt::Errorf("json: invalid use of ,string struct tag, trying to unmarshal %q into %v", item, t);
  }

  inline auto indirect(reflect::Value v,
    bool decodingNull) -> std::tuple<Unmarshaler, encoding::TextUnmarshaler, reflect::Value> {
    auto v0 = v;
    auto haveAddr = false;

    if (reflect::Value::Kind(v) != reflect::Pointer && reflect::Value::Type(v).Name() != "" &&
      reflect::Value::CanAddr(v)) {
      haveAddr = true;
      v = reflect::Value::Addr(v);
    }
    for (;;) {
      if (reflect::Value::Kind(v) == reflect::Interface && !reflect::Value::IsNil(v)) {
        auto e = reflect::Value::Elem(v);
        if (reflect::Value::Kind(e) == reflect::Pointer && !reflect::Value::IsNil(e) &&
          (!decodingNull || reflect::Value::Kind(reflect::Value::Elem(e)) == reflect::Pointer)) {
          haveAddr = false;
          v = e;
          continue;
        }
      }

      if (reflect::Value::Kind(v) != reflect::Pointer) {
        break;
      }

      if (decodingNull && reflect::Value::CanSet(v)) {
        break;
      }

      if (reflect::Value::Kind(reflect::Value::Elem(v)) == reflect::Interface &&
        reflect::Value::Equal(reflect::Value::Elem(reflect::Value::Elem(v)), v)) {
        v = reflect::Value::Elem(v);
        break;
      }
      if (reflect::Value::IsNil(v)) {
        reflect::Value::Set(v, reflect::New(reflect::Value::Type(v).Elem()));
      }
      if (reflect::Value::Type(v).NumMethod() > 0 && reflect::Value::CanInterface(v)) {
        if (auto [u, ok] = reflect::TypeAssert<Unmarshaler>(v); ok) {
          return {u, nullptr, reflect::Value{}};
        }
        if (!decodingNull) {
          if (auto [u, ok] = reflect::TypeAssert<encoding::TextUnmarshaler>(v); ok) {
            return {nullptr, u, reflect::Value{}};
          }
        }
      }

      if (haveAddr) {
        v = v0;
        haveAddr = false;
      } else {
        v = reflect::Value::Elem(v);
      }
    }
    return {nullptr, nullptr, v};
  }

  struct decodeState {
    slice<uint8_t> data{};
    int64_t off{};
    int64_t opcode{};
    detail::scanner scan{};
    ptr<detail::errorContext> errorContext{};
    error savedError{};
    bool useNumber{};
    bool disallowUnknownFields{};

    static auto readIndex(decodeState* d) -> int64_t {
      return d->off - 1;
    }

    static auto init(decodeState* d, slice<uint8_t> data) -> decodeState* {
      d->data = std::move(data);
      d->off = 0;
      d->savedError = nullptr;
      if (d->errorContext != nullptr) {
        d->errorContext->Struct = nullptr;
        d->errorContext->FieldStack = eo::subslice(d->errorContext->FieldStack, 0, 0);
      }
      return d;
    }

    static void saveError(decodeState* d, error err) {
      if (d->savedError == nullptr) {
        d->savedError = addErrorContext(d, std::move(err));
      }
    }

    static void saveTypeError(decodeState* d, std::string value, reflect::Type t, int64_t offset) {
      saveError(d, New<UnmarshalTypeError>(UnmarshalTypeError{.Value = std::move(value), .Type = t, .Offset = offset}));
    }

    static auto addErrorContext(decodeState* d, error err) -> error {
      if (d->errorContext != nullptr &&
        (d->errorContext->Struct != nullptr || eo::len(d->errorContext->FieldStack) > 0)) {
        if (auto [e, ok] = eo::type_assert_ok<ptr<UnmarshalTypeError>>(err); ok) {
          e->Struct = d->errorContext->Struct.Name();
          auto fieldStack = d->errorContext->FieldStack;
          if (e->Field != "") {
            fieldStack = eo::append(fieldStack, {e->Field});
          }
          e->Field = strings::Join(fieldStack, ".");
        }
      }
      return err;
    }

    static void skip(decodeState* d) {
      auto* s = &d->scan;
      auto data = d->data;
      auto i = d->off;
      auto depth = eo::len(s->parseState);
      for (;;) {
        auto op = s->step(s, eo::at(data, i));
        i++;
        if (eo::len(s->parseState) < depth) {
          d->off = i;
          d->opcode = op;
          return;
        }
      }
    }

    static void scanNext(decodeState* d) {
      if (d->off < eo::len(d->data)) {
        d->opcode = d->scan.step(&d->scan, eo::at(d->data, d->off));
        d->off++;
      } else {
        d->opcode = scanner::eof(&d->scan);
        d->off = eo::len(d->data) + 1;
      }
    }

    static void scanWhile(decodeState* d, int64_t op) {
      auto* s = &d->scan;
      auto data = d->data;
      auto i = d->off;
      while (i < eo::len(data)) {
        auto newOp = s->step(s, eo::at(data, i));
        i++;
        if (newOp != op) {
          d->opcode = newOp;
          d->off = i;
          return;
        }
      }

      d->off = eo::len(data) + 1;
      d->opcode = scanner::eof(&d->scan);
    }

    static void rescanLiteral(decodeState* d) {
      auto data = d->data;
      auto i = d->off;
      switch (eo::at(data, i - 1)) {
      case '"':
        for (; i < eo::len(data); i++) {
          auto c = eo::at(data, i);
          if (c == '\\') {
            i++;
          } else if (c == '"') {
            i++;
            break;
          }
        }
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
        for (; i < eo::len(data); i++) {
          auto c = eo::at(data, i);
          if (!(('0' <= c && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
            break;
          }
        }
        break;
      case 't':
        i += 3;
        break;
      case 'f':
        i += 4;
        break;
      case 'n':
        i += 3;
        break;
      default:
        break;
      }
      if (i < eo::len(data)) {
        d->opcode = stateEndValue(&d->scan, eo::at(data, i));
      } else {
        d->opcode = scanEnd;
      }
      d->off = i + 1;
    }

    static auto value(decodeState* d, reflect::Value v) -> error {
      switch (d->opcode) {
      default:
        panicPhase();

      case scanBeginArray:
        if (reflect::Value::IsValid(v)) {
          if (auto err = array(d, v); err != nullptr) {
            return err;
          }
        } else {
          skip(d);
        }
        scanNext(d);
        break;

      case scanBeginObject:
        if (reflect::Value::IsValid(v)) {
          if (auto err = object(d, v); err != nullptr) {
            return err;
          }
        } else {
          skip(d);
        }
        scanNext(d);
        break;

      case scanBeginLiteral: {
        auto start = readIndex(d);
        rescanLiteral(d);

        if (reflect::Value::IsValid(v)) {
          if (auto err = literalStore(d, eo::subslice(d->data, start, readIndex(d)), v, false); err != nullptr) {
            return err;
          }
        }
        break;
      }
      }
      return nullptr;
    }

    static auto valueQuoted(decodeState* d) -> any {
      switch (d->opcode) {
      default:
        panicPhase();

      case scanBeginArray:
      case scanBeginObject:
        skip(d);
        scanNext(d);
        break;

      case scanBeginLiteral: {
        auto v = literalInterface(d);
        if (v == nullptr || eo::type_assert_ok<std::string>(v).second) {
          return v;
        }
        break;
      }
      }
      return unquotedValue{};
    }

    static auto array(decodeState* d, reflect::Value v) -> error {
      auto [u, ut, pv] = indirect(v, false);
      if (u != nullptr) {
        auto start = readIndex(d);
        skip(d);
        return u.UnmarshalJSON(eo::subslice(d->data, start, d->off));
      }
      if (ut != nullptr) {
        saveTypeError(d, "array", reflect::Value::Type(v), d->off);
        skip(d);
        return nullptr;
      }
      v = pv;

      auto k = reflect::Value::Kind(v);
      if (k == reflect::Interface && reflect::Value::NumMethod(v) == 0) {
        auto ai = arrayInterface(d);
        reflect::Value::Set(v, reflect::ValueOf(ai));
        return nullptr;
      }
      if (k != reflect::Array && k != reflect::Slice) {
        saveTypeError(d, "array", reflect::Value::Type(v), d->off);
        skip(d);
        return nullptr;
      }

      int64_t i = 0;
      for (;;) {
        scanWhile(d, scanSkipSpace);
        if (d->opcode == scanEndArray) {
          break;
        }

        if (reflect::Value::Kind(v) == reflect::Slice) {
          if (i >= reflect::Value::Cap(v)) {
            reflect::Value::Grow(v, 1);
          }
          if (i >= reflect::Value::Len(v)) {
            reflect::Value::SetLen(v, i + 1);
          }
        }

        if (i < reflect::Value::Len(v)) {
          if (auto err = value(d, reflect::Value::Index(v, i)); err != nullptr) {
            return err;
          }
        } else {
          if (auto err = value(d, reflect::Value{}); err != nullptr) {
            return err;
          }
        }
        i++;

        if (d->opcode == scanSkipSpace) {
          scanWhile(d, scanSkipSpace);
        }
        if (d->opcode == scanEndArray) {
          break;
        }
        if (d->opcode != scanArrayValue) {
          panicPhase();
        }
      }

      if (i < reflect::Value::Len(v)) {
        if (reflect::Value::Kind(v) == reflect::Array) {
          for (; i < reflect::Value::Len(v); i++) {
            reflect::Value::SetZero(reflect::Value::Index(v, i));
          }
        } else {
          reflect::Value::SetLen(v, i);
        }
      }
      if (i == 0 && reflect::Value::Kind(v) == reflect::Slice) {
        reflect::Value::Set(v, reflect::MakeSlice(reflect::Value::Type(v), 0, 0));
      }
      return nullptr;
    }

    static auto object(decodeState* d, reflect::Value v) -> error {
      auto [u, ut, pv] = indirect(v, false);
      if (u != nullptr) {
        auto start = readIndex(d);
        skip(d);
        return u.UnmarshalJSON(eo::subslice(d->data, start, d->off));
      }
      if (ut != nullptr) {
        saveTypeError(d, "object", reflect::Value::Type(v), d->off);
        skip(d);
        return nullptr;
      }
      v = pv;
      auto t = reflect::Value::Type(v);

      if (reflect::Value::Kind(v) == reflect::Interface && reflect::Value::NumMethod(v) == 0) {
        auto oi = objectInterface(d);
        reflect::Value::Set(v, reflect::ValueOf(oi));
        return nullptr;
      }

      auto fields = structFields{};

      if (reflect::Value::Kind(v) == reflect::Map) {
        auto kk = t.Key().Kind();
        if (!(kk == reflect::String || isIntKind(kk) || isUintKind(kk))) {
          if (!reflect::PointerTo(t.Key()).Implements(textUnmarshalerType)) {
            saveTypeError(d, "object", t, d->off);
            skip(d);
            return nullptr;
          }
        }
        if (reflect::Value::IsNil(v)) {
          reflect::Value::Set(v, reflect::MakeMap(t));
        }
      } else if (reflect::Value::Kind(v) == reflect::Struct) {
        fields = cachedTypeFields(t);
      } else {
        saveTypeError(d, "object", t, d->off);
        skip(d);
        return nullptr;
      }

      auto mapElem = reflect::Value{};
      auto origErrorContext = detail::errorContext{};
      if (d->errorContext != nullptr) {
        origErrorContext = *d->errorContext;
      }

      for (;;) {
        scanWhile(d, scanSkipSpace);
        if (d->opcode == scanEndObject) {
          break;
        }
        if (d->opcode != scanBeginLiteral) {
          panicPhase();
        }

        auto start = readIndex(d);
        rescanLiteral(d);
        auto item = eo::subslice(d->data, start, readIndex(d));
        auto [key, ok] = unquoteBytes(item);
        if (!ok) {
          panicPhase();
        }

        auto subv = reflect::Value{};
        auto destring = false;

        if (reflect::Value::Kind(v) == reflect::Map) {
          auto elemType = t.Elem();
          if (!reflect::Value::IsValid(mapElem)) {
            mapElem = reflect::Value::Elem(reflect::New(elemType));
          } else {
            reflect::Value::SetZero(mapElem);
          }
          subv = mapElem;
        } else {
          auto f = eo::map_get(fields.byExactName, eo::string_from_bytes(key));
          if (f == nullptr) {
            f = eo::map_get(fields.byFoldedName, eo::string_from_bytes(foldName(key)));
          }
          if (f != nullptr) {
            subv = v;
            destring = f->quoted;
            if (d->errorContext == nullptr) {
              d->errorContext = New<detail::errorContext>();
            }
            for (int64_t i = 0; i < eo::len(f->index); i++) {
              auto ind = eo::at(f->index, i);
              if (reflect::Value::Kind(subv) == reflect::Pointer) {
                if (reflect::Value::IsNil(subv)) {
                  if (!reflect::Value::CanSet(subv)) {
                    saveError(d,
                      fmt::Errorf("json: cannot set embedded pointer to unexported struct: %v",
                        reflect::Value::Type(subv).Elem()));
                    subv = reflect::Value{};
                    destring = false;
                    break;
                  }
                  reflect::Value::Set(subv, reflect::New(reflect::Value::Type(subv).Elem()));
                }
                subv = reflect::Value::Elem(subv);
              }
              if (i < eo::len(f->index) - 1) {
                d->errorContext->FieldStack =
                  eo::append(d->errorContext->FieldStack, {reflect::Value::Type(subv).Field(ind).Name});
              }
              subv = reflect::Value::Field(subv, ind);
            }
            d->errorContext->Struct = t;
            d->errorContext->FieldStack = eo::append(d->errorContext->FieldStack, {f->name});
          } else if (d->disallowUnknownFields) {
            saveError(d, fmt::Errorf("json: unknown field %q", key));
          }
        }

        if (d->opcode == scanSkipSpace) {
          scanWhile(d, scanSkipSpace);
        }
        if (d->opcode != scanObjectKey) {
          panicPhase();
        }
        scanWhile(d, scanSkipSpace);

        if (destring) {
          auto qv = valueQuoted(d);
          if (qv == nullptr) {
            if (auto err = literalStore(d, nullLiteral, subv, false); err != nullptr) {
              return err;
            }
          } else if (auto [s, isString] = eo::type_assert_ok<std::string>(qv); isString) {
            if (auto err = literalStore(d, eo::bytes_from_string(s), subv, true); err != nullptr) {
              return err;
            }
          } else {
            saveError(d,
              fmt::Errorf("json: invalid use of ,string struct tag, trying to unmarshal unquoted value into %v",
                reflect::Value::Type(subv)));
          }
        } else {
          if (auto err = value(d, subv); err != nullptr) {
            return err;
          }
        }

        if (reflect::Value::Kind(v) == reflect::Map) {
          auto kt = t.Key();
          auto kv = reflect::Value{};
          if (reflect::PointerTo(kt).Implements(textUnmarshalerType)) {
            kv = reflect::New(kt);
            if (auto err = literalStore(d, item, kv, true); err != nullptr) {
              return err;
            }
            kv = reflect::Value::Elem(kv);
          } else {
            auto kk = kt.Kind();
            if (kk == reflect::String) {
              kv = reflect::Value::Elem(reflect::New(kt));
              reflect::Value::SetString(kv, eo::string_from_bytes(key));
            } else if (isIntKind(kk)) {
              auto s = eo::string_from_bytes(key);
              auto [n, err] = strconv::ParseInt(s, 10, 64);
              if (err != nullptr || kt.OverflowInt(n)) {
                saveTypeError(d, "number " + s, kt, start + 1);
              } else {
                kv = reflect::Value::Elem(reflect::New(kt));
                reflect::Value::SetInt(kv, n);
              }
            } else if (isUintKind(kk)) {
              auto s = eo::string_from_bytes(key);
              auto [n, err] = strconv::ParseUint(s, 10, 64);
              if (err != nullptr || kt.OverflowUint(n)) {
                saveTypeError(d, "number " + s, kt, start + 1);
              } else {
                kv = reflect::Value::Elem(reflect::New(kt));
                reflect::Value::SetUint(kv, n);
              }
            } else {
              eo::panic(std::string{"json: Unexpected key type"});
            }
          }
          if (reflect::Value::IsValid(kv)) {
            reflect::Value::SetMapIndex(v, kv, subv);
          }
        }

        if (d->opcode == scanSkipSpace) {
          scanWhile(d, scanSkipSpace);
        }
        if (d->errorContext != nullptr) {
          d->errorContext->FieldStack =
            eo::subslice(d->errorContext->FieldStack, 0, eo::len(origErrorContext.FieldStack));
          d->errorContext->Struct = origErrorContext.Struct;
        }
        if (d->opcode == scanEndObject) {
          break;
        }
        if (d->opcode != scanObjectValue) {
          panicPhase();
        }
      }
      return nullptr;
    }

    static auto convertNumber(decodeState* d, std::string s) -> std::tuple<any, error> {
      if (d->useNumber) {
        return {Number{s}, nullptr};
      }
      auto [f, err] = strconv::ParseFloat(s, 64);
      if (err != nullptr) {
        return {nullptr,
          New<UnmarshalTypeError>(
            UnmarshalTypeError{.Value = "number " + s, .Type = reflect::TypeFor<double>(), .Offset = d->off})};
      }
      return {f, nullptr};
    }

    static auto literalStore(decodeState* d, slice<uint8_t> item, reflect::Value v, bool fromQuoted) -> error {
      if (eo::len(item) == 0) {
        saveError(d, invalidStringTag(item, reflect::Value::Type(v)));
        return nullptr;
      }
      auto isNull = eo::at(item, 0) == 'n';
      auto [u, ut, pv] = indirect(v, isNull);
      if (u != nullptr) {
        return u.UnmarshalJSON(item);
      }
      if (ut != nullptr) {
        if (eo::at(item, 0) != '"') {
          if (fromQuoted) {
            saveError(d, invalidStringTag(item, reflect::Value::Type(v)));
            return nullptr;
          }
          auto val = std::string{"number"};
          switch (eo::at(item, 0)) {
          case 'n':
            val = "null";
            break;
          case 't':
          case 'f':
            val = "bool";
            break;
          default:
            break;
          }
          saveTypeError(d, val, reflect::Value::Type(v), readIndex(d));
          return nullptr;
        }
        auto [s, ok] = unquoteBytes(item);
        if (!ok) {
          if (fromQuoted) {
            return invalidStringTag(item, reflect::Value::Type(v));
          }
          panicPhase();
        }
        return ut.UnmarshalText(s);
      }

      v = pv;

      auto item_ = eo::string_from_bytes(item);
      auto k = reflect::Value::Kind(v);
      switch (auto c = eo::at(item, 0); c) {
      case 'n':
        if (fromQuoted && item_ != "null") {
          saveError(d, invalidStringTag(item, reflect::Value::Type(v)));
          break;
        }
        if (k == reflect::Interface || k == reflect::Pointer || k == reflect::Map || k == reflect::Slice) {
          reflect::Value::SetZero(v);
        }
        break;
      case 't':
      case 'f': {
        auto value = eo::at(item, 0) == 't';
        if (fromQuoted && item_ != "true" && item_ != "false") {
          saveError(d, invalidStringTag(item, reflect::Value::Type(v)));
          break;
        }
        if (k == reflect::Bool) {
          reflect::Value::SetBool(v, value);
        } else if (k == reflect::Interface) {
          if (reflect::Value::NumMethod(v) == 0) {
            reflect::Value::Set(v, reflect::ValueOf(value));
          } else {
            saveTypeError(d, "bool", reflect::Value::Type(v), readIndex(d));
          }
        } else {
          if (fromQuoted) {
            saveError(d, invalidStringTag(item, reflect::Value::Type(v)));
          } else {
            saveTypeError(d, "bool", reflect::Value::Type(v), readIndex(d));
          }
        }
        break;
      }

      case '"': {
        auto [s, ok] = unquoteBytes(item);
        if (!ok) {
          if (fromQuoted) {
            return invalidStringTag(item, reflect::Value::Type(v));
          }
          panicPhase();
        }
        if (k == reflect::Slice) {
          if (reflect::Value::Type(v).Elem().Kind() != reflect::Uint8) {
            saveTypeError(d, "string", reflect::Value::Type(v), readIndex(d));
            break;
          }
          auto b = eo::make_slice<uint8_t>(base64::Encoding::DecodedLen(base64::StdEncoding, eo::len(s)));
          auto [n, err] = base64::Encoding::Decode(base64::StdEncoding, b, s);
          if (err != nullptr) {
            saveError(d, err);
            break;
          }
          reflect::Value::SetBytes(v, eo::subslice(b, 0, n));
        } else if (k == reflect::String) {
          auto t = eo::string_from_bytes(s);
          if (reflect::Value::Type(v) == numberType && !isValidNumber(t)) {
            return fmt::Errorf("json: invalid number literal, trying to unmarshal %q into Number", item);
          }
          reflect::Value::SetString(v, t);
        } else if (k == reflect::Interface) {
          if (reflect::Value::NumMethod(v) == 0) {
            reflect::Value::Set(v, reflect::ValueOf(eo::string_from_bytes(s)));
          } else {
            saveTypeError(d, "string", reflect::Value::Type(v), readIndex(d));
          }
        } else {
          saveTypeError(d, "string", reflect::Value::Type(v), readIndex(d));
        }
        break;
      }

      default:
        if (c != '-' && (c < '0' || c > '9')) {
          if (fromQuoted) {
            return invalidStringTag(item, reflect::Value::Type(v));
          }
          panicPhase();
        }
        if (k == reflect::Interface) {
          auto [n, err] = convertNumber(d, item_);
          if (err != nullptr) {
            saveError(d, err);
            break;
          }
          if (reflect::Value::NumMethod(v) != 0) {
            saveTypeError(d, "number", reflect::Value::Type(v), readIndex(d));
            break;
          }
          reflect::Value::Set(v, reflect::ValueOf(n));
        } else if (isIntKind(k)) {
          auto [n, err] = strconv::ParseInt(item_, 10, 64);
          if (err != nullptr || reflect::Value::OverflowInt(v, n)) {
            saveTypeError(d, "number " + item_, reflect::Value::Type(v), readIndex(d));
            break;
          }
          reflect::Value::SetInt(v, n);
        } else if (isUintKind(k)) {
          auto [n, err] = strconv::ParseUint(item_, 10, 64);
          if (err != nullptr || reflect::Value::OverflowUint(v, n)) {
            saveTypeError(d, "number " + item_, reflect::Value::Type(v), readIndex(d));
            break;
          }
          reflect::Value::SetUint(v, n);
        } else if (k == reflect::Float32 || k == reflect::Float64) {
          auto [n, err] = strconv::ParseFloat(item_, reflect::Value::Type(v).Bits());
          if (err != nullptr || reflect::Value::OverflowFloat(v, n)) {
            saveTypeError(d, "number " + item_, reflect::Value::Type(v), readIndex(d));
            break;
          }
          reflect::Value::SetFloat(v, n);
        } else {
          if (k == reflect::String && reflect::Value::Type(v) == numberType) {
            reflect::Value::SetString(v, item_);
            break;
          }
          if (fromQuoted) {
            return invalidStringTag(item, reflect::Value::Type(v));
          }
          saveTypeError(d, "number", reflect::Value::Type(v), readIndex(d));
        }
        break;
      }
      return nullptr;
    }

    static auto valueInterface(decodeState* d) -> any {
      auto val = any{};
      switch (d->opcode) {
      default:
        panicPhase();
      case scanBeginArray:
        val = arrayInterface(d);
        scanNext(d);
        break;
      case scanBeginObject:
        val = objectInterface(d);
        scanNext(d);
        break;
      case scanBeginLiteral:
        val = literalInterface(d);
        break;
      }
      return val;
    }

    static auto arrayInterface(decodeState* d) -> slice<any> {
      auto v = eo::make_slice<any>(0);
      for (;;) {
        scanWhile(d, scanSkipSpace);
        if (d->opcode == scanEndArray) {
          break;
        }

        v = eo::append(v, {valueInterface(d)});

        if (d->opcode == scanSkipSpace) {
          scanWhile(d, scanSkipSpace);
        }
        if (d->opcode == scanEndArray) {
          break;
        }
        if (d->opcode != scanArrayValue) {
          panicPhase();
        }
      }
      return v;
    }

    static auto objectInterface(decodeState* d) -> map<std::string, any> {
      auto m = eo::make_map<std::string, any>();
      for (;;) {
        scanWhile(d, scanSkipSpace);
        if (d->opcode == scanEndObject) {
          break;
        }
        if (d->opcode != scanBeginLiteral) {
          panicPhase();
        }

        auto start = readIndex(d);
        rescanLiteral(d);
        auto item = eo::subslice(d->data, start, readIndex(d));
        auto [key, ok] = unquote(item);
        if (!ok) {
          panicPhase();
        }

        if (d->opcode == scanSkipSpace) {
          scanWhile(d, scanSkipSpace);
        }
        if (d->opcode != scanObjectKey) {
          panicPhase();
        }
        scanWhile(d, scanSkipSpace);

        auto _eo_call_1 = valueInterface(d);
        eo::map_entry(m, key) = std::move(_eo_call_1);

        if (d->opcode == scanSkipSpace) {
          scanWhile(d, scanSkipSpace);
        }
        if (d->opcode == scanEndObject) {
          break;
        }
        if (d->opcode != scanObjectValue) {
          panicPhase();
        }
      }
      return m;
    }

    static auto literalInterface(decodeState* d) -> any {
      auto start = readIndex(d);
      rescanLiteral(d);

      auto item = eo::subslice(d->data, start, readIndex(d));

      switch (auto c = eo::at(item, 0); c) {
      case 'n':
        return nullptr;

      case 't':
      case 'f':
        return c == 't';

      case '"': {
        auto [s, ok] = unquote(item);
        if (!ok) {
          panicPhase();
        }
        return s;
      }

      default: {
        if (c != '-' && (c < '0' || c > '9')) {
          panicPhase();
        }
        auto [n, err] = convertNumber(d, eo::string_from_bytes(item));
        if (err != nullptr) {
          saveError(d, err);
        }
        return n;
      }
      }
    }

    static auto unmarshal(decodeState* d, const any& v) -> error {
      auto rv = reflect::ValueOf(v);
      if (reflect::Value::Kind(rv) != reflect::Pointer || reflect::Value::IsNil(rv)) {
        return New<InvalidUnmarshalError>(InvalidUnmarshalError{reflect::TypeOf(v)});
      }

      scanner::reset(&d->scan);
      scanWhile(d, scanSkipSpace);
      auto err = value(d, rv);
      if (err != nullptr) {
        return addErrorContext(d, err);
      }
      return d->savedError;
    }
  };

  inline auto getu4(slice<uint8_t> s) -> int32_t {
    if (eo::len(s) < 6 || eo::at(s, 0) != '\\' || eo::at(s, 1) != 'u') {
      return -1;
    }
    int32_t r = 0;
    for (auto c : eo::subslice(s, 2, 6)) {
      if ('0' <= c && c <= '9') {
        c = c - '0';
      } else if ('a' <= c && c <= 'f') {
        c = c - 'a' + 10;
      } else if ('A' <= c && c <= 'F') {
        c = c - 'A' + 10;
      } else {
        return -1;
      }
      r = r * 16 + int32_t{c};
    }
    return r;
  }

  inline auto unquote(slice<uint8_t> s) -> std::tuple<std::string, bool> {
    auto [b, ok] = unquoteBytes(std::move(s));
    return {eo::string_from_bytes(b), ok};
  }

  inline auto unquoteBytes(slice<uint8_t> s) -> std::tuple<slice<uint8_t>, bool> {
    if (eo::len(s) < 2 || eo::at(s, 0) != '"' || eo::at(s, eo::len(s) - 1) != '"') {
      return {nullptr, false};
    }
    s = eo::subslice(s, 1, eo::len(s) - 1);

    int64_t r = 0;
    while (r < eo::len(s)) {
      auto c = eo::at(s, r);
      if (c == '\\' || c == '"' || c < ' ') {
        break;
      }
      auto [rr, size] = utf8::DecodeRune(eo::subslice(s, r));
      if (rr == utf8::RuneError && size == 1) {
        break;
      }
      r += size;
    }
    if (r == eo::len(s)) {
      return {s, true};
    }

    auto b = eo::make_slice<uint8_t>(eo::len(s) + 2 * utf8::UTFMax);
    auto w = eo::copy(b, eo::subslice(s, 0, r));
    while (r < eo::len(s)) {
      if (w >= eo::len(b) - 2 * utf8::UTFMax) {
        auto nb = eo::make_slice<uint8_t>((eo::len(b) + utf8::UTFMax) * 2);
        eo::copy(nb, eo::subslice(b, 0, w));
        b = nb;
      }
      auto c = eo::at(s, r);
      if (c == '\\') {
        r++;
        if (r >= eo::len(s)) {
          return {nullptr, false};
        }
        switch (eo::at(s, r)) {
        default:
          return {nullptr, false};
        case '"':
        case '\\':
        case '/':
        case '\'':
          eo::at(b, w) = eo::at(s, r);
          r++;
          w++;
          break;
        case 'b':
          eo::at(b, w) = '\b';
          r++;
          w++;
          break;
        case 'f':
          eo::at(b, w) = '\f';
          r++;
          w++;
          break;
        case 'n':
          eo::at(b, w) = '\n';
          r++;
          w++;
          break;
        case 'r':
          eo::at(b, w) = '\r';
          r++;
          w++;
          break;
        case 't':
          eo::at(b, w) = '\t';
          r++;
          w++;
          break;
        case 'u': {
          r--;
          auto rr = getu4(eo::subslice(s, r));
          if (rr < 0) {
            return {nullptr, false};
          }
          r += 6;
          if (utf16::IsSurrogate(rr)) {
            auto rr1 = getu4(eo::subslice(s, r));
            if (auto dec = utf16::DecodeRune(rr, rr1); dec != unicode::ReplacementChar) {
              r += 6;
              w += utf8::EncodeRune(eo::subslice(b, w), dec);
              break;
            }
            rr = unicode::ReplacementChar;
          }
          w += utf8::EncodeRune(eo::subslice(b, w), rr);
          break;
        }
        }
      } else if (c == '"' || c < ' ') {
        return {nullptr, false};
      } else if (c < utf8::RuneSelf) {
        eo::at(b, w) = c;
        r++;
        w++;
      } else {
        auto [rr, size] = utf8::DecodeRune(eo::subslice(s, r));
        r += size;
        w += utf8::EncodeRune(eo::subslice(b, w), rr);
      }
    }
    return {eo::subslice(b, 0, w), true};
  }

} // namespace detail

// Go's json.Unmarshal.
inline auto Unmarshal(slice<uint8_t> data, const any& v) -> error {
  auto d = detail::decodeState{};
  auto err = detail::checkValid(data, &d.scan);
  if (err != nullptr) {
    return err;
  }

  detail::decodeState::init(&d, data);
  return detail::decodeState::unmarshal(&d, v);
}

// stream.go.

// Go's json.Token.
using Token = any;

// Go's json.Delim.
struct Delim : eo::defined<Delim, int32_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "json.Delim";

  static auto String(Delim d) -> std::string {
    return eo::string_from_rune(int32_t(d));
  }

  using _eo_methods = eo::methods<eo::method<&Delim::String, "String">>;
};

namespace detail {

  inline constexpr int64_t tokenTopValue = 0;
  inline constexpr int64_t tokenArrayStart = 1;
  inline constexpr int64_t tokenArrayValue = 2;
  inline constexpr int64_t tokenArrayComma = 3;
  inline constexpr int64_t tokenObjectStart = 4;
  inline constexpr int64_t tokenObjectKey = 5;
  inline constexpr int64_t tokenObjectColon = 6;
  inline constexpr int64_t tokenObjectValue = 7;
  inline constexpr int64_t tokenObjectComma = 8;

  inline auto nonSpace(slice<uint8_t> b) -> bool {
    for (auto c : b) {
      if (!isSpace(c)) {
        return true;
      }
    }
    return false;
  }

} // namespace detail

// Go's json.Decoder.
struct Decoder {
  static constexpr std::string_view _eo_name = "json.Decoder";

  io::Reader r{};
  slice<uint8_t> buf{};
  detail::decodeState d{};
  int64_t scanp{};
  int64_t scanned{};
  detail::scanner scan{};
  error err{};

  int64_t tokenState{};
  slice<int64_t> tokenStack{};

  static void UseNumber(ptr<Decoder> dec) {
    dec->d.useNumber = true;
  }

  static void DisallowUnknownFields(ptr<Decoder> dec) {
    dec->d.disallowUnknownFields = true;
  }

  static auto Decode(ptr<Decoder> dec, const any& v) -> error {
    if (dec->err != nullptr) {
      return dec->err;
    }

    if (auto err = tokenPrepareForDecode(dec); err != nullptr) {
      return err;
    }

    if (!tokenValueAllowed(dec)) {
      return New<SyntaxError>(SyntaxError{.msg = "not at beginning of value", .Offset = InputOffset(dec)});
    }

    auto [n, err] = readValue(dec);
    if (err != nullptr) {
      return err;
    }
    detail::decodeState::init(&dec->d, eo::subslice(dec->buf, dec->scanp, dec->scanp + n));
    dec->scanp += n;

    err = detail::decodeState::unmarshal(&dec->d, v);

    tokenValueEnd(dec);

    return err;
  }

  static auto Buffered(ptr<Decoder> dec) -> io::Reader {
    return bytes::NewReader(eo::subslice(dec->buf, dec->scanp));
  }

  static auto readValue(const ptr<Decoder>& dec) -> std::tuple<int64_t, error> {
    detail::scanner::reset(&dec->scan);

    auto scanp = dec->scanp;
    auto err = error{};
    for (auto done = false; !done && scanp >= 0;) {
      for (; scanp < eo::len(dec->buf); scanp++) {
        auto c = eo::at(dec->buf, scanp);
        dec->scan.bytes++;
        auto op = dec->scan.step(&dec->scan, c);
        if (op == detail::scanEnd) {
          dec->scan.bytes--;
          done = true;
          break;
        }
        if (op == detail::scanEndObject || op == detail::scanEndArray) {
          if (detail::stateEndValue(&dec->scan, ' ') == detail::scanEnd) {
            scanp++;
            done = true;
            break;
          }
        } else if (op == detail::scanError) {
          dec->err = dec->scan.err;
          return {0, dec->scan.err};
        }
      }
      if (done) {
        break;
      }

      if (err != nullptr) {
        if (err == io::EOF_) {
          if (dec->scan.step(&dec->scan, ' ') == detail::scanEnd) {
            break;
          }
          if (detail::nonSpace(dec->buf)) {
            err = io::ErrUnexpectedEOF;
          }
        }
        dec->err = err;
        return {0, err};
      }

      auto n = scanp - dec->scanp;
      err = refill(dec);
      scanp = dec->scanp + n;
    }
    return {scanp - dec->scanp, nullptr};
  }

  static auto refill(const ptr<Decoder>& dec) -> error {
    if (dec->scanp > 0) {
      dec->scanned += dec->scanp;
      auto n = eo::copy(dec->buf, eo::subslice(dec->buf, dec->scanp));
      dec->buf = eo::subslice(dec->buf, 0, n);
      dec->scanp = 0;
    }

    constexpr int64_t minRead = 512;
    if (eo::cap(dec->buf) - eo::len(dec->buf) < minRead) {
      auto newBuf = eo::make_slice<uint8_t>(eo::len(dec->buf), 2 * eo::cap(dec->buf) + minRead);
      eo::copy(newBuf, dec->buf);
      dec->buf = newBuf;
    }

    auto [n, err] = dec->r.Read(eo::subslice(dec->buf, eo::len(dec->buf), eo::cap(dec->buf)));
    dec->buf = eo::subslice(dec->buf, 0, eo::len(dec->buf) + n);

    return err;
  }

  static auto tokenPrepareForDecode(const ptr<Decoder>& dec) -> error {
    switch (dec->tokenState) {
    case detail::tokenArrayComma: {
      auto [c, err] = peek(dec);
      if (err != nullptr) {
        return err;
      }
      if (c != ',') {
        return New<SyntaxError>(SyntaxError{"expected comma after array element", InputOffset(dec)});
      }
      dec->scanp++;
      dec->tokenState = detail::tokenArrayValue;
      break;
    }
    case detail::tokenObjectColon: {
      auto [c, err] = peek(dec);
      if (err != nullptr) {
        return err;
      }
      if (c != ':') {
        return New<SyntaxError>(SyntaxError{"expected colon after object key", InputOffset(dec)});
      }
      dec->scanp++;
      dec->tokenState = detail::tokenObjectValue;
      break;
    }
    default:
      break;
    }
    return nullptr;
  }

  static auto tokenValueAllowed(const ptr<Decoder>& dec) -> bool {
    switch (dec->tokenState) {
    case detail::tokenTopValue:
    case detail::tokenArrayStart:
    case detail::tokenArrayValue:
    case detail::tokenObjectValue:
      return true;
    default:
      return false;
    }
  }

  static void tokenValueEnd(const ptr<Decoder>& dec) {
    switch (dec->tokenState) {
    case detail::tokenArrayStart:
    case detail::tokenArrayValue:
      dec->tokenState = detail::tokenArrayComma;
      break;
    case detail::tokenObjectValue:
      dec->tokenState = detail::tokenObjectComma;
      break;
    default:
      break;
    }
  }

  static auto Token(ptr<Decoder> dec) -> std::tuple<json::Token, error> {
    for (;;) {
      auto [c, err] = peek(dec);
      if (err != nullptr) {
        return {nullptr, err};
      }
      switch (c) {
      case '[':
        if (!tokenValueAllowed(dec)) {
          return tokenError(dec, c);
        }
        dec->scanp++;
        dec->tokenStack = eo::append(dec->tokenStack, {dec->tokenState});
        dec->tokenState = detail::tokenArrayStart;
        return {Delim{'['}, nullptr};

      case ']':
        if (dec->tokenState != detail::tokenArrayStart && dec->tokenState != detail::tokenArrayComma) {
          return tokenError(dec, c);
        }
        dec->scanp++;
        dec->tokenState = eo::at(dec->tokenStack, eo::len(dec->tokenStack) - 1);
        dec->tokenStack = eo::subslice(dec->tokenStack, 0, eo::len(dec->tokenStack) - 1);
        tokenValueEnd(dec);
        return {Delim{']'}, nullptr};

      case '{':
        if (!tokenValueAllowed(dec)) {
          return tokenError(dec, c);
        }
        dec->scanp++;
        dec->tokenStack = eo::append(dec->tokenStack, {dec->tokenState});
        dec->tokenState = detail::tokenObjectStart;
        return {Delim{'{'}, nullptr};

      case '}':
        if (dec->tokenState != detail::tokenObjectStart && dec->tokenState != detail::tokenObjectComma) {
          return tokenError(dec, c);
        }
        dec->scanp++;
        dec->tokenState = eo::at(dec->tokenStack, eo::len(dec->tokenStack) - 1);
        dec->tokenStack = eo::subslice(dec->tokenStack, 0, eo::len(dec->tokenStack) - 1);
        tokenValueEnd(dec);
        return {Delim{'}'}, nullptr};

      case ':':
        if (dec->tokenState != detail::tokenObjectColon) {
          return tokenError(dec, c);
        }
        dec->scanp++;
        dec->tokenState = detail::tokenObjectValue;
        continue;

      case ',':
        if (dec->tokenState == detail::tokenArrayComma) {
          dec->scanp++;
          dec->tokenState = detail::tokenArrayValue;
          continue;
        }
        if (dec->tokenState == detail::tokenObjectComma) {
          dec->scanp++;
          dec->tokenState = detail::tokenObjectKey;
          continue;
        }
        return tokenError(dec, c);

      case '"':
        if (dec->tokenState == detail::tokenObjectStart || dec->tokenState == detail::tokenObjectKey) {
          auto x = New<std::string>();
          auto old = dec->tokenState;
          dec->tokenState = detail::tokenTopValue;
          auto err = Decode(dec, x);
          dec->tokenState = old;
          if (err != nullptr) {
            return {nullptr, err};
          }
          dec->tokenState = detail::tokenObjectColon;
          return {*x, nullptr};
        }
        [[fallthrough]];

      default: {
        if (!tokenValueAllowed(dec)) {
          return tokenError(dec, c);
        }
        auto x = New<any>();
        if (auto err = Decode(dec, x); err != nullptr) {
          return {nullptr, err};
        }
        return {*x, nullptr};
      }
      }
    }
  }

  static auto tokenError(const ptr<Decoder>& dec, uint8_t c) -> std::tuple<json::Token, error> {
    auto context = std::string{};
    switch (dec->tokenState) {
    case detail::tokenTopValue:
      context = " looking for beginning of value";
      break;
    case detail::tokenArrayStart:
    case detail::tokenArrayValue:
    case detail::tokenObjectValue:
      context = " looking for beginning of value";
      break;
    case detail::tokenArrayComma:
      context = " after array element";
      break;
    case detail::tokenObjectKey:
      context = " looking for beginning of object key string";
      break;
    case detail::tokenObjectColon:
      context = " after object key";
      break;
    case detail::tokenObjectComma:
      context = " after object key:value pair";
      break;
    default:
      break;
    }
    return {nullptr,
      New<SyntaxError>(SyntaxError{"invalid character " + detail::quoteChar(c) + context, InputOffset(dec)})};
  }

  static auto More(ptr<Decoder> dec) -> bool {
    auto [c, err] = peek(dec);
    return err == nullptr && c != ']' && c != '}';
  }

  static auto peek(const ptr<Decoder>& dec) -> std::tuple<uint8_t, error> {
    auto err = error{};
    for (;;) {
      for (auto i = dec->scanp; i < eo::len(dec->buf); i++) {
        auto c = eo::at(dec->buf, i);
        if (detail::isSpace(c)) {
          continue;
        }
        dec->scanp = i;
        return {c, nullptr};
      }
      if (err != nullptr) {
        return {0, err};
      }
      err = refill(dec);
    }
  }

  static auto InputOffset(ptr<Decoder> dec) -> int64_t {
    return dec->scanned + dec->scanp;
  }

  using _eo_methods = eo::methods<eo::method<&Decoder::Buffered, "Buffered">,
    eo::method<&Decoder::Decode, "Decode">,
    eo::method<&Decoder::DisallowUnknownFields, "DisallowUnknownFields">,
    eo::method<&Decoder::InputOffset, "InputOffset">,
    eo::method<&Decoder::More, "More">,
    eo::method<&Decoder::Token, "Token">,
    eo::method<&Decoder::UseNumber, "UseNumber">>;
};

// Go's json.NewDecoder.
inline auto NewDecoder(io::Reader r) -> ptr<Decoder> {
  auto dec = New<Decoder>();
  dec->r = std::move(r);
  return dec;
}

// Go's json.Encoder.
struct Encoder {
  static constexpr std::string_view _eo_name = "json.Encoder";

  io::Writer w{};
  error err{};
  bool escapeHTML{};

  slice<uint8_t> indentBuf{};
  std::string indentPrefix{};
  std::string indentValue{};

  static auto Encode(ptr<Encoder> enc, const any& v) -> error {
    if (enc->err != nullptr) {
      return enc->err;
    }

    auto e = detail::newEncodeState();

    auto err = detail::encodeState::marshal(e, v, detail::encOpts{.escapeHTML = enc->escapeHTML});
    if (err != nullptr) {
      return err;
    }

    detail::encodeState::WriteByte(e, '\n');

    auto b = detail::encodeState::Bytes(e);
    if (enc->indentPrefix != "" || enc->indentValue != "") {
      std::tie(enc->indentBuf, err) =
        detail::appendIndent(eo::subslice(enc->indentBuf, 0, 0), b, enc->indentPrefix, enc->indentValue);
      if (err != nullptr) {
        return err;
      }
      b = enc->indentBuf;
    }
    if (std::tie(std::ignore, err) = enc->w.Write(b); err != nullptr) {
      enc->err = err;
    }
    return err;
  }

  static void SetIndent(ptr<Encoder> enc, std::string prefix, std::string indent) {
    enc->indentPrefix = std::move(prefix);
    enc->indentValue = std::move(indent);
  }

  static void SetEscapeHTML(ptr<Encoder> enc, bool on) {
    enc->escapeHTML = on;
  }

  using _eo_methods = eo::methods<eo::method<&Encoder::Encode, "Encode">,
    eo::method<&Encoder::SetEscapeHTML, "SetEscapeHTML">,
    eo::method<&Encoder::SetIndent, "SetIndent">>;
};

// Go's json.NewEncoder.
inline auto NewEncoder(io::Writer w) -> ptr<Encoder> {
  auto enc = New<Encoder>();
  enc->w = std::move(w);
  enc->escapeHTML = true;
  return enc;
}

// Go's json.RawMessage.
struct RawMessage : eo::defined<RawMessage, slice<uint8_t>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "json.RawMessage";

  static auto MarshalJSON(RawMessage m) -> std::tuple<slice<uint8_t>, error> {
    if (m == nullptr) {
      return {eo::bytes_from_string("null"), nullptr};
    }
    return {m, nullptr};
  }

  static auto UnmarshalJSON(ptr<RawMessage> m, slice<uint8_t> data) -> error {
    if (m == nullptr) {
      return errors::New("json.RawMessage: UnmarshalJSON on nil pointer");
    }
    *m = RawMessage{eo::append_slice(eo::subslice(*m, 0, 0), data)};
    return nullptr;
  }

  using _eo_methods = eo::methods<eo::method<&RawMessage::MarshalJSON, "MarshalJSON">,
    eo::method<&RawMessage::UnmarshalJSON, "UnmarshalJSON">>;
};
} // namespace eo::json
