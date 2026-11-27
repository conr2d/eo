// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/errors.h>

#include <eo/time/ticker.h>
#include <eo/time/timer.h>

#include <boost/asio/this_coro.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's time, in part: Duration, Month, Weekday, Time with its monotonic reading, Location with the
// time zones of the system's tzdata, Format, Parse, ParseDuration, and the marshalers, ported from
// Go 1.26's time.go, format.go, format_rfc3339.go, zoneinfo.go, zoneinfo_read.go, and
// zoneinfo_unix.go. time.cpp holds the code. Sleep takes a Duration, and NewTimer and NewTicker
// still take a std::chrono duration and send a std::chrono time point.

namespace eo::time {

// Go's time.Duration: the elapsed time between two instants as an int64 count of nanoseconds.
struct Duration : eo::defined<Duration, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "time.Duration";

  static auto String(Duration d) -> std::string;
  static auto Nanoseconds(Duration d) -> int64_t;
  static auto Microseconds(Duration d) -> int64_t;
  static auto Milliseconds(Duration d) -> int64_t;
  static auto Seconds(Duration d) -> double;
  static auto Minutes(Duration d) -> double;
  static auto Hours(Duration d) -> double;
  static auto Truncate(Duration d, Duration m) -> Duration;
  static auto Round(Duration d, Duration m) -> Duration;
  static auto Abs(Duration d) -> Duration;

  using _eo_methods = eo::methods<eo::method<&Duration::Abs, "Abs">,
    eo::method<&Duration::Hours, "Hours">,
    eo::method<&Duration::Microseconds, "Microseconds">,
    eo::method<&Duration::Milliseconds, "Milliseconds">,
    eo::method<&Duration::Minutes, "Minutes">,
    eo::method<&Duration::Nanoseconds, "Nanoseconds">,
    eo::method<&Duration::Round, "Round">,
    eo::method<&Duration::Seconds, "Seconds">,
    eo::method<&Duration::String, "String">,
    eo::method<&Duration::Truncate, "Truncate">>;
};

inline constexpr Duration Nanosecond = Duration{1};
inline constexpr Duration Microsecond = Duration{1000};
inline constexpr Duration Millisecond = Duration{1000000};
inline constexpr Duration Second = Duration{1000000000};
inline constexpr Duration Minute = Duration{60000000000};
inline constexpr Duration Hour = Duration{3600000000000};

// Go's time.Month, January being 1.
struct Month : eo::defined<Month, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "time.Month";

  static auto String(Month m) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Month::String, "String">>;
};

inline constexpr Month January = Month{1};
inline constexpr Month February = Month{2};
inline constexpr Month March = Month{3};
inline constexpr Month April = Month{4};
inline constexpr Month May = Month{5};
inline constexpr Month June = Month{6};
inline constexpr Month July = Month{7};
inline constexpr Month August = Month{8};
inline constexpr Month September = Month{9};
inline constexpr Month October = Month{10};
inline constexpr Month November = Month{11};
inline constexpr Month December = Month{12};

// Go's time.Weekday, Sunday being 0.
struct Weekday : eo::defined<Weekday, int64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "time.Weekday";

  static auto String(Weekday d) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Weekday::String, "String">>;
};

inline constexpr Weekday Sunday = Weekday{0};
inline constexpr Weekday Monday = Weekday{1};
inline constexpr Weekday Tuesday = Weekday{2};
inline constexpr Weekday Wednesday = Weekday{3};
inline constexpr Weekday Thursday = Weekday{4};
inline constexpr Weekday Friday = Weekday{5};
inline constexpr Weekday Saturday = Weekday{6};

