package main

import (
	"fmt"
	"sync"
)

var rw sync.RWMutex

// reader takes a read lock while main holds one.
func reader(got chan string, done chan bool) {
	rw.RLock()
	got <- "second reader in"
	rw.RUnlock()
	done <- true
}

func main() {
	got := make(chan string)
	done := make(chan bool)
	rw.RLock()
	go reader(got, done)
	fmt.Println(<-got)
	fmt.Println("TryLock while reading:", rw.TryLock())
	<-done
	rw.RUnlock()
	fmt.Println("TryLock after RUnlock:", rw.TryLock())
	fmt.Println("TryRLock while writing:", rw.TryRLock())
	rw.Unlock()
	fmt.Println("TryRLock after Unlock:", rw.TryRLock())
	rw.RUnlock()
}
