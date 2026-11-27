// Provisional: strings.Replacer's Replace is called as a member, as in strings_bytes_random.
#include <eo/embed.h>
#include <eo/encoding/json.h>
#include <eo/fmt.h>
#include <eo/os.h>
#include <eo/strconv.h>
#include <eo/strings.h>
#include <eo/time.h>

#include <cstdint>
#include <span>
#include <string>
#include <tuple>

using namespace eo;
using namespace std::string_literals;

struct event;

auto next() -> uint64_t;
auto pick(int64_t m) -> int64_t;
auto table(std::string name) -> slice<slice<std::string>>;
auto load(std::string name) -> ptr<time::Location>;
void show(std::string label, time::Time t, error err);
void parsing();
void formatting();
auto randomLayout() -> std::string;
auto randomTime() -> time::Time;
void instants();
void durations();
void locations();
void marshaling();
auto tzif(std::string footer) -> slice<uint8_t>;
void rules();
func<> monotonic();

struct event {
  static constexpr std::string_view _eo_name = "main.event";

  time::Time At{};
  time::Duration D{};
  ptr<time::Time> P{};
  time::Month Month{};

  auto operator==(const event&) const -> bool = default;
};

auto _eo_embed_tables() -> std::span<const eo::embed::file>;
auto _eo_embed_zoneinfo() -> std::span<const eo::embed::file>;

std::string tables = embed::string_of(_eo_embed_tables());

embed::FS zoneinfo = embed::FS(_eo_embed_zoneinfo());

uint64_t seed = 7;

constexpr int64_t rounds = 100;

slice<ptr<time::Location>> zones{};

slice<std::string> pieces = slice<std::string>::of({"2006"s,
  "06"s,
  "01"s,
  "1"s,
  "Jan"s,
  "January"s,
  "02"s,
  "2"s,
  "_2"s,
  "__2"s,
  "002"s,
  "Mon"s,
  "Monday"s,
  "15"s,
  "3"s,
  "03"s,
  "04"s,
  "4"s,
  "05"s,
  "5"s,
  "PM"s,
  "pm"s,
  "MST"s,
  "Z0700"s,
  "Z070000"s,
  "Z07"s,
  "Z07:00"s,
  "Z07:00:00"s,
  "-0700"s,
  "-070000"s,
  "-07"s,
  "-07:00"s,
  "-07:00:00"s,
  ".0"s,
  ".000"s,
  ".999"s,
  ".999999999"s,
  ",000"s,
  ",9"s,
  " "s,
  "-"s,
  ":"s,
  "T"s,
  "/"s,
  "x"s,
  "Ja"s,
  "Mo"s,
  "_"s,
  "0"s,
  "."s,
  ","s,
  "J"s,
  "M"s,
  "P"s});

const std::string mutations = "0123456789 :-+.,ZTJMPajmp"s;

auto next() -> uint64_t {
  seed = seed * uint64_t{6364136223846793005u} + uint64_t{1442695040888963407u};
  return seed;
}

auto pick(int64_t m) -> int64_t {
  return int64_t(eo::rem(next() >> 32, uint64_t(m)));
}

auto table(std::string name) -> slice<slice<std::string>> {
  slice<slice<std::string>> out{};
  auto in = false;
  for (auto [_eo_range_key, line] : eo::range(strings::Split(tables, "\n"s))) {
    if (strings::HasPrefix(line, "== "s)) {
      in = line == "== "s + name;
      continue;
    }
    if (!in || line == ""s) {
      continue;
    }
    slice<std::string> fields{};
    for (auto [_eo_range_key, f] : eo::range(strings::Split(line, "\t"s))) {
      auto [s, err] = strconv::Unquote(f);
      if (err != nullptr) {
        panic(err);
      }
      fields = eo::append(fields, {s});
    }
    out = eo::append(out, {fields});
  }
  if (eo::len(out) == 0) {
    panic("empty table "s + name);
  }
  return out;
}

auto load(std::string name) -> ptr<time::Location> {
  auto [data, err] = embed::FS::ReadFile(zoneinfo, "zoneinfo/"s + name);
  if (err != nullptr) {
    panic(err);
  }
  ptr<time::Location> loc{};
  std::tie(loc, err) = time::LoadLocationFromTZData(name, data);
  if (err != nullptr) {
    panic(err);
  }
  return loc;
}

void show(std::string label, time::Time t, error err) {
  if (err != nullptr) {
    fmt::Printf("%s: error %v\n"s, label, err);
    return;
  }
  auto [name, offset] = time::Time::Zone(t);
  auto _eo_call_0 = time::Time::Unix(t);
  auto _eo_call_1 = time::Time::Nanosecond(t);
  fmt::Printf("%s: %v [%s %d %d %d]\n"s, label, t, name, offset, _eo_call_0, _eo_call_1);
}

