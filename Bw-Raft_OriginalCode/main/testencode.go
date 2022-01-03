package main


import(
	"bytes"
	"fmt"
	"../labgob"
)

type Log struct {
    Term    int       //  "term when entry was received by leader"
    Command interface{} //"command for state machine,"
}


func main()  {
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)
	e.Encode(1)

	log := make([]Log,2)
	log[0].Term = 1
	log[1].Term = 2
	log[1].Command = nil
	fmt.Println(log)


	if e.Encode(log) != nil{
		fmt.Println("ERR")
	}
	e.Encode(2)

	data := w.Bytes()

	r := bytes.NewBuffer(data)
	d := labgob.NewDecoder(r)
	var term int
	var term1 int
	var clog []Log


	d.Decode(&term) 
	d.Decode(&clog)
	d.Decode(&term1) 

	//d.Decode(&log)
	
	//if (len(appendLog) > 0){
		//fmt.Println(term,"AppendLog", rf.log)
		fmt.Println(term1,clog)
	//}
}