package main

import "fmt"

type Counter struct {
	n       int
	results chan int
}

func (c *Counter) loop() {
	c.n += 10
	c.results <- c.n
}

func (c *Counter) Start() {
	go c.loop()
}

// spawn returns before the goroutine that Start launches has run, so c must outlive spawn.
func spawn(results chan int, n int) {
	var c Counter
	c.n = n
	c.results = results
	c.Start()
}

func main() {
	results := make(chan int)
	spawn(results, 1)
	spawn(results, 2)
	fmt.Println("sum", <-results+<-results)
}
