package main

import (
	"fmt"
	"sync"
	"time"
)

func buff0TickTack() {
	tick := make(chan int)
	tack := make(chan int)
	go func() {
		for i := range tick {
			fmt.Println("tick", i)
			time.Sleep(500 * time.Millisecond)
			i++
			tack <- i
		}
		close(tack)
	}()
	tick <- 0
	for i := range tack {
		fmt.Println("tack", i)
		time.Sleep(500 * time.Millisecond)
		i++
		if i < 10 {
			tick <- i
		} else {
			close(tick)
		}
	}
}

func buff0() {
	tick := make(chan int)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		for i := range tick {
			fmt.Println("<-tick", i)
			time.Sleep(500 * time.Millisecond)
			i++
		}
		wg.Done()
	}()
	tick <- 0
	fmt.Println("tick <- 0")
	tick <- 1
	fmt.Println("tick <- 1")
	tick <- 2
	fmt.Println("tick <- 2")
	tick <- 3
	fmt.Println("tick <- 3")
	tick <- 4
	fmt.Println("tick <- 4")
	close(tick)
	wg.Wait()
}

func buff1() {
	tick := make(chan int, 1)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		for i := range tick {
			fmt.Println("<-tick", i)
			time.Sleep(500 * time.Millisecond)
			i++
		}
		wg.Done()
	}()
	tick <- 0
	fmt.Println("tick <- 0")
	tick <- 1
	fmt.Println("tick <- 1")
	tick <- 2
	fmt.Println("tick <- 2")
	tick <- 3
	fmt.Println("tick <- 3")
	tick <- 4
	fmt.Println("tick <- 4")
	close(tick)
	wg.Wait()
}

func buff2() {
	tick := make(chan int, 2)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		for i := range tick {
			fmt.Println("<-tick", i)
			time.Sleep(500 * time.Millisecond)
			i++
		}
		wg.Done()
	}()
	tick <- 0
	fmt.Println("tick <- 0")
	tick <- 1
	fmt.Println("tick <- 1")
	tick <- 2
	fmt.Println("tick <- 2")
	tick <- 3
	fmt.Println("tick <- 3")
	tick <- 4
	fmt.Println("tick <- 4")
	close(tick)
	wg.Wait()
}

func buff4() {
	tick := make(chan int, 4)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		for i := range tick {
			fmt.Println("<-tick", i)
			time.Sleep(500 * time.Millisecond)
			i++
		}
		wg.Done()
	}()
	tick <- 0
	fmt.Println("tick <- 0")
	tick <- 1
	fmt.Println("tick <- 1")
	tick <- 2
	fmt.Println("tick <- 2")
	tick <- 3
	fmt.Println("tick <- 3")
	tick <- 4
	fmt.Println("tick <- 4")
	close(tick)
	wg.Wait()
}

func main() {
	fmt.Println("***** buff0 *****")
	buff0()
	fmt.Println("***** buff1 *****")
	buff1()
	fmt.Println("***** buff2 *****")
	buff2()
	fmt.Println("***** buff4 *****")
	buff4()
	fmt.Println("***** buff0TickTack *****")
	buff0TickTack()
}
