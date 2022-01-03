package georaft

import config "../config"

type Op struct {
	// Your definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	Option string
	Key string 
	Value string
	Id int64
	Seq int64
}

//Helper function
func send(ch chan bool) {
    select {
    case <-ch: //if already set, consume it then resent to avoid block
    default:
    }
    ch <- true
}

type State int
const (
    Follower State = iota // value --> 0
    Candidate             // value --> 1
    Leader                // value --> 2
)
const NULL int32 = -1


type Log struct {
    Term    int32       //  "term when entry was received by leader"
    // Debug 原来是interface{}类型，但是因为json序列化和反序列化时Command类型转化有问题
    Command  config.Op  //"command for state machine,"
}


type ApplyMsg struct {
    CommandValid bool
    Command      interface{}
    CommandIndex int32
}


func  Min(a int32, b int32) int32 {
	if a > b{
		return b
	}
	return a
}

type IntSlice []int32
func (s IntSlice) Len() int { return len(s) }
func (s IntSlice) Swap(i, j int){ s[i], s[j] = s[j], s[i] }
func (s IntSlice) Less(i, j int) bool { return s[i] < s[j] }