package main

import (
	"fmt"
	"os"
)

func main() {
	fmt.Println("before exit")
	os.Exit(3)
}
