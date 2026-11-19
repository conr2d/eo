package main

import "fmt"

type Point struct {
	X, Y int
}

func add(m map[string]int, k string) {
	m[k]++
}

func main() {
	ages := map[string]int{"bob": 31, "alice": 29}
	alias := ages
	alias["carol"] = 40
	add(ages, "bob")
	fmt.Println(ages, len(ages), len(alias))

	fmt.Println(ages["dave"], len(ages))
	v, ok := ages["alice"]
	fmt.Println(v, ok)
	v, ok = ages["dave"]
	fmt.Println(v, ok)
	if _, ok := ages["carol"]; ok {
		fmt.Println("carol present")
	}

	delete(ages, "bob")
	delete(ages, "nobody")
	fmt.Println(ages, len(ages))

	counts := make(map[rune]int)
	for _, r := range "hello" {
		counts[r]++
	}
	counts['z'] += 2
	fmt.Println(counts)

	sum := 0
	for k, v := range counts {
		sum += int(k) * v
	}
	fmt.Println(sum)

	var nilMap map[string]int
	fmt.Println(nilMap == nil, len(nilMap), nilMap["x"], nilMap)
	for range nilMap {
		fmt.Println("unreachable")
	}
	delete(nilMap, "x")

	grid := map[Point]string{{1, 2}: "a", {0, 5}: "b"}
	grid[Point{1, 2}] = "c"
	fmt.Println(grid, grid[Point{0, 5}], len(grid))

	groups := map[string][]int{}
	groups["even"] = append(groups["even"], 2, 4)
	groups["odd"] = append(groups["odd"], 1)
	fmt.Println(groups)

	nested := map[string]map[string]int{"x": {"y": 1}}
	nested["x"]["z"] = 2
	fmt.Println(nested, nested["w"]["y"], len(nested))

	m := map[int]bool{3: true, -1: false, 2: true}
	m[len(m)] = false
	fmt.Println(m)
	fmt.Printf("%v|%d|%T|%T\n", m, map[string]int{"k": 7}, m, groups)

	clear(ages)
	fmt.Println(ages, len(ages), ages == nil, len(alias))

	q := map[string]int{"x": 1}
	p := &q
	(*p)["y"] = 2
	fmt.Println(p, len(q))
}
