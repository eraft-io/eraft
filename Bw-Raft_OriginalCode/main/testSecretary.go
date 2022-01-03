
package main

import (
	 SE "../georaft"
	 "fmt"
	 "log"
	 "golang.org/x/net/context"
	"google.golang.org/grpc"
	"time"
	RPC "../grpc/georaft"
	"flag"
	"strings"
	
)



func  L2SsendAppendEntries(address string , args  *RPC.AppendEntriesArgs){

	fmt.Println("StartAppendEntries")

	// Initialize Client
	conn, err := grpc.Dial( address , grpc.WithInsecure(),grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	client := RPC.NewSecretaryClient(conn)


	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	//args := &RPC.AppendEntriesArgs{}
	r, err := client.L2SAppendEntries(ctx,args)
	if err != nil {
		log.Printf("could not greet: %v", err)
	}
	log.Printf("Append reply: %s", r)
	//fmt.Println("Append name is ABC")
}



func main()  {


	var add = flag.String("address", "", "Input Your address")
	var follower = flag.String("follower", "", "Input Your follower")
	var observer = flag.String("observer", "", "Input Your observer")
	flag.Parse()

	// Local address	
	address := *add

	// Observer's address
	observers := strings.Split( *observer, ",")

	// Follower's address
	followers := strings.Split( *follower, ",")

	if (address == ""){
		fmt.Println("address is nill")
		return 
	}

	//fmt.Println(len(followers))
	if (len(followers) == 0 || followers[0] == ""){
		fmt.Println("followers is nill")
		return 
	}

	fmt.Println(followers, observers)
	

	se := SE.MakeSecretary(address, followers, observers)


	if (se != nil){
		fmt.Println("SUCCESS")
		 for{
			args := &RPC.AppendEntriesArgs{}
			L2SsendAppendEntries("localhost:5000", args)
			time.Sleep(time.Second )

			fmt.Println("L2SsendAppendEntries")
		} 
	}else{
		fmt.Println("FAIL")
	}
}