void parsing() {
  fmt::Println("-- parse tables"s);
  auto nyc = load("America/New_York"s);
  for (auto [_eo_range_key, name] : eo::range(slice<std::string>::of({"parseTests"s, "rubyTests"s}))) {
    for (auto [_eo_range_key, f] : eo::range(table(name))) {
      auto [t, err] = time::Parse(eo::at(f, 1), eo::at(f, 2));
      show(eo::at(f, 1) + " | "s + eo::at(f, 2), t, err);
      std::tie(t, err) = time::ParseInLocation(eo::at(f, 1), eo::at(f, 2), nyc);
      show("  in NYC"s, t, err);
    }
  }
  for (auto [_eo_range_key, f] : eo::range(table("parseErrorTests"s))) {
    auto [t, err] = time::Parse(eo::at(f, 0), eo::at(f, 1));
    show(eo::at(f, 0) + " | "s + eo::at(f, 1), t, err);
  }
  for (auto [_eo_range_key, f] : eo::range(table("secondsTimeZoneOffsetTests"s))) {
    auto [t, err] = time::Parse(eo::at(f, 0), eo::at(f, 1));
    show(eo::at(f, 0) + " | "s + eo::at(f, 1), t, err);
  }
  auto layouts = slice<std::string>::of(
    {time::ANSIC, time::RFC3339, time::RFC3339Nano, "01-02"s, "2006-01-02"s, "Jan _2"s, "MST"s, "Z07:00"s, "-07"s});
  for (auto [_eo_range_key, name] : eo::range(slice<std::string>::of(
         {"dayOutOfRangeTests"s, "monthOutOfRangeTests"s, "longFractionalDigitsTests"s, "parseTimeZoneTests"s}))) {
    for (auto [_eo_range_key, f] : eo::range(table(name))) {
      for (auto [_eo_range_key, layout] : eo::range(layouts)) {
        auto [t, err] = time::Parse(layout, eo::at(f, 0));
        show(layout + " | "s + eo::at(f, 0), t, err);
      }
    }
  }
  for (auto [_eo_range_key, f] : eo::range(table("jsonTests"s))) {
    time::Time t{};
    auto err = time::Time::UnmarshalJSON(ref(t), eo::bytes_from_string(eo::at(f, 0)));
    show("json "s + eo::at(f, 0), t, err);
    slice<uint8_t> b{};
    std::tie(b, err) = time::Time::MarshalJSON(t);
    fmt::Printf("  %s %v\n"s, b, err);
  }
}

void formatting() {
  fmt::Println("-- format tables"s);
  auto t = time::Unix(0, 1233810057012345600);
  slice<std::string> layouts{};
  for (auto [_eo_range_key, f] : eo::range(table("formatTests"s))) {
    layouts = eo::append(layouts, {eo::at(f, 1)});
  }
  for (auto [_eo_range_key, f] : eo::range(table("nextStdChunkTests"s))) {
    auto _eo_call_0 = strings::NewReplacer(slice<std::string>::of({"("s, ""s, ")"s, ""s}));
    auto _eo_call_1 = _eo_call_0->Replace(eo::at(f, 0));
    layouts = eo::append(layouts, {_eo_call_1});
  }
  layouts = eo::append(layouts,
    {time::Layout,
      time::RFC822,
      time::RFC822Z,
      "Z070000 Z07:00:00 Z07 -070000 -07:00:00 -07 MST"s,
      ".0 .00 .999 ,000 ,999999999 .0000000000 .9999999999"s,
      "January Monday Janet Mondays"s,
      ""s});
  for (auto [_eo_range_key, loc] : eo::range(zones)) {
    auto u = time::Time::In(t, loc);
    for (auto [_eo_range_key, layout] : eo::range(layouts)) {
      fmt::Printf("%s %q: %q\n"s, loc, layout, time::Time::Format(u, layout));
    }
    fmt::Printf("%s: %v %s\n"s, loc, u, time::Time::GoString(u));
  }
}

auto randomLayout() -> std::string {
  auto n = 1 + pick(8);
  auto s = ""s;
  for (auto i = int64_t{0}; i < n; i++) {
    auto _eo_call_0 = pick(eo::len(pieces));
    s += eo::at(pieces, _eo_call_0);
  }
  return s;
}

auto randomTime() -> time::Time {
  int64_t sec{};
  if (auto _eo_switch = pick(4); _eo_switch == 0) {
    sec = int64_t(next() >> 34) - int64_t{536870912};
  } else if (_eo_switch == 1) {
    sec = int64_t(next() >> 28) - int64_t{34359738368};
  } else if (_eo_switch == 2) {
    sec = int64_t(next() >> 1);
    if ((next() & 1) == 0) {
      sec = -sec;
    }
  } else {
    sec = 1700000000 + int64_t(pick(1000000000)) - 500000000;
  }
  auto nsec = int64_t(pick(1000000000));
  if (pick(3) == 0) {
    nsec = int64_t(pick(1000)) * 1000000;
  }
  auto _eo_call_1 = time::Unix(sec, nsec);
  auto _eo_call_2 = pick(eo::len(zones));
  return time::Time::In(_eo_call_1, eo::at(zones, _eo_call_2));
}

