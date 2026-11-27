// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// Go's time package, ported from Go 1.26's time.go, format.go, format_rfc3339.go, zoneinfo.go,
// zoneinfo_read.go, zoneinfo_unix.go, and sys_unix.go. Go's int is int64_t, and each function keeps
// Go's name and order of operations, so that each result, overflow, and error is Go's.
#include <eo/syscall.h>
#include <eo/time.h>
#include <eo/utf8.h>

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string_view>

namespace eo::time {

namespace {

  using sv = std::string_view;

  constexpr uint64_t hasMonotonic = uint64_t{1} << 63;
  constexpr uint64_t nsecMask = (uint64_t{1} << 30) - 1;
  constexpr unsigned nsecShift = 30;

  constexpr int64_t secondsPerMinute = 60;
  constexpr int64_t secondsPerHour = 60 * secondsPerMinute;
  constexpr int64_t secondsPerDay = 24 * secondsPerHour;
  constexpr int64_t marchThruDecember = 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31;
  constexpr int64_t absoluteYears = 292277022400;
  // Go's -(absoluteYears*365.2425 + marchThruDecember) * secondsPerDay, an exact constant.
  static_assert(absoluteYears * 3652425 % 10000 == 0);
  constexpr int64_t absoluteToInternal = -(absoluteYears * 3652425 / 10000 + marchThruDecember) * secondsPerDay;
  constexpr int64_t internalToAbsolute = -absoluteToInternal;
  constexpr int64_t unixToInternal = (1969 * 365 + 1969 / 4 - 1969 / 100 + 1969 / 400) * secondsPerDay;
  constexpr int64_t internalToUnix = -unixToInternal;
  constexpr int64_t absoluteToUnix = absoluteToInternal + internalToUnix;
  constexpr int64_t wallToInternal = (1884 * 365 + 1884 / 4 - 1884 / 100 + 1884 / 400) * secondsPerDay;
  constexpr int64_t maxWall = wallToInternal + ((int64_t{1} << 33) - 1);
  constexpr int64_t minWall = wallToInternal;

  constexpr int64_t alpha = INT64_MIN;
  constexpr int64_t omega = INT64_MAX;

  constexpr Duration minDuration = Duration{INT64_MIN};
  constexpr Duration maxDuration = Duration{INT64_MAX};

  // Go's rune iteration over a string decodes UTF-8, which these loops need only to find ASCII
  // bytes: the byte offsets of the other runes are never used except as a position to stop at.

  // The methods of Go's *Time that the package uses.

  auto nsec(const Time& t) -> int32_t {
    return int32_t(t.wall & nsecMask);
  }

  auto sec(const Time& t) -> int64_t {
    if ((t.wall & hasMonotonic) != 0) {
      return wallToInternal + int64_t(t.wall << 1 >> (nsecShift + 1));
    }
    return t.ext;
  }

  auto unixSec(const Time& t) -> int64_t {
    return sec(t) + internalToUnix;
  }

  void stripMono(Time& t) {
    if ((t.wall & hasMonotonic) != 0) {
      t.ext = sec(t);
      t.wall &= nsecMask;
    }
  }

  void addSec(Time& t, int64_t d) {
    if ((t.wall & hasMonotonic) != 0) {
      auto s = int64_t(t.wall << 1 >> (nsecShift + 1));
      auto dsec = s + d;
      if (0 <= dsec && dsec <= (int64_t{1} << 33) - 1) {
        t.wall = (t.wall & nsecMask) | uint64_t(dsec) << nsecShift | hasMonotonic;
        return;
      }
      stripMono(t);
    }
    auto sum = t.ext + d;
    if ((sum > t.ext) == (d > 0)) {
      t.ext = sum;
    } else if (d > 0) {
      t.ext = INT64_MAX;
    } else {
      t.ext = -INT64_MAX;
    }
  }

  void setLoc(Time& t, ptr<Location> loc) {
    if (loc.get() == &detail::utcLoc) {
      loc = nullptr;
    }
    stripMono(t);
    t.loc = std::move(loc);
  }

  // The Location methods of zoneinfo.go.

  void initLocal();

  std::once_flag localOnce;

  auto get(Location* l) -> Location* {
    if (l == nullptr) {
      return &detail::utcLoc;
    }
    if (l == &detail::localLoc) {
      std::call_once(localOnce, initLocal);
    }
    return l;
  }

  struct lookup_result {
    std::string name;
    int64_t offset;
    int64_t start;
    int64_t end;
    bool isDST;
  };

  auto tzset(sv s, int64_t lastTxSec, int64_t sec) -> std::tuple<lookup_result, bool>;

  auto firstZoneUsed(const Location* l) -> bool {
    for (const auto& tx : l->tx) {
      if (tx.index == 0) {
        return true;
      }
    }
    return false;
  }

  auto lookupFirstZone(const Location* l) -> int64_t {
    if (!firstZoneUsed(l)) {
      return 0;
    }
    if (eo::len(l->tx) > 0 && eo::at(l->zone, eo::at(l->tx, 0).index).isDST) {
      for (auto zi = int64_t(eo::at(l->tx, 0).index) - 1; zi >= 0; zi--) {
        if (!eo::at(l->zone, zi).isDST) {
          return zi;
        }
      }
    }
    for (int64_t zi = 0; zi < eo::len(l->zone); zi++) {
      if (!eo::at(l->zone, zi).isDST) {
        return zi;
      }
    }
    return 0;
  }

  auto lookup(Location* l, int64_t sec) -> lookup_result {
    l = get(l);
    if (eo::len(l->zone) == 0) {
      return {"UTC", 0, alpha, omega, false};
    }
    if (auto* zone = l->cacheZone.get(); zone != nullptr && l->cacheStart <= sec && sec < l->cacheEnd) {
      return {zone->name, zone->offset, l->cacheStart, l->cacheEnd, zone->isDST};
    }
    if (eo::len(l->tx) == 0 || sec < eo::at(l->tx, 0).when) {
      const auto& zone = eo::at(l->zone, lookupFirstZone(l));
      auto end = eo::len(l->tx) > 0 ? eo::at(l->tx, 0).when : omega;
      return {zone.name, zone.offset, alpha, end, zone.isDST};
    }
    const auto& tx = l->tx;
    auto end = omega;
    int64_t lo = 0;
    auto hi = eo::len(tx);
    while (hi - lo > 1) {
      auto m = int64_t(uint64_t(lo + hi) >> 1);
      auto lim = eo::at(tx, m).when;
      if (sec < lim) {
        end = lim;
        hi = m;
      } else {
        lo = m;
      }
    }
    const auto& zone = eo::at(l->zone, eo::at(tx, lo).index);
    auto result = lookup_result{zone.name, zone.offset, eo::at(tx, lo).when, end, zone.isDST};
    if (lo == eo::len(tx) - 1 && !l->extend.empty()) {
      if (auto [e, ok] = tzset(l->extend, result.start, sec); ok) {
        return e;
      }
    }
    return result;
  }

  auto lookupName(Location* l, sv name, int64_t unix) -> std::tuple<int64_t, bool> {
    l = get(l);
    for (const auto& zone : l->zone) {
      if (zone.name == name) {
        auto r = lookup(l, unix - zone.offset);
        if (r.name == zone.name) {
          return {r.offset, true};
        }
      }
    }
    for (const auto& zone : l->zone) {
      if (zone.name == name) {
        return {zone.offset, true};
      }
    }
    return {0, false};
  }

  // The absolute dates of time.go.

  auto isLeap(int64_t year) -> bool {
    int64_t mask = 0xf;
    if (year % 25 != 0) {
      mask = 3;
    }
    return (year & mask) == 0;
  }

  auto daysBefore(Month m) -> int64_t {
    int64_t adj = 0;
    if (m >= March) {
      adj = -2;
    }
    return (214 * int64_t(m) - 211) / 7 + adj;
  }

  auto daysIn(Month m, int64_t year) -> int64_t {
    if (m == February) {
      if (isLeap(year)) {
        return 29;
      }
      return 28;
    }
    return 30 + int64_t((m + (m >> 3)) & Month{1});
  }

  auto dateToAbsDays(int64_t year, Month month, int64_t day) -> uint64_t {
    auto amonth = uint32_t(int64_t(month));
    uint32_t janFeb = 0;
    if (amonth < 3) {
      janFeb = 1;
    }
    amonth += 12 * janFeb;
    auto y = uint64_t(year) - uint64_t(janFeb) + uint64_t(absoluteYears);
    uint32_t ayday = (979 * amonth - 2919) >> 5;
    auto century = y / 100;
    auto cyear = uint32_t(y % 100);
    uint32_t cday = 1461 * cyear / 4;
    auto centurydays = 146097 * century / 4;
    return centurydays + uint64_t(int64_t(uint32_t(cday + ayday)) + day - 1);
  }

  struct split_days {
    uint64_t century;
    int64_t cyear;
    int64_t ayday;
  };

  auto days_of(uint64_t abs) -> uint64_t {
    return abs / uint64_t(secondsPerDay);
  }

  auto split(uint64_t days) -> split_days {
    auto d = 4 * days + 3;
    auto century = d / 146097;
    auto cd = uint32_t(d % 146097) | 3;
    auto product = uint64_t{2939745} * cd;
    auto hi = uint32_t(product >> 32);
    auto lo = uint32_t(product);
    return {century, int64_t(hi), int64_t(lo / 2939745 / 4)};
  }

  auto split_yday(int64_t ayday) -> std::tuple<int64_t, int64_t> {
    uint32_t d = 2141 * uint32_t(ayday) + 197913;
    return {int64_t(d >> 16), 1 + int64_t((d & 0xFFFF) / 2141)};
  }

  auto janFeb(int64_t ayday) -> int64_t {
    return ayday >= marchThruDecember ? 1 : 0;
  }

  auto month_of(int64_t m, int64_t janFeb) -> Month {
    return Month{m - janFeb * 12};
  }

  auto leap(uint64_t century, int64_t cyear) -> int64_t {
    int64_t y4ok = cyear % 4 == 0 ? 1 : 0;
    int64_t y100ok = cyear != 0 ? 1 : 0;
    int64_t y400ok = century % 4 == 0 ? 1 : 0;
    return y4ok & (y100ok | y400ok);
  }

  auto year_of(uint64_t century, int64_t cyear, int64_t janFeb) -> int64_t {
    return int64_t(century * 100 - uint64_t(absoluteYears)) + cyear + janFeb;
  }

  auto yday_of(int64_t ayday, int64_t janFeb, int64_t leap) -> int64_t {
    return ayday + (1 + 31 + 28) + (leap & ~janFeb) - 365 * janFeb;
  }

  auto date(uint64_t days) -> std::tuple<int64_t, Month, int64_t> {
    auto [century, cyear, ayday] = split(days);
    auto [amonth, day] = split_yday(ayday);
    auto jf = janFeb(ayday);
    return {year_of(century, cyear, jf), month_of(amonth, jf), day};
  }

  auto yearYday(uint64_t days) -> std::tuple<int64_t, int64_t> {
    auto [century, cyear, ayday] = split(days);
    auto jf = janFeb(ayday);
    return {year_of(century, cyear, jf), yday_of(ayday, jf, leap(century, cyear))};
  }

  auto weekday(uint64_t days) -> Weekday {
    return Weekday{int64_t((days + uint64_t(int64_t(Wednesday))) % 7)};
  }

  auto clock(uint64_t abs) -> std::tuple<int64_t, int64_t, int64_t> {
    auto s = int64_t(abs % uint64_t(secondsPerDay));
    auto hour = s / secondsPerHour;
    s -= hour * secondsPerHour;
    auto min = s / secondsPerMinute;
    s -= min * secondsPerMinute;
    return {hour, min, s};
  }

  auto absSec(const Time& t) -> uint64_t {
    auto* l = t.loc.get();
    if (l == nullptr || l == &detail::localLoc) {
      l = get(l);
    }
    auto s = unixSec(t);
    if (l != &detail::utcLoc) {
      if (auto* zone = l->cacheZone.get(); zone != nullptr && l->cacheStart <= s && s < l->cacheEnd) {
        s += zone->offset;
      } else {
        s += lookup(l, s).offset;
      }
    }
    return uint64_t(s + (unixToInternal + internalToAbsolute));
  }

  auto locabs(const Time& t) -> std::tuple<std::string, int64_t, uint64_t> {
    auto* l = t.loc.get();
    if (l == nullptr || l == &detail::localLoc) {
      l = get(l);
    }
    auto s = unixSec(t);
    auto name = std::string{};
    int64_t offset = 0;
    if (l != &detail::utcLoc) {
      if (auto* zone = l->cacheZone.get(); zone != nullptr && l->cacheStart <= s && s < l->cacheEnd) {
        name = zone->name;
        offset = zone->offset;
      } else {
        auto r = lookup(l, s);
        name = std::move(r.name);
        offset = r.offset;
      }
      s += offset;
    } else {
      name = "UTC";
    }
    return {std::move(name), offset, uint64_t(s + (unixToInternal + internalToAbsolute))};
  }

  auto unixTime(int64_t sec, int32_t nsec) -> Time {
    return Time{uint64_t(int64_t(nsec)), sec + unixToInternal, Local};
  }

  auto lessThanHalf(Duration x, Duration y) -> bool {
    return uint64_t(int64_t(x)) + uint64_t(int64_t(x)) < uint64_t(int64_t(y));
  }

  auto subMono(int64_t t, int64_t u) -> Duration {
    auto d = int64_t(uint64_t(t) - uint64_t(u));
    if (d < 0 && t > u) {
      return maxDuration;
    }
    if (d > 0 && t < u) {
      return minDuration;
    }
    return Duration{d};
  }

