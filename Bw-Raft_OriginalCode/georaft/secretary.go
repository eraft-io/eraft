package georaft

import (
	"fmt"
	"log"
	"net"
	"sync"

	RPC "../grpc/georaft"
	Per "../persister"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	//"sync/atomic"
	"time"
	// "math/rand"
	"encoding/json"

	config "../config"
)

type Secretary struct {
	mu *sync.Mutex // Lock to protect shared access to this peer's state
	// peers     []*labrpc.ClientEnd // RPC end points of all peers
	// persister *Persister          // Object to hold this peer's persisted state
	me int32 // this peer's index into peers[]
	// state a Secretary server must maintain.
	state State
	//Persistent state on all servers:(Updated on stable storage before responding to RPCs)
	currentTerm int32 // "latest term server has seen (initialized to 0 increases monotonically)"
	votedFor    int32 // "candidateId that received vote in current term (or null if none)"
	log         []Log // "log entries;(first index is 1)"
	//Volatile state on all servers:
	commitIndex int32 // "index of highest log entry known to be committed (initialized to 0, increases monotonically)"
	lastApplied int32 // "index of highest log entry applied to state machine (initialized to 0, increases monotonically)"

	//Volatile state on leaders：(Reinitialized after election)
	nextIndex  []int32 // "for each server,index of the next log entry to send to that server"
	matchIndex []int32 // "for each server,index of highest log entry known to be replicated on server(initialized to 0, im)"

	secnextIndex  []int32 // "for each server,index of the next log entry to send to that server"
	secmatchIndex []int32 // "for each server,index of highest log entry known to be replicated on server(initialized to 0, im)"

	obnextIndex  []int32 // "for each server,index of the next log entry to send to that server"
	obmatchIndex []int32 // "for each server,index of highest log entry known to be replicated on server(initialized to 0, im)"

	//channel
	applyCh chan int  // from Make()
	killCh  chan bool //for Kill()
	//handle rpc
	voteCh      chan bool
	appendLogCh chan bool

	// New
	persist *Per.Persister
	//client RPC.SecretaryClient
	address   string
	followers []string
	observers []string
}

func (rf *Secretary) getPrevLogIdx(i int) int32 {
	return rf.nextIndex[i] - 1
}

func (rf *Secretary) getPrevLogTerm(i int) int32 {
	prevLogIdx := rf.getPrevLogIdx(i)
	if prevLogIdx < 0 {
		return -1
	}
	return rf.log[prevLogIdx].Term
}

func (rf *Secretary) getLastLogIdx() int32 {
	return int32(len(rf.log) - 1)
}

func (rf *Secretary) getLastLogTerm() int32 {
	idx := rf.getLastLogIdx()
	if idx < 0 {
		return -1
	}
	return rf.log[idx].Term
}

//If there exists an N such that N > commitIndex,
// a majority of matchIndex[i] ≥ N, and log[N].term == currentTerm: set commitIndex = N (§5.3, §5.4).
/* func (rf *Secretary) updateCommitIndex() {
   // rf.matchIndex[rf.me] =int32( len(rf.log) - 1)
    copyMatchIndex := make([]int32,len(rf.matchIndex))
    //fmt.Println(rf.matchIndex)
    copy(copyMatchIndex,rf.matchIndex)
    sort.Sort(sort.Reverse(IntSlice(copyMatchIndex)))
    N := int32( copyMatchIndex[ (len(copyMatchIndex) - 1)/2] )
    //fmt.Println("N: ", N)
    if N > rf.commitIndex && rf.log[N].Term == rf.currentTerm {
        rf.commitIndex = N
        rf.updateLastApplied()
    }
}
*/
func (rf *Secretary) updateLastApplied() {
	for rf.lastApplied < rf.commitIndex {
		rf.lastApplied++
		curLog := rf.log[rf.lastApplied]
		m := curLog.Command //.(config.Op)
		if m.Option == "Put" {
			rf.persist.Put(m.Key, m.Value)
			if rf.state == Leader {
				rf.applyCh <- 1
			}
		}
	}
}

func (rf *Secretary) startAppendLog() {
	go rf.startAppendLogToFollower()
}