void instants() {
  fmt::Println("-- instants"s);
  for (auto i = int64_t{0}; i < rounds; i++) {
    auto t = randomTime();
    auto [y, m, d] = time::Time::Date(t);
    auto [h, mi, s] = time::Time::Clock(t);
    auto [iy, iw] = time::Time::ISOWeek(t);
    auto [name, offset] = time::Time::Zone(t);
    auto [start, end] = time::Time::ZoneBounds(t);
    auto _eo_call_0 = time::Time::YearDay(t);
    auto _eo_call_1 = time::Time::Weekday(t);
    auto _eo_call_2 = time::Time::Nanosecond(t);
    auto _eo_call_3 = time::Time::IsDST(t);
    fmt::Printf("%d: %v | %d %v %d %d:%d:%d %d %v %d %d-%d %s %d %v %v %v\n"s,
      i,
      t,
      y,
      m,
      d,
      h,
      mi,
      s,
      _eo_call_0,
      _eo_call_1,
      _eo_call_2,
      iy,
      iw,
      name,
      offset,
      _eo_call_3,
      start,
      end);
    auto _eo_call_4 = time::Time::Year(t);
    auto _eo_call_5 = time::Time::Month(t);
    auto _eo_call_6 = time::Time::Day(t);
    auto _eo_call_7 = time::Time::Hour(t);
    auto _eo_call_8 = time::Time::Minute(t);
    auto _eo_call_9 = time::Time::Second(t);
    auto _eo_call_10 = time::Time::UnixMilli(t);
    auto _eo_call_11 = time::Time::UnixMicro(t);
    auto _eo_call_12 = time::Time::UnixNano(t);
    auto _eo_call_13 = time::Time::UTC(t);
    fmt::Printf("  %d %d %d %d %d %d %d %v %v %v\n"s,
      _eo_call_4,
      _eo_call_5,
      _eo_call_6,
      _eo_call_7,
      _eo_call_8,
      _eo_call_9,
      _eo_call_10,
      _eo_call_11,
      _eo_call_12,
      _eo_call_13);
    auto layout = randomLayout();
    auto s1 = time::Time::Format(t, layout);
    auto _eo_call_14 = time::Time::Format(t, time::RFC3339Nano);
    auto _eo_call_15 = time::Time::GoString(t);
    fmt::Printf("  %q %q %q %q\n"s, layout, s1, _eo_call_14, _eo_call_15);
    auto [p, err] = time::ParseInLocation(layout, s1, time::Time::Location(t));
    show("  parse"s, p, err);
    std::tie(p, err) = time::Parse(layout, s1);
    show("  parse UTC"s, p, err);
    auto b = eo::bytes_from_string(s1);
    if (eo::len(b) > 0) {
      auto _eo_call_17 = pick(eo::len(b));
      auto _eo_call_18 = pick(eo::len(mutations));
      eo::at(b, _eo_call_17) = eo::at(mutations, _eo_call_18);
    }
    std::tie(p, err) = time::Parse(layout, eo::string_from_bytes(b));
    show("  mutated "s + strconv::Quote(eo::string_from_bytes(b)), p, err);
    std::tie(p, err) = time::Parse(time::RFC3339, time::Time::Format(t, time::RFC3339));
    show("  rfc3339"s, p, err);
    auto d1 = time::Duration(int64_t(next()));
    if (pick(2) == 0) {
      auto _eo_call_21 = next();
      auto _eo_call_22 = pick(64);
      d1 = time::Duration(int64_t(eo::shr(_eo_call_21, uint64_t(_eo_call_22))));
    }
    auto a = time::Time::Add(t, d1);
    fmt::Printf("  add %v: %v sub %v\n"s, d1, a, time::Time::Sub(a, t));
    auto _eo_call_24 = pick(41);
    auto _eo_call_25 = pick(301);
    auto _eo_call_26 = pick(2001);
    auto _eo_call_27 = time::Time::AddDate(t, _eo_call_24 - 20, _eo_call_25 - 150, _eo_call_26 - 1000);
    fmt::Printf("  adddate %v\n"s, _eo_call_27);
    auto r = time::Duration(1 + pick(1000000));
    if (auto _eo_switch = pick(4); _eo_switch == 0) {
      r *= time::Second;
    } else if (_eo_switch == 1) {
      auto _eo_call_28 = next();
      auto _eo_call_29 = pick(63);
      r = time::Duration(int64_t(eo::shr(_eo_call_28, uint64_t(1 + _eo_call_29))));
    }
    auto _eo_call_30 = time::Time::Round(t, r);
    auto _eo_call_31 = time::Time::Truncate(t, r);
    fmt::Printf("  round %v: %v %v\n"s, r, _eo_call_30, _eo_call_31);
    auto u = randomTime();
    auto _eo_call_32 = time::Time::Before(t, u);
    auto _eo_call_33 = time::Time::After(t, u);
    auto _eo_call_34 = time::Time::In(t, time::UTC);
    auto _eo_call_35 = time::Time::Equal(t, _eo_call_34);
    auto _eo_call_36 = time::Time::Compare(t, u);
    fmt::Printf("  cmp %v %v %v %d\n"s, _eo_call_32, _eo_call_33, _eo_call_35, _eo_call_36);
    slice<uint8_t> j{};
    std::tie(j, err) = time::Time::MarshalJSON(t);
    fmt::Printf("  json %s %v\n"s, j, err);
    slice<uint8_t> x{};
    std::tie(x, err) = time::Time::MarshalText(t);
    fmt::Printf("  text %s %v\n"s, x, err);
    slice<uint8_t> bin{};
    std::tie(bin, err) = time::Time::MarshalBinary(t);
    fmt::Printf("  binary %x %v\n"s, bin, err);
    time::Time back{};
    if (err == nullptr) {
      err = time::Time::UnmarshalBinary(ref(back), bin);
      fmt::Printf("  unbinary %v %v %v\n"s, back, time::Time::Equal(back, t), err);
    }
    if (j != nullptr) {
      err = time::Time::UnmarshalJSON(ref(back), j);
      fmt::Printf("  unjson %v %v %v\n"s, back, time::Time::Equal(back, t), err);
    }
  }
}

