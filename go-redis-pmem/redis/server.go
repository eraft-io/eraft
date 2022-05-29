///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"strconv"
	"strings"

	"github.com/vmware/go-pmem-transaction/pmem"
	"github.com/vmware/go-pmem-transaction/transaction"
)

type (
	server struct {
		db       *redisDb
		commands map[string](*redisCommand)
	}

	redisDb struct {
		dict   *dict
		expire *dict
		magic  int
	}

	redisCommand struct {
		name string
		proc func(*client)
		flag int
	}

	client struct {
		s       *server
		db      *redisDb
		conn    *net.TCPConn
		rBuffer *bufio.Reader
		wBuffer *bufio.Writer

		argc int
		argv [][]byte

		querybuf     []byte
		multibulklen int
		bulklen      int
		replybuf     [][]byte

		cmd *redisCommand
	}

	sharedObjects struct {
		crlf, czero, cone, cnegone,
		ok, nullbulk, emptybulk, emptymultibulk, pong,
		syntaxerr, wrongtypeerr, outofrangeerr, nokeyerr,
		bulkhead, inthead, arrayhead,
		maxstring, minstring []byte
	}
)

const (
	DATABASE string = "./database"
	PORT     string = ":6379"
	MAGIC    int    = 0x3F4F357F7C9824B3

	CMD_WRITE    int = 1 << 0
	CMD_READONLY int = 1 << 1
	CMD_LARGE    int = 1 << 2
)

