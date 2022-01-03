
package main

import (
	 OB "../georaft"
	 "fmt"
	 "log"
	 "time"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	RPC "../grpc/georaft"
	"flag"
)





func  sendAppendEntries(address string , args  *RPC.AppendEntriesArgs){

	fmt.Println("StartAppendEntries")

	// Initialize Client
	conn, err := grpc.Dial( address , grpc.WithInsecure(),grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	client := RPC.NewObserverClient(conn)


	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	//args := &RPC.AppendEntriesArgs{}
	r, err := client.AppendEntries(ctx,args)
	if err != nil {
		log.Printf("could not greet: %v", err)
	}
	log.Printf("Append reply: %s", r)
	//fmt.Println("Append name is ABC")
}



func main()  {
	var add = flag.String("address", "", "Input Your address")
	flag.Parse()

	if (*add == ""){
		fmt.Println("address is null")
		return 
	}

	ob := OB.MakeObserver(*add)


	if (ob != nil){
		fmt.Println("SUCCESS")


	}else{
		fmt.Println("FAIL")
	}

	time.Sleep(time.Second * 10000 )




}