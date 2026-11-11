package main

import "fmt"

func describe(s string) string {
	return s[:1] + "…" + s[len(s)-1:]
}

func main() {
	s := "héllo, 世界"
	fmt.Println(len(s), s[0], s[1], s[2])
	fmt.Println(s[1:3], s[8:], s[:0] == "")
	t := s
	s += "!"
	fmt.Println(t, s, t < s, len(t), len(s))
	fmt.Println(describe(s), describe("ab"))
	nul := "a\x00b\xffc"
	fmt.Println(len(nul), nul[1], nul[3], nul[2:3])
	raw := `C:\path\n` + "\t|"
	fmt.Println(raw, len(raw))
	fmt.Println(string(rune(0x4e16)), string(rune(-1)), string(rune(0xd800)) == "\uFFFD", string(rune(0x1F600)))
	fmt.Println("\\x41", len("\\x41"))
	fmt.Println("\xff" > "a", "ab" < "b", "é" == "\u00e9", "\377" == "\xff")
}