var (
	shared            sharedObjects
	redisCommandTable = [...]redisCommand{
		redisCommand{"PING", pingCommand, CMD_READONLY},
		redisCommand{"GET", getCommand, CMD_READONLY},
		redisCommand{"GETRANGE", getrangeCommand, CMD_READONLY},
		redisCommand{"MGET", mgetCommand, CMD_READONLY},
		redisCommand{"LLEN", llenCommand, CMD_READONLY},
		redisCommand{"LINDEX", lindexCommand, CMD_READONLY},
		redisCommand{"LRANGE", lrangeCommand, CMD_READONLY},
		redisCommand{"HGET", hgetCommand, CMD_READONLY},
		redisCommand{"HMGET", hmgetCommand, CMD_READONLY},
		redisCommand{"HLEN", hlenCommand, CMD_READONLY},
		redisCommand{"HSTRLEN", hstrlenCommand, CMD_READONLY},
		redisCommand{"HKEYS", hkeysCommand, CMD_READONLY},
		redisCommand{"HVALS", hvalsCommand, CMD_READONLY},
		redisCommand{"HGETALL", hgetallCommand, CMD_READONLY},
		redisCommand{"HEXISTS", hexistsCommand, CMD_READONLY},
		redisCommand{"SCARD", scardCommand, CMD_READONLY},
		redisCommand{"SISMEMBER", sismemberCommand, CMD_READONLY},
		redisCommand{"ZCARD", zcardCommand, CMD_READONLY},
		redisCommand{"ZSCORE", zscoreCommand, CMD_READONLY},
		redisCommand{"ZRANK", zrankCommand, CMD_READONLY},
		redisCommand{"ZREVRANK", zrevrankCommand, CMD_READONLY},
		redisCommand{"ZCOUNT", zcountCommand, CMD_READONLY},
		redisCommand{"ZLEXCOUNT", zlexcountCommand, CMD_READONLY},
		redisCommand{"ZRANGE", zrangeCommand, CMD_READONLY},
		redisCommand{"ZREVRANGE", zrevrangeCommand, CMD_READONLY},
		redisCommand{"ZRANGEBYSCORE", zrangebyscoreCommand, CMD_READONLY},
		redisCommand{"ZREVRANGEBYSCORE", zrevrangebyscoreCommand, CMD_READONLY},
		redisCommand{"ZRANGEBYLEX", zrangebylexCommand, CMD_READONLY},
		redisCommand{"ZREVRANGEBYLEX", zrevrangebylexCommand, CMD_READONLY},
		redisCommand{"EXISTS", existsCommand, CMD_READONLY},
		redisCommand{"DBSIZE", dbsizeCommand, CMD_READONLY},
		redisCommand{"SELECT", selectCommand, CMD_READONLY},
		redisCommand{"RANDOMKEY", randomkeyCommand, CMD_READONLY},
		redisCommand{"STRLEN", strlenCommand, CMD_READONLY},
		redisCommand{"TTL", ttlCommand, CMD_READONLY},
		redisCommand{"PTTL", pttlCommand, CMD_READONLY},
		redisCommand{"APPEND", appendCommand, CMD_WRITE},
		redisCommand{"SET", setCommand, CMD_WRITE},
		redisCommand{"SETNX", setnxCommand, CMD_WRITE},
		redisCommand{"SETEX", setexCommand, CMD_WRITE},
		redisCommand{"SINTER", sinterCommand, CMD_READONLY},
		redisCommand{"SDIFF", sdiffCommand, CMD_READONLY},
		redisCommand{"SUNION", sunionCommand, CMD_READONLY},
		redisCommand{"SMEMBERS", sinterCommand, CMD_READONLY},
		redisCommand{"SRANDMEMBER", srandmemberCommand, CMD_READONLY},
		redisCommand{"PSETEX", psetexCommand, CMD_WRITE},
		redisCommand{"SETRANGE", setrangeCommand, CMD_WRITE},
		redisCommand{"GETSET", getsetCommand, CMD_WRITE},
		redisCommand{"MSET", msetCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"MSETNX", msetnxCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"INCR", incrCommand, CMD_WRITE},
		redisCommand{"INCRBY", incrbyCommand, CMD_WRITE},
		redisCommand{"INCRBYFLOAT", incrbyfloatCommand, CMD_WRITE},
		redisCommand{"DECR", decrCommand, CMD_WRITE},
		redisCommand{"DECRBY", decrbyCommand, CMD_WRITE},
		redisCommand{"LPUSH", lpushCommand, CMD_WRITE},
		redisCommand{"RPUSH", rpushCommand, CMD_WRITE},
		redisCommand{"LPUSHX", lpushxCommand, CMD_WRITE},
		redisCommand{"RPUSHX", rpushxCommand, CMD_WRITE},
		redisCommand{"LINSERT", linsertCommand, CMD_WRITE},
		redisCommand{"LSET", lsetCommand, CMD_WRITE},
		redisCommand{"LPOP", lpopCommand, CMD_WRITE},
		redisCommand{"RPOP", rpopCommand, CMD_WRITE},
		redisCommand{"RPOPLPUSH", rpoplpushCommand, CMD_WRITE},
		redisCommand{"LREM", lremCommand, CMD_WRITE},
		redisCommand{"LTRIM", ltrimCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"HSET", hsetCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"HSETNX", hsetnxCommand, CMD_WRITE},
		redisCommand{"HINCRBY", hincrbyCommand, CMD_WRITE},
		redisCommand{"HINCRBYFLOAT", hincrbyfloatCommand, CMD_WRITE},
		redisCommand{"HMSET", hsetCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"HDEL", hdelCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"SADD", saddCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"SREM", sremCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"SMOVE", smoveCommand, CMD_WRITE},
		redisCommand{"SPOP", spopCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"SINTERSTORE", sinterstoreCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"SDIFFSTORE", sdiffstoreCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"SUNIONSTORE", sunionstoreCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZADD", zaddCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZINCRBY", zincrbyCommand, CMD_WRITE},
		redisCommand{"ZREM", zremCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZREMRANGEBYSCORE", zremrangebyscoreCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZREMRANGEBYRANK", zremrangebyrankCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZREMRANGEBYLEX", zremrangebylexCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZUNIONSTORE", zunionstoreCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"ZINTERSTORE", zinterstoreCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"DEL", delCommand, CMD_WRITE | CMD_LARGE},
		redisCommand{"FLUSHDB", flushdbCommand, CMD_WRITE},
		redisCommand{"EXPIRE", expireCommand, CMD_WRITE},
		redisCommand{"EXPIREAT", expireatCommand, CMD_WRITE},
		redisCommand{"PEXPIRE", pexpireCommand, CMD_WRITE},
		redisCommand{"PEXPIREAT", pexpireatCommand, CMD_WRITE},
		redisCommand{"PERSIST", persistCommand, CMD_WRITE}}

	pstart, pend uintptr
)

func RunServer() {
	s := new(server)
	s.Start()
}

func (s *server) Start() {
	// Initialize database
	s.init(DATABASE)
	// accept client connections
	tcpAddr, err := net.ResolveTCPAddr("tcp4", PORT)
	fatalError(err)

	listener, err := net.ListenTCP("tcp", tcpAddr)
	fatalError(err)

	go s.Cron()
	fmt.Println("Go-redis is ready to accept connections")
	for {
		conn, err := listener.AcceptTCP()
		if err != nil {
			continue
		}
		// run as a goroutine
		go s.handleClient(conn)
	}
}

func populateDb(db *redisDb) {
	txn("undo") {
		db.dict = NewDict(1024, 32)
		db.expire = NewDict(128, 1)
		db.magic = MAGIC
	}
}