  auto runtimeNano() -> int64_t {
    struct timespec ts {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return int64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec;
  }

  auto runtimeNow() -> std::tuple<int64_t, int32_t, int64_t> {
    struct timespec ts {};
    clock_gettime(CLOCK_REALTIME, &ts);
    return {int64_t(ts.tv_sec), int32_t(ts.tv_nsec), runtimeNano()};
  }

  // Go's startNano, so that a monotonic reading is never zero. It is set on first use rather than
  // in this file's static initialization, which another translation unit's may precede.
  auto startNano() -> int64_t {
    static const int64_t start = runtimeNano() - 1;
    return start;
  }

  auto norm(int64_t hi, int64_t lo, int64_t base) -> std::tuple<int64_t, int64_t> {
    if (lo < 0) {
      auto n = (-lo - 1) / base + 1;
      hi -= n;
      lo += n * base;
    }
    if (lo >= base) {
      auto n = lo / base;
      hi += n;
      lo -= n * base;
    }
    return {hi, lo};
  }

  // Go's div: the quotient's parity and the remainder of t divided by d.
  auto div(const Time& t, Duration dd) -> std::tuple<int64_t, Duration> {
    auto d = int64_t(dd);
    constexpr int64_t second = 1000000000;
    auto neg = false;
    auto ns = nsec(t);
    auto s = sec(t);
    if (s < 0) {
      neg = true;
      s = -s;
      ns = -ns;
      if (ns < 0) {
        ns += 1000000000;
        s--;
      }
    }
    int64_t qmod2 = 0;
    int64_t r = 0;
    if (d < second && second % (d + d) == 0) {
      qmod2 = int64_t(ns / int32_t(d)) & 1;
      r = ns % int32_t(d);
    } else if (d % second == 0) {
      auto d1 = d / second;
      qmod2 = (s / d1) & 1;
      r = (s % d1) * second + ns;
    } else {
      auto us = uint64_t(s);
      auto tmp = (us >> 32) * 1000000000;
      auto u1 = tmp >> 32;
      auto u0 = tmp << 32;
      tmp = (us & 0xFFFFFFFF) * 1000000000;
      auto u0x = u0;
      u0 = u0 + tmp;
      if (u0 < u0x) {
        u1++;
      }
      u0x = u0;
      u0 = u0 + uint64_t(ns);
      if (u0 < u0x) {
        u1++;
      }
      auto d1 = uint64_t(d);
      while (d1 >> 63 != 1) {
        d1 <<= 1;
      }
      uint64_t d0 = 0;
      for (;;) {
        qmod2 = 0;
        if (u1 > d1 || (u1 == d1 && u0 >= d0)) {
          qmod2 = 1;
          u0x = u0;
          u0 = u0 - d0;
          if (u0 > u0x) {
            u1--;
          }
          u1 -= d1;
        }
        if (d1 == 0 && d0 == uint64_t(d)) {
          break;
        }
        d0 >>= 1;
        d0 |= (d1 & 1) << 63;
        d1 >>= 1;
      }
      r = int64_t(u0);
    }
    if (neg && r != 0) {
      qmod2 ^= 1;
      r = d - r;
    }
    return {qmod2, Duration{r}};
  }

  // The formatting of format.go.

  const std::array<sv, 7> longDayNames = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const std::array<sv, 7> shortDayNames = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const std::array<sv, 12> shortMonthNames = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const std::array<sv, 12> longMonthNames = {"January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"};

  // fmtInt formats v into the tail of buf[:w] and returns the index where the output begins.
  auto fmtInt(char* buf, int64_t w, uint64_t v) -> int64_t {
    if (v == 0) {
      w--;
      buf[w] = '0';
    } else {
      while (v > 0) {
        w--;
        buf[w] = char(v % 10 + '0');
        v /= 10;
      }
    }
    return w;
  }

  // fmtFrac formats the fraction of v/10**prec into the tail of buf[:w], omitting trailing zeros,
  // and returns the index where the output begins and v/10**prec.
  auto fmtFrac(char* buf, int64_t w, uint64_t v, int64_t prec) -> std::tuple<int64_t, uint64_t> {
    auto print = false;
    for (int64_t i = 0; i < prec; i++) {
      auto digit = v % 10;
      print = print || digit != 0;
      if (print) {
        w--;
        buf[w] = char(digit + '0');
      }
      v /= 10;
    }
    if (print) {
      w--;
      buf[w] = '.';
    }
    return {w, v};
  }

  enum : int64_t {
    stdNeedDate = 1 << 8,
    stdNeedYday = 1 << 9,
    stdNeedClock = 1 << 10,
    stdArgShift = 16,
    stdSeparatorShift = 28,
    stdMask = (1 << stdArgShift) - 1,
  };

  enum : int64_t {
    stdLongMonth = 1 + stdNeedDate,
    stdMonth,
    stdNumMonth,
    stdZeroMonth,
    stdLongWeekDay,
    stdWeekDay,
    stdDay,
    stdUnderDay,
    stdZeroDay,
    stdUnderYearDay = 10 + stdNeedYday,
    stdZeroYearDay,
    stdHour = 12 + stdNeedClock,
    stdHour12,
    stdZeroHour12,
    stdMinute,
    stdZeroMinute,
    stdSecond,
    stdZeroSecond,
    stdLongYear = 19 + stdNeedDate,
    stdYear,
    stdPM = 21 + stdNeedClock,
    stdpm,
    stdTZ = 23,
    stdISO8601TZ,
    stdISO8601SecondsTZ,
    stdISO8601ShortTZ,
    stdISO8601ColonTZ,
    stdISO8601ColonSecondsTZ,
    stdNumTZ,
    stdNumSecondsTz,
    stdNumShortTZ,
    stdNumColonTZ,
    stdNumColonSecondsTZ,
    stdFracSecond0,
    stdFracSecond9,
  };

  constexpr std::array<int64_t, 6> std0x = {
    stdZeroMonth, stdZeroDay, stdZeroHour12, stdZeroMinute, stdZeroSecond, stdYear};

  auto startsWithLowerCase(sv str) -> bool {
    if (str.empty()) {
      return false;
    }
    auto c = str[0];
    return 'a' <= c && c <= 'z';
  }

  auto isDigit(sv s, int64_t i) -> bool {
    if (int64_t(s.size()) <= i) {
      return false;
    }
    auto c = s[size_t(i)];
    return '0' <= c && c <= '9';
  }

  auto stdFracSecond(int64_t code, int64_t n, int64_t c) -> int64_t {
    if (c == '.') {
      return code | ((n & 0xfff) << stdArgShift);
    }
    return code | ((n & 0xfff) << stdArgShift) | int64_t{1} << stdSeparatorShift;
  }

  auto digitsLen(int64_t std) -> int64_t {
    return (std >> stdArgShift) & 0xfff;
  }

  auto separator(int64_t std) -> char {
    if ((std >> stdSeparatorShift) == 0) {
      return '.';
    }
    return ',';
  }

  struct chunk {
    sv prefix;
    int64_t std;
    sv suffix;
  };

  auto nextStdChunk(sv layout) -> chunk {
    auto n = int64_t(layout.size());
    auto at = [&](int64_t i, int64_t len) { return layout.substr(size_t(i), size_t(len)); };
    for (int64_t i = 0; i < n; i++) {
      auto c = int64_t(uint8_t(layout[size_t(i)]));
      auto before = layout.substr(0, size_t(i));
      switch (c) {
      case 'J':
        if (n >= i + 3 && at(i, 3) == "Jan") {
          if (n >= i + 7 && at(i, 7) == "January") {
            return {before, stdLongMonth, layout.substr(size_t(i + 7))};
          }
          if (!startsWithLowerCase(layout.substr(size_t(i + 3)))) {
            return {before, stdMonth, layout.substr(size_t(i + 3))};
          }
        }
        break;
      case 'M':
        if (n >= i + 3) {
          if (at(i, 3) == "Mon") {
            if (n >= i + 6 && at(i, 6) == "Monday") {
              return {before, stdLongWeekDay, layout.substr(size_t(i + 6))};
            }
            if (!startsWithLowerCase(layout.substr(size_t(i + 3)))) {
              return {before, stdWeekDay, layout.substr(size_t(i + 3))};
            }
          }
          if (at(i, 3) == "MST") {
            return {before, stdTZ, layout.substr(size_t(i + 3))};
          }
        }
        break;
      case '0':
        if (n >= i + 2 && '1' <= layout[size_t(i + 1)] && layout[size_t(i + 1)] <= '6') {
          return {before, std0x[size_t(layout[size_t(i + 1)] - '1')], layout.substr(size_t(i + 2))};
        }
        if (n >= i + 3 && layout[size_t(i + 1)] == '0' && layout[size_t(i + 2)] == '2') {
          return {before, stdZeroYearDay, layout.substr(size_t(i + 3))};
        }
        break;
      case '1':
        if (n >= i + 2 && layout[size_t(i + 1)] == '5') {
          return {before, stdHour, layout.substr(size_t(i + 2))};
        }
        return {before, stdNumMonth, layout.substr(size_t(i + 1))};
      case '2':
        if (n >= i + 4 && at(i, 4) == "2006") {
          return {before, stdLongYear, layout.substr(size_t(i + 4))};
        }
        return {before, stdDay, layout.substr(size_t(i + 1))};
      case '_':
        if (n >= i + 2 && layout[size_t(i + 1)] == '2') {
          if (n >= i + 5 && at(i + 1, 4) == "2006") {
            return {layout.substr(0, size_t(i + 1)), stdLongYear, layout.substr(size_t(i + 5))};
          }
          return {before, stdUnderDay, layout.substr(size_t(i + 2))};
        }
        if (n >= i + 3 && layout[size_t(i + 1)] == '_' && layout[size_t(i + 2)] == '2') {
          return {before, stdUnderYearDay, layout.substr(size_t(i + 3))};
        }
        break;
      case '3':
        return {before, stdHour12, layout.substr(size_t(i + 1))};
      case '4':
        return {before, stdMinute, layout.substr(size_t(i + 1))};
      case '5':
        return {before, stdSecond, layout.substr(size_t(i + 1))};
      case 'P':
        if (n >= i + 2 && layout[size_t(i + 1)] == 'M') {
          return {before, stdPM, layout.substr(size_t(i + 2))};
        }
        break;
      case 'p':
        if (n >= i + 2 && layout[size_t(i + 1)] == 'm') {
          return {before, stdpm, layout.substr(size_t(i + 2))};
        }
        break;
      case '-':
        if (n >= i + 7 && at(i, 7) == "-070000") {
          return {before, stdNumSecondsTz, layout.substr(size_t(i + 7))};
        }
        if (n >= i + 9 && at(i, 9) == "-07:00:00") {
          return {before, stdNumColonSecondsTZ, layout.substr(size_t(i + 9))};
        }
        if (n >= i + 5 && at(i, 5) == "-0700") {
          return {before, stdNumTZ, layout.substr(size_t(i + 5))};
        }
        if (n >= i + 6 && at(i, 6) == "-07:00") {
          return {before, stdNumColonTZ, layout.substr(size_t(i + 6))};
        }
        if (n >= i + 3 && at(i, 3) == "-07") {
          return {before, stdNumShortTZ, layout.substr(size_t(i + 3))};
        }
        break;
      case 'Z':
        if (n >= i + 7 && at(i, 7) == "Z070000") {
          return {before, stdISO8601SecondsTZ, layout.substr(size_t(i + 7))};
        }
        if (n >= i + 9 && at(i, 9) == "Z07:00:00") {
          return {before, stdISO8601ColonSecondsTZ, layout.substr(size_t(i + 9))};
        }
        if (n >= i + 5 && at(i, 5) == "Z0700") {
          return {before, stdISO8601TZ, layout.substr(size_t(i + 5))};
        }
        if (n >= i + 6 && at(i, 6) == "Z07:00") {
          return {before, stdISO8601ColonTZ, layout.substr(size_t(i + 6))};
        }
        if (n >= i + 3 && at(i, 3) == "Z07") {
          return {before, stdISO8601ShortTZ, layout.substr(size_t(i + 3))};
        }
        break;
      case '.':
      case ',':
        if (i + 1 < n && (layout[size_t(i + 1)] == '0' || layout[size_t(i + 1)] == '9')) {
          auto ch = layout[size_t(i + 1)];
          auto j = i + 1;
          while (j < n && layout[size_t(j)] == ch) {
            j++;
          }
          if (!isDigit(layout, j)) {
            auto code = stdFracSecond0;
            if (layout[size_t(i + 1)] == '9') {
              code = stdFracSecond9;
            }
            auto std = stdFracSecond(code, j - (i + 1), c);
            return {before, std, layout.substr(size_t(j))};
          }
        }
        break;
      default:
        break;
      }
    }
    return {layout, 0, sv{}};
  }

  auto match(sv s1, sv s2) -> bool {
    for (size_t i = 0; i < s1.size(); i++) {
      auto c1 = uint8_t(s1[i]);
      auto c2 = uint8_t(s2[i]);
      if (c1 != c2) {
        c1 |= 'a' - 'A';
        c2 |= 'a' - 'A';
        if (c1 != c2 || c1 < 'a' || c1 > 'z') {
          return false;
        }
      }
    }
    return true;
  }

  // errBad, the placeholder error of the parsing functions, which is never returned.
  struct bad {};

  template<size_t N>
  auto lookup(const std::array<sv, N>& tab, sv val) -> std::tuple<int64_t, sv, bool> {
    for (size_t i = 0; i < N; i++) {
      auto v = tab[i];
      if (val.size() >= v.size() && match(val.substr(0, v.size()), v)) {
        return {int64_t(i), val.substr(v.size()), true};
      }
    }
    return {-1, val, false};
  }

  // appendInt appends the decimal form of x, padded with zeros to width.
  void appendInt(std::string& b, int64_t x, int64_t width) {
    auto u = uint64_t(x);
    if (x < 0) {
      b += '-';
      u = uint64_t(-x);
    }
    auto utod = [](uint64_t u) { return char('0' + u); };
    if (width == 2 && u < 100) {
      b += utod(u / 10);
      b += utod(u % 10);
      return;
    }
    if (width == 4 && u < 10000) {
      b += utod(u / 1000);
      b += utod(u / 100 % 10);
      b += utod(u / 10 % 10);
      b += utod(u % 10);
      return;
    }
    int64_t n = 0;
    if (u == 0) {
      n = 1;
    }
    for (auto u2 = u; u2 > 0; u2 /= 10) {
      n++;
    }
    for (auto pad = width - n; pad > 0; pad--) {
      b += '0';
    }
    auto start = b.size();
    b.resize(start + size_t(n));
    auto i = int64_t(b.size()) - 1;
    while (u >= 10 && i > 0) {
      auto q = u / 10;
      b[size_t(i)] = utod(u - q * 10);
      u = q;
      i--;
    }
    b[size_t(i)] = utod(u);
  }

  template<typename S>
  auto leadingInt(S s) -> std::tuple<uint64_t, S, bool> {
    size_t i = 0;
    uint64_t x = 0;
    for (; i < s.size(); i++) {
      auto c = s[i];
      if (c < '0' || c > '9') {
        break;
      }
      if (x > (uint64_t{1} << 63) / 10) {
        return {0, S{}, false};
      }
      x = x * 10 + uint64_t(c) - '0';
      if (x > uint64_t{1} << 63) {
        return {0, S{}, false};
      }
    }
    return {x, s.substr(i), true};
  }

  // atoi returns false where Go returns errAtoi.
  auto atoi(sv s) -> std::tuple<int64_t, bool> {
    auto neg = false;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
      neg = s[0] == '-';
      s = s.substr(1);
    }
    auto [q, rem, ok] = leadingInt(s);
    auto x = int64_t(q);
    if (!ok || !rem.empty()) {
      return {0, false};
    }
    if (neg) {
      x = -x;
    }
    return {x, true};
  }