inline const std::string Layout = "01/02 03:04:05PM '06 -0700"s;
inline const std::string ANSIC = "Mon Jan _2 15:04:05 2006"s;
inline const std::string UnixDate = "Mon Jan _2 15:04:05 MST 2006"s;
inline const std::string RubyDate = "Mon Jan 02 15:04:05 -0700 2006"s;
inline const std::string RFC822 = "02 Jan 06 15:04 MST"s;
inline const std::string RFC822Z = "02 Jan 06 15:04 -0700"s;
inline const std::string RFC850 = "Monday, 02-Jan-06 15:04:05 MST"s;
inline const std::string RFC1123 = "Mon, 02 Jan 2006 15:04:05 MST"s;
inline const std::string RFC1123Z = "Mon, 02 Jan 2006 15:04:05 -0700"s;
inline const std::string RFC3339 = "2006-01-02T15:04:05Z07:00"s;
inline const std::string RFC3339Nano = "2006-01-02T15:04:05.999999999Z07:00"s;
inline const std::string Kitchen = "3:04PM"s;
inline const std::string Stamp = "Jan _2 15:04:05"s;
inline const std::string StampMilli = "Jan _2 15:04:05.000"s;
inline const std::string StampMicro = "Jan _2 15:04:05.000000"s;
inline const std::string StampNano = "Jan _2 15:04:05.000000000"s;
inline const std::string DateTime = "2006-01-02 15:04:05"s;
inline const std::string DateOnly = "2006-01-02"s;
inline const std::string TimeOnly = "15:04:05"s;

namespace detail {

  // Go's time.zone: an abbreviated name, an offset in seconds east of UTC, and whether it is
  // daylight saving time.
  struct zone {
    static constexpr std::string_view _eo_name = "time.zone";

    std::string name{};
    int64_t offset{};
    bool isDST{};

    auto operator==(const zone&) const -> bool = default;
  };

  // Go's time.zoneTrans: a transition, in seconds since 1970, to the zone at index.
  struct zoneTrans {
    static constexpr std::string_view _eo_name = "time.zoneTrans";

    int64_t when{};
    uint8_t index{};
    bool isstd{};
    bool isutc{};

    auto operator==(const zoneTrans&) const -> bool = default;
  };

} // namespace detail

// Go's time.Location: the zones in use in a geographical area, with the transitions between them
// and the TZ string that extends them past the last transition. cacheZone points into zone, or to a
// zone of its own that the extension gave, as in Go.
struct Location {
  static constexpr std::string_view _eo_name = "time.Location";

  std::string name{};
  slice<detail::zone> zone{};
  slice<detail::zoneTrans> tx{};
  std::string extend{};
  int64_t cacheStart{};
  int64_t cacheEnd{};
  ptr<detail::zone> cacheZone{};

  static auto String(ptr<Location> l) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Location::String, "String">>;
};

namespace detail {

  inline Location utcLoc{.name = "UTC"};
  inline Location localLoc{};

} // namespace detail

// Go's time.UTC and time.Local, which point to the package's utcLoc and localLoc. Local is loaded
// from TZ or /etc/localtime when a Time first needs it, as in Go.
inline ptr<Location> UTC = ref(detail::utcLoc);
inline ptr<Location> Local = ref(detail::localLoc);

// Go's time.Time: an instant with nanosecond precision, the wall clock in wall and ext, a monotonic
// clock reading in ext when wall's top bit is set, and the Location, nil meaning UTC.
struct Time {
  static constexpr std::string_view _eo_name = "time.Time";

  uint64_t wall{};
  int64_t ext{};
  ptr<eo::time::Location> loc{};

  auto operator==(const Time&) const -> bool = default;

