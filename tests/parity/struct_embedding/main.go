package main

import "fmt"

type Base struct {
	ID   int
	Name string
}

func (b Base) Describe() string {
	return "base " + b.Name
}

func (b *Base) Rename(name string) {
	b.Name = name
}

type Counter struct {
	N int
}

func (c *Counter) Inc() int {
	c.N++
	return c.N
}

func (c Counter) Get() int {
	return c.N
}

// Item embeds a Base by value and a Counter through a pointer. Its own Name hides Base.Name.
type Item struct {
	Base
	*Counter
	Name string
}

// Wrapper embeds an Item, so Base's fields and methods are promoted two levels.
type Wrapper struct {
	Item
	Extra int
}

func main() {
	it := Item{Base: Base{ID: 1, Name: "base"}, Counter: &Counter{}, Name: "item"}
	fmt.Println(it.ID, it.Name, it.Base.Name, it.N)

	it.ID = 2
	it.Rename("renamed")
	fmt.Println(it.Describe(), it.ID, it.Name)

	first := it.Inc()
	second := it.Inc()
	fmt.Println(first, second, it.Get(), it.Counter.N)

	shared := it
	shared.Base.ID = 3
	fmt.Println(shared.Inc())
	fmt.Println(it.N, it.ID, shared.ID)

	w := Wrapper{Item: it, Extra: 7}
	w.ID = 4
	w.Rename("wrapped")
	fmt.Println(w.Describe(), w.ID, w.Item.Base.ID, it.ID, w.Inc(), w.Extra)

	p := &w
	p.ID = 5
	p.Rename("via pointer")
	fmt.Println(p.Describe(), p.ID, p.Get())

	fmt.Println(Base{ID: 6, Name: "printed"})
	fmt.Println(Wrapper{Item: Item{Name: "x"}, Extra: 1})

	defer func() {
		fmt.Println("recovered:", recover())
	}()
	empty := Item{Name: "empty"}
	fmt.Println(empty.Get())
}