//Leader Section:
func (rf *Secretary) startAppendLogToFollower() {
	for i := 0; i < len(rf.followers); i++ {
		go func(idx int) {
			for {
				rf.mu.Lock()
				appendLog := rf.log[rf.nextIndex[idx]:]
				data, _ := json.Marshal(appendLog)
				fmt.Println("send ", rf.currentTerm)
				args := RPC.AppendEntriesArgs{
					Term:         rf.currentTerm,
					LeaderId:     rf.me,
					PrevLogIndex: rf.getPrevLogIdx(idx),
					PrevLogTerm:  rf.getPrevLogTerm(idx),
					Log:          data,
					LeaderCommit: rf.commitIndex,
				}
				rf.mu.Unlock()

				//	fmt.Println("  args.Term", rf.currentTerm, args.Term)
				reply, ret := rf.S2FsendAppendEntries(rf.followers[idx], &args)
				rf.mu.Lock()
				if !ret {
					fmt.Println("connect  ", rf.followers[idx], "error !")
					rf.mu.Unlock()
					return
				}
				if rf.currentTerm != args.Term {
					fmt.Println(" rf.currentTerm != args.Term", rf.currentTerm, args.Term)
					rf.mu.Unlock()
					return
				}
				if reply.Term > rf.currentTerm { //all server rule 1 If RPC response contains term T > currentTerm:
					//rf.beFollower(reply.Term) // set currentTerm = T, convert to follower (§5.1)
					rf.currentTerm = reply.Term
					fmt.Println("reply.Term > rf.currentTerm ")
					rf.mu.Unlock()
					return
				}
				if reply.Success { //If successful：update nextIndex and matchIndex for follower
					rf.matchIndex[idx] = args.PrevLogIndex + int32(len(appendLog))
					rf.nextIndex[idx] = rf.matchIndex[idx] + 1
					rf.mu.Unlock()
					return
				} else { //If AppendEntries fails because of log inconsistency: decrement nextIndex and retry
					tarIndex := reply.ConflictIndex //If it does not find an entry with that term
					if reply.ConflictTerm != NULL {
						logSize := len(rf.log)         //first search its log for conflictTerm
						for i := 0; i < logSize; i++ { //if it finds an entry in its log with that term,
							if rf.log[i].Term != reply.ConflictTerm {
								continue
							}
							for i < logSize && rf.log[i].Term == reply.ConflictTerm {
								i++
							} //set nextIndex to be the one
							tarIndex = int32(i) //beyond the index of the last entry in that term in its log
						}
					}
					rf.nextIndex[idx] = tarIndex
					rf.mu.Unlock()
				}
			}
		}(i)
	}
}

func (rf *Secretary) S2FsendAppendEntries(address string, args *RPC.AppendEntriesArgs) (*RPC.AppendEntriesReply, bool) {
	// Initialize Client
	// fmt.Println("sendAppendEntries")
	conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	//rf.client = RPC.NewGeoRAFTClient(conn)

	client := RPC.NewGeoRAFTClient(conn)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	//args := &RPC.AppendEntriesArgs{}
	reply, err := client.AppendEntries(ctx, args)
	if err != nil {
		fmt.Println(" sendAppendEntries could not greet: ", err, address)
		return reply, false
	}
	return reply, true
}

func (rf *Secretary) L2OsendAppendEntries(address string, args *RPC.AppendEntriesArgs) {

	fmt.Println("StartAppendEntries")

	// Initialize Client
	conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	client := RPC.NewObserverClient(conn)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	//args := &RPC.AppendEntriesArgs{}
	r, err := client.AppendEntries(ctx, args)
	if err != nil {
		log.Printf("could not greet: %v", err)
	}
	log.Printf("Append reply: %s", r)
	//fmt.Println("Append name is ABC")
}