void durations() {
  fmt::Println("-- durations"s);
  for (auto [_eo_range_key, name] :
    eo::range(slice<std::string>::of({"parseDurationTests"s, "parseDurationErrorTests"s}))) {
    for (auto [_eo_range_key, f] : eo::range(table(name))) {
      auto [d, err] = time::ParseDuration(eo::at(f, 0));
      fmt::Printf("%q: %d %v %v\n"s, eo::at(f, 0), int64_t(d), d, err);
    }
  }
  auto units = slice<std::string>::of({"ns"s, "us"s, "µs"s, "μs"s, "ms"s, "s"s, "m"s, "h"s, "x"s, ""s, "S"s, "µ"s});
  for (auto i = int64_t{0}; i < rounds; i++) {
    auto _eo_call_0 = next();
    auto _eo_call_1 = pick(64);
    auto d = time::Duration(int64_t(eo::shr(_eo_call_0, uint64_t(_eo_call_1))));
    if (pick(2) == 0) {
      d = -d;
    }
    auto _eo_call_2 = next();
    auto _eo_call_3 = pick(64);
    auto m = time::Duration(int64_t(eo::shr(_eo_call_2, uint64_t(_eo_call_3))));
    if (pick(5) == 0) {
      m = -m;
    }
    auto _eo_call_4 = time::Duration::Nanoseconds(d);
    auto _eo_call_5 = time::Duration::Microseconds(d);
    auto _eo_call_6 = time::Duration::Milliseconds(d);
    auto _eo_call_7 = time::Duration::Seconds(d);
    auto _eo_call_8 = time::Duration::Minutes(d);
    auto _eo_call_9 = time::Duration::Hours(d);
    auto _eo_call_10 = time::Duration::Truncate(d, m);
    auto _eo_call_11 = time::Duration::Round(d, m);
    auto _eo_call_12 = time::Duration::Abs(d);
    fmt::Printf("%d %v %d %d %d %v %v %v | %v %v %v\n"s,
      int64_t(d),
      d,
      _eo_call_4,
      _eo_call_5,
      _eo_call_6,
      _eo_call_7,
      _eo_call_8,
      _eo_call_9,
      _eo_call_10,
      _eo_call_11,
      _eo_call_12);
    auto [back, err] = time::ParseDuration(time::Duration::String(d));
    fmt::Printf("  %v %v\n"s, back == d, err);
    auto s = ""s;
    for (auto k = 1 + pick(3); k > 0; k--) {
      if (pick(4) == 0) {
        s += "-"s;
      }
      auto _eo_call_13 = next();
      auto _eo_call_14 = pick(64);
      s += strconv::FormatUint(eo::shr(_eo_call_13, uint64_t(_eo_call_14)), 10);
      if (pick(3) == 0) {
        auto _eo_call_15 = next();
        auto _eo_call_16 = pick(64);
        auto _eo_call_17 = strconv::FormatUint(eo::shr(_eo_call_15, uint64_t(_eo_call_16)), 10);
        s += "."s + _eo_call_17;
      }
      auto _eo_call_18 = pick(eo::len(units));
      s += eo::at(units, _eo_call_18);
    }
    time::Duration p{};
    std::tie(p, err) = time::ParseDuration(s);
    fmt::Printf("  %q %d %v\n"s, s, int64_t(p), err);
  }
  auto _eo_call_19 = time::Duration::Abs(time::Duration{-9223372036854775807 - 1});
  auto _eo_call_20 = time::Duration::Round(time::Duration{-9223372036854775807 - 1}, time::Duration{3});
  fmt::Println(time::Duration{-9223372036854775807 - 1}, time::Duration{9223372036854775807}, _eo_call_19, _eo_call_20);
  fmt::Println(time::Month{13},
    time::Month{0},
    time::Month{-1},
    time::Weekday{7},
    time::Weekday{-1},
    time::March,
    time::Saturday);
}

