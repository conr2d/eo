package main

import "fmt"

func count() {
	for i := 0; i < 6; i++ {
		defer func() { fmt.Println("deferred", i) }()
		i++
	}
	fmt.Println("end of count")
}

func main() {
	count()
}
