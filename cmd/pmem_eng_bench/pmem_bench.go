package main

// !!! need go-pmem go compiler
//
// import (
// 	"fmt"
// 	"math/rand"
// 	"time"

// 	"github.com/vmware/go-pmem-transaction/pmem"
// )

// func main() {
// 	rand.Seed(time.Now().UTC().UnixNano())
// 	firstInit := pmem.Init("./list_database")
// 	var rptr *storage_eng
// 	if firstInit {
// 		// Create a new named object called dbRoot and point it to rptr
// 		rptr = (*SkipList)(pmem.New("dbRoot", rptr))
// 		rptr.Init()
// 		rptr.Insert([]byte{0x97, 0x98}, []byte("hello my engine1"))
// 		rptr.Insert([]byte{0x97, 0x99}, []byte("hello my engine2"))
// 		rptr.Insert([]byte{0x97, 0x97}, []byte("hello my engine3"))
// 	} else {
// 		// Retrieve the named object dbRoot
// 		rptr = (*SkipList)(pmem.Get("dbRoot", rptr))

// 		rptr.Delete([]byte{0x97, 0x98})
// 		for e := rptr.Front(); e != nil; e = e.Next() {
// 			fmt.Printf("%x \n", e.Key)
// 			fmt.Printf("%s \n", e.Value)
// 		}
// 	}
// }