void locations() {
  fmt::Println("-- locations"s);
  for (auto [_eo_range_key, name] : eo::range(slice<std::string>::of({""s,
         "UTC"s,
         "Local"s,
         "America/New_York"s,
         "../etc/passwd"s,
         "/etc/localtime"s,
         "\\x"s,
         "Nowhere/Zone"s,
         "America"s,
         "Etc/GMT+5"s,
         "Asia/Kolkata"s,
         "posixrules"s,
         "Factory"s}))) {
    auto [loc, err] = time::LoadLocation(name);
    if (err != nullptr) {
      fmt::Printf("%q: error %v\n"s, name, err);
      continue;
    }
    fmt::Printf("%q: %v %v %v\n"s, name, loc, loc == time::UTC, loc == time::Local);
  }
  for (auto [_eo_range_key, data] : eo::range(slice<std::string>::of({""s,
         "TZif"s,
         "TZif2"s,
         "TZif\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}"s,
         "nope"s}))) {
    auto err = std::get<1>(time::LoadLocationFromTZData("x"s, eo::bytes_from_string(data)));
    fmt::Printf("%q: %v\n"s, data, err);
  }
  auto _eo_call_0 = time::FixedZone(""s, 3600);
  auto _eo_call_1 = time::FixedZone(""s, 3600);
  auto _eo_call_2 = time::FixedZone(""s, 3601);
  auto _eo_call_3 = time::FixedZone(""s, 3601);
  auto _eo_call_4 = time::FixedZone("A"s, 3600);
  auto _eo_call_5 = time::FixedZone("A"s, 3600);
  auto _eo_call_6 = time::FixedZone(""s, 54000);
  auto _eo_call_7 = time::FixedZone(""s, 54000);
  fmt::Println(_eo_call_0 == _eo_call_1, _eo_call_2 == _eo_call_3, _eo_call_4 == _eo_call_5, _eo_call_6 == _eo_call_7);
  auto _eo_call_8 = time::FixedZone(""s, -1800);
  auto _eo_call_9 = time::FixedZone("X\x{01}\""s, 0);
  auto _eo_call_10 = time::Location::String(_eo_call_9);
  fmt::Println(time::UTC, time::Local, _eo_call_8, _eo_call_10);
  auto t = time::Date(2009, time::February, 5, 5, 0, 57, 12345600, time::FixedZone("Non-ASCII character ⏰"s, 10800));
  auto _eo_call_11 = time::Time::GoString(t);
  auto _eo_call_12 = time::Date(2009, time::February, 5, 5, 0, 57, 12345600, time::Local);
  auto _eo_call_13 = time::Time::GoString(_eo_call_12);
  fmt::Println(_eo_call_11, _eo_call_13);
  auto ny = load("America/New_York"s);
  for (auto [_eo_range_key, s] : eo::range(slice<std::string>::of({"2021-03-14 01:59:59"s,
         "2021-03-14 02:00:00"s,
         "2021-03-14 02:30:00"s,
         "2021-03-14 03:00:00"s,
         "2021-11-07 00:59:59"s,
         "2021-11-07 01:00:00"s,
         "2021-11-07 01:30:00"s,
         "2021-11-07 02:00:00"s}))) {
    auto [u, err] = time::ParseInLocation(time::DateTime, s, ny);
    show(s, u, err);
    show("  local"s, time::Time::Local(u), nullptr);
  }
  for (auto [_eo_range_key, f] : eo::range(slice<slice<int64_t>>::of({slice<int64_t>::of({2016, 12, 31, 23, 59, 60}),
         slice<int64_t>::of({2000, 2, 30, 0, 0, 0}),
         slice<int64_t>::of({2000, 0, 0, 0, 0, 0}),
         slice<int64_t>::of({-1, 13, 32, 25, 61, 61}),
         slice<int64_t>::of({1, 1, 1, 0, 0, 0}),
         slice<int64_t>::of({0, 1, 1, 0, 0, 0}),
         slice<int64_t>::of({292277026596, 12, 4, 15, 30, 7}),
         slice<int64_t>::of({-292277022399, 1, 1, 0, 0, 0})}))) {
    for (auto [_eo_range_key, loc] : eo::range(zones)) {
      auto _eo_call_14 = fmt::Sprint(f, loc);
      auto _eo_call_15 = time::Date(eo::at(f, 0),
        time::Month(eo::at(f, 1)),
        eo::at(f, 2),
        eo::at(f, 3),
        eo::at(f, 4),
        eo::at(f, 5),
        -1,
        loc);
      show(_eo_call_14, _eo_call_15, nullptr);
    }
  }
  time::Time zero{};
  auto _eo_call_16 = time::Time::IsZero(zero);
  auto _eo_call_17 = time::Time::Location(zero);
  auto _eo_call_18 = time::Time::GoString(zero);
  auto _eo_call_19 = time::Time::Unix(zero);
  auto _eo_call_20 = time::Time::Weekday(zero);
  auto _eo_call_21 = time::Time::YearDay(zero);
  fmt::Println(zero, _eo_call_16, _eo_call_17, _eo_call_18, _eo_call_19, _eo_call_20, _eo_call_21);
  auto bad = slice<std::string>::of({""s,
    "\x{03}"s,
    "\x{01}\x{00}"s,
    "\x{02}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}\x{00}"s,
    "\x{01}\x{00}\x{00}\x{00}\x{0e}\x{d5}\x{0b}\x{a0}\x{e8}\x{00}\x{00}\x{00}\x{00}\x{ff}\x{ff}"s});
  for (auto [_eo_range_key, b] : eo::range(bad)) {
    time::Time u{};
    auto err = time::Time::UnmarshalBinary(ref(u), eo::bytes_from_string(b));
    fmt::Printf("%q: %v %v\n"s, b, u, err);
  }
  for (auto [_eo_range_key, j] : eo::range(slice<std::string>::of({"null"s,
         "\"\""s,
         "1"s,
         "\"2006-01-02T15:04:05Z\""s,
         "\"2006-01-02T15:04:05,5Z\""s,
         "\"2006-01-02T1:04:05Z\""s,
         "\"2006-01-02T15:04:05+24:00\""s,
         "\"2006-01-02T15:04:05+23:60\""s}))) {
    time::Time u{};
    auto err = time::Time::UnmarshalJSON(ref(u), eo::bytes_from_string(j));
    fmt::Printf("%s: %v %v\n"s, j, u, err);
  }
  auto _eo_call_22 = time::Date(10000, time::Month{1}, 1, 0, 0, 0, 0, time::UTC);
  auto _eo_call_23 = time::Date(-1, time::Month{1}, 1, 0, 0, 0, 0, time::UTC);
  auto _eo_call_24 = time::FixedZone(""s, 86400);
  auto _eo_call_25 = time::Date(2000, time::Month{1}, 1, 0, 0, 0, 0, _eo_call_24);
  auto _eo_call_26 = time::FixedZone(""s, -1);
  auto _eo_call_27 = time::Date(2000, time::Month{1}, 1, 0, 0, 0, 0, _eo_call_26);
  for (auto [_eo_range_key, u] :
    eo::range(slice<time::Time>::of({_eo_call_22, _eo_call_23, _eo_call_25, _eo_call_27}))) {
    auto [j, err] = time::Time::MarshalJSON(u);
    auto [x, err2] = time::Time::MarshalText(u);
    auto [b, err3] = time::Time::MarshalBinary(u);
    auto [a, err4] = time::Time::AppendText(u, eo::bytes_from_string("x"s));
    fmt::Printf("%v: %q %v %q %v %x %v %q %v\n"s, u, j, err, x, err2, b, err3, a, err4);
  }
}

