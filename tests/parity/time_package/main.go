package main

// Checks package time against Go: the string inputs of Go's own format_test.go and time_test.go
// tables (tables.txt, extracted from Go 1.26), then thousands of instants, layouts, values, and
// durations drawn from a fixed generator, in the time zones of zoneinfo, copies of tzdata 2026f's
// files, so that the output does not depend on the system's tzdata. Local is the system's
// America/New_York, set through TZ before its first use.

import (
	"embed"
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

//go:embed tables.txt
var tables string

//go:embed zoneinfo
var zoneinfo embed.FS

var seed uint64 = 7

// rounds is the number of random instants and durations.
const rounds = 100

// next is a 64-bit linear congruential generator, so that both sides draw the same values.
func next() uint64 {
	seed = seed*6364136223846793005 + 1442695040888963407
	return seed
}

// pick returns a number below m from the high bits, whose period is long.
func pick(m int) int {
	return int((next() >> 32) % uint64(m))
}

// table returns the fields of each line of the named section of tables.txt.
func table(name string) [][]string {
	var out [][]string
	in := false
	for _, line := range strings.Split(tables, "\n") {
		if strings.HasPrefix(line, "== ") {
			in = line == "== "+name
			continue
		}
		if !in || line == "" {
			continue
		}
		var fields []string
		for _, f := range strings.Split(line, "\t") {
			s, err := strconv.Unquote(f)
			if err != nil {
				panic(err)
			}
			fields = append(fields, s)
		}
		out = append(out, fields)
	}
	if len(out) == 0 {
		panic("empty table " + name)
	}
	return out
}

var zones []*time.Location

func load(name string) *time.Location {
	data, err := zoneinfo.ReadFile("zoneinfo/" + name)
	if err != nil {
		panic(err)
	}
	loc, err := time.LoadLocationFromTZData(name, data)
	if err != nil {
		panic(err)
	}
	return loc
}

func show(label string, t time.Time, err error) {
	if err != nil {
		fmt.Printf("%s: error %v\n", label, err)
		return
	}
	name, offset := t.Zone()
	fmt.Printf("%s: %v [%s %d %d %d]\n", label, t, name, offset, t.Unix(), t.Nanosecond())
}

func parsing() {
	fmt.Println("-- parse tables")
	nyc := load("America/New_York")
	for _, name := range []string{"parseTests", "rubyTests"} {
		for _, f := range table(name) {
			t, err := time.Parse(f[1], f[2])
			show(f[1]+" | "+f[2], t, err)
			t, err = time.ParseInLocation(f[1], f[2], nyc)
			show("  in NYC", t, err)
		}
	}
	for _, f := range table("parseErrorTests") {
		t, err := time.Parse(f[0], f[1])
		show(f[0]+" | "+f[1], t, err)
	}
	for _, f := range table("secondsTimeZoneOffsetTests") {
		t, err := time.Parse(f[0], f[1])
		show(f[0]+" | "+f[1], t, err)
	}
	layouts := []string{time.ANSIC, time.RFC3339, time.RFC3339Nano, "01-02", "2006-01-02", "Jan _2", "MST", "Z07:00", "-07"}
	for _, name := range []string{"dayOutOfRangeTests", "monthOutOfRangeTests", "longFractionalDigitsTests", "parseTimeZoneTests"} {
		for _, f := range table(name) {
			for _, layout := range layouts {
				t, err := time.Parse(layout, f[0])
				show(layout+" | "+f[0], t, err)
			}
		}
	}
	for _, f := range table("jsonTests") {
		var t time.Time
		err := t.UnmarshalJSON([]byte(f[0]))
		show("json "+f[0], t, err)
		b, err := t.MarshalJSON()
		fmt.Printf("  %s %v\n", b, err)
	}
}

func formatting() {
	fmt.Println("-- format tables")
	t := time.Unix(0, 1233810057012345600)
	var layouts []string
	for _, f := range table("formatTests") {
		layouts = append(layouts, f[1])
	}
	for _, f := range table("nextStdChunkTests") {
		layouts = append(layouts, strings.NewReplacer("(", "", ")", "").Replace(f[0]))
	}
	layouts = append(layouts, time.Layout, time.RFC822, time.RFC822Z, "Z070000 Z07:00:00 Z07 -070000 -07:00:00 -07 MST",
		".0 .00 .999 ,000 ,999999999 .0000000000 .9999999999", "January Monday Janet Mondays", "")
	for _, loc := range zones {
		u := t.In(loc)
		for _, layout := range layouts {
			fmt.Printf("%s %q: %q\n", loc, layout, u.Format(layout))
		}
		fmt.Printf("%s: %v %s\n", loc, u, u.GoString())
	}
}

var pieces = []string{
	"2006", "06", "01", "1", "Jan", "January", "02", "2", "_2", "__2", "002", "Mon", "Monday", "15", "3", "03",
	"04", "4", "05", "5", "PM", "pm", "MST", "Z0700", "Z070000", "Z07", "Z07:00", "Z07:00:00", "-0700",
	"-070000", "-07", "-07:00", "-07:00:00", ".0", ".000", ".999", ".999999999", ",000", ",9", " ", "-", ":",
	"T", "/", "x", "Ja", "Mo", "_", "0", ".", ",", "J", "M", "P",
}

const mutations = "0123456789 :-+.,ZTJMPajmp"

func randomLayout() string {
	n := 1 + pick(8)
	s := ""
	for i := 0; i < n; i++ {
		s += pieces[pick(len(pieces))]
	}
	return s
}

func randomTime() time.Time {
	var sec int64
	switch pick(4) {
	case 0:
		sec = int64(next()>>34) - 1<<29
	case 1:
		sec = int64(next()>>28) - 1<<35
	case 2:
		sec = int64(next() >> 1)
		if next()&1 == 0 {
			sec = -sec
		}
	default:
		sec = 1700000000 + int64(pick(1000000000)) - 500000000
	}
	nsec := int64(pick(1000000000))
	if pick(3) == 0 {
		nsec = int64(pick(1000)) * 1000000
	}
	return time.Unix(sec, nsec).In(zones[pick(len(zones))])
}

func instants() {
	fmt.Println("-- instants")
	for i := 0; i < rounds; i++ {
		t := randomTime()
		y, m, d := t.Date()
		h, mi, s := t.Clock()
		iy, iw := t.ISOWeek()
		name, offset := t.Zone()
		start, end := t.ZoneBounds()
		fmt.Printf("%d: %v | %d %v %d %d:%d:%d %d %v %d %d-%d %s %d %v %v %v\n", i, t, y, m, d, h, mi, s, t.YearDay(),
			t.Weekday(), t.Nanosecond(), iy, iw, name, offset, t.IsDST(), start, end)
		fmt.Printf("  %d %d %d %d %d %d %d %v %v %v\n", t.Year(), t.Month(), t.Day(), t.Hour(), t.Minute(), t.Second(),
			t.UnixMilli(), t.UnixMicro(), t.UnixNano(), t.UTC())
		layout := randomLayout()
		s1 := t.Format(layout)
		fmt.Printf("  %q %q %q %q\n", layout, s1, t.Format(time.RFC3339Nano), t.GoString())
		p, err := time.ParseInLocation(layout, s1, t.Location())
		show("  parse", p, err)
		p, err = time.Parse(layout, s1)
		show("  parse UTC", p, err)
		b := []byte(s1)
		if len(b) > 0 {
			b[pick(len(b))] = mutations[pick(len(mutations))]
		}
		p, err = time.Parse(layout, string(b))
		show("  mutated "+strconv.Quote(string(b)), p, err)
		p, err = time.Parse(time.RFC3339, t.Format(time.RFC3339))
		show("  rfc3339", p, err)
		d1 := time.Duration(int64(next()))
		if pick(2) == 0 {
			d1 = time.Duration(int64(next() >> uint(pick(64))))
		}
		a := t.Add(d1)
		fmt.Printf("  add %v: %v sub %v\n", d1, a, a.Sub(t))
		fmt.Printf("  adddate %v\n", t.AddDate(pick(41)-20, pick(301)-150, pick(2001)-1000))
		r := time.Duration(1 + pick(1000000))
		switch pick(4) {
		case 0:
			r *= time.Second
		case 1:
			r = time.Duration(int64(next() >> uint(1+pick(63))))
		}
		fmt.Printf("  round %v: %v %v\n", r, t.Round(r), t.Truncate(r))
		u := randomTime()
		fmt.Printf("  cmp %v %v %v %d\n", t.Before(u), t.After(u), t.Equal(t.In(time.UTC)), t.Compare(u))
		j, err := t.MarshalJSON()
		fmt.Printf("  json %s %v\n", j, err)
		x, err := t.MarshalText()
		fmt.Printf("  text %s %v\n", x, err)
		bin, err := t.MarshalBinary()
		fmt.Printf("  binary %x %v\n", bin, err)
		var back time.Time
		if err == nil {
			err = back.UnmarshalBinary(bin)
			fmt.Printf("  unbinary %v %v %v\n", back, back.Equal(t), err)
		}
		if j != nil {
			err = back.UnmarshalJSON(j)
			fmt.Printf("  unjson %v %v %v\n", back, back.Equal(t), err)
		}
	}
}

func durations() {
	fmt.Println("-- durations")
	for _, name := range []string{"parseDurationTests", "parseDurationErrorTests"} {
		for _, f := range table(name) {
			d, err := time.ParseDuration(f[0])
			fmt.Printf("%q: %d %v %v\n", f[0], int64(d), d, err)
		}
	}
	units := []string{"ns", "us", "µs", "μs", "ms", "s", "m", "h", "x", "", "S", "µ"}
	for i := 0; i < rounds; i++ {
		d := time.Duration(int64(next() >> uint(pick(64))))
		if pick(2) == 0 {
			d = -d
		}
		m := time.Duration(int64(next() >> uint(pick(64))))
		if pick(5) == 0 {
			m = -m
		}
		fmt.Printf("%d %v %d %d %d %v %v %v | %v %v %v\n", int64(d), d, d.Nanoseconds(), d.Microseconds(), d.Milliseconds(),
			d.Seconds(), d.Minutes(), d.Hours(), d.Truncate(m), d.Round(m), d.Abs())
		back, err := time.ParseDuration(d.String())
		fmt.Printf("  %v %v\n", back == d, err)
		s := ""
		for k := 1 + pick(3); k > 0; k-- {
			if pick(4) == 0 {
				s += "-"
			}
			s += strconv.FormatUint(next()>>uint(pick(64)), 10)
			if pick(3) == 0 {
				s += "." + strconv.FormatUint(next()>>uint(pick(64)), 10)
			}
			s += units[pick(len(units))]
		}
		p, err := time.ParseDuration(s)
		fmt.Printf("  %q %d %v\n", s, int64(p), err)
	}
	fmt.Println(time.Duration(-1<<63), time.Duration(1<<63-1), time.Duration(-1<<63).Abs(), time.Duration(-1<<63).Round(3))
	fmt.Println(time.Month(13), time.Month(0), time.Month(-1), time.Weekday(7), time.Weekday(-1), time.March, time.Saturday)
}

func locations() {
	fmt.Println("-- locations")
	for _, name := range []string{"", "UTC", "Local", "America/New_York", "../etc/passwd", "/etc/localtime", "\\x",
		"Nowhere/Zone", "America", "Etc/GMT+5", "Asia/Kolkata", "posixrules", "Factory"} {
		loc, err := time.LoadLocation(name)
		if err != nil {
			fmt.Printf("%q: error %v\n", name, err)
			continue
		}
		fmt.Printf("%q: %v %v %v\n", name, loc, loc == time.UTC, loc == time.Local)
	}
	for _, data := range []string{"", "TZif", "TZif2", "TZif\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", "nope"} {
		_, err := time.LoadLocationFromTZData("x", []byte(data))
		fmt.Printf("%q: %v\n", data, err)
	}
	fmt.Println(time.FixedZone("", 3600) == time.FixedZone("", 3600), time.FixedZone("", 3601) == time.FixedZone("", 3601),
		time.FixedZone("A", 3600) == time.FixedZone("A", 3600), time.FixedZone("", 15*3600) == time.FixedZone("", 15*3600))
	fmt.Println(time.UTC, time.Local, time.FixedZone("", -1800), time.FixedZone("X\x01\"", 0).String())
	t := time.Date(2009, time.February, 5, 5, 0, 57, 12345600, time.FixedZone("Non-ASCII character ⏰", 3*60*60))
	fmt.Println(t.GoString(), time.Date(2009, time.February, 5, 5, 0, 57, 12345600, time.Local).GoString())
	ny := load("America/New_York")
	for _, s := range []string{"2021-03-14 01:59:59", "2021-03-14 02:00:00", "2021-03-14 02:30:00", "2021-03-14 03:00:00",
		"2021-11-07 00:59:59", "2021-11-07 01:00:00", "2021-11-07 01:30:00", "2021-11-07 02:00:00"} {
		u, err := time.ParseInLocation(time.DateTime, s, ny)
		show(s, u, err)
		show("  local", u.Local(), nil)
	}
	for _, f := range [][]int{{2016, 12, 31, 23, 59, 60}, {2000, 2, 30, 0, 0, 0}, {2000, 0, 0, 0, 0, 0}, {-1, 13, 32, 25, 61, 61},
		{1, 1, 1, 0, 0, 0}, {0, 1, 1, 0, 0, 0}, {292277026596, 12, 4, 15, 30, 7}, {-292277022399, 1, 1, 0, 0, 0}} {
		for _, loc := range zones {
			show(fmt.Sprint(f, loc), time.Date(f[0], time.Month(f[1]), f[2], f[3], f[4], f[5], -1, loc), nil)
		}
	}
	var zero time.Time
	fmt.Println(zero, zero.IsZero(), zero.Location(), zero.GoString(), zero.Unix(), zero.Weekday(), zero.YearDay())
	bad := []string{"", "\x03", "\x01\x00", "\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		"\x01\x00\x00\x00\x0e\xd5\x0b\xa0\xe8\x00\x00\x00\x00\xff\xff"}
	for _, b := range bad {
		var u time.Time
		err := u.UnmarshalBinary([]byte(b))
		fmt.Printf("%q: %v %v\n", b, u, err)
	}
	for _, j := range []string{"null", "\"\"", "1", "\"2006-01-02T15:04:05Z\"", "\"2006-01-02T15:04:05,5Z\"",
		"\"2006-01-02T1:04:05Z\"", "\"2006-01-02T15:04:05+24:00\"", "\"2006-01-02T15:04:05+23:60\""} {
		var u time.Time
		err := u.UnmarshalJSON([]byte(j))
		fmt.Printf("%s: %v %v\n", j, u, err)
	}
	for _, u := range []time.Time{time.Date(10000, 1, 1, 0, 0, 0, 0, time.UTC), time.Date(-1, 1, 1, 0, 0, 0, 0, time.UTC),
		time.Date(2000, 1, 1, 0, 0, 0, 0, time.FixedZone("", 24*3600)), time.Date(2000, 1, 1, 0, 0, 0, 0, time.FixedZone("", -1))} {
		j, err := u.MarshalJSON()
		x, err2 := u.MarshalText()
		b, err3 := u.MarshalBinary()
		a, err4 := u.AppendText([]byte("x"))
		fmt.Printf("%v: %q %v %q %v %x %v %q %v\n", u, j, err, x, err2, b, err3, a, err4)
	}
}

type event struct {
	At    time.Time
	D     time.Duration
	P     *time.Time
	Month time.Month
}

func marshaling() {
	fmt.Println("-- encoding")
	at := time.Date(2024, time.February, 29, 23, 59, 59, 123456789, load("Asia/Kolkata"))
	b, err := json.Marshal(event{At: at, D: 90 * time.Minute, P: &at, Month: time.May})
	fmt.Println(string(b), err)
	var e event
	err = json.Unmarshal(b, &e)
	fmt.Println(e.At, e.D, *e.P, e.Month, err, e.At.Equal(at))
	err = json.Unmarshal([]byte(`{"At":"2024-13-01T00:00:00Z"}`), &e)
	fmt.Println(err)
	b, err = json.Marshal(map[string]time.Time{"z": {}, "a": time.Unix(0, 1).UTC()})
	fmt.Println(string(b), err)
	_, err = json.Marshal(time.Date(10000, time.January, 1, 0, 0, 0, 0, time.UTC))
	fmt.Println(err)
	fmt.Printf("%v|%+v|%#v|%s|%d|%v|%#v|%v|%v\n", at, at, at, at.Location(), time.March, time.Duration(1500), time.Duration(7), time.Sunday, time.Month(5))
	var zero time.Time
	fmt.Printf("%v|%#v|%v\n", zero, zero, zero.Location())
}

// tzif returns a TZif file of version 2 with one zone and no transitions, whose footer, a POSIX TZ
// string, gives every offset.
func tzif(footer string) []byte {
	var b []byte
	for i := 0; i < 2; i++ {
		b = append(b, "TZif2"...)
		b = append(b, make([]byte, 15)...)
		for _, n := range []uint32{0, 0, 0, 0, 1, 4} {
			b = append(b, byte(n>>24), byte(n>>16), byte(n>>8), byte(n))
		}
		b = append(b, 0, 0, 0, 0, 0, 0)
		b = append(b, "LMT\x00"...)
	}
	b = append(b, '\n')
	b = append(b, footer...)
	b = append(b, '\n')
	return b
}

func rules() {
	fmt.Println("-- rules")
	footers := []string{"EST5EDT,M3.2.0,M11.1.0", "EST5EDT", "<+0330>-3:30", "XST-1XDT,J60/2,J300/3", "AAA3BBB,0/0,365/25",
		"CET-1CEST,M3.5.0,M10.5.0/3", "<-03>3<-02>,M3.5.0/-2,M10.5.0/-1", "IST-2IDT,M3.4.4/26,M10.5.0", "NZST-12NZDT,M9.5.0,M4.1.0/3",
		"AEST-10AEDT-11,M10.1.0,M4.1.0/3", "XXX-14", "ABC+167:59:59DEF,J1/0,J365/24", "AB1", "ABC", "ABC1DEF,M13.1.0,M1.1.0",
		"ABC1DEF,M3.6.0,M10.1.0", "ABC1DEF;J60,J61", "ABC1DEF,J0,J10", "<ABC1", "ABC25:00", "ABC1DEF,J59,J60/-167:59:59"}
	years := []int{1900, 2000, 2023, 2024, 2100, -4}
	for _, f := range footers {
		loc, err := time.LoadLocationFromTZData("rule", tzif(f))
		if err != nil {
			fmt.Printf("%q: %v\n", f, err)
			continue
		}
		for _, y := range years {
			for m := 1; m <= 12; m += 3 {
				t := time.Date(y, time.Month(m), 1+pick(28), pick(24), pick(60), 0, 0, time.UTC).In(loc)
				start, end := t.ZoneBounds()
				fmt.Printf("%q %v %v %v %v\n", f, t, t.IsDST(), start, end)
			}
			show(fmt.Sprint(f, y), time.Date(y, time.March, 1, 2, 30, 0, 0, loc), nil)
		}
	}
	for _, v := range []string{"1900-02-29", "2000-02-29", "2100-02-29", "2400-02-29", "1600-02-29", "2004-02-29"} {
		t, err := time.Parse(time.DateOnly, v)
		show(v, t, err)
	}
	for _, v := range []string{"1900-060", "2000-060", "2000-366", "1900-366", "2100-365", "2100-059", "2400-060"} {
		t, err := time.Parse("2006-002", v)
		show(v, t, err)
	}
	for _, y := range []int{1900, 2000, 2100, 2400, 1700, 1600} {
		t := time.Date(y, time.February, 29, 0, 0, 0, 0, time.UTC)
		fmt.Println(t, t.YearDay(), t.AddDate(0, 0, 1), time.Date(y, time.December, 31, 0, 0, 0, 0, time.UTC).YearDay())
	}
}

func monotonic() {
	fmt.Println("-- monotonic")
	t := time.Now()
	u := t.Add(time.Hour)
	fmt.Println(u.Sub(t), t.Sub(u), strings.Contains(t.String(), " m=+"), strings.Contains(t.Round(0).String(), " m="),
		strings.Contains(t.UTC().String(), " m="), strings.Contains(t.Add(time.Second).String(), " m=+"))
	fmt.Println(time.Since(t) >= 0, time.Until(u) > 0, time.Until(u) <= time.Hour, t.Equal(t.Round(0)), t == t.Round(0),
		t.Location(), t.Before(u), u.After(t), t.Compare(u))
	fmt.Println(t.Year() >= 2024, t.Truncate(time.Second).Nanosecond(), t.Round(0).Equal(t))
	before := time.Now()
	time.Sleep(2 * time.Millisecond)
	time.Sleep(0)
	time.Sleep(-time.Second)
	fmt.Println(time.Since(before) >= 2*time.Millisecond)
}

func main() {
	if err := os.Setenv("TZ", "America/New_York"); err != nil {
		panic(err)
	}
	for _, name := range []string{"America/New_York", "Europe/London", "Asia/Kolkata", "Australia/Lord_Howe",
		"Pacific/Chatham", "America/Sao_Paulo", "Africa/Casablanca", "Antarctica/Troll", "Europe/Dublin",
		"America/St_Johns", "Asia/Tehran", "Pacific/Kiritimati", "Europe/Moscow", "America/Nuuk"} {
		zones = append(zones, load(name))
	}
	zones = append(zones, time.UTC, time.Local, time.FixedZone("", 0), time.FixedZone("ABC", 5*3600+30*60),
		time.FixedZone("", -12345), time.FixedZone("", 14*3600), time.FixedZone("Z", -1))
	parsing()
	formatting()
	instants()
	durations()
	locations()
	rules()
	marshaling()
	monotonic()
}