  void appendNano(std::string& b, int64_t nanosec, int64_t std) {
    auto trim = (std & stdMask) == stdFracSecond9;
    auto n = digitsLen(std);
    if (trim && (n == 0 || nanosec == 0)) {
      return;
    }
    auto dot = separator(std);
    b += dot;
    appendInt(b, nanosec, 9);
    if (n < 9) {
      b.resize(b.size() - 9 + size_t(n));
    }
    if (trim) {
      while (!b.empty() && b.back() == '0') {
        b.pop_back();
      }
      if (!b.empty() && b.back() == dot) {
        b.pop_back();
      }
    }
  }

  void appendFormatRFC3339(const Time& t, std::string& b, bool nanos) {
    auto [name, offset, abs] = locabs(t);
    auto [year, month, day] = date(days_of(abs));
    appendInt(b, year, 4);
    b += '-';
    appendInt(b, int64_t(month), 2);
    b += '-';
    appendInt(b, day, 2);
    b += 'T';
    auto [hour, min, sec] = clock(abs);
    appendInt(b, hour, 2);
    b += ':';
    appendInt(b, min, 2);
    b += ':';
    appendInt(b, sec, 2);
    if (nanos) {
      auto std = stdFracSecond(stdFracSecond9, 9, '.');
      appendNano(b, Time::Nanosecond(t), std);
    }
    if (offset == 0) {
      b += 'Z';
      return;
    }
    auto zone = offset / 60;
    if (zone < 0) {
      b += '-';
      zone = -zone;
    } else {
      b += '+';
    }
    appendInt(b, zone / 60, 2);
    b += ':';
    appendInt(b, zone % 60, 2);
  }

  // appendStrictRFC3339 returns the error message, or an empty one.
  auto appendStrictRFC3339(const Time& t, std::string& b) -> std::string {
    auto n0 = b.size();
    appendFormatRFC3339(t, b, true);
    auto num2 = [](sv b) { return uint8_t(10 * (b[0] - '0') + (b[1] - '0')); };
    if (b[n0 + 4] != '-') {
      return "year outside of range [0,9999]";
    }
    if (b.back() != 'Z') {
      auto c = b[b.size() - 6];
      if (('0' <= c && c <= '9') || num2(sv(b).substr(b.size() - 5)) >= 24) {
        return "timezone hour outside of range [0,23]";
      }
    }
    return {};
  }

  void appendFormat(const Time& t, std::string& b, sv layout) {
    auto [name, offset, abs] = locabs(t);
    auto days = days_of(abs);
    int64_t year = -1;
    auto month = Month{};
    int64_t day = 0;
    int64_t yday = -1;
    int64_t hour = -1;
    int64_t min = 0;
    int64_t sec = 0;
    while (!layout.empty()) {
      auto [prefix, std, suffix] = nextStdChunk(layout);
      if (!prefix.empty()) {
        b += prefix;
      }
      if (std == 0) {
        break;
      }
      layout = suffix;
      if (year < 0 && (std & stdNeedDate) != 0) {
        std::tie(year, month, day) = date(days);
      }
      if (yday < 0 && (std & stdNeedYday) != 0) {
        yday = std::get<1>(yearYday(days));
      }
      if (hour < 0 && (std & stdNeedClock) != 0) {
        std::tie(hour, min, sec) = clock(abs);
      }
      switch (std & stdMask) {
      case stdYear: {
        auto y = year;
        if (y < 0) {
          y = -y;
        }
        appendInt(b, y % 100, 2);
        break;
      }
      case stdLongYear:
        appendInt(b, year, 4);
        break;
      case stdMonth:
        b += Month::String(month).substr(0, 3);
        break;
      case stdLongMonth:
        b += Month::String(month);
        break;
      case stdNumMonth:
        appendInt(b, int64_t(month), 0);
        break;
      case stdZeroMonth:
        appendInt(b, int64_t(month), 2);
        break;
      case stdWeekDay:
        b += Weekday::String(weekday(days)).substr(0, 3);
        break;
      case stdLongWeekDay:
        b += Weekday::String(weekday(days));
        break;
      case stdDay:
        appendInt(b, day, 0);
        break;
      case stdUnderDay:
        if (day < 10) {
          b += ' ';
        }
        appendInt(b, day, 0);
        break;
      case stdZeroDay:
        appendInt(b, day, 2);
        break;
      case stdUnderYearDay:
        if (yday < 100) {
          b += ' ';
          if (yday < 10) {
            b += ' ';
          }
        }
        appendInt(b, yday, 0);
        break;
      case stdZeroYearDay:
        appendInt(b, yday, 3);
        break;
      case stdHour:
        appendInt(b, hour, 2);
        break;
      case stdHour12: {
        auto hr = hour % 12;
        if (hr == 0) {
          hr = 12;
        }
        appendInt(b, hr, 0);
        break;
      }
      case stdZeroHour12: {
        auto hr = hour % 12;
        if (hr == 0) {
          hr = 12;
        }
        appendInt(b, hr, 2);
        break;
      }
      case stdMinute:
        appendInt(b, min, 0);
        break;
      case stdZeroMinute:
        appendInt(b, min, 2);
        break;
      case stdSecond:
        appendInt(b, sec, 0);
        break;
      case stdZeroSecond:
        appendInt(b, sec, 2);
        break;
      case stdPM:
        b += hour >= 12 ? "PM" : "AM";
        break;
      case stdpm:
        b += hour >= 12 ? "pm" : "am";
        break;
      case stdISO8601TZ:
      case stdISO8601ColonTZ:
      case stdISO8601SecondsTZ:
      case stdISO8601ShortTZ:
      case stdISO8601ColonSecondsTZ:
      case stdNumTZ:
      case stdNumColonTZ:
      case stdNumSecondsTz:
      case stdNumShortTZ:
      case stdNumColonSecondsTZ: {
        if (offset == 0 &&
          (std == stdISO8601TZ || std == stdISO8601ColonTZ || std == stdISO8601SecondsTZ || std == stdISO8601ShortTZ ||
            std == stdISO8601ColonSecondsTZ)) {
          b += 'Z';
          break;
        }
        auto zone = offset / 60;
        auto absoffset = offset;
        if (zone < 0) {
          b += '-';
          zone = -zone;
          absoffset = -absoffset;
        } else {
          b += '+';
        }
        appendInt(b, zone / 60, 2);
        if (std == stdISO8601ColonTZ || std == stdNumColonTZ || std == stdISO8601ColonSecondsTZ ||
          std == stdNumColonSecondsTZ) {
          b += ':';
        }
        if (std != stdNumShortTZ && std != stdISO8601ShortTZ) {
          appendInt(b, zone % 60, 2);
        }
        if (std == stdISO8601SecondsTZ || std == stdNumSecondsTz || std == stdNumColonSecondsTZ ||
          std == stdISO8601ColonSecondsTZ) {
          if (std == stdNumColonSecondsTZ || std == stdISO8601ColonSecondsTZ) {
            b += ':';
          }
          appendInt(b, absoffset % 60, 2);
        }
        break;
      }
      case stdTZ: {
        if (!name.empty()) {
          b += name;
          break;
        }
        auto zone = offset / 60;
        if (zone < 0) {
          b += '-';
          zone = -zone;
        } else {
          b += '+';
        }
        appendInt(b, zone / 60, 2);
        appendInt(b, zone % 60, 2);
        break;
      }
      case stdFracSecond0:
      case stdFracSecond9:
        appendNano(b, Time::Nanosecond(t), std);
        break;
      default:
        break;
      }
    }
  }

  void appendFormatAny(const Time& t, std::string& b, sv layout) {
    if (layout == RFC3339) {
      appendFormatRFC3339(t, b, false);
    } else if (layout == RFC3339Nano) {
      appendFormatRFC3339(t, b, true);
    } else {
      appendFormat(t, b, layout);
    }
  }

  // The parsing of format.go.

  constexpr sv lowerhex = "0123456789abcdef";

  // Go's quote, which escapes the bytes of each rune that is not printable ASCII as \x.
  auto quote(sv s) -> std::string {
    auto buf = std::string{"\""};
    for (size_t i = 0; i < s.size();) {
      auto [c, size] = eo::detail::decode_rune(s, i);
      if (c >= 0x80 || c < ' ') {
        int64_t width = 0;
        if (c == 0xFFFD) {
          width = 1;
          if (i + 2 < s.size() && s.substr(i, 3) == "\xef\xbf\xbd") {
            width = 3;
          }
        } else {
          width = int64_t(size);
        }
        for (int64_t j = 0; j < width; j++) {
          auto byte = uint8_t(s[i + size_t(j)]);
          buf += "\\x";
          buf += lowerhex[byte >> 4];
          buf += lowerhex[byte & 0xF];
        }
      } else {
        if (c == '"' || c == '\\') {
          buf += '\\';
        }
        buf += char(c);
      }
      i += size;
    }
    buf += '"';
    return buf;
  }

  auto newParseError(sv layout, sv value, sv layoutElem, sv valueElem, std::string message) -> error {
    return New<ParseError>(ParseError{std::string(layout),
      std::string(value),
      std::string(layoutElem),
      std::string(valueElem),
      std::move(message)});
  }

  // getnum and getnum3 return false where Go returns errBad.
  auto getnum(sv s, bool fixed) -> std::tuple<int64_t, sv, bool> {
    if (!isDigit(s, 0)) {
      return {0, s, false};
    }
    if (!isDigit(s, 1)) {
      if (fixed) {
        return {0, s, false};
      }
      return {int64_t(s[0] - '0'), s.substr(1), true};
    }
    return {int64_t(s[0] - '0') * 10 + int64_t(s[1] - '0'), s.substr(2), true};
  }

  auto getnum3(sv s, bool fixed) -> std::tuple<int64_t, sv, bool> {
    int64_t n = 0;
    int64_t i = 0;
    for (i = 0; i < 3 && isDigit(s, i); i++) {
      n = n * 10 + int64_t(s[size_t(i)] - '0');
    }
    if (i == 0 || (fixed && i != 3)) {
      return {0, s, false};
    }
    return {n, s.substr(size_t(i)), true};
  }

  auto cutspace(sv s) -> sv {
    while (!s.empty() && s[0] == ' ') {
      s = s.substr(1);
    }
    return s;
  }

  auto skip(sv value, sv prefix) -> std::tuple<sv, bool> {
    while (!prefix.empty()) {
      if (prefix[0] == ' ') {
        if (!value.empty() && value[0] != ' ') {
          return {value, false};
        }
        prefix = cutspace(prefix);
        value = cutspace(value);
        continue;
      }
      if (value.empty() || value[0] != prefix[0]) {
        return {value, false};
      }
      prefix = prefix.substr(1);
      value = value.substr(1);
    }
    return {value, true};
  }

  auto commaOrPeriod(char b) -> bool {
    return b == '.' || b == ',';
  }

  // parseNanoseconds returns false where Go returns errBad or errAtoi.
  auto parseNanoseconds(sv value, int64_t nbytes) -> std::tuple<int64_t, std::string, bool> {
    if (!commaOrPeriod(value[0])) {
      return {0, {}, false};
    }
    if (nbytes > 10) {
      value = value.substr(0, 10);
      nbytes = 10;
    }
    auto [ns, ok] = atoi(value.substr(1, size_t(nbytes - 1)));
    if (!ok) {
      return {ns, {}, false};
    }
    if (ns < 0) {
      return {ns, "fractional second", true};
    }
    auto scaleDigits = 10 - nbytes;
    for (int64_t i = 0; i < scaleDigits; i++) {
      ns *= 10;
    }
    return {ns, {}, true};
  }

  auto parseSignedOffset(sv value) -> int64_t {
    auto sign = value[0];
    if (sign != '-' && sign != '+') {
      return 0;
    }
    auto [x, rem, ok] = leadingInt(value.substr(1));
    if (!ok || value.substr(1) == rem) {
      return 0;
    }
    if (x > 23) {
      return 0;
    }
    return int64_t(value.size() - rem.size());
  }

  auto parseGMT(sv value) -> int64_t {
    value = value.substr(3);
    if (value.empty()) {
      return 3;
    }
    return 3 + parseSignedOffset(value);
  }

  auto parseTimeZone(sv value) -> std::tuple<int64_t, bool> {
    if (value.size() < 3) {
      return {0, false};
    }
    if (value.size() >= 4 && (value.substr(0, 4) == "ChST" || value.substr(0, 4) == "MeST")) {
      return {4, true};
    }
    if (value.substr(0, 3) == "GMT") {
      return {parseGMT(value), true};
    }
    if (value[0] == '+' || value[0] == '-') {
      auto length = parseSignedOffset(value);
      return {length, length > 0};
    }
    int64_t nUpper = 0;
    for (nUpper = 0; nUpper < 6; nUpper++) {
      if (nUpper >= int64_t(value.size())) {
        break;
      }
      if (auto c = value[size_t(nUpper)]; c < 'A' || 'Z' < c) {
        break;
      }
    }
    switch (nUpper) {
    case 0:
    case 1:
    case 2:
    case 6:
      return {0, false};
    case 5:
      if (value[4] == 'T') {
        return {5, true};
      }
      break;
    case 4:
      if (value[3] == 'T' || value.substr(0, 4) == "WITA") {
        return {4, true};
      }
      break;
    case 3:
      return {3, true};
    default:
      break;
    }
    return {0, false};
  }