void marshaling() {
  fmt::Println("-- encoding"s);
  auto at = New<time::Time>(time::Date(2024, time::February, 29, 23, 59, 59, 123456789, load("Asia/Kolkata"s)));
  auto [b, err] = json::Marshal(event{.At = *at, .D = time::Duration{5400000000000}, .P = at, .Month = time::May});
  fmt::Println(eo::string_from_bytes(b), err);
  auto e = New<event>();
  err = json::Unmarshal(b, e);
  fmt::Println(e->At, e->D, *e->P, e->Month, err, time::Time::Equal(e->At, *at));
  err = json::Unmarshal(eo::bytes_from_string(R"({"At":"2024-13-01T00:00:00Z"})"s), e);
  fmt::Println(err);
  auto _eo_call_0 = time::Unix(0, 1);
  auto _eo_call_1 = time::Time::UTC(_eo_call_0);
  std::tie(b, err) = json::Marshal(map<std::string, time::Time>::of({{"z"s, time::Time{}}, {"a"s, _eo_call_1}}));
  fmt::Println(eo::string_from_bytes(b), err);
  err = std::get<1>(json::Marshal(time::Date(10000, time::January, 1, 0, 0, 0, 0, time::UTC)));
  fmt::Println(err);
  fmt::Printf("%v|%+v|%#v|%s|%d|%v|%#v|%v|%v\n"s,
    *at,
    *at,
    *at,
    time::Time::Location(*at),
    time::March,
    time::Duration{1500},
    time::Duration{7},
    time::Sunday,
    time::Month{5});
  time::Time zero{};
  fmt::Printf("%v|%#v|%v\n"s, zero, zero, time::Time::Location(zero));
}

