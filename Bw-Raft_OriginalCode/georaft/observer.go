package georaft



import (
	"log"
	"net"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"sync"
	"google.golang.org/grpc/reflection"
	OB "../grpc/georaft"
	"fmt"

	"../labgob"
	"bytes"
	//"os"
)



type Observer struct {
    mu        sync.Mutex          // Lock to protect shared access to this peer's state

    //Persistent state on all servers:(Updated on stable storage before responding to RPCs)
    currentTerm int32    "latest term server has seen (initialized to 0 increases monotonically)"
    log         []Log  	"log entries;(first index is 1)"

    //Volatile state on all servers:
    commitIndex int32    "index of highest log entry known to be committed (initialized to 0, increases monotonically)"
	lastApplied int32    "index of highest log entry applied to state machine (initialized to 0, increases monotonically)"
	
	address string

}

 
func (ob *Observer)AppendEntries(ctx context.Context, args *OB.AppendEntriesArgs) (*OB.AppendEntriesReply, error) {
	//raft.msg = raft.msg + in.Term
	
	r := bytes.NewBuffer(args.Log)
    d := labgob.NewDecoder(r)
	var log []Log 
	d.Decode(&log) 	

	
    
	fmt.Println("Observer AppendEntries CALL",log )

	reply := &OB.AppendEntriesReply{}
	//rf.currentTerm ++
	reply.Term = ob.currentTerm 
    reply.Success = false
    reply.ConflictTerm = -1
	reply.ConflictIndex = 0


	ob.currentTerm = args.Term

	return reply, nil

}


func (ob *Observer) RegisterServer(address string)  {
	// Register Server 
	for{
		lis, err := net.Listen("tcp", address)
		if err != nil {
			log.Fatalf("failed to listen: %v", err)
		}
		s := grpc.NewServer()
		OB.RegisterObserverServer(s, ob /* &Raft{} */)
		// Register reflection service on gRPC server.
		reflection.Register(s)
		if err := s.Serve(lis); err != nil {
			fmt.Println("failed to serve: %v", err)
		}
		
	}
	
}

func (ob *Observer) init (add string) {
    ob.currentTerm = 0
    ob.log = make([]Log,1) 
    ob.commitIndex = 0
    ob.lastApplied = 0
	ob.address = add;	
	go  ob.RegisterServer(ob.address)

	
}

func MakeObserver(address string) *Observer {
	observer := &Observer{}
	observer.init(address)
	return observer
} 