  auto parse(sv layout, sv value, ptr<Location> defaultLocation, ptr<Location> local) -> std::tuple<Time, error> {
    auto alayout = layout;
    auto avalue = value;
    auto rangeErrString = std::string{};
    auto amSet = false;
    auto pmSet = false;
    int64_t year = 0;
    int64_t month = -1;
    int64_t day = -1;
    int64_t yday = -1;
    int64_t hour = 0;
    int64_t min = 0;
    int64_t sec = 0;
    int64_t nsec = 0;
    auto z = ptr<Location>{};
    int64_t zoneOffset = -1;
    auto zoneName = sv{};

    for (;;) {
      auto ok = true;
      auto [prefix, std, suffix] = nextStdChunk(layout);
      auto stdstr = layout.substr(prefix.size(), layout.size() - prefix.size() - suffix.size());
      std::tie(value, ok) = skip(value, prefix);
      if (!ok) {
        return {Time{}, newParseError(alayout, avalue, prefix, value, "")};
      }
      if (std == 0) {
        if (!value.empty()) {
          return {Time{}, newParseError(alayout, avalue, "", value, ": extra text: " + quote(value))};
        }
        break;
      }
      layout = suffix;
      auto p = sv{};
      auto hold = value;
      switch (std & stdMask) {
      case stdYear:
        if (value.size() < 2) {
          ok = false;
          break;
        }
        p = value.substr(0, 2);
        value = value.substr(2);
        std::tie(year, ok) = atoi(p);
        if (!ok) {
          break;
        }
        if (year >= 69) {
          year += 1900;
        } else {
          year += 2000;
        }
        break;
      case stdLongYear:
        if (value.size() < 4 || !isDigit(value, 0)) {
          ok = false;
          break;
        }
        p = value.substr(0, 4);
        value = value.substr(4);
        std::tie(year, ok) = atoi(p);
        break;
      case stdMonth:
        std::tie(month, value, ok) = lookup(shortMonthNames, value);
        month++;
        break;
      case stdLongMonth:
        std::tie(month, value, ok) = lookup(longMonthNames, value);
        month++;
        break;
      case stdNumMonth:
      case stdZeroMonth:
        std::tie(month, value, ok) = getnum(value, std == stdZeroMonth);
        if (ok && (month <= 0 || 12 < month)) {
          rangeErrString = "month";
        }
        break;
      case stdWeekDay:
        std::tie(std::ignore, value, ok) = lookup(shortDayNames, value);
        break;
      case stdLongWeekDay:
        std::tie(std::ignore, value, ok) = lookup(longDayNames, value);
        break;
      case stdDay:
      case stdUnderDay:
      case stdZeroDay:
        if (std == stdUnderDay && !value.empty() && value[0] == ' ') {
          value = value.substr(1);
        }
        std::tie(day, value, ok) = getnum(value, std == stdZeroDay);
        break;
      case stdUnderYearDay:
      case stdZeroYearDay:
        for (int64_t i = 0; i < 2; i++) {
          if (std == stdUnderYearDay && !value.empty() && value[0] == ' ') {
            value = value.substr(1);
          }
        }
        std::tie(yday, value, ok) = getnum3(value, std == stdZeroYearDay);
        break;
      case stdHour:
        std::tie(hour, value, ok) = getnum(value, false);
        if (hour < 0 || 24 <= hour) {
          rangeErrString = "hour";
        }
        break;
      case stdHour12:
      case stdZeroHour12:
        std::tie(hour, value, ok) = getnum(value, std == stdZeroHour12);
        if (hour < 0 || 12 < hour) {
          rangeErrString = "hour";
        }
        break;
      case stdMinute:
      case stdZeroMinute:
        std::tie(min, value, ok) = getnum(value, std == stdZeroMinute);
        if (min < 0 || 60 <= min) {
          rangeErrString = "minute";
        }
        break;
      case stdSecond:
      case stdZeroSecond: {
        std::tie(sec, value, ok) = getnum(value, std == stdZeroSecond);
        if (!ok) {
          break;
        }
        if (sec < 0 || 60 <= sec) {
          rangeErrString = "second";
          break;
        }
        if (value.size() >= 2 && commaOrPeriod(value[0]) && isDigit(value, 1)) {
          auto next = nextStdChunk(layout).std & stdMask;
          if (next == stdFracSecond0 || next == stdFracSecond9) {
            break;
          }
          size_t n = 2;
          for (; n < value.size() && isDigit(value, int64_t(n)); n++) {
          }
          std::tie(nsec, rangeErrString, ok) = parseNanoseconds(value, int64_t(n));
          value = value.substr(n);
        }
        break;
      }
      case stdPM:
        if (value.size() < 2) {
          ok = false;
          break;
        }
        p = value.substr(0, 2);
        value = value.substr(2);
        if (p == "PM") {
          pmSet = true;
        } else if (p == "AM") {
          amSet = true;
        } else {
          ok = false;
        }
        break;
      case stdpm:
        if (value.size() < 2) {
          ok = false;
          break;
        }
        p = value.substr(0, 2);
        value = value.substr(2);
        if (p == "pm") {
          pmSet = true;
        } else if (p == "am") {
          amSet = true;
        } else {
          ok = false;
        }
        break;
      case stdISO8601TZ:
      case stdISO8601ShortTZ:
      case stdISO8601ColonTZ:
      case stdISO8601SecondsTZ:
      case stdISO8601ColonSecondsTZ:
        if (!value.empty() && value[0] == 'Z') {
          value = value.substr(1);
          z = UTC;
          break;
        }
        [[fallthrough]];
      case stdNumTZ:
      case stdNumShortTZ:
      case stdNumColonTZ:
      case stdNumSecondsTz:
      case stdNumColonSecondsTZ: {
        auto sign = sv{};
        auto hh = sv{};
        auto mm = sv{};
        auto seconds = sv{};
        if (std == stdISO8601ColonTZ || std == stdNumColonTZ) {
          if (value.size() < 6) {
            ok = false;
            break;
          }
          if (value[3] != ':') {
            ok = false;
            break;
          }
          sign = value.substr(0, 1);
          hh = value.substr(1, 2);
          mm = value.substr(4, 2);
          seconds = "00";
          value = value.substr(6);
        } else if (std == stdNumShortTZ || std == stdISO8601ShortTZ) {
          if (value.size() < 3) {
            ok = false;
            break;
          }
          sign = value.substr(0, 1);
          hh = value.substr(1, 2);
          mm = "00";
          seconds = "00";
          value = value.substr(3);
        } else if (std == stdISO8601ColonSecondsTZ || std == stdNumColonSecondsTZ) {
          if (value.size() < 9) {
            ok = false;
            break;
          }
          if (value[3] != ':' || value[6] != ':') {
            ok = false;
            break;
          }
          sign = value.substr(0, 1);
          hh = value.substr(1, 2);
          mm = value.substr(4, 2);
          seconds = value.substr(7, 2);
          value = value.substr(9);
        } else if (std == stdISO8601SecondsTZ || std == stdNumSecondsTz) {
          if (value.size() < 7) {
            ok = false;
            break;
          }
          sign = value.substr(0, 1);
          hh = value.substr(1, 2);
          mm = value.substr(3, 2);
          seconds = value.substr(5, 2);
          value = value.substr(7);
        } else {
          if (value.size() < 5) {
            ok = false;
            break;
          }
          sign = value.substr(0, 1);
          hh = value.substr(1, 2);
          mm = value.substr(3, 2);
          seconds = "00";
          value = value.substr(5);
        }
        int64_t hr = 0;
        int64_t mn = 0;
        int64_t ss = 0;
        std::tie(hr, std::ignore, ok) = getnum(hh, true);
        if (ok) {
          std::tie(mn, std::ignore, ok) = getnum(mm, true);
          if (ok) {
            std::tie(ss, std::ignore, ok) = getnum(seconds, true);
          }
        }
        if (hr > 24) {
          rangeErrString = "time zone offset hour";
        }
        if (mn > 60) {
          rangeErrString = "time zone offset minute";
        }
        if (ss > 60) {
          rangeErrString = "time zone offset second";
        }
        zoneOffset = (hr * 60 + mn) * 60 + ss;
        switch (sign[0]) {
        case '+':
          break;
        case '-':
          zoneOffset = -zoneOffset;
          break;
        default:
          ok = false;
          break;
        }
        break;
      }
      case stdTZ: {
        if (value.size() >= 3 && value.substr(0, 3) == "UTC") {
          z = UTC;
          value = value.substr(3);
          break;
        }
        auto [n, found] = parseTimeZone(value);
        if (!found) {
          ok = false;
          break;
        }
        zoneName = value.substr(0, size_t(n));
        value = value.substr(size_t(n));
        break;
      }
      case stdFracSecond0: {
        auto ndigit = 1 + digitsLen(std);
        if (int64_t(value.size()) < ndigit) {
          ok = false;
          break;
        }
        std::tie(nsec, rangeErrString, ok) = parseNanoseconds(value, ndigit);
        value = value.substr(size_t(ndigit));
        break;
      }
      case stdFracSecond9: {
        if (value.size() < 2 || !commaOrPeriod(value[0]) || value[1] < '0' || '9' < value[1]) {
          break;
        }
        size_t i = 0;
        while (i + 1 < value.size() && '0' <= value[i + 1] && value[i + 1] <= '9') {
          i++;
        }
        std::tie(nsec, rangeErrString, ok) = parseNanoseconds(value, int64_t(1 + i));
        value = value.substr(1 + i);
        break;
      }
      default:
        break;
      }
      if (!rangeErrString.empty()) {
        return {Time{}, newParseError(alayout, avalue, stdstr, value, ": " + rangeErrString + " out of range")};
      }
      if (!ok) {
        return {Time{}, newParseError(alayout, avalue, stdstr, hold, "")};
      }
    }
    if (pmSet && hour < 12) {
      hour += 12;
    } else if (amSet && hour == 12) {
      hour = 0;
    }

    if (yday >= 0) {
      int64_t d = 0;
      int64_t m = 0;
      if (isLeap(year)) {
        if (yday == 31 + 29) {
          m = int64_t(February);
          d = 29;
        } else if (yday > 31 + 29) {
          yday--;
        }
      }
      if (yday < 1 || yday > 365) {
        return {Time{}, newParseError(alayout, avalue, "", value, ": day-of-year out of range")};
      }
      if (m == 0) {
        m = (yday - 1) / 31 + 1;
        if (daysBefore(Month{m + 1}) < yday) {
          m++;
        }
        d = yday - daysBefore(Month{m});
      }
      if (month >= 0 && month != m) {
        return {Time{}, newParseError(alayout, avalue, "", value, ": day-of-year does not match month")};
      }
      month = m;
      if (day >= 0 && day != d) {
        return {Time{}, newParseError(alayout, avalue, "", value, ": day-of-year does not match day")};
      }
      day = d;
    } else {
      if (month < 0) {
        month = int64_t(January);
      }
      if (day < 0) {
        day = 1;
      }
    }

    if (day < 1 || day > daysIn(Month{month}, year)) {
      return {Time{}, newParseError(alayout, avalue, "", value, ": day out of range")};
    }

    if (z != nullptr) {
      return {time::Date(year, Month{month}, day, hour, min, sec, nsec, z), nullptr};
    }

    if (zoneOffset != -1) {
      auto t = time::Date(year, Month{month}, day, hour, min, sec, nsec, UTC);
      addSec(t, -zoneOffset);
      auto r = lookup(local.get(), unixSec(t));
      if (r.offset == zoneOffset && (zoneName.empty() || r.name == zoneName)) {
        setLoc(t, local);
        return {t, nullptr};
      }
      setLoc(t, FixedZone(std::string(zoneName), zoneOffset));
      return {t, nullptr};
    }

    if (!zoneName.empty()) {
      auto t = time::Date(year, Month{month}, day, hour, min, sec, nsec, UTC);
      auto [offset, found] = lookupName(local.get(), zoneName, unixSec(t));
      if (found) {
        addSec(t, -offset);
        setLoc(t, local);
        return {t, nullptr};
      }
      if (zoneName.size() > 3 && zoneName.substr(0, 3) == "GMT") {
        offset = std::get<0>(atoi(zoneName.substr(3)));
        offset *= 3600;
      }
      setLoc(t, FixedZone(std::string(zoneName), offset));
      return {t, nullptr};
    }

    return {time::Date(year, Month{month}, day, hour, min, sec, nsec, defaultLocation), nullptr};
  }

  // parseRFC3339 parses s, which is at least as long as its prefix, as Go does.
  auto parseRFC3339(sv s, ptr<Location> local) -> std::tuple<Time, bool> {
    auto ok = true;
    auto parseUint = [&ok](sv s, int64_t min, int64_t max) -> int64_t {
      int64_t x = 0;
      for (auto c : s) {
        if (c < '0' || '9' < c) {
          ok = false;
          return min;
        }
        x = x * 10 + int64_t(c) - '0';
      }
      if (x < min || max < x) {
        ok = false;
        return min;
      }
      return x;
    };

    if (s.size() < sv("2006-01-02T15:04:05").size()) {
      return {Time{}, false};
    }
    auto year = parseUint(s.substr(0, 4), 0, 9999);
    auto month = parseUint(s.substr(5, 2), 1, 12);
    auto day = parseUint(s.substr(8, 2), 1, daysIn(Month{month}, year));
    auto hour = parseUint(s.substr(11, 2), 0, 23);
    auto min = parseUint(s.substr(14, 2), 0, 59);
    auto sec = parseUint(s.substr(17, 2), 0, 59);
    if (!ok || !(s[4] == '-' && s[7] == '-' && s[10] == 'T' && s[13] == ':' && s[16] == ':')) {
      return {Time{}, false};
    }
    s = s.substr(19);

    int64_t nsec = 0;
    if (s.size() >= 2 && s[0] == '.' && isDigit(s, 1)) {
      size_t n = 2;
      for (; n < s.size() && isDigit(s, int64_t(n)); n++) {
      }
      nsec = std::get<0>(parseNanoseconds(s, int64_t(n)));
      s = s.substr(n);
    }

    auto t = time::Date(year, Month{month}, day, hour, min, sec, nsec, UTC);
    if (s.size() != 1 || s[0] != 'Z') {
      if (s.size() != sv("-07:00").size()) {
        return {Time{}, false};
      }
      auto hr = parseUint(s.substr(1, 2), 0, 23);
      auto mm = parseUint(s.substr(4, 2), 0, 59);
      if (!ok || !((s[0] == '-' || s[0] == '+') && s[3] == ':')) {
        return {Time{}, false};
      }
      auto zoneOffset = (hr * 60 + mm) * 60;
      if (s[0] == '-') {
        zoneOffset *= -1;
      }
      addSec(t, -zoneOffset);

      if (lookup(local.get(), unixSec(t)).offset == zoneOffset) {
        setLoc(t, local);
      } else {
        setLoc(t, FixedZone("", zoneOffset));
      }
    }
    return {t, true};
  }

