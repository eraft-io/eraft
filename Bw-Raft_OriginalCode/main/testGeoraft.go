
package main

import (
	 Raft "../georaft"
	 "fmt"
	"time"
	"flag"
	"strings"
	
)




func main()  {


	var add = flag.String("address", "", "Input Your address")
	var follower = flag.String("follower", "", "Input Your follower")
	var secretary = flag.String("secretary", "", "Input Your secretary")
	var observer = flag.String("observer", "", "Input Your observer")
	flag.Parse()

	// Local address	
	address := *add

	// Observer's address
	observers := strings.Split( *observer, ",")

	// Follower's address
	followers := strings.Split( *follower, ",")
	
	// secretary's address
	secretaries := strings.Split( *secretary, ",")

	if (address == ""){
		fmt.Println("address is nill")
		return 
	}

	//fmt.Println(len(followers))
	if (len(followers) == 0 || followers[0] == ""){
		fmt.Println("followers is nill")
		return 
	}

	fmt.Println("a", address, "f", followers, "o",observers,"s" ,secretaries)
	

	rf := Raft.MakeGeoRaft(address, followers,secretaries ,observers)


	if (rf != nil){
		fmt.Println("SUCCESS")
		 
	}else{
		fmt.Println("FAIL")
	}




	time.Sleep(time.Second * 10000 )




}