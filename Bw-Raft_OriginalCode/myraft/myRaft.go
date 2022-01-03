package myraft

import (
	"encoding/json"
	"fmt"
	"log"
	"math/rand"
	"net"
	"sort"
	"sync"
	"sync/atomic"
	"time"

	config "../config"
	RPC "../grpc/myraft"
	Per "../persister"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
	//"os"
)

type State int

const (
	Follower  State = iota // value --> 0
	Candidate              // value --> 1
	Leader                 // value --> 2
)
const NULL int32 = -1

type Log struct {
	Term int32 //  "term when entry was received by leader"
	// Debug 原来是interface{}类型，但是因为json序列化和反序列化时Command类型转化有问题
	Command config.Op //"command for state machine,"
}

type ApplyMsg struct {
	CommandValid bool
	Command      interface{}
	CommandIndex int32
}

type Raft struct {
	mu *sync.Mutex // Lock to protect shared access to this peer's state
	// peers     []*labrpc.ClientEnd // RPC end points of all peers
	// persister *Persister          // Object to hold this peer's persisted state
	me int32 // this peer's index into peers[]

	// state a Raft server must maintain.
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

	//channel
	applyCh chan int  // from Make()
	killCh  chan bool //for Kill()
	//handle rpc
	voteCh      chan bool
	appendLogCh chan bool

	// New
	persist *Per.Persister
	client  RPC.RAFTClient
	address string
	members []string
	delay   int
}

//Helper function
func send(ch chan bool) {
	select {
	case <-ch: //if already set, consume it then resent to avoid block
	default:
	}
	ch <- true
}

func (rf *Raft) getPrevLogIdx(i int) int32 {
	return rf.nextIndex[i] - 1
}

func (rf *Raft) getPrevLogTerm(i int) int32 {
	prevLogIdx := rf.getPrevLogIdx(i)
	if prevLogIdx < 0 {
		return -1
	}
	return rf.log[prevLogIdx].Term
}

func (rf *Raft) getLastLogIdx() int32 {
	return int32(len(rf.log) - 1)
}

func (rf *Raft) getLastLogTerm() int32 {
	idx := rf.getLastLogIdx()
	if idx < 0 {
		return -1
	}
	return rf.log[idx].Term
}

// Add to sort int32
type IntSlice []int32

func (s IntSlice) Len() int           { return len(s) }
func (s IntSlice) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s IntSlice) Less(i, j int) bool { return s[i] < s[j] }

//If there exists an N such that N > commitIndex,
// a majority of matchIndex[i] ≥ N, and log[N].term == currentTerm: set commitIndex = N (§5.3, §5.4).
func (rf *Raft) updateCommitIndex() {
	// rf.matchIndex[rf.me] =int32( len(rf.log) - 1)
	copyMatchIndex := make([]int32, len(rf.matchIndex))
	//fmt.Println(rf.matchIndex)
	copy(copyMatchIndex, rf.matchIndex)
	sort.Sort(sort.Reverse(IntSlice(copyMatchIndex)))
	N := int32(copyMatchIndex[(len(copyMatchIndex)-1)/2])
	//fmt.Println("N: ", N)
	if N > rf.commitIndex && rf.log[N].Term == rf.currentTerm {
		rf.commitIndex = N
		rf.updateLastApplied()
	}
}

type Op struct {
	// Your definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	Option string
	Key    string
	Value  string
	Id     int64
	Seq    int64
}

func (rf *Raft) updateLastApplied() {
	for rf.lastApplied < rf.commitIndex {
		rf.lastApplied++
		curLog := rf.log[rf.lastApplied]
		m := curLog.Command //.(config.Op)
		if m.Option == "Put" {
			//  fmt.Println("Put key value :", m.Key, m.Value)//, curLog.Command.(config.Op).Key,"value",curLog.Command.(config.Op).Value  )
			rf.persist.Put(m.Key, m.Value)
			if rf.state == Leader {
				rf.applyCh <- 1
			}
		}
	}
}

func (rf *Raft) RequestVote(ctx context.Context, args *RPC.RequestVoteArgs) (*RPC.RequestVoteReply, error) {
	/*  rf.mu.Lock()
	defer rf.mu.Unlock() */
	if args.Term > rf.currentTerm { //all server rule 1 If RPC request or response contains term T > currentTerm:
		rf.beFollower(args.Term) // set currentTerm = T, convert to follower (§5.1)
	}
	reply := &RPC.RequestVoteReply{}
	reply.Term = rf.currentTerm
	reply.VoteGranted = false
	if (args.Term < rf.currentTerm) || (rf.votedFor != NULL && rf.votedFor != args.CandidateId) {
		// Reply false if term < currentTerm (§5.1)  If votedFor is not null and not candidateId,
	} else if args.LastLogTerm < rf.getLastLogTerm() || (args.LastLogTerm == rf.getLastLogTerm() && args.LastLogIndex < rf.getLastLogIdx()) {
		//If the logs have last entries with different terms, then the log with the later term is more up-to-date.
		// If the logs end with the same term, then whichever log is longer is more up-to-date.
		// Reply false if candidate’s log is at least as up-to-date as receiver’s log
	} else {
		//grant vote
		rf.votedFor = args.CandidateId
		reply.VoteGranted = true
		rf.state = Follower
		// rf.persist()
		send(rf.voteCh) //because If election timeout elapses without receiving granting vote to candidate, so wake up

	}
	// Debug TODO......
	reply.VoteGranted = true
	return reply, nil
}

