package main

import (
	"fmt"
	"sync"
)

type info struct {
	key string
	n   int
}

// cache is shaped like go-ethereum's rlp type cache: every critical section of mu runs to its end
// without blocking, so mu.Lock() maps to BlockingLock and its callers stay plain functions.
type cache struct {
	mu    sync.Mutex
	m     map[string]*info
	built int
}

func (c *cache) get(key string) *info {
	c.mu.Lock()
	defer c.mu.Unlock()
	if i := c.m[key]; i != nil {
		return i
	}
	i := &info{key: key, n: len(key) * 10}
	c.m[key] = i
	c.built++
	return i
}

func (c *cache) count() int {
	c.mu.Lock()
	n := c.built
	c.mu.Unlock()
	return n
}

func main() {
	c := new(cache)
	c.m = make(map[string]*info)
	keys := []string{"a", "bb", "ccc", "dddd", "eeeee"}
	sums := make([]int, 8)
	var wg sync.WaitGroup
	for g := 0; g < 8; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := 0; i < 500; i++ {
				sums[g] += c.get(keys[(g+i)%len(keys)]).n
			}
		}()
	}
	wg.Wait()
	fmt.Println(c.count(), sums)
	fmt.Println(len(c.m), c.m["eeeee"].n, c.m["a"].key, c.get("a") == c.get("a"))
	fmt.Println(c.mu.TryLock(), c.mu.TryLock())
	c.mu.Unlock()
	fmt.Println(c.mu.TryLock())
	c.mu.Unlock()
}