func (s *server) init(path string) {
	firstInit := pmem.Init(path)

	if firstInit { // indicates a first time initialization
		var dbr *redisDb
		db := (*redisDb)(pmem.New("dbRoot", dbr))
		populateDb(db)
		s.db = db
	} else {
		var dbr *redisDb
		db := (*redisDb)(pmem.Get("dbRoot", dbr))
		if db.magic != MAGIC {
			// Previous initialization did not complete successfully. Re-populate
			// data members in db.
			populateDb(db)
		}
		s.db = db
		txn("undo") {
			s.db.swizzle()
		}
	}
	s.populateCommandTable()
	createSharedObjects()
}

func (s *server) populateCommandTable() {
	s.commands = make(map[string](*redisCommand))
	for i, v := range redisCommandTable {
		s.commands[v.name] = &redisCommandTable[i]
	}
}

func createSharedObjects() {
	shared = sharedObjects{
		crlf:           []byte("\r\n"),
		czero:          []byte(":0\r\n"),
		cone:           []byte(":1\r\n"),
		cnegone:        []byte(":-1\r\n"),
		ok:             []byte("+OK\r\n"),
		nullbulk:       []byte("$-1\r\n"),
		emptybulk:      []byte("$0\r\n\r\n"),
		emptymultibulk: []byte("*0\r\n"),
		pong:           []byte("+PONG\r\n"),
		syntaxerr:      []byte("-ERR syntax error\r\n"),
		wrongtypeerr:   []byte("-WRONGTYPE Operation against a key holding the wrong kind of value\r\n"),
		outofrangeerr:  []byte("-ERR index out of range\r\n"),
		nokeyerr:       []byte("-ERR no such key\r\n"),
		bulkhead:       []byte("$"),
		inthead:        []byte(":"),
		arrayhead:      []byte("*"),
		maxstring:      []byte("maxstring"),
		minstring:      []byte("minstring")}
}

func (s *server) Cron() {
	go s.db.Cron()
}

func (s *server) handleClient(conn *net.TCPConn) {
	c := s.newClient(conn)
	c.conn.SetNoDelay(false) // try batching packet to improve tp.
	c.processInput()
	conn.Close()
}

func (s *server) newClient(conn *net.TCPConn) *client {
	return &client{s: s,
		db:           s.db,
		conn:         conn,
		rBuffer:      bufio.NewReader(conn),
		wBuffer:      bufio.NewWriter(conn),
		argc:         0,
		argv:         nil,
		querybuf:     make([]byte, 1024),
		multibulklen: 0,
		bulklen:      -1,
		replybuf:     nil,
		cmd:          nil}
}

// Process input buffer and call command.
func (c *client) processInput() {
	pos := 0        // current buffer pos for network reading
	curr := 0       // curr pos of processing
	finish := false // finish processing a query
	for {
		n, err := c.conn.Read(c.querybuf[pos:])
		if err != nil { // TODO: check error
			fmt.Println("Reading from client:", err)
			return
		}
		if n == 0 {
			fmt.Println("Client closed connection")
			return
		}

		//fmt.Printf("%d, %d, %d, %q\n", pos, curr, n, string(c.querybuf[pos:pos+n]))
		pos += n
		//fmt.Println("Input buffer ", pos, n, len(c.querybuf), curr)

		// process multi bulk buffer
		curr, finish = c.processMultibulkBuffer(curr, pos)
		for finish {
			// Get one full query
			if c.argc > 0 {
				//fmt.Println("Process command")
				c.processCommand()
			}
			c.reset()
			curr, finish = c.processMultibulkBuffer(curr, pos)
		}

		// expand query buffer if full. TODO: set max query buffer length.
		if pos == len(c.querybuf) {
			nBuf := make([]byte, len(c.querybuf)*2)
			copy(nBuf, c.querybuf[curr:pos])
			pos -= curr
			curr = 0
			c.querybuf = nBuf
		}
		if pos == curr {
			pos = 0
			curr = 0
		}
		c.wBuffer.Flush()
	}
}

// Process RESP array of bulk strings, e.g., "*2\r\n$4\r\nLLEN\r\n$6\r\nmylist\r\n"
func (c *client) processMultibulkBuffer(begin, end int) (int, bool) {
	newline := -1
	if c.multibulklen == 0 {
		newline = findNewLine(c.querybuf[begin:end])
		if newline == -1 {
			return begin, false
		}
		if c.querybuf[begin] != '*' {
			fmt.Println("Protocal error! expected '*' for multibulk len ", begin, string(c.querybuf[begin:]))
			os.Exit(1)
		}
		// has to exclude '*'/'\r' with +1/-1
		size, err := slice2i(c.querybuf[begin+1 : begin+newline-1])
		fatalError(err)
		begin += newline + 1
		if size <= 0 {
			return begin, true
		}
		c.multibulklen = size
	}
	for c.multibulklen > 0 {
		// read bulk len if unknown
		if c.bulklen == -1 {
			newline = findNewLine(c.querybuf[begin:end])
			if newline == -1 {
				return begin, false
			}
			if c.querybuf[begin] != '$' {
				fmt.Println("Protocal error! expected '$' for bulk len ", begin, string(c.querybuf[begin:]))
				os.Exit(1)
			}
			// has to exclude '$'/'\r' with +1/-1
			size, err := slice2i(c.querybuf[begin+1 : begin+newline-1])
			fatalError(err)
			c.bulklen = size
			begin += newline + 1
		}

		// read bulk argument
		if end-begin < c.bulklen+2 {
			// not enough data (+2 == trailing \r\n)
			return begin, false
		} else {
			arg := make([]byte, c.bulklen)
			copy(arg, c.querybuf[begin:])
			c.argc += 1
			c.argv = append(c.argv, arg)
			begin += c.bulklen + 2
			c.bulklen = -1
			c.multibulklen--
		}
	}
	// successuflly process the whole mutlibulk query if reach here
	return begin, true
}

