package main


import (
	"../myraft"
	 "os"
	/*"fmt"*/
	"time"
)






 func main() {

	

	raft := myraft.MakeRaft(os.Args)
	raft.GetState()

	time.Sleep(time.Second*1200)



}  