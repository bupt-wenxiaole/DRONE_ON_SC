package main

import (
	"sync"
	"os"
	"log"
	"bufio"
	"strings"
	"net"
	"strconv"
)

func master(filepath string, w *sync.RWMutex){
	f, err := os.OpenFile("ip.txt", os.O_RDWR|os.O_APPEND, 0660)
	defer f.Close()
	if err != nil {
		log.Fatal(err)
	}
	w.Lock()
	address := []byte("0,10.2.152.24:8888\n")
	n, err := f.Write(address)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("wrote %d bytes address to file", n)
	f.Sync()
	w.Unlock()
}

func worker(filepath string, w* sync.RWMutex, id int) {
	f, err := os.OpenFile("ip.txt", os.O_RDWR|os.O_APPEND, 0660)
	defer f.Close()
	if err != nil {
		log.Fatal(err)
	}
	var selfIP string
	if id % 2 == 0 {
		selfIP = "10.2.152.24"
	}else {
		selfIP = "10.2.152.23"
	}
	selfPort := 10000
	w.Lock()
	scanner := bufio.NewScanner(f)
	lineNum := 0
	for scanner.Scan() {
		if lineNum == 0 {
			masterAddress := scanner.Text()
			log.Println(masterAddress)  // worker i get same master address
		} else {
			currentLineContens := scanner.Text()
			currentLineURL := strings.Split(currentLineContens, ",")[1]
			currentIP, _, err := net.SplitHostPort(currentLineURL)
			if err != nil {
				log.Fatal(err)
			}
			if selfIP == currentIP {
				selfPort++
			}
		}
		lineNum++
	}

	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
	lineToWrite := strconv.Itoa(lineNum) + "," + selfIP + ":" + strconv.Itoa(selfPort) + "\n"
	stuffToWrite := []byte(lineToWrite)
	n, err := f.Write(stuffToWrite)
	if err != nil {
		log.Fatal(err)

	}
	log.Printf("wrote %d bytes address to file", n)
	f.Sync()
	w.Unlock()
}
func main() {
	filepath := "ip.txt"
	var mutex sync.RWMutex
	var wg sync.WaitGroup
	wg.Add(1)
	go func(p string, m *sync.RWMutex) {
		defer wg.Done()
		master(p, m)
	}(filepath, &mutex)
	wg.Wait()
	//rank id != config id == worker id
	for id := 1; id <= 10; id++ {
		wg.Add(1)
		go func(p string, m *sync.RWMutex, id int) {
			defer wg.Done()
			worker(p, m, id)
		}(filepath, &mutex, id)
	}
	wg.Wait()
	return
}

