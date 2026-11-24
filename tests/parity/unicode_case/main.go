package main

import (
	"fmt"
	"strings"
	"unicode"
)

// An FNV-1a hash of every code point's mappings, so that both programs check all of them.
var h uint64 = 14695981039346656037

func mix(v uint64) {
	h ^= v
	h *= 1099511628211
}

func main() {
	spaces := 0
	folds := 0
	for r := rune(-2); r <= unicode.MaxRune+2; r++ {
		mix(uint64(unicode.ToUpper(r)))
		mix(uint64(unicode.ToLower(r)))
		mix(uint64(unicode.ToTitle(r)))
		f := unicode.SimpleFold(r)
		mix(uint64(f))
		if f != r {
			folds++
		}
		if unicode.IsSpace(r) {
			spaces++
			mix(1)
		}
	}
	fmt.Println(h, spaces, folds)

	// Strings of every 97th code point, through strings.ToUpper, ToLower, and EqualFold.
	var b strings.Builder
	for r := rune(0); r <= unicode.MaxRune; r += 97 {
		b.WriteRune(r)
	}
	s := b.String()
	upper := strings.ToUpper(s)
	lower := strings.ToLower(s)
	h = 14695981039346656037
	for i := 0; i < len(upper); i++ {
		mix(uint64(upper[i]))
	}
	for i := 0; i < len(lower); i++ {
		mix(uint64(lower[i]))
	}
	fmt.Println(len(s), len(upper), len(lower), h, strings.EqualFold(s, upper), strings.EqualFold(upper, lower))

	// Code points with interesting orbits and mappings.
	for _, r := range []rune{'k', 'K', 0x212A, 's', 0x17F, 0xDF, 0x1E9E, 0x1C5, 0x3A3, 0x3C2, 0x130, 0x131, 0x10400, -1, 0x110000} {
		fmt.Printf("%U %U %U %U %U %v | ", r, unicode.ToUpper(r), unicode.ToLower(r), unicode.ToTitle(r), unicode.SimpleFold(r), unicode.IsSpace(r))
	}
	fmt.Println()
	fmt.Println(strings.EqualFold("Straße", "STRASSE"), strings.EqualFold("k", "\u212a"), strings.EqualFold("ǅ", "ǆ"), strings.EqualFold("σ", "ς"), strings.EqualFold("Σ", "ς"))
}