auto tzif(std::string footer) -> slice<uint8_t> {
  slice<uint8_t> b{};
  for (auto i = int64_t{0}; i < 2; i++) {
    b = eo::append_slice(b, "TZif2"s);
    b = eo::append_slice(b, eo::make_slice<uint8_t>(15));
    for (auto [_eo_range_key, n] : eo::range(slice<uint32_t>::of({0, 0, 0, 0, 1, 4}))) {
      b = eo::append(b, {uint8_t(n >> 24), uint8_t(n >> 16), uint8_t(n >> 8), uint8_t(n)});
    }
    b = eo::append(b, {0, 0, 0, 0, 0, 0});
    b = eo::append_slice(b, "LMT\x{00}"s);
  }
  b = eo::append(b, {10});
  b = eo::append_slice(b, footer);
  b = eo::append(b, {10});
  return b;
}

void rules() {
  fmt::Println("-- rules"s);
  auto footers = slice<std::string>::of({"EST5EDT,M3.2.0,M11.1.0"s,
    "EST5EDT"s,
    "<+0330>-3:30"s,
    "XST-1XDT,J60/2,J300/3"s,
    "AAA3BBB,0/0,365/25"s,
    "CET-1CEST,M3.5.0,M10.5.0/3"s,
    "<-03>3<-02>,M3.5.0/-2,M10.5.0/-1"s,
    "IST-2IDT,M3.4.4/26,M10.5.0"s,
    "NZST-12NZDT,M9.5.0,M4.1.0/3"s,
    "AEST-10AEDT-11,M10.1.0,M4.1.0/3"s,
    "XXX-14"s,
    "ABC+167:59:59DEF,J1/0,J365/24"s,
    "AB1"s,
    "ABC"s,
    "ABC1DEF,M13.1.0,M1.1.0"s,
    "ABC1DEF,M3.6.0,M10.1.0"s,
    "ABC1DEF;J60,J61"s,
    "ABC1DEF,J0,J10"s,
    "<ABC1"s,
    "ABC25:00"s,
    "ABC1DEF,J59,J60/-167:59:59"s});
  auto years = slice<int64_t>::of({1900, 2000, 2023, 2024, 2100, -4});
  for (auto [_eo_range_key, f] : eo::range(footers)) {
    auto [loc, err] = time::LoadLocationFromTZData("rule"s, tzif(f));
    if (err != nullptr) {
      fmt::Printf("%q: %v\n"s, f, err);
      continue;
    }
    for (auto [_eo_range_key, y] : eo::range(years)) {
      for (auto m = int64_t{1}; m <= 12; m += 3) {
        auto _eo_call_0 = pick(28);
        auto _eo_call_1 = pick(24);
        auto _eo_call_2 = pick(60);
        auto _eo_call_3 = time::Date(y, time::Month(m), 1 + _eo_call_0, _eo_call_1, _eo_call_2, 0, 0, time::UTC);
        auto t = time::Time::In(_eo_call_3, loc);
        auto [start, end] = time::Time::ZoneBounds(t);
        fmt::Printf("%q %v %v %v %v\n"s, f, t, time::Time::IsDST(t), start, end);
      }
      auto _eo_call_4 = fmt::Sprint(f, y);
      auto _eo_call_5 = time::Date(y, time::March, 1, 2, 30, 0, 0, loc);
      show(_eo_call_4, _eo_call_5, nullptr);
    }
  }
  for (auto [_eo_range_key, v] : eo::range(slice<std::string>::of(
         {"1900-02-29"s, "2000-02-29"s, "2100-02-29"s, "2400-02-29"s, "1600-02-29"s, "2004-02-29"s}))) {
    auto [t, err] = time::Parse(time::DateOnly, v);
    show(v, t, err);
  }
  for (auto [_eo_range_key, v] : eo::range(slice<std::string>::of(
         {"1900-060"s, "2000-060"s, "2000-366"s, "1900-366"s, "2100-365"s, "2100-059"s, "2400-060"s}))) {
    auto [t, err] = time::Parse("2006-002"s, v);
    show(v, t, err);
  }
  for (auto [_eo_range_key, y] : eo::range(slice<int64_t>::of({1900, 2000, 2100, 2400, 1700, 1600}))) {
    auto t = time::Date(y, time::February, 29, 0, 0, 0, 0, time::UTC);
    auto _eo_call_6 = time::Time::YearDay(t);
    auto _eo_call_7 = time::Time::AddDate(t, 0, 0, 1);
    auto _eo_call_8 = time::Date(y, time::December, 31, 0, 0, 0, 0, time::UTC);
    auto _eo_call_9 = time::Time::YearDay(_eo_call_8);
    fmt::Println(t, _eo_call_6, _eo_call_7, _eo_call_9);
  }
}