// Return index of newline in slice, -1 if not found
func findNewLine(buf []byte) int {
	for i, c := range buf[:] {
		if c == '\n' {
			return i
		}
	}
	return -1
}

func slice2i(buf []byte) (int, error) {
	return strconv.Atoi(string(buf))
}

func (c *client) reset() {
	c.argc = 0
	c.argv = c.argv[0:0]
	c.multibulklen = 0
	c.bulklen = -1
}

// currenlty only support simple SET/GET that is used by memtierbenchmark
func (c *client) processCommand() {
	c.lookupCommand()
	if c.cmd == nil {
		c.notSupported()
	} else {
		// c.printCommand()
		txn ("undo") {
		c.cmd.proc(c)
		// TODO: mohitv remove below...used for crash
		//fmt.Println("going to sleep before committing tx, crash now")
		//time.Sleep(2 * time.Second)
		//fmt.Println("oops...woke up!")
		}
	}
}

func (c *client) lookupCommand() {
	c.cmd = c.s.commands[strings.ToUpper(string(c.argv[0]))]
}

func (c *client) printCommand() {
	for _, q := range c.argv {
		fmt.Print(string(q), " ")
	}
	fmt.Print("\n")
}

func (c *client) notSupported() {
	fmt.Print("Command not supported! ")
	for _, q := range c.argv {
		fmt.Print(string(q), " ")
	}
	fmt.Print("\n")
	c.addReply(shared.syntaxerr)
	c.wBuffer.Flush()
}

func (c *client) addReply(s []byte) {
	c.wBuffer.Write(s)
}

func (c *client) addReplyBulk(s []byte) {
	if c.replybuf == nil {
		c.wBuffer.Write(shared.bulkhead)
		c.wBuffer.Write([]byte(strconv.Itoa(len(s)))) // not efficient
		c.wBuffer.Write(shared.crlf)
		c.wBuffer.Write(s)
		c.wBuffer.Write(shared.crlf)
	} else {
		c.replybuf = append(c.replybuf, s)
	}
}

func (c *client) addReplyLongLong(ll int64) {
	c.wBuffer.Write(shared.inthead)
	c.wBuffer.Write([]byte(strconv.FormatInt(ll, 10))) // not efficient
	c.wBuffer.Write(shared.crlf)
}

func (c *client) addReplyDouble(d float64) {
	s := strconv.FormatFloat(d, 'g', -1, 64)
	c.addReplyBulk([]byte(s))
}

func (c *client) addReplyMultiBulkLen(ll int) {
	c.wBuffer.Write(shared.arrayhead)
	c.wBuffer.Write([]byte(strconv.Itoa(ll))) // not efficient
	c.wBuffer.Write(shared.crlf)
}

func (c *client) addDeferredMultiBulkLength() {
	c.replybuf = make([][]byte, 1)
}

func (c *client) setDeferredMultiBulkLength(ll int) {
	replies := c.replybuf[1:]
	c.replybuf = nil
	if ll != len(replies) {
		panic("setDeferredMultiBulkLength: length does not match!")
	}
	c.addReplyMultiBulkLen(ll)
	for _, r := range replies {
		c.addReplyBulk(r)
	}
}

func (c *client) addReplyError(err []byte) {
	c.wBuffer.Write([]byte("-ERR "))
	c.wBuffer.Write(err)
	c.wBuffer.Write(shared.crlf)
}

func (c *client) cleanup() {
	c.conn.Close()
}

func fatalError(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
		os.Exit(1)
	}
}

func getClient() net.Conn {
	tcpAddr, err := net.ResolveTCPAddr("tcp4", PORT)
	fatalError(err)

	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	fatalError(err)

	return conn
}

func pingCommand(c *client) {
	if c.argc == 1 {
		c.addReply(shared.pong)
	} else {
		c.addReply(c.argv[1])
	}
}
