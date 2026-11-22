package main

import "fmt"

type Hash [4]byte

// Bytes returns a slice of its receiver, a copy of the caller's Hash, so the receiver is moved to heap.
func (h Hash) Bytes() []byte { return h[:] }

func (h Hash) String() string { return fmt.Sprintf("0x%x", h[:]) }

// SetBytes keeps the last bytes of b, as go-ethereum's common.Hash does.
func (h *Hash) SetBytes(b []byte) {
	if len(b) > len(h) {
		b = b[len(b)-len(h):]
	}
	copy(h[len(h)-len(b):], b)
}

type Pair [2]int

type Header struct {
	Parent Hash
	Number uint64
}

func main() {
	var h Hash
	h.SetBytes([]byte{1, 2})
	b := h.Bytes()
	b[0] = 9
	fmt.Println(h, b, h[3], len(h))
	h.SetBytes([]byte{5, 6, 7, 8, 9, 10})
	fmt.Println(h, [4]byte(h), Hash([4]byte{1}))
	fmt.Printf("%v %s %d %x %T\n", h, h, h, h, h)

	k := Hash{0xaa, 0xbb}
	idx := map[Hash]int{k: 1}
	idx[Hash{0xaa, 0xbb}]++
	_, found := idx[Hash{}]
	fmt.Println(idx, found)

	hdr := Header{Parent: k, Number: 7}
	child := Header{Parent: hdr.Parent, Number: hdr.Number + 1}
	fmt.Println(hdr, child, hdr.Parent == child.Parent, hdr == child)
	fmt.Printf("%v %x\n", child, child)

	pr := Pair{3, 4}
	pr[0] += 10
	fmt.Println(pr, pr == Pair{13, 4}, Pair([2]int{1, 2}))
	fmt.Printf("%v %d %T\n", pr, &pr, pr)
	for i, v := range pr {
		fmt.Printf("%d=%d ", i, v)
	}
	fmt.Println()
	s := []byte{1, 2, 3, 4, 5}
	fmt.Println(Hash(s))
	var x any = pr
	_, isArr := x.([2]int)
	fmt.Println(x.(Pair)[1], isArr)
}