//L2SAppendEntries RPC handler.
func (rf *Secretary) L2SAppendEntries(ctx context.Context, args *RPC.AppendEntriesArgs) (*RPC.L2SAppendEntriesReply, error) {

	//func (rf *Secretary)L2SAppendEntries(ctx context.Context, args *RPC.AppendEntriesArgs) (*RPC.AppendEntriesReply, error) {//now only for heartbeat
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if args.LeaderCommit > rf.commitIndex {
		rf.commitIndex = Min(args.LeaderCommit, rf.getLastLogIdx())
		rf.updateLastApplied()
	}

	if args.Term > rf.currentTerm { //all server rule 1 If RPC request or response contains term T > currentTerm:
		rf.currentTerm = args.Term
	}
	reply := &RPC.L2SAppendEntriesReply{}

	reply.Term = rf.currentTerm
	reply.Success = false
	reply.ConflictTerm = NULL
	reply.ConflictIndex = 0
	//1. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm (§5.3)
	var prevLogIndexTerm int32 = -1
	var logSize int32 = int32(len(rf.log))
	if args.PrevLogIndex >= 0 && args.PrevLogIndex < int32(len(rf.log)) {
		prevLogIndexTerm = rf.log[args.PrevLogIndex].Term
	}
	if prevLogIndexTerm != args.PrevLogTerm {
		reply.ConflictIndex = logSize
		if prevLogIndexTerm == -1 { //If a follower does not have prevLogIndex in its log,
			//it should return with conflictIndex = len(log) and conflictTerm = None.
		} else { //If a follower does have prevLogIndex in its log, but the term does not match
			reply.ConflictTerm = prevLogIndexTerm //it should return conflictTerm = log[prevLogIndex].Term,
			i := int32(0)
			for ; i < logSize; i++ { //and then search its log for
				if rf.log[i].Term == reply.ConflictTerm { //the first index whose entry has term equal to conflictTerm
					reply.ConflictIndex = i
					break
				}
			}
		}
		return reply, nil
	}

	var log []Log
	json.Unmarshal(args.Log, &log)

	//2. Reply false if term < currentTerm (§5.1)
	if args.Term < rf.currentTerm {
		return reply, nil
	}

	index := args.PrevLogIndex
	for i := 0; i < len(log); i++ {
		index++
		if index < logSize {
			if rf.log[index].Term == log[i].Term {
				continue
			} else { //3. If an existing entry conflicts with a new one (same index but different terms),
				rf.log = rf.log[:index] //delete the existing entry and all that follow it (§5.3)
			}
		}
		rf.log = append(rf.log, log[i:]...) //4. Append any new entries not already in the log
		// rf.persist()
		//fmt.Println(args.Term,"Append Secretary Log ",rf.log)
		break
	}
	//5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)

	reply.Success = true
	data, _ := json.Marshal(rf.matchIndex)
	reply.MatchIndex = data

	return reply, nil
}

func (rf *Secretary) RegisterServer(address string) {
	// Register Server
	for {

		lis, err := net.Listen("tcp", address)
		if err != nil {
			log.Fatalf("failed to listen: %v", err)
		}
		s := grpc.NewServer()
		RPC.RegisterSecretaryServer(s, rf)
		// Register reflection service on gRPC server.
		reflection.Register(s)
		if err := s.Serve(lis); err != nil {
			fmt.Printf("failed to serve: %v \n", err)
		}

	}

}

func (rf *Secretary) init() {

	rf.state = Follower
	rf.currentTerm = 0
	rf.votedFor = -1
	rf.log = make([]Log, 1) //(first index is 1)

	rf.commitIndex = 0
	rf.lastApplied = 0
	//because gorountne only send the chan to below goroutine,to avoid block, need 1 buffer
	rf.voteCh = make(chan bool, 1)
	rf.appendLogCh = make(chan bool, 1)
	rf.killCh = make(chan bool, 1)
	heartbeatTime := time.Duration(150) * time.Millisecond
	go func() {
		for {

			rf.startAppendLog()
			time.Sleep(heartbeatTime)
			fmt.Println(rf.log)

		}
	}()
	go rf.RegisterServer(rf.address)

}

func (rf *Secretary) Start(command interface{}) (int32, int32, bool) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	var index int32 = -1
	var term int32 = rf.currentTerm
	isLeader := true // (rf.state == Leader)
	//If command received from client: append entry to local log, respond after entry applied to state machine (§5.3)
	if isLeader {
		index = rf.getLastLogIdx() + 1
		newLog := Log{
			rf.currentTerm,
			command.(config.Op),
		}
		rf.log = append(rf.log, newLog)
		// rf.persist()
		fmt.Println(rf.currentTerm, "#### new Log ####", newLog)
		rf.startAppendLog()

	}
	//fmt.Println("new Log",  rf.log)
	return index, term, isLeader
}

func MakeSecretary(add string, follower []string, observer []string, persist *Per.Persister,
	mu *sync.Mutex, applyCh chan int) *Secretary {
	Secretary := &Secretary{}
	if len(follower) <= 1 {
		panic("#######Address is less 1, you should set follower's address!######")
	}

	Secretary.nextIndex = make([]int32, len(follower))
	Secretary.matchIndex = make([]int32, len(follower))

	Secretary.address = add
	Secretary.persist = persist
	Secretary.applyCh = applyCh
	Secretary.mu = mu
	Secretary.currentTerm = 100
	Secretary.followers = make([]string, len(follower))
	for i := 0; i < len(follower); i++ {
		Secretary.followers[i] = follower[i]
		fmt.Println(Secretary.followers[i])
	}
	Secretary.observers = make([]string, len(observer))
	for i := 0; i < len(observer); i++ {
		Secretary.observers[i] = observer[i]
		fmt.Println(Secretary.observers[i])
	}
	Secretary.init()
	return Secretary
}