  auto parseStrictRFC3339(slice<uint8_t> b) -> std::tuple<Time, error> {
    auto s = eo::string_from_bytes(b);
    auto [t, ok] = parseRFC3339(s, Local);
    if (!ok) {
      auto [t, err] = time::Parse(RFC3339, s);
      if (err != nullptr) {
        return {Time{}, err};
      }
      // Go's strict checks that follow are disabled (https://go.dev/issue/54580).
      return {t, nullptr};
    }
    return {t, nullptr};
  }

  auto leadingFraction(sv s) -> std::tuple<uint64_t, double, sv> {
    size_t i = 0;
    uint64_t x = 0;
    double scale = 1;
    auto overflow = false;
    for (; i < s.size(); i++) {
      auto c = s[i];
      if (c < '0' || c > '9') {
        break;
      }
      if (overflow) {
        continue;
      }
      if (x > uint64_t(INT64_MAX) / 10) {
        overflow = true;
        continue;
      }
      auto y = x * 10 + uint64_t(c) - '0';
      if (y > uint64_t{1} << 63) {
        overflow = true;
        continue;
      }
      x = y;
      scale *= 10;
    }
    return {x, scale, s.substr(i)};
  }

  // Go's *time.parseDurationError.
  struct parseDurationError {
    static constexpr std::string_view _eo_name = "time.parseDurationError";

    std::string message;
    std::string value;

    auto operator==(const parseDurationError&) const -> bool = default;

    static auto Error(ptr<parseDurationError> e) -> std::string {
      return "time: " + e->message + " " + quote(e->value);
    }

    using _eo_methods = eo::methods<eo::method<&parseDurationError::Error, "Error">>;
  };

  auto durationError(std::string message, sv value) -> error {
    return New<parseDurationError>(parseDurationError{std::move(message), std::string(value)});
  }

  auto unitOf(sv u) -> std::tuple<uint64_t, bool> {
    if (u == "ns") {
      return {uint64_t(int64_t(Nanosecond)), true};
    }
    if (u == "us" || u == "\xc2\xb5s" || u == "\xce\xbcs") {
      return {uint64_t(int64_t(Microsecond)), true};
    }
    if (u == "ms") {
      return {uint64_t(int64_t(Millisecond)), true};
    }
    if (u == "s") {
      return {uint64_t(int64_t(Second)), true};
    }
    if (u == "m") {
      return {uint64_t(int64_t(Minute)), true};
    }
    if (u == "h") {
      return {uint64_t(int64_t(Hour)), true};
    }
    return {0, false};
  }

  // The time zones of zoneinfo.go and zoneinfo_read.go.

  auto fixedZone(std::string name, int64_t offset) -> ptr<Location> {
    auto l = New<Location>(Location{
      .name = name,
      .zone = slice<detail::zone>::of({detail::zone{name, offset, false}}),
      .tx = slice<detail::zoneTrans>::of({detail::zoneTrans{alpha, 0, false, false}}),
      .cacheStart = alpha,
      .cacheEnd = omega,
    });
    l->cacheZone = ptr<detail::zone>(l->zone, &*l->zone.begin());
    return l;
  }

  auto tzsetName(sv s) -> std::tuple<sv, sv, bool> {
    if (s.empty()) {
      return {sv{}, sv{}, false};
    }
    if (s[0] != '<') {
      for (size_t i = 0; i < s.size();) {
        auto [r, size] = eo::detail::decode_rune(s, i);
        switch (r) {
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
        case ',':
        case '-':
        case '+':
          if (i < 3) {
            return {sv{}, sv{}, false};
          }
          return {s.substr(0, i), s.substr(i), true};
        default:
          break;
        }
        i += size;
      }
      if (s.size() < 3) {
        return {sv{}, sv{}, false};
      }
      return {s, sv{}, true};
    }
    for (size_t i = 0; i < s.size();) {
      auto [r, size] = eo::detail::decode_rune(s, i);
      if (r == '>') {
        return {s.substr(1, i - 1), s.substr(i + 1), true};
      }
      i += size;
    }
    return {sv{}, sv{}, false};
  }

  auto tzsetNum(sv s, int64_t min, int64_t max) -> std::tuple<int64_t, sv, bool> {
    if (s.empty()) {
      return {0, sv{}, false};
    }
    int64_t num = 0;
    for (size_t i = 0; i < s.size();) {
      auto [r, size] = eo::detail::decode_rune(s, i);
      if (r < '0' || r > '9') {
        if (i == 0 || num < min) {
          return {0, sv{}, false};
        }
        return {num, s.substr(i), true};
      }
      num *= 10;
      num += int64_t(r) - '0';
      if (num > max) {
        return {0, sv{}, false};
      }
      i += size;
    }
    if (num < min) {
      return {0, sv{}, false};
    }
    return {num, sv{}, true};
  }

  auto tzsetOffset(sv s) -> std::tuple<int64_t, sv, bool> {
    if (s.empty()) {
      return {0, sv{}, false};
    }
    auto neg = false;
    if (s[0] == '+') {
      s = s.substr(1);
    } else if (s[0] == '-') {
      s = s.substr(1);
      neg = true;
    }

    auto [hours, rest, ok] = tzsetNum(s, 0, 24 * 7);
    if (!ok) {
      return {0, sv{}, false};
    }
    s = rest;
    auto off = hours * secondsPerHour;
    if (s.empty() || s[0] != ':') {
      if (neg) {
        off = -off;
      }
      return {off, s, true};
    }

    auto [mins, rest2, ok2] = tzsetNum(s.substr(1), 0, 59);
    if (!ok2) {
      return {0, sv{}, false};
    }
    s = rest2;
    off += mins * secondsPerMinute;
    if (s.empty() || s[0] != ':') {
      if (neg) {
        off = -off;
      }
      return {off, s, true};
    }

    auto [secs, rest3, ok3] = tzsetNum(s.substr(1), 0, 59);
    if (!ok3) {
      return {0, sv{}, false};
    }
    s = rest3;
    off += secs;

    if (neg) {
      off = -off;
    }
    return {off, s, true};
  }

  enum ruleKind : int64_t {
    ruleJulian,
    ruleDOY,
    ruleMonthWeekDay
  };

  struct rule {
    ruleKind kind{};
    int64_t day{};
    int64_t week{};
    int64_t mon{};
    int64_t time{};
  };

  auto tzsetRule(sv s) -> std::tuple<rule, sv, bool> {
    auto r = rule{};
    if (s.empty()) {
      return {rule{}, sv{}, false};
    }
    auto ok = false;
    if (s[0] == 'J') {
      int64_t jday = 0;
      std::tie(jday, s, ok) = tzsetNum(s.substr(1), 1, 365);
      if (!ok) {
        return {rule{}, sv{}, false};
      }
      r.kind = ruleJulian;
      r.day = jday;
    } else if (s[0] == 'M') {
      int64_t mon = 0;
      std::tie(mon, s, ok) = tzsetNum(s.substr(1), 1, 12);
      if (!ok || s.empty() || s[0] != '.') {
        return {rule{}, sv{}, false};
      }
      int64_t week = 0;
      std::tie(week, s, ok) = tzsetNum(s.substr(1), 1, 5);
      if (!ok || s.empty() || s[0] != '.') {
        return {rule{}, sv{}, false};
      }
      int64_t day = 0;
      std::tie(day, s, ok) = tzsetNum(s.substr(1), 0, 6);
      if (!ok) {
        return {rule{}, sv{}, false};
      }
      r.kind = ruleMonthWeekDay;
      r.day = day;
      r.week = week;
      r.mon = mon;
    } else {
      int64_t day = 0;
      std::tie(day, s, ok) = tzsetNum(s, 0, 365);
      if (!ok) {
        return {rule{}, sv{}, false};
      }
      r.kind = ruleDOY;
      r.day = day;
    }

    if (s.empty() || s[0] != '/') {
      r.time = 2 * secondsPerHour;
      return {r, s, true};
    }

    auto [offset, rest, ok2] = tzsetOffset(s.substr(1));
    if (!ok2) {
      return {rule{}, sv{}, false};
    }
    r.time = offset;

    return {r, rest, true};
  }

