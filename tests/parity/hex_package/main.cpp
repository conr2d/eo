#include <eo/encoding/hex.h>
#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

void decode(std::string s) {
  auto [b, err] = hex::DecodeString(s);
  auto ibe = New<hex::InvalidByteError>();
  auto isInvalid = errors::As(err, ibe);
  fmt::Println(s, b, eo::len(b), err, err == hex::ErrLength, isInvalid, uint8_t(*ibe));
}

void attempt(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, "recovered:"s, recover()); });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  auto src = slice<uint8_t>::of({0x00, 0x01, 0xab, 0xff, 'h', 'i'});
  auto s = hex::EncodeToString(src);
  auto _eo_call_0 = hex::EncodedLen(eo::len(src));
  auto _eo_call_1 = hex::DecodedLen(eo::len(s));
  auto _eo_call_2 = hex::EncodeToString(nullptr);
  fmt::Println(s, _eo_call_0, _eo_call_1, _eo_call_2 == ""s);
  auto dst = eo::make_slice<uint8_t>(14);
  auto n = hex::Encode(dst, src);
  fmt::Println(n, eo::string_from_bytes(eo::subslice(dst, 0, n)), eo::subslice(dst, 12));

  for (auto [_eo_range_key, in] : eo::range(slice<std::string>::of(
         {"00ab"s, "DEADbeef"s, ""s, "abc"s, "zz"s, "a"s, "0g"s, "g0"s, "0x12"s, "abz"s, "ab\xff"s}))) {
    decode(in);
  }
  auto out = eo::make_slice<uint8_t>(4);
  auto [m, err] = hex::Decode(out, eo::bytes_from_string("01020z04"s));
  fmt::Println(m, out, err);
  fmt::Printf("%v %T %d\n"s, hex::InvalidByteError{10}, hex::InvalidByteError{233}, hex::InvalidByteError{97});

  attempt("short encode"s, [=]() mutable { hex::Encode(eo::make_slice<uint8_t>(3), slice<uint8_t>::of({1, 2})); });
  attempt("short decode"s, [=]() mutable { hex::Decode(eo::make_slice<uint8_t>(1), eo::bytes_from_string("0102"s)); });
  co_return;
}