  static auto IsZero(Time t) -> bool;
  static auto After(Time t, Time u) -> bool;
  static auto Before(Time t, Time u) -> bool;
  static auto Compare(Time t, Time u) -> int64_t;
  static auto Equal(Time t, Time u) -> bool;
  static auto Date(Time t) -> std::tuple<int64_t, eo::time::Month, int64_t>;
  static auto Year(Time t) -> int64_t;
  static auto Month(Time t) -> eo::time::Month;
  static auto Day(Time t) -> int64_t;
  static auto Weekday(Time t) -> eo::time::Weekday;
  static auto ISOWeek(Time t) -> std::tuple<int64_t, int64_t>;
  static auto Clock(Time t) -> std::tuple<int64_t, int64_t, int64_t>;
  static auto Hour(Time t) -> int64_t;
  static auto Minute(Time t) -> int64_t;
  static auto Second(Time t) -> int64_t;
  static auto Nanosecond(Time t) -> int64_t;
  static auto YearDay(Time t) -> int64_t;
  static auto Add(Time t, Duration d) -> Time;
  static auto Sub(Time t, Time u) -> Duration;
  static auto AddDate(Time t, int64_t years, int64_t months, int64_t days) -> Time;
  static auto UTC(Time t) -> Time;
  static auto Local(Time t) -> Time;
  static auto In(Time t, ptr<eo::time::Location> loc) -> Time;
  static auto Location(Time t) -> ptr<eo::time::Location>;
  static auto Zone(Time t) -> std::tuple<std::string, int64_t>;
  static auto ZoneBounds(Time t) -> std::tuple<Time, Time>;
  static auto Unix(Time t) -> int64_t;
  static auto UnixMilli(Time t) -> int64_t;
  static auto UnixMicro(Time t) -> int64_t;
  static auto UnixNano(Time t) -> int64_t;
  static auto AppendBinary(Time t, slice<uint8_t> b) -> std::tuple<slice<uint8_t>, error>;
  static auto MarshalBinary(Time t) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalBinary(ptr<Time> t, slice<uint8_t> data) -> error;
  static auto GobEncode(Time t) -> std::tuple<slice<uint8_t>, error>;
  static auto GobDecode(ptr<Time> t, slice<uint8_t> data) -> error;
  static auto MarshalJSON(Time t) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalJSON(ptr<Time> t, slice<uint8_t> data) -> error;
  static auto AppendText(Time t, slice<uint8_t> b) -> std::tuple<slice<uint8_t>, error>;
  static auto MarshalText(Time t) -> std::tuple<slice<uint8_t>, error>;
  static auto UnmarshalText(ptr<Time> t, slice<uint8_t> data) -> error;
  static auto IsDST(Time t) -> bool;
  static auto Truncate(Time t, Duration d) -> Time;
  static auto Round(Time t, Duration d) -> Time;
  static auto String(Time t) -> std::string;
  static auto GoString(Time t) -> std::string;
  static auto Format(Time t, const std::string& layout) -> std::string;
  static auto AppendFormat(Time t, slice<uint8_t> b, const std::string& layout) -> slice<uint8_t>;

  using _eo_methods = eo::methods<eo::method<&Time::Add, "Add">,
    eo::method<&Time::AddDate, "AddDate">,
    eo::method<&Time::After, "After">,
    eo::method<&Time::AppendBinary, "AppendBinary">,
    eo::method<&Time::AppendFormat, "AppendFormat">,
    eo::method<&Time::AppendText, "AppendText">,
    eo::method<&Time::Before, "Before">,
    eo::method<&Time::Clock, "Clock">,
    eo::method<&Time::Compare, "Compare">,
    eo::method<&Time::Date, "Date">,
    eo::method<&Time::Day, "Day">,
    eo::method<&Time::Equal, "Equal">,
    eo::method<&Time::Format, "Format">,
    eo::method<&Time::GoString, "GoString">,
    eo::method<&Time::GobDecode, "GobDecode">,
    eo::method<&Time::GobEncode, "GobEncode">,
    eo::method<&Time::Hour, "Hour">,
    eo::method<&Time::ISOWeek, "ISOWeek">,
    eo::method<&Time::In, "In">,
    eo::method<&Time::IsDST, "IsDST">,
    eo::method<&Time::IsZero, "IsZero">,
    eo::method<&Time::Local, "Local">,
    eo::method<&Time::Location, "Location">,
    eo::method<&Time::MarshalBinary, "MarshalBinary">,
    eo::method<&Time::MarshalJSON, "MarshalJSON">,
    eo::method<&Time::MarshalText, "MarshalText">,
    eo::method<&Time::Minute, "Minute">,
    eo::method<&Time::Month, "Month">,
    eo::method<&Time::Nanosecond, "Nanosecond">,
    eo::method<&Time::Round, "Round">,
    eo::method<&Time::Second, "Second">,
    eo::method<&Time::String, "String">,
    eo::method<&Time::Sub, "Sub">,
    eo::method<&Time::Truncate, "Truncate">,
    eo::method<&Time::UTC, "UTC">,
    eo::method<&Time::Unix, "Unix">,
    eo::method<&Time::UnixMicro, "UnixMicro">,
    eo::method<&Time::UnixMilli, "UnixMilli">,
    eo::method<&Time::UnixNano, "UnixNano">,
    eo::method<&Time::UnmarshalBinary, "UnmarshalBinary">,
    eo::method<&Time::UnmarshalJSON, "UnmarshalJSON">,
    eo::method<&Time::UnmarshalText, "UnmarshalText">,
    eo::method<&Time::Weekday, "Weekday">,
    eo::method<&Time::Year, "Year">,
    eo::method<&Time::YearDay, "YearDay">,
    eo::method<&Time::Zone, "Zone">,
    eo::method<&Time::ZoneBounds, "ZoneBounds">>;
};