  auto tzruleTime(int64_t year, rule r, int64_t off) -> int64_t {
    int64_t s = 0;
    switch (r.kind) {
    case ruleJulian:
      s = (r.day - 1) * secondsPerDay;
      if (isLeap(year) && r.day >= 60) {
        s += secondsPerDay;
      }
      break;
    case ruleDOY:
      s = r.day * secondsPerDay;
      break;
    case ruleMonthWeekDay: {
      auto m1 = (r.mon + 9) % 12 + 1;
      auto yy0 = year;
      if (r.mon <= 2) {
        yy0--;
      }
      auto yy1 = yy0 / 100;
      auto yy2 = yy0 % 100;
      auto dow = ((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
      if (dow < 0) {
        dow += 7;
      }
      auto d = r.day - dow;
      if (d < 0) {
        d += 7;
      }
      for (int64_t i = 1; i < r.week; i++) {
        if (d + 7 >= daysIn(Month{r.mon}, year)) {
          break;
        }
        d += 7;
      }
      d += daysBefore(Month{r.mon});
      if (isLeap(year) && r.mon > 2) {
        d++;
      }
      s = d * secondsPerDay;
      break;
    }
    }
    return s + r.time - off;
  }

  auto tzset(sv s, int64_t lastTxSec, int64_t sec) -> std::tuple<lookup_result, bool> {
    auto fail = std::tuple<lookup_result, bool>{lookup_result{"", 0, 0, 0, false}, false};
    auto stdName = sv{};
    auto dstName = sv{};
    int64_t stdOffset = 0;
    int64_t dstOffset = 0;

    auto ok = false;
    std::tie(stdName, s, ok) = tzsetName(s);
    if (ok) {
      std::tie(stdOffset, s, ok) = tzsetOffset(s);
    }
    if (!ok) {
      return fail;
    }

    stdOffset = -stdOffset;

    if (s.empty() || s[0] == ',') {
      return {lookup_result{std::string(stdName), stdOffset, lastTxSec, omega, false}, true};
    }

    std::tie(dstName, s, ok) = tzsetName(s);
    if (ok) {
      if (s.empty() || s[0] == ',') {
        dstOffset = stdOffset + secondsPerHour;
      } else {
        std::tie(dstOffset, s, ok) = tzsetOffset(s);
        dstOffset = -dstOffset;
      }
    }
    if (!ok) {
      return fail;
    }

    if (s.empty()) {
      s = ",M3.2.0,M11.1.0";
    }
    if (s[0] != ',' && s[0] != ';') {
      return fail;
    }
    s = s.substr(1);

    auto startRule = rule{};
    auto endRule = rule{};
    std::tie(startRule, s, ok) = tzsetRule(s);
    if (!ok || s.empty() || s[0] != ',') {
      return fail;
    }
    s = s.substr(1);
    std::tie(endRule, s, ok) = tzsetRule(s);
    if (!ok || !s.empty()) {
      return fail;
    }

    auto [year, yday] = yearYday(days_of(uint64_t(sec + unixToInternal + internalToAbsolute)));

    auto ysec = (yday - 1) * secondsPerDay + sec % secondsPerDay;

    auto ystart = sec - ysec;

    auto startSec = tzruleTime(year, startRule, stdOffset);
    auto endSec = tzruleTime(year, endRule, dstOffset);
    auto dstIsDST = true;
    auto stdIsDST = false;
    if (endSec < startSec) {
      std::swap(startSec, endSec);
      std::swap(stdName, dstName);
      std::swap(stdOffset, dstOffset);
      std::swap(stdIsDST, dstIsDST);
    }

    if (ysec < startSec) {
      return {lookup_result{std::string(stdName), stdOffset, ystart, startSec + ystart, stdIsDST}, true};
    }
    if (ysec >= endSec) {
      return {lookup_result{std::string(stdName), stdOffset, endSec + ystart, ystart + 365 * secondsPerDay, stdIsDST},
        true};
    }
    return {lookup_result{std::string(dstName), dstOffset, startSec + ystart, endSec + ystart, dstIsDST}, true};
  }

  // Go's dataIO, which reads big-endian values from a TZif file.
  struct dataIO {
    sv p;
    bool error;

    auto read(int64_t n) -> sv {
      if (int64_t(p.size()) < n) {
        p = sv{};
        error = true;
        return sv{};
      }
      auto q = p.substr(0, size_t(n));
      p = p.substr(size_t(n));
      return q;
    }

    auto big4() -> std::tuple<uint32_t, bool> {
      auto q = read(4);
      if (q.size() < 4) {
        error = true;
        return {0, false};
      }
      return {uint32_t(uint8_t(q[3])) | uint32_t(uint8_t(q[2])) << 8 | uint32_t(uint8_t(q[1])) << 16 |
          uint32_t(uint8_t(q[0])) << 24,
        true};
    }

    auto big8() -> std::tuple<uint64_t, bool> {
      auto [n1, ok1] = big4();
      auto [n2, ok2] = big4();
      if (!ok1 || !ok2) {
        error = true;
        return {0, false};
      }
      return {(uint64_t(n1) << 32) | uint64_t(n2), true};
    }

    auto byte() -> std::tuple<uint8_t, bool> {
      auto q = read(1);
      if (q.size() < 1) {
        error = true;
        return {0, false};
      }
      return {uint8_t(q[0]), true};
    }

    auto rest() -> sv {
      auto r = p;
      p = sv{};
      return r;
    }
  };

  auto byteString(sv p) -> std::string {
    if (auto i = p.find('\0'); i != sv::npos) {
      p = p.substr(0, i);
    }
    return std::string(p);
  }

  auto findZone(const slice<detail::zone>& zones, sv name, int64_t offset, bool isDST) -> int64_t {
    for (int64_t i = 0; i < eo::len(zones); i++) {
      const auto& z = eo::at(zones, i);
      if (z.name == name && z.offset == offset && z.isDST == isDST) {
        return i;
      }
    }
    return -1;
  }

  // errBadData and errLocation are made on first use, as startNano is.
  auto errBadData() -> error {
    static const error e = errors::New("malformed time zone information");
    return e;
  }

  auto errLocation() -> error {
    static const error e = errors::New("time: invalid location name");
    return e;
  }

  // Go's time.fileSizeError.
  struct fileSizeError : eo::defined<fileSizeError, std::string> {
    using defined::defined;
    static constexpr std::string_view _eo_name = "time.fileSizeError";

    static auto Error(fileSizeError f) -> std::string {
      return "time: file " + std::string(f) + " is too large";
    }

    using _eo_methods = eo::methods<eo::method<&fileSizeError::Error, "Error">>;
  };

  constexpr int64_t maxFileSize = 10 << 20;

  auto errno_error(int e) -> error {
    return syscall::Errno{uint64_t(e)};
  }

  auto is_enoent(const error& err) -> bool {
    auto [e, ok] = eo::type_assert_ok<syscall::Errno>(err);
    return ok && uint64_t(e) == syscall::detail::enoent;
  }

  // Go's open, read, and closefd of sys_unix.go, which retry on EINTR as syscall does not, but
  // which Go's runtime makes rare by restarting system calls.
  auto open_file(const std::string& name) -> std::tuple<int, error> {
    for (;;) {
      auto fd = ::open(name.c_str(), O_RDONLY | O_CLOEXEC);
      if (fd >= 0) {
        return {fd, nullptr};
      }
      if (errno != EINTR) {
        return {-1, errno_error(errno)};
      }
    }
  }

  auto read_file(int fd, char* buf, size_t n) -> std::tuple<int64_t, error> {
    for (;;) {
      auto m = ::read(fd, buf, n);
      if (m >= 0) {
        return {int64_t(m), nullptr};
      }
      if (errno != EINTR) {
        return {-1, errno_error(errno)};
      }
    }
  }

  auto readFile(const std::string& name) -> std::tuple<std::string, error> {
    auto [f, err] = open_file(name);
    if (err != nullptr) {
      return {std::string{}, err};
    }
    auto ret = std::string{};
    std::array<char, 4096> buf{};
    int64_t n = 0;
    for (;;) {
      std::tie(n, err) = read_file(f, buf.data(), buf.size());
      if (n > 0) {
        ret.append(buf.data(), size_t(n));
      }
      if (n == 0 || err != nullptr) {
        break;
      }
      if (int64_t(ret.size()) > maxFileSize) {
        ::close(f);
        return {std::string{}, fileSizeError(name)};
      }
    }
    ::close(f);
    return {std::move(ret), err};
  }

  // get4, get2, preadn, and loadTzinfoFromZip work on slice<uint8_t>, as Go's work on []byte, so that
  // a malformed zip file panics where Go's slicing does, with Go's message.
  auto get4(slice<uint8_t> b) -> int64_t {
    if (eo::len(b) < 4) {
      return 0;
    }
    return int64_t(eo::at(b, 0)) | int64_t(eo::at(b, 1)) << 8 | int64_t(eo::at(b, 2)) << 16 |
      int64_t(eo::at(b, 3)) << 24;
  }

  auto get2(slice<uint8_t> b) -> int64_t {
    if (eo::len(b) < 2) {
      return 0;
    }
    return int64_t(eo::at(b, 0)) | int64_t(eo::at(b, 1)) << 8;
  }

  // Go's preadn: reads len(buf) bytes at off, or at off from the end when off is negative.
  auto preadn(int fd, slice<uint8_t> buf, int64_t off) -> error {
    auto whence = SEEK_SET;
    if (off < 0) {
      whence = SEEK_END;
    }
    if (::lseek(fd, off, whence) < 0) {
      return errno_error(errno);
    }
    while (eo::len(buf) > 0) {
      auto [m, err] = read_file(fd, reinterpret_cast<char*>(buf.data()), size_t(eo::len(buf)));
      if (m <= 0) {
        if (err == nullptr) {
          return errors::New("short read");
        }
        return err;
      }
      buf = eo::subslice(buf, m);
    }
    return nullptr;
  }

  auto loadTzinfoFromZip(const std::string& zipfile, const std::string& name) -> std::tuple<std::string, error> {
    auto [fd, err] = open_file(zipfile);
    if (err != nullptr) {
      return {std::string{}, err};
    }
    // Go's defer closefd(fd).
    struct closer {
      int fd;
      ~closer() {
        ::close(fd);
      }
    } close_fd{fd};

    constexpr int64_t zecheader = 0x06054b50;
    constexpr int64_t zcheader = 0x02014b50;
    constexpr int64_t ztailsize = 22;
    constexpr int64_t zheadersize = 30;
    constexpr int64_t zheader = 0x04034b50;

    auto corrupt = [&] {
      return std::tuple<std::string, error>{std::string{}, errors::New("corrupt zip file " + zipfile)};
    };

    auto buf = eo::make_slice<uint8_t>(ztailsize);
    if (auto e = preadn(fd, buf, -ztailsize); e != nullptr || get4(buf) != zecheader) {
      return corrupt();
    }
    auto n = get2(eo::subslice(buf, 10));
    auto size = get4(eo::subslice(buf, 12));
    auto off = get4(eo::subslice(buf, 16));

    buf = eo::make_slice<uint8_t>(size);
    if (auto e = preadn(fd, buf, off); e != nullptr) {
      return corrupt();
    }

    for (int64_t i = 0; i < n; i++) {
      if (get4(buf) != zcheader) {
        break;
      }
      auto meth = get2(eo::subslice(buf, 10));
      auto fsize = get4(eo::subslice(buf, 24));
      auto namelen = get2(eo::subslice(buf, 28));
      auto xlen = get2(eo::subslice(buf, 30));
      auto fclen = get2(eo::subslice(buf, 32));
      auto foff = get4(eo::subslice(buf, 42));
      auto zname = eo::subslice(buf, 46, 46 + namelen);
      buf = eo::subslice(buf, 46 + namelen + xlen + fclen);
      if (eo::string_from_bytes(zname) != name) {
        continue;
      }
      if (meth != 0) {
        return {std::string{}, errors::New("unsupported compression for " + name + " in " + zipfile)};
      }

      buf = eo::make_slice<uint8_t>(zheadersize + namelen);
      if (auto e = preadn(fd, buf, foff); e != nullptr || get4(buf) != zheader || get2(eo::subslice(buf, 8)) != meth ||
          get2(eo::subslice(buf, 26)) != namelen ||
          eo::string_from_bytes(eo::subslice(buf, 30, 30 + namelen)) != name) {
        return corrupt();
      }
      xlen = get2(eo::subslice(buf, 28));

      buf = eo::make_slice<uint8_t>(fsize);
      if (auto e = preadn(fd, buf, foff + 30 + namelen + xlen); e != nullptr) {
        return corrupt();
      }

      return {eo::string_from_bytes(buf), nullptr};
    }

    return {std::string{}, syscall::Errno{syscall::detail::enoent}};
  }

  auto loadTzinfoFromDirOrZip(const std::string& dir, std::string name) -> std::tuple<std::string, error> {
    if (dir.size() > 4 && sv(dir).substr(dir.size() - 4) == ".zip") {
      return loadTzinfoFromZip(dir, name);
    }
    if (!dir.empty()) {
      name = dir + "/" + name;
    }
    return readFile(name);
  }

  auto loadLocation(const std::string& name, std::initializer_list<sv> sources) -> std::tuple<ptr<Location>, error> {
    auto firstErr = error{};
    for (auto source : sources) {
      auto [zoneData, err] = loadTzinfoFromDirOrZip(std::string(source), name);
      if (err == nullptr) {
        auto [z, e] = LoadLocationFromTZData(name, eo::bytes_from_string(zoneData));
        if (e == nullptr) {
          return {z, nullptr};
        }
        err = e;
      }
      if (firstErr == nullptr && !is_enoent(err)) {
        firstErr = err;
      }
    }
    if (firstErr != nullptr) {
      return {nullptr, firstErr};
    }
    return {nullptr, errors::New("unknown time zone " + name)};
  }

  constexpr std::initializer_list<sv> platformZoneSources = {
    "/usr/share/zoneinfo/",
    "/usr/share/lib/zoneinfo/",
    "/usr/lib/locale/TZ/",
    "/etc/zoneinfo",
  };

  void initLocal() {
    auto& localLoc = detail::localLoc;
    auto [tz, ok] = syscall::Getenv("TZ");
    if (!ok) {
      auto [z, err] = loadLocation("localtime", {"/etc"});
      if (err == nullptr) {
        localLoc = *z;
        localLoc.name = "Local";
        return;
      }
    } else if (!tz.empty()) {
      if (tz[0] == ':') {
        tz = tz.substr(1);
      }
      if (!tz.empty() && tz[0] == '/') {
        if (auto [z, err] = loadLocation(tz, {""}); err == nullptr) {
          localLoc = *z;
          if (tz == "/etc/localtime") {
            localLoc.name = "Local";
          } else {
            localLoc.name = tz;
          }
          return;
        }
      } else if (!tz.empty() && tz != "UTC") {
        if (auto [z, err] = loadLocation(tz, platformZoneSources); err == nullptr) {
          localLoc = *z;
          return;
        }
      }
    }
    localLoc.name = "UTC";
  }

  auto containsDotDot(sv s) -> bool {
    if (s.size() < 2) {
      return false;
    }
    for (size_t i = 0; i < s.size() - 1; i++) {
      if (s[i] == '.' && s[i + 1] == '.') {
        return true;
      }
    }
    return false;
  }

} // namespace

// Duration

auto Duration::String(Duration d) -> std::string {
  std::array<char, 32> arr{};
  auto* buf = arr.data();
  int64_t w = 32;
  auto u = uint64_t(int64_t(d));
  auto neg = int64_t(d) < 0;
  if (neg) {
    u = -u;
  }
  if (u < uint64_t(int64_t(Second))) {
    int64_t prec = 0;
    w--;
    buf[w] = 's';
    w--;
    if (u == 0) {
      buf[w] = '0';
      return std::string(buf + w, size_t(32 - w));
    }
    if (u < uint64_t(int64_t(Microsecond))) {
      prec = 0;
      buf[w] = 'n';
    } else if (u < uint64_t(int64_t(Millisecond))) {
      prec = 3;
      w--;
      buf[w] = '\xc2';
      buf[w + 1] = '\xb5';
    } else {
      prec = 6;
      buf[w] = 'm';
    }
    std::tie(w, u) = fmtFrac(buf, w, u, prec);
    w = fmtInt(buf, w, u);
  } else {
    w--;
    buf[w] = 's';
    std::tie(w, u) = fmtFrac(buf, w, u, 9);
    w = fmtInt(buf, w, u % 60);
    u /= 60;
    if (u > 0) {
      w--;
      buf[w] = 'm';
      w = fmtInt(buf, w, u % 60);
      u /= 60;
      if (u > 0) {
        w--;
        buf[w] = 'h';
        w = fmtInt(buf, w, u);
      }
    }
  }
  if (neg) {
    w--;
    buf[w] = '-';
  }
  return std::string(buf + w, size_t(32 - w));
}

auto Duration::Nanoseconds(Duration d) -> int64_t {
  return int64_t(d);
}

auto Duration::Microseconds(Duration d) -> int64_t {
  return int64_t(d) / 1000;
}

auto Duration::Milliseconds(Duration d) -> int64_t {
  return int64_t(d) / 1000000;
}

auto Duration::Seconds(Duration d) -> double {
  auto sec = int64_t(d) / int64_t(Second);
  auto nsec = int64_t(d) % int64_t(Second);
  return double(sec) + double(nsec) / 1e9;
}

auto Duration::Minutes(Duration d) -> double {
  auto min = int64_t(d) / int64_t(Minute);
  auto nsec = int64_t(d) % int64_t(Minute);
  return double(min) + double(nsec) / (60 * 1e9);
}

auto Duration::Hours(Duration d) -> double {
  auto hour = int64_t(d) / int64_t(Hour);
  auto nsec = int64_t(d) % int64_t(Hour);
  return double(hour) + double(nsec) / (60 * 60 * 1e9);
}

auto Duration::Truncate(Duration d, Duration m) -> Duration {
  if (int64_t(m) <= 0) {
    return d;
  }
  return Duration{int64_t(d) - int64_t(d) % int64_t(m)};
}

auto Duration::Round(Duration d, Duration m) -> Duration {
  if (int64_t(m) <= 0) {
    return d;
  }
  auto r = Duration{int64_t(d) % int64_t(m)};
  if (int64_t(d) < 0) {
    r = -r;
    if (lessThanHalf(r, m)) {
      return d + r;
    }
    if (auto d1 = d - m + r; d1 < d) {
      return d1;
    }
    return minDuration;
  }
  if (lessThanHalf(r, m)) {
    return d - r;
  }
  if (auto d1 = d + m - r; d1 > d) {
    return d1;
  }
  return maxDuration;
}

auto Duration::Abs(Duration d) -> Duration {
  if (int64_t(d) >= 0) {
    return d;
  }
  if (d == minDuration) {
    return maxDuration;
  }
  return -d;
}

// Month and Weekday

auto Month::String(Month m) -> std::string {
  if (January <= m && m <= December) {
    return std::string(longMonthNames[size_t(int64_t(m) - 1)]);
  }
  std::array<char, 20> buf{};
  auto n = fmtInt(buf.data(), 20, uint64_t(int64_t(m)));
  return "%!Month(" + std::string(buf.data() + n, size_t(20 - n)) + ")";
}

auto Weekday::String(Weekday d) -> std::string {
  if (Sunday <= d && d <= Saturday) {
    return std::string(longDayNames[size_t(int64_t(d))]);
  }
  std::array<char, 20> buf{};
  auto n = fmtInt(buf.data(), 20, uint64_t(int64_t(d)));
  return "%!Weekday(" + std::string(buf.data() + n, size_t(20 - n)) + ")";
}

// Location

auto Location::String(ptr<Location> l) -> std::string {
  return get(l.get())->name;
}

auto FixedZone(const std::string& name, int64_t offset) -> ptr<Location> {
  constexpr int64_t hoursBeforeUTC = 12;
  constexpr int64_t hoursAfterUTC = 14;
  auto hour = offset / 60 / 60;
  if (name.empty() && -hoursBeforeUTC <= hour && hour <= +hoursAfterUTC && hour * 60 * 60 == offset) {
    static const auto unnamedFixedZones = [] {
      auto zones = std::array<ptr<Location>, hoursBeforeUTC + 1 + hoursAfterUTC>{};
      for (auto hr = -hoursBeforeUTC; hr <= +hoursAfterUTC; hr++) {
        zones[size_t(hr + hoursBeforeUTC)] = fixedZone("", hr * 60 * 60);
      }
      return zones;
    }();
    return unnamedFixedZones[size_t(hour + hoursBeforeUTC)];
  }
  return fixedZone(name, offset);
}

auto LoadLocation(const std::string& name) -> std::tuple<ptr<Location>, error> {
  if (name.empty() || name == "UTC") {
    return {UTC, nullptr};
  }
  if (name == "Local") {
    return {Local, nullptr};
  }
  if (containsDotDot(name) || name[0] == '/' || name[0] == '\\') {
    return {nullptr, errLocation()};
  }
  static const auto zoneinfo = std::get<0>(syscall::Getenv("ZONEINFO"));
  auto firstErr = error{};
  if (!zoneinfo.empty()) {
    if (auto [zoneData, err] = loadTzinfoFromDirOrZip(zoneinfo, name); err == nullptr) {
      if (auto [z, e] = LoadLocationFromTZData(name, eo::bytes_from_string(zoneData)); e == nullptr) {
        return {z, nullptr};
      }
      // Go's inner err shadows the outer one, which is nil here, so firstErr stays nil and the
      // system's error is returned.
      firstErr = err;
    } else if (!is_enoent(err)) {
      firstErr = err;
    }
  }
  if (auto [z, err] = loadLocation(name, platformZoneSources); err == nullptr) {
    return {z, nullptr};
  } else if (firstErr == nullptr) {
    firstErr = err;
  }
  return {nullptr, firstErr};
}

auto LoadLocationFromTZData(const std::string& name, slice<uint8_t> data) -> std::tuple<ptr<Location>, error> {
  auto bytes = eo::string_from_bytes(data);
  auto d = dataIO{bytes, false};

  if (auto magic = d.read(4); magic != "TZif") {
    return {nullptr, errBadData()};
  }

  int64_t version = 0;
  if (auto p = d.read(16); p.size() != 16) {
    return {nullptr, errBadData()};
  } else {
    switch (p[0]) {
    case 0:
      version = 1;
      break;
    case '2':
      version = 2;
      break;
    case '3':
      version = 3;
      break;
    default:
      return {nullptr, errBadData()};
    }
  }

  enum {
    NUTCLocal,
    NStdWall,
    NLeap,
    NTime,
    NZone,
    NChar
  };
  std::array<int64_t, 6> n{};
  for (auto& ni : n) {
    auto [nn, ok] = d.big4();
    if (!ok) {
      return {nullptr, errBadData()};
    }
    ni = int64_t(nn);
  }

  auto is64 = false;
  if (version > 1) {
    auto skip = n[NTime] * 4 + n[NTime] + n[NZone] * 6 + n[NChar] + n[NLeap] * 8 + n[NStdWall] + n[NUTCLocal];
    skip += 4 + 16;
    d.read(skip);

    is64 = true;

    for (auto& ni : n) {
      auto [nn, ok] = d.big4();
      if (!ok) {
        return {nullptr, errBadData()};
      }
      ni = int64_t(nn);
    }
  }

  int64_t size = 4;
  if (is64) {
    size = 8;
  }

  auto txtimes = dataIO{d.read(n[NTime] * size), false};
  auto txzones = d.read(n[NTime]);
  auto zonedata = dataIO{d.read(n[NZone] * 6), false};
  auto abbrev = d.read(n[NChar]);
  d.read(n[NLeap] * (size + 4));
  auto isstd = d.read(n[NStdWall]);
  auto isutc = d.read(n[NUTCLocal]);

  if (d.error) {
    return {nullptr, errBadData()};
  }

  auto extend = std::string{};
  auto rest = d.rest();
  if (rest.size() > 2 && rest[0] == '\n' && rest.back() == '\n') {
    extend = std::string(rest.substr(1, rest.size() - 2));
  }

  auto nzone = n[NZone];
  if (nzone == 0) {
    return {nullptr, errBadData()};
  }
  auto zones = eo::make_slice<detail::zone>(nzone);
  for (auto& zone : zones) {
    auto [nz, ok] = zonedata.big4();
    if (!ok) {
      return {nullptr, errBadData()};
    }
    zone.offset = int64_t(int32_t(nz));
    auto [b, okb] = zonedata.byte();
    if (!okb) {
      return {nullptr, errBadData()};
    }
    zone.isDST = b != 0;
    std::tie(b, okb) = zonedata.byte();
    if (!okb || int64_t(b) >= int64_t(abbrev.size())) {
      return {nullptr, errBadData()};
    }
    zone.name = byteString(abbrev.substr(b));
  }

  auto tx = eo::make_slice<detail::zoneTrans>(n[NTime]);
  for (int64_t i = 0; i < eo::len(tx); i++) {
    auto& t = eo::at(tx, i);
    int64_t when = 0;
    if (!is64) {
      auto [n4, ok] = txtimes.big4();
      if (!ok) {
        return {nullptr, errBadData()};
      }
      when = int64_t(int32_t(n4));
    } else {
      auto [n8, ok] = txtimes.big8();
      if (!ok) {
        return {nullptr, errBadData()};
      }
      when = int64_t(n8);
    }
    t.when = when;
    if (int64_t(uint8_t(txzones[size_t(i)])) >= eo::len(zones)) {
      return {nullptr, errBadData()};
    }
    t.index = uint8_t(txzones[size_t(i)]);
    if (i < int64_t(isstd.size())) {
      t.isstd = isstd[size_t(i)] != 0;
    }
    if (i < int64_t(isutc.size())) {
      t.isutc = isutc[size_t(i)] != 0;
    }
  }

  if (eo::len(tx) == 0) {
    tx = eo::append(tx, {detail::zoneTrans{.when = alpha, .index = 0}});
  }

  auto l = New<Location>(Location{.name = name, .zone = zones, .tx = tx, .extend = extend});

  auto sec = std::get<0>(runtimeNow());
  for (int64_t i = 0; i < eo::len(tx); i++) {
    if (eo::at(tx, i).when <= sec && (i + 1 == eo::len(tx) || sec < eo::at(tx, i + 1).when)) {
      l->cacheStart = eo::at(tx, i).when;
      l->cacheEnd = omega;
      l->cacheZone = ptr<detail::zone>(l->zone, &eo::at(l->zone, eo::at(tx, i).index));
      if (i + 1 < eo::len(tx)) {
        l->cacheEnd = eo::at(tx, i + 1).when;
      } else if (!l->extend.empty()) {
        if (auto [e, ok] = tzset(l->extend, l->cacheStart, sec); ok) {
          l->cacheStart = e.start;
          l->cacheEnd = e.end;
          if (auto zoneIdx = findZone(l->zone, e.name, e.offset, e.isDST); zoneIdx != -1) {
            l->cacheZone = ptr<detail::zone>(l->zone, &eo::at(l->zone, zoneIdx));
          } else {
            l->cacheZone = New<detail::zone>(detail::zone{e.name, e.offset, e.isDST});
          }
        }
      }
      break;
    }
  }

  return {l, nullptr};
}

// Time

auto Time::IsZero(Time t) -> bool {
  return t.wall == 0 && t.ext == 0;
}

auto Time::After(Time t, Time u) -> bool {
  if ((t.wall & u.wall & hasMonotonic) != 0) {
    return t.ext > u.ext;
  }
  auto ts = sec(t);
  auto us = sec(u);
  return ts > us || (ts == us && nsec(t) > nsec(u));
}

auto Time::Before(Time t, Time u) -> bool {
  if ((t.wall & u.wall & hasMonotonic) != 0) {
    return t.ext < u.ext;
  }
  auto ts = sec(t);
  auto us = sec(u);
  return ts < us || (ts == us && nsec(t) < nsec(u));
}

auto Time::Compare(Time t, Time u) -> int64_t {
  int64_t tc = 0;
  int64_t uc = 0;
  if ((t.wall & u.wall & hasMonotonic) != 0) {
    tc = t.ext;
    uc = u.ext;
  } else {
    tc = sec(t);
    uc = sec(u);
    if (tc == uc) {
      tc = nsec(t);
      uc = nsec(u);
    }
  }
  if (tc < uc) {
    return -1;
  }
  if (tc > uc) {
    return +1;
  }
  return 0;
}

auto Time::Equal(Time t, Time u) -> bool {
  if ((t.wall & u.wall & hasMonotonic) != 0) {
    return t.ext == u.ext;
  }
  return sec(t) == sec(u) && nsec(t) == nsec(u);
}

auto Time::Date(Time t) -> std::tuple<int64_t, eo::time::Month, int64_t> {
  return date(days_of(absSec(t)));
}

auto Time::Year(Time t) -> int64_t {
  auto [century, cyear, ayday] = split(days_of(absSec(t)));
  return year_of(century, cyear, janFeb(ayday));
}

auto Time::Month(Time t) -> eo::time::Month {
  auto ayday = split(days_of(absSec(t))).ayday;
  return month_of(std::get<0>(split_yday(ayday)), janFeb(ayday));
}

auto Time::Day(Time t) -> int64_t {
  return std::get<1>(split_yday(split(days_of(absSec(t))).ayday));
}

auto Time::Weekday(Time t) -> eo::time::Weekday {
  return weekday(days_of(absSec(t)));
}

auto Time::ISOWeek(Time t) -> std::tuple<int64_t, int64_t> {
  auto days = days_of(absSec(t));
  auto thu = days + uint64_t(int64_t(Thursday) - (int64_t(weekday(days - 1)) + 1));
  auto [year, yday] = yearYday(thu);
  return {year, (yday - 1) / 7 + 1};
}

auto Time::Clock(Time t) -> std::tuple<int64_t, int64_t, int64_t> {
  return clock(absSec(t));
}

auto Time::Hour(Time t) -> int64_t {
  return int64_t(absSec(t) % uint64_t(secondsPerDay)) / secondsPerHour;
}

auto Time::Minute(Time t) -> int64_t {
  return int64_t(absSec(t) % uint64_t(secondsPerHour)) / secondsPerMinute;
}

auto Time::Second(Time t) -> int64_t {
  return int64_t(absSec(t) % uint64_t(secondsPerMinute));
}

auto Time::Nanosecond(Time t) -> int64_t {
  return nsec(t);
}

auto Time::YearDay(Time t) -> int64_t {
  return std::get<1>(yearYday(days_of(absSec(t))));
}

auto Time::Add(Time t, Duration d) -> Time {
  auto dd = int64_t(d);
  auto dsec = dd / 1000000000;
  auto ns = nsec(t) + int32_t(dd % 1000000000);
  if (ns >= 1000000000) {
    dsec++;
    ns -= 1000000000;
  } else if (ns < 0) {
    dsec--;
    ns += 1000000000;
  }
  t.wall = (t.wall & ~nsecMask) | uint64_t(int64_t(ns));
  addSec(t, dsec);
  if ((t.wall & hasMonotonic) != 0) {
    auto te = t.ext + dd;
    if ((dd < 0 && te > t.ext) || (dd > 0 && te < t.ext)) {
      stripMono(t);
    } else {
      t.ext = te;
    }
  }
  return t;
}

auto Time::Sub(Time t, Time u) -> Duration {
  if ((t.wall & u.wall & hasMonotonic) != 0) {
    return subMono(t.ext, u.ext);
  }
  auto d = Duration{sec(t) - sec(u)} * eo::time::Second + Duration{nsec(t) - nsec(u)};
  if (Time::Equal(Time::Add(u, d), t)) {
    return d;
  }
  if (Time::Before(t, u)) {
    return minDuration;
  }
  return maxDuration;
}

auto Time::AddDate(Time t, int64_t years, int64_t months, int64_t days) -> Time {
  auto [year, month, day] = Time::Date(t);
  auto [hour, min, sec] = Time::Clock(t);
  return time::Date(year + years,
    month + eo::time::Month{months},
    day + days,
    hour,
    min,
    sec,
    nsec(t),
    Time::Location(t));
}

auto Time::UTC(Time t) -> Time {
  setLoc(t, ref(detail::utcLoc));
  return t;
}

auto Time::Local(Time t) -> Time {
  setLoc(t, time::Local);
  return t;
}

auto Time::In(Time t, ptr<eo::time::Location> loc) -> Time {
  if (loc == nullptr) {
    panic("time: missing Location in call to Time.In"s);
  }
  setLoc(t, std::move(loc));
  return t;
}

auto Time::Location(Time t) -> ptr<eo::time::Location> {
  auto l = t.loc;
  if (l == nullptr) {
    l = time::UTC;
  }
  return l;
}

auto Time::Zone(Time t) -> std::tuple<std::string, int64_t> {
  auto r = lookup(t.loc.get(), unixSec(t));
  return {std::move(r.name), r.offset};
}

auto Time::ZoneBounds(Time t) -> std::tuple<Time, Time> {
  auto r = lookup(t.loc.get(), unixSec(t));
  auto start = Time{};
  auto end = Time{};
  if (r.start != alpha) {
    start = unixTime(r.start, 0);
    setLoc(start, t.loc);
  }
  if (r.end != omega) {
    end = unixTime(r.end, 0);
    setLoc(end, t.loc);
  }
  return {start, end};
}

auto Time::Unix(Time t) -> int64_t {
  return unixSec(t);
}

auto Time::UnixMilli(Time t) -> int64_t {
  return unixSec(t) * 1000 + int64_t(nsec(t)) / 1000000;
}

auto Time::UnixMicro(Time t) -> int64_t {
  return unixSec(t) * 1000000 + int64_t(nsec(t)) / 1000;
}

auto Time::UnixNano(Time t) -> int64_t {
  return unixSec(t) * 1000000000 + int64_t(nsec(t));
}

auto Time::AppendBinary(Time t, slice<uint8_t> b) -> std::tuple<slice<uint8_t>, error> {
  constexpr uint8_t timeBinaryVersionV1 = 1;
  constexpr uint8_t timeBinaryVersionV2 = 2;
  int16_t offsetMin = 0;
  int8_t offsetSec = 0;
  auto version = timeBinaryVersionV1;

  if (Time::Location(t) == time::UTC) {
    offsetMin = -1;
  } else {
    auto offset = std::get<1>(Time::Zone(t));
    if (offset % 60 != 0) {
      version = timeBinaryVersionV2;
      offsetSec = int8_t(offset % 60);
    }

    offset /= 60;
    if (offset < -32768 || offset == -1 || offset > 32767) {
      return {b, errors::New("Time.MarshalBinary: unexpected zone offset")};
    }
    offsetMin = int16_t(offset);
  }

  auto s = sec(t);
  auto ns = nsec(t);
  b = eo::append(b,
    {version,
      uint8_t(s >> 56),
      uint8_t(s >> 48),
      uint8_t(s >> 40),
      uint8_t(s >> 32),
      uint8_t(s >> 24),
      uint8_t(s >> 16),
      uint8_t(s >> 8),
      uint8_t(s),
      uint8_t(ns >> 24),
      uint8_t(ns >> 16),
      uint8_t(ns >> 8),
      uint8_t(ns),
      uint8_t(offsetMin >> 8),
      uint8_t(offsetMin)});
  if (version == timeBinaryVersionV2) {
    b = eo::append(b, {uint8_t(offsetSec)});
  }
  return {b, nullptr};
}

auto Time::MarshalBinary(Time t) -> std::tuple<slice<uint8_t>, error> {
  auto [b, err] = Time::AppendBinary(t, eo::make_slice<uint8_t>(0, 16));
  if (err != nullptr) {
    return {nullptr, err};
  }
  return {b, nullptr};
}

auto Time::UnmarshalBinary(ptr<Time> t, slice<uint8_t> data) -> error {
  auto buf = data;
  if (eo::len(buf) == 0) {
    return errors::New("Time.UnmarshalBinary: no data");
  }

  auto version = eo::at(buf, 0);
  if (version != 1 && version != 2) {
    return errors::New("Time.UnmarshalBinary: unsupported version");
  }

  int64_t wantLen = 1 + 8 + 4 + 2;
  if (version == 2) {
    wantLen++;
  }
  if (eo::len(buf) != wantLen) {
    return errors::New("Time.UnmarshalBinary: invalid length");
  }

  buf = eo::subslice(buf, 1);
  auto byte = [&](int64_t i) { return int64_t(eo::at(buf, i)); };
  auto s = byte(7) | byte(6) << 8 | byte(5) << 16 | byte(4) << 24 | byte(3) << 32 | byte(2) << 40 | byte(1) << 48 |
    byte(0) << 56;

  buf = eo::subslice(buf, 8);
  auto ns = int32_t(uint32_t(byte(3)) | uint32_t(byte(2)) << 8 | uint32_t(byte(1)) << 16 | uint32_t(byte(0)) << 24);

  buf = eo::subslice(buf, 4);
  auto offset = int64_t(int16_t(uint16_t(byte(1)) | uint16_t(byte(0)) << 8)) * 60;
  if (version == 2) {
    offset += byte(2);
  }

  *t = Time{};
  t->wall = uint64_t(int64_t(ns));
  t->ext = s;

  if (offset == -1 * 60) {
    setLoc(*t, ref(detail::utcLoc));
  } else if (lookup(time::Local.get(), unixSec(*t)).offset == offset) {
    setLoc(*t, time::Local);
  } else {
    setLoc(*t, FixedZone("", offset));
  }

  return nullptr;
}

auto Time::GobEncode(Time t) -> std::tuple<slice<uint8_t>, error> {
  return Time::MarshalBinary(t);
}

auto Time::GobDecode(ptr<Time> t, slice<uint8_t> data) -> error {
  return Time::UnmarshalBinary(t, data);
}

auto Time::MarshalJSON(Time t) -> std::tuple<slice<uint8_t>, error> {
  auto b = std::string{"\""};
  auto err = appendStrictRFC3339(t, b);
  b += '"';
  if (!err.empty()) {
    return {nullptr, errors::New("Time.MarshalJSON: " + err)};
  }
  auto out = eo::make_slice<uint8_t>(0, int64_t(RFC3339Nano.size() + 2));
  return {eo::append_slice(out, b), nullptr};
}

auto Time::UnmarshalJSON(ptr<Time> t, slice<uint8_t> data) -> error {
  if (eo::string_from_bytes(data) == "null") {
    return nullptr;
  }
  if (eo::len(data) < 2 || eo::at(data, 0) != '"' || eo::at(data, eo::len(data) - 1) != '"') {
    return errors::New("Time.UnmarshalJSON: input is not a JSON string");
  }
  data = eo::subslice(data, 1, eo::len(data) - 1);
  auto [parsed, err] = parseStrictRFC3339(data);
  *t = parsed;
  return err;
}

auto Time::AppendText(Time t, slice<uint8_t> b) -> std::tuple<slice<uint8_t>, error> {
  auto s = std::string{};
  auto err = appendStrictRFC3339(t, s);
  if (!err.empty()) {
    return {nullptr, errors::New("Time.AppendText: " + err)};
  }
  return {eo::append_slice(b, s), nullptr};
}

auto Time::MarshalText(Time t) -> std::tuple<slice<uint8_t>, error> {
  auto s = std::string{};
  auto err = appendStrictRFC3339(t, s);
  if (!err.empty()) {
    return {nullptr, errors::New("Time.MarshalText: " + err)};
  }
  return {eo::append_slice(eo::make_slice<uint8_t>(0, int64_t(RFC3339Nano.size())), s), nullptr};
}

auto Time::UnmarshalText(ptr<Time> t, slice<uint8_t> data) -> error {
  auto [parsed, err] = parseStrictRFC3339(data);
  *t = parsed;
  return err;
}

auto Time::IsDST(Time t) -> bool {
  return lookup(t.loc.get(), Time::Unix(t)).isDST;
}

auto Time::Truncate(Time t, Duration d) -> Time {
  stripMono(t);
  if (int64_t(d) <= 0) {
    return t;
  }
  auto r = std::get<1>(div(t, d));
  return Time::Add(t, -r);
}

auto Time::Round(Time t, Duration d) -> Time {
  stripMono(t);
  if (int64_t(d) <= 0) {
    return t;
  }
  auto r = std::get<1>(div(t, d));
  if (lessThanHalf(r, d)) {
    return Time::Add(t, -r);
  }
  return Time::Add(t, d - r);
}

auto Time::String(Time t) -> std::string {
  auto s = Time::Format(t, "2006-01-02 15:04:05.999999999 -0700 MST");

  if ((t.wall & hasMonotonic) != 0) {
    auto m2 = uint64_t(t.ext);
    auto sign = '+';
    if (t.ext < 0) {
      sign = '-';
      m2 = -m2;
    }
    auto m1 = m2 / 1000000000;
    m2 = m2 % 1000000000;
    auto m0 = m1 / 1000000000;
    m1 = m1 % 1000000000;
    auto buf = std::string{" m="};
    buf += sign;
    int64_t wid = 0;
    if (m0 != 0) {
      appendInt(buf, int64_t(m0), 0);
      wid = 9;
    }
    appendInt(buf, int64_t(m1), wid);
    buf += '.';
    appendInt(buf, int64_t(m2), 9);
    s += buf;
  }
  return s;
}

auto Time::GoString(Time t) -> std::string {
  auto abs = absSec(t);
  auto [year, month, day] = date(days_of(abs));
  auto [hour, minute, second] = clock(abs);

  auto buf = std::string{"time.Date("};
  appendInt(buf, year, 0);
  if (January <= month && month <= December) {
    buf += ", time.";
    buf += longMonthNames[size_t(int64_t(month) - 1)];
  } else {
    appendInt(buf, int64_t(month), 0);
  }
  buf += ", ";
  appendInt(buf, day, 0);
  buf += ", ";
  appendInt(buf, hour, 0);
  buf += ", ";
  appendInt(buf, minute, 0);
  buf += ", ";
  appendInt(buf, second, 0);
  buf += ", ";
  appendInt(buf, Time::Nanosecond(t), 0);
  buf += ", ";
  auto loc = Time::Location(t);
  if (loc == time::UTC || loc == nullptr) {
    buf += "time.UTC";
  } else if (loc == time::Local) {
    buf += "time.Local";
  } else {
    buf += "time.Location(";
    buf += quote(loc->name);
    buf += ')';
  }
  buf += ')';
  return buf;
}

auto Time::Format(Time t, const std::string& layout) -> std::string {
  auto b = std::string{};
  b.reserve(layout.size() + 10);
  appendFormatAny(t, b, layout);
  return b;
}

auto Time::AppendFormat(Time t, slice<uint8_t> b, const std::string& layout) -> slice<uint8_t> {
  auto s = std::string{};
  appendFormatAny(t, s, layout);
  return eo::append_slice(b, s);
}

auto ParseError::Error(ptr<ParseError> e) -> std::string {
  if (e->Message.empty()) {
    return "parsing time " + quote(e->Value) + " as " + quote(e->Layout) + ": cannot parse " + quote(e->ValueElem) +
      " as " + quote(e->LayoutElem);
  }
  return "parsing time " + quote(e->Value) + e->Message;
}

// Functions

auto Now() -> Time {
  // startNano is read first, so that it precedes the reading below, as Go's is set at package
  // initialization.
  auto start = startNano();
  auto [sec, nsec, mono] = runtimeNow();
  if (mono == 0) {
    return Time{uint64_t(int64_t(nsec)), sec + unixToInternal, Local};
  }
  mono -= start;
  sec += unixToInternal - minWall;
  if (uint64_t(sec) >> 33 != 0) {
    return Time{uint64_t(int64_t(nsec)), sec + minWall, Local};
  }
  return Time{hasMonotonic | uint64_t(sec) << nsecShift | uint64_t(int64_t(nsec)), mono, Local};
}

auto Since(Time t) -> Duration {
  if ((t.wall & hasMonotonic) != 0) {
    return subMono(runtimeNano() - startNano(), t.ext);
  }
  return Time::Sub(Now(), t);
}

auto Until(Time t) -> Duration {
  if ((t.wall & hasMonotonic) != 0) {
    return subMono(t.ext, runtimeNano() - startNano());
  }
  return Time::Sub(t, Now());
}

auto Unix(int64_t sec, int64_t nsec) -> Time {
  if (nsec < 0 || nsec >= 1000000000) {
    auto n = nsec / 1000000000;
    sec += n;
    nsec -= n * 1000000000;
    if (nsec < 0) {
      nsec += 1000000000;
      sec--;
    }
  }
  return unixTime(sec, int32_t(nsec));
}

auto UnixMilli(int64_t msec) -> Time {
  return time::Unix(msec / 1000, (msec % 1000) * 1000000);
}

auto UnixMicro(int64_t usec) -> Time {
  return time::Unix(usec / 1000000, (usec % 1000000) * 1000);
}

auto Date(
  int64_t year, Month month, int64_t day, int64_t hour, int64_t min, int64_t sec, int64_t nsec, ptr<Location> loc)
  -> Time {
  if (loc == nullptr) {
    panic("time: missing Location in call to Date"s);
  }

  auto m = int64_t(month) - 1;
  std::tie(year, m) = norm(year, m, 12);
  month = Month{m + 1};

  std::tie(sec, nsec) = norm(sec, nsec, 1000000000);
  std::tie(min, sec) = norm(min, sec, 60);
  std::tie(hour, min) = norm(hour, min, 60);
  std::tie(day, hour) = norm(day, hour, 24);

  auto unix = int64_t(dateToAbsDays(year, month, day)) * secondsPerDay +
    (hour * secondsPerHour + min * secondsPerMinute + sec) + absoluteToUnix;

  auto r = lookup(loc.get(), unix);
  auto offset = r.offset;
  if (offset != 0) {
    auto utc = unix - offset;
    if (utc < r.start || utc >= r.end) {
      offset = lookup(loc.get(), utc).offset;
    }
    unix -= offset;
  }

  auto t = unixTime(unix, int32_t(nsec));
  setLoc(t, loc);
  return t;
}

auto Parse(const std::string& layout, const std::string& value) -> std::tuple<Time, error> {
  if (layout == RFC3339 || layout == RFC3339Nano) {
    if (auto [t, ok] = parseRFC3339(value, Local); ok) {
      return {t, nullptr};
    }
  }
  return parse(layout, value, UTC, Local);
}

auto ParseInLocation(
  const std::string& layout, const std::string& value, ptr<Location> loc) -> std::tuple<Time, error> {
  if (layout == RFC3339 || layout == RFC3339Nano) {
    if (auto [t, ok] = parseRFC3339(value, loc); ok) {
      return {t, nullptr};
    }
  }
  return parse(layout, value, loc, loc);
}

auto ParseDuration(const std::string& str) -> std::tuple<Duration, error> {
  auto orig = sv(str);
  auto s = orig;
  uint64_t d = 0;
  auto neg = false;

  if (!s.empty()) {
    auto c = s[0];
    if (c == '-' || c == '+') {
      neg = c == '-';
      s = s.substr(1);
    }
  }
  if (s == "0") {
    return {Duration{0}, nullptr};
  }
  if (s.empty()) {
    return {Duration{0}, durationError("invalid duration", orig)};
  }
  while (!s.empty()) {
    uint64_t v = 0;
    uint64_t f = 0;
    double scale = 1;

    if (!(s[0] == '.' || ('0' <= s[0] && s[0] <= '9'))) {
      return {Duration{0}, durationError("invalid duration", orig)};
    }
    auto pl = s.size();
    auto [lv, rest, ok] = leadingInt(s);
    if (!ok) {
      return {Duration{0}, durationError("invalid duration", orig)};
    }
    v = lv;
    s = rest;
    auto pre = pl != s.size();

    auto post = false;
    if (!s.empty() && s[0] == '.') {
      s = s.substr(1);
      auto pl2 = s.size();
      std::tie(f, scale, s) = leadingFraction(s);
      post = pl2 != s.size();
    }
    if (!pre && !post) {
      return {Duration{0}, durationError("invalid duration", orig)};
    }

    size_t i = 0;
    for (; i < s.size(); i++) {
      auto c = s[i];
      if (c == '.' || ('0' <= c && c <= '9')) {
        break;
      }
    }
    if (i == 0) {
      return {Duration{0}, durationError("missing unit in duration", orig)};
    }
    auto u = s.substr(0, i);
    s = s.substr(i);
    auto [unit, found] = unitOf(u);
    if (!found) {
      return {Duration{0}, durationError("unknown unit " + quote(u) + " in duration", orig)};
    }
    if (v > (uint64_t{1} << 63) / unit) {
      return {Duration{0}, durationError("invalid duration", orig)};
    }
    v *= unit;
    if (f > 0) {
      v += uint64_t(double(f) * (double(unit) / scale));
      if (v > uint64_t{1} << 63) {
        return {Duration{0}, durationError("invalid duration", orig)};
      }
    }
    d += v;
    if (d > uint64_t{1} << 63) {
      return {Duration{0}, durationError("invalid duration", orig)};
    }
  }
  if (neg) {
    return {-Duration{int64_t(d)}, nullptr};
  }
  if (d > uint64_t(INT64_MAX)) {
    return {Duration{0}, durationError("invalid duration", orig)};
  }
  return {Duration{int64_t(d)}, nullptr};
}

} // namespace eo::time
