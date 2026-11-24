#include <eo/encoding/binary.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

void roundTrip(binary::ByteOrder order) {
  auto b = eo::make_slice<uint8_t>(8);
  order.PutUint16(b, 0x0102);
  auto v16 = order.Uint16(b);
  order.PutUint32(b, 0x01020304);
  auto v32 = order.Uint32(b);
  order.PutUint64(b, uint64_t{72623859790382856});
  auto v64 = order.Uint64(b);
  fmt::Println(order, b, v16, v32, v64);
}

void appendAll(binary::AppendByteOrder order) {
  slice<uint8_t> b;
  b = order.AppendUint16(b, 0xabcd);
  b = order.AppendUint32(b, 0x01020304);
  b = order.AppendUint64(b, uint64_t{1234605616436508552});
  fmt::Println(order.String(), b);
}

void uvarint(slice<uint8_t> buf) {
  auto [x, n] = binary::Uvarint(buf);
  auto [y, m] = binary::Varint(buf);
  fmt::Println(buf, x, n, y, m);
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
  auto b = slice<uint8_t>::of({1, 2, 3, 4, 5, 6, 7, 8});
  auto _eo_call_0 = binary::bigEndian::Uint16(binary::BigEndian, b);
  auto _eo_call_1 = binary::littleEndian::Uint16(binary::LittleEndian, b);
  auto _eo_call_2 = binary::bigEndian::Uint32(binary::BigEndian, b);
  auto _eo_call_3 = binary::littleEndian::Uint32(binary::LittleEndian, b);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2, _eo_call_3);
  auto _eo_call_4 = binary::bigEndian::Uint64(binary::BigEndian, b);
  auto _eo_call_5 = binary::littleEndian::Uint64(binary::LittleEndian, b);
  fmt::Println(_eo_call_4, _eo_call_5);
  binary::bigEndian::PutUint32(binary::BigEndian, eo::subslice(b, 4), uint32_t{3735928559u});
  binary::littleEndian::PutUint16(binary::LittleEndian, b, 0xbeef);
  fmt::Println(b);
  roundTrip(binary::BigEndian);
  roundTrip(binary::LittleEndian);
  appendAll(binary::BigEndian);
  appendAll(binary::LittleEndian);
  auto out = binary::bigEndian::AppendUint64(binary::BigEndian, slice<uint8_t>::of({9}), 1);
  fmt::Println(out, binary::BigEndian, binary::littleEndian::GoString(binary::LittleEndian));
  fmt::Printf("%v %s %T %T\n"s, binary::BigEndian, binary::LittleEndian, binary::BigEndian, binary::LittleEndian);

  // Varints, with the lengths of each size and the overflow and short-buffer results.
  for (auto [_eo_range_key, x] : eo::range(slice<uint64_t>::of({0,
         1,
         127,
         128,
         300,
         16383,
         16384,
         uint64_t{9223372036854775807},
         uint64_t{9223372036854775808u},
         uint64_t{18446744073709551615u}}))) {
    auto buf = eo::make_slice<uint8_t>(binary::MaxVarintLen64);
    auto n = binary::PutUvarint(buf, x);
    auto [y, m] = binary::Uvarint(eo::subslice(buf, 0, n));
    fmt::Printf("%v %v %v | "s, n, y == x, m);
  }
  fmt::Println();
  for (auto [_eo_range_key, x] : eo::range(
         slice<int64_t>::of({0, -1, 1, -64, 64, -65, int64_t{4611686018427387904}, int64_t{-4611686018427387904}}))) {
    auto buf = eo::make_slice<uint8_t>(binary::MaxVarintLen64);
    auto n = binary::PutVarint(buf, x);
    auto [y, m] = binary::Varint(eo::subslice(buf, 0, n));
    fmt::Printf("%v %v %v | "s, eo::subslice(buf, 0, n), y, m);
  }
  fmt::Println();
  uvarint(nullptr);
  uvarint(slice<uint8_t>::of({0x80}));
  uvarint(slice<uint8_t>::of({0xac, 0x02, 0xff}));
  uvarint(slice<uint8_t>::of({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}));
  uvarint(slice<uint8_t>::of({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02}));
  uvarint(slice<uint8_t>::of({0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01}));
  auto _eo_call_6 = binary::AppendUvarint(nullptr, 300);
  auto _eo_call_7 = binary::AppendVarint(slice<uint8_t>::of({7}), -3);
  fmt::Println(_eo_call_6, _eo_call_7, binary::MaxVarintLen16, binary::MaxVarintLen32);

  attempt("uint64"s, [=]() mutable { binary::bigEndian::Uint64(binary::BigEndian, eo::subslice(b, 0, 7)); });
  attempt("put"s, [=]() mutable { binary::littleEndian::PutUint32(binary::LittleEndian, eo::subslice(b, 6), 1); });
  attempt("uvarint"s, [=]() mutable { binary::PutUvarint(eo::make_slice<uint8_t>(1), 300); });
  attempt("nil"s, [=]() mutable { binary::bigEndian::Uint16(binary::BigEndian, nullptr); });
  co_return;
}