// Go's time.ParseError, which Parse returns through error as a *ParseError.
struct ParseError {
  static constexpr std::string_view _eo_name = "time.ParseError";

  std::string Layout{};
  std::string Value{};
  std::string LayoutElem{};
  std::string ValueElem{};
  std::string Message{};

  auto operator==(const ParseError&) const -> bool = default;

  static auto Error(ptr<ParseError> e) -> std::string;

  using _eo_methods = eo::methods<eo::method<&ParseError::Error, "Error">>;
};

auto Now() -> Time;
auto Since(Time t) -> Duration;
auto Until(Time t) -> Duration;
auto Unix(int64_t sec, int64_t nsec) -> Time;
auto UnixMilli(int64_t msec) -> Time;
auto UnixMicro(int64_t usec) -> Time;
// Date panics with Go's message when loc is nil.
auto Date(
  int64_t year, Month month, int64_t day, int64_t hour, int64_t min, int64_t sec, int64_t nsec, ptr<Location> loc)
  -> Time;
auto Parse(const std::string& layout, const std::string& value) -> std::tuple<Time, error>;
auto ParseInLocation(const std::string& layout, const std::string& value, ptr<Location> loc) -> std::tuple<Time, error>;
auto ParseDuration(const std::string& s) -> std::tuple<Duration, error>;
auto FixedZone(const std::string& name, int64_t offset) -> ptr<Location>;
// LoadLocation looks in the directory that ZONEINFO names, then in the system's tzdata, as Go does
// on Unix. Go's last resorts, the tzdata that a program embeds with time/tzdata and the
// zoneinfo.zip of GOROOT, are not searched.
auto LoadLocation(const std::string& name) -> std::tuple<ptr<Location>, error>;
auto LoadLocationFromTZData(const std::string& name, slice<uint8_t> data) -> std::tuple<ptr<Location>, error>;

// Go's time.Sleep: suspends the goroutine for at least d. A d of zero or less returns at once, as
// in Go.
inline func<> Sleep(Duration d) {
  using namespace boost::asio;
  if (int64_t(d) <= 0) {
    co_return;
  }
  co_await steady_timer((co_await this_coro::executor), std::chrono::nanoseconds(int64_t(d))).async_wait(use_awaitable);
}

// Sleep for a std::chrono duration, for C++ that is not translated from Go.
template<class Rep, class Period>
func<> Sleep(const std::chrono::duration<Rep, Period>& d) {
  using namespace boost::asio;
  co_await steady_timer((co_await this_coro::executor), d).async_wait(use_awaitable);
}

} // namespace eo::time