func<> monotonic() {
  fmt::Println("-- monotonic"s);
  auto t = time::Now();
  auto u = time::Time::Add(t, time::Hour);
  auto _eo_call_0 = time::Time::Sub(u, t);
  auto _eo_call_1 = time::Time::Sub(t, u);
  auto _eo_call_2 = time::Time::String(t);
  auto _eo_call_3 = strings::Contains(_eo_call_2, " m=+"s);
  auto _eo_call_4 = time::Time::Round(t, time::Duration{0});
  auto _eo_call_5 = time::Time::String(_eo_call_4);
  auto _eo_call_6 = strings::Contains(_eo_call_5, " m="s);
  auto _eo_call_7 = time::Time::UTC(t);
  auto _eo_call_8 = time::Time::String(_eo_call_7);
  auto _eo_call_9 = strings::Contains(_eo_call_8, " m="s);
  auto _eo_call_10 = time::Time::Add(t, time::Second);
  auto _eo_call_11 = time::Time::String(_eo_call_10);
  auto _eo_call_12 = strings::Contains(_eo_call_11, " m=+"s);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_3, _eo_call_6, _eo_call_9, _eo_call_12);
  auto _eo_call_13 = time::Since(t);
  auto _eo_call_14 = time::Until(u);
  auto _eo_call_15 = time::Until(u);
  auto _eo_call_16 = time::Time::Round(t, time::Duration{0});
  auto _eo_call_17 = time::Time::Equal(t, _eo_call_16);
  auto _eo_call_18 = time::Time::Round(t, time::Duration{0});
  auto _eo_call_19 = time::Time::Location(t);
  auto _eo_call_20 = time::Time::Before(t, u);
  auto _eo_call_21 = time::Time::After(u, t);
  auto _eo_call_22 = time::Time::Compare(t, u);
  fmt::Println(_eo_call_13 >= time::Duration{0},
    _eo_call_14 > time::Duration{0},
    _eo_call_15 <= time::Hour,
    _eo_call_17,
    t == _eo_call_18,
    _eo_call_19,
    _eo_call_20,
    _eo_call_21,
    _eo_call_22);
  auto _eo_call_23 = time::Time::Year(t);
  auto _eo_call_24 = time::Time::Truncate(t, time::Second);
  auto _eo_call_25 = time::Time::Nanosecond(_eo_call_24);
  auto _eo_call_26 = time::Time::Round(t, time::Duration{0});
  auto _eo_call_27 = time::Time::Equal(_eo_call_26, t);
  fmt::Println(_eo_call_23 >= 2024, _eo_call_25, _eo_call_27);
  auto before = time::Now();
  co_await time::Sleep(time::Duration{2000000});
  co_await time::Sleep(time::Duration{0});
  co_await time::Sleep(time::Duration{-1000000000});
  fmt::Println(time::Since(before) >= time::Duration{2000000});
}

func<> eo_main() {
  if (auto err = os::Setenv("TZ"s, "America/New_York"s); err != nullptr) {
    panic(err);
  }
  for (auto [_eo_range_key, name] : eo::range(slice<std::string>::of({"America/New_York"s,
         "Europe/London"s,
         "Asia/Kolkata"s,
         "Australia/Lord_Howe"s,
         "Pacific/Chatham"s,
         "America/Sao_Paulo"s,
         "Africa/Casablanca"s,
         "Antarctica/Troll"s,
         "Europe/Dublin"s,
         "America/St_Johns"s,
         "Asia/Tehran"s,
         "Pacific/Kiritimati"s,
         "Europe/Moscow"s,
         "America/Nuuk"s}))) {
    auto _eo_call_0 = load(name);
    zones = eo::append(zones, {_eo_call_0});
  }
  auto _eo_call_1 = time::FixedZone(""s, 0);
  auto _eo_call_2 = time::FixedZone("ABC"s, 19800);
  auto _eo_call_3 = time::FixedZone(""s, -12345);
  auto _eo_call_4 = time::FixedZone(""s, 50400);
  auto _eo_call_5 = time::FixedZone("Z"s, -1);
  zones = eo::append(zones, {time::UTC, time::Local, _eo_call_1, _eo_call_2, _eo_call_3, _eo_call_4, _eo_call_5});
  parsing();
  formatting();
  instants();
  durations();
  locations();
  rules();
  marshaling();
  co_await monotonic();
}