//Leader Section:
func (rf *Raft) startAppendLog() {
	fmt.Println("startAppendLog")

	for i := 0; i < len(rf.members); i++ {
		go func(idx int) {
			for {
				rf.mu.Lock()
				if rf.state != Leader {
					rf.mu.Unlock()
					return
				} //send initial empty AppendEntries RPCs (heartbeat) to each server

				appendLog := rf.log[rf.nextIndex[idx]:]
				data, _ := json.Marshal(appendLog)

				args := RPC.AppendEntriesArgs{
					Term:         rf.currentTerm,
					LeaderId:     rf.me,
					PrevLogIndex: rf.getPrevLogIdx(idx),
					PrevLogTerm:  rf.getPrevLogTerm(idx),
					//If last log index ≥ nextIndex for a follower:send AppendEntries RPC with log entries starting at nextIndex
					//nextIndex > last log index, rf.log[rf.nextIndex[idx]:] will be empty then like a heartbeat
					Log:          data,
					LeaderCommit: rf.commitIndex,
				}
				rf.mu.Unlock()
				//:= &RPC.AppendEntriesReply{}
				reply, ret := rf.sendAppendEntries(rf.members[idx], &args)
				rf.mu.Lock()
				if !ret || rf.state != Leader || rf.currentTerm != args.Term {
					rf.mu.Unlock()
					return
				}
				if reply.Term > rf.currentTerm { //all server rule 1 If RPC response contains term T > currentTerm:
					rf.beFollower(reply.Term) // set currentTerm = T, convert to follower (§5.1)
					rf.mu.Unlock()
					return
				}
				if reply.Success { //If successful：update nextIndex and matchIndex for follower
					rf.matchIndex[idx] = args.PrevLogIndex + int32(len(appendLog))
					rf.nextIndex[idx] = rf.matchIndex[idx] + 1
					rf.updateCommitIndex()
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

//Leader Section:
func (rf *Raft) sendHeartBeat() {
	fmt.Println("send heart beat")
	for i := 0; i < len(rf.members); i++ {
		go func(idx int) {
			for {
				args := RPC.AppendEntriesArgs{
					Term:         rf.currentTerm,
					LeaderId:     rf.me,
					PrevLogIndex: rf.getPrevLogIdx(idx),
					PrevLogTerm:  rf.getPrevLogTerm(idx),
					//If last log index ≥ nextIndex for a follower:send AppendEntries RPC with log entries starting at nextIndex
					//nextIndex > last log index, rf.log[rf.nextIndex[idx]:] will be empty then like a heartbeat
					Log:          nil,
					LeaderCommit: rf.commitIndex,
				}
				//:= &RPC.AppendEntriesReply{}
				reply, ret := rf.sendAppendEntries(rf.members[idx], &args)
				if !ret || rf.state != Leader || rf.currentTerm != args.Term {
					return
				}
				if reply.Term > rf.currentTerm { //all server rule 1 If RPC response contains term T > currentTerm:
					rf.beFollower(reply.Term) // set currentTerm = T, convert to follower (§5.1)
					return
				}
			}

		}(i)
	}
}

func (rf *Raft) sendAppendEntries(address string, args *RPC.AppendEntriesArgs) (*RPC.AppendEntriesReply, bool) {
	// Initialize Client

	time.Sleep(time.Millisecond * time.Duration(rf.delay+rand.Intn(25)))

	conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	rf.client = RPC.NewRAFTClient(conn)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()
	//args := &RPC.AppendEntriesArgs{}
	reply, err := rf.client.AppendEntries(ctx, args)
	if err != nil {
		fmt.Println(" sendAppendEntries could not greet: ", err, address)
		return reply, false
	}
	return reply, true
}

//AppendEntries RPC handler.
func (rf *Raft) AppendEntries(ctx context.Context, args *RPC.AppendEntriesArgs) (*RPC.AppendEntriesReply, error) { //now only for heartbeat
	rf.mu.Lock()
	defer rf.mu.Unlock()
	//	time.Sleep(time.Millisecond * time.Duration(rf.delay))
	defer send(rf.appendLogCh)      //If election timeout elapses without receiving AppendEntries RPC from current leader
	if args.Term > rf.currentTerm { //all server rule 1 If RPC request or response contains term T > currentTerm:
		rf.beFollower(args.Term) // set currentTerm = T, convert to follower (§5.1)
	}
	reply := &RPC.AppendEntriesReply{}

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
		//fmt.Println(args.Term,"Append RAft Log ",rf.log)
		break
	}
	//5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
	if args.LeaderCommit > rf.commitIndex {
		rf.commitIndex = Min(args.LeaderCommit, rf.getLastLogIdx())
		rf.updateLastApplied()
	}
	reply.Success = true
	return reply, nil
}

func Min(a int32, b int32) int32 {
	if a > b {
		return b
	}
	return a
}

func (rf *Raft) RegisterServer(address string) {
	// Register Server
	for {

		lis, err := net.Listen("tcp", address)
		if err != nil {
			log.Fatalf("failed to listen: %v", err)
		}
		s := grpc.NewServer()
		RPC.RegisterRAFTServer(s, rf)
		// Register reflection service on gRPC server.
		reflection.Register(s)
		if err := s.Serve(lis); err != nil {
			fmt.Printf("failed to serve: %v \n", err)
		}

	}

}

//Follower Section:
func (rf *Raft) beFollower(term int32) {
	rf.state = Follower
	rf.votedFor = NULL
	rf.currentTerm = term
	//rf.persist()
}

func (rf *Raft) beLeader() {

	if rf.state != Candidate {
		return
	}
	rf.state = Leader
	//initialize leader data
	rf.nextIndex = make([]int32, len(rf.members))
	rf.matchIndex = make([]int32, len(rf.members))
	for i := 0; i < len(rf.nextIndex); i++ { //(initialized to leader last log index + 1)
		rf.nextIndex[i] = rf.getLastLogIdx() + 1
	}
	fmt.Println(rf.address, "#####become LEADER####", rf.currentTerm)
}

//Candidate Section:
// If AppendEntries RPC received from new leader: convert to follower implemented in AppendEntries RPC Handler
func (rf *Raft) beCandidate() { //Reset election timer are finished in caller
	fmt.Println(rf.address, " become Candidate ", rf.currentTerm)
	rf.state = Candidate
	rf.currentTerm++    //Increment currentTerm
	rf.votedFor = rf.me //vote myself first
	//ask for other's vote
	go rf.startElection() //Send RequestVote RPCs to all other servers
}

func (rf *Raft) GetState() (int32, bool) {
	var term int32
	var isleader bool
	rf.mu.Lock()
	defer rf.mu.Unlock()
	term = rf.currentTerm
	isleader = (rf.state == Leader)
	return term, isleader
}

//If election timeout elapses: start new election handled in caller
func (rf *Raft) startElection() {

	fmt.Println("startElection")

	args := RPC.RequestVoteArgs{
		Term:         rf.currentTerm,
		CandidateId:  rf.me,
		LastLogIndex: rf.getLastLogIdx(),
		LastLogTerm:  rf.getLastLogTerm(),
	}
	// TODO.....
	var votes int32 = 1
	for i := 0; i < len(rf.members); i++ {
		if rf.address == rf.members[i] {
			continue
		}
		go func(idx int) {
			//fmt.Println("sendRequestVote to :", rf.members[idx])
			//reply := RPC.RequestVoteReply{Term:9999, VoteGranted: false}
			ret, reply := rf.sendRequestVote(rf.members[idx], &args /* ,&reply */)

			if ret {
				/* rf.mu.Lock()
				   defer rf.mu.Unlock() */
				if reply.Term > rf.currentTerm {
					//fmt.Println( "reply.beFollower ")
					rf.beFollower(reply.Term)
					return
				}
				if rf.state != Candidate || rf.currentTerm != args.Term {
					//fmt.Println("rf.state != Candidate || rf.currentTerm != args.Term")
					return
				}
				if reply.VoteGranted {
					//fmt.Println( "#########reply.VoteGranted ############3")
					atomic.AddInt32(&votes, 1)
				} else {
					//	fmt.Println( "#########reply.VoteGranted false ", reply.Term)
				}
				if atomic.LoadInt32(&votes) > int32(len(rf.members)/2) {
					rf.beLeader()
					//fmt.Println("rf.beLeader()")
					send(rf.voteCh) //after be leader, then notify 'select' goroutine will sending out heartbeats immediately
				}
			}
		}(i)

	}
}

func (rf *Raft) init() {

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
			select {
			case <-rf.killCh:
				return
			default:
			}
			electionTime := time.Duration(rand.Intn(350)+500) * time.Millisecond

			// rf.mu.Lock()
			state := rf.state
			// rf.mu.Unlock()
			switch state {
			case Follower, Candidate:
				select {
				case <-rf.voteCh:
				case <-rf.appendLogCh:
				case <-time.After(electionTime):
					//    rf.mu.Lock()
					fmt.Println("######## time.After(electionTime) #######")
					rf.beCandidate() //becandidate, Reset election timer, then start election
					//    rf.mu.Unlock()
				}
			case Leader:
				rf.startAppendLog()
				time.Sleep(heartbeatTime)
			}
		}
	}()

	// Add New

	go rf.RegisterServer(rf.address)

}

func (rf *Raft) Start(command interface{}) (int32, int32, bool) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	var index int32 = -1
	var term int32 = rf.currentTerm
	isLeader := (rf.state == Leader)
	//If command received from client: append entry to local log, respond after entry applied to state machine (§5.3)
	if isLeader {
		index = rf.getLastLogIdx() + 1
		newLog := Log{
			rf.currentTerm,
			command.(config.Op),
		}
		rf.log = append(rf.log, newLog)
		// rf.persist()
		rf.startAppendLog()

	}
	//fmt.Println("new Log",  rf.log)
	return index, term, isLeader
}

func (rf *Raft) sendRequestVote(address string, args *RPC.RequestVoteArgs) (bool, *RPC.RequestVoteReply) {
	//fmt.
	// Initialize Client
	conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	rf.client = RPC.NewRAFTClient(conn)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	//var err error
	reply, err := rf.client.RequestVote(ctx, args)
	/* *reply.Term = *r.Term
	*reply.VoteGranted = *r.VoteGranted */
	if err != nil {
		return false, reply
	} else {
		return true, reply

	}
}

func MakeRaft(add string, mem []string, persist *Per.Persister,
	mu *sync.Mutex, applyCh chan int, delay int) *Raft {
	raft := &Raft{}
	if len(mem) <= 1 {
		panic("#######Address is less 1, you should set follower's address!######")
	}
	raft.address = add
	raft.persist = persist
	raft.applyCh = applyCh
	raft.mu = mu
	raft.members = make([]string, len(mem))
	raft.delay = delay
	for i := 0; i < len(mem); i++ {
		raft.members[i] = mem[i]
		fmt.Println(raft.members[i])
	}
	raft.init()
	return raft
}
