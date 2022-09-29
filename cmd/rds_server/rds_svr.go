package main

import (
	"fmt"
	"net"
	"os"
	"strconv"
	"strings"

	"github.com/eraft-io/eraft/shardkvserver"
)

func main() {
	RunRdsServer(":12306", os.Args[1])
}

func RunRdsServer(addr string, cfgSvrs string) {
	lis, err := net.Listen("tcp", addr)
	if err != nil {
		fmt.Printf(err.Error())
		return
	}
	defer lis.Close()

	kvCli := shardkvserver.MakeKvClient(cfgSvrs)
	for {
		conn, err := lis.Accept()
		if err != nil {
			fmt.Printf(err.Error())
			continue
		}
		go HandleBuf(conn, kvCli)
	}
}

func HandleBuf(conn net.Conn, cli *shardkvserver.KvClient) {
	defer conn.Close()
	for {
		var buf [10240]byte
		n, err := conn.Read(buf[:])
		if err != nil {
			break
		}
		req_buf := buf[:n]
		pos := 0
		param_len := 0
		param_idx := 0
		// fmt.Printf("red_buf %s \n", req_buf)
		for i := 0; i < len(req_buf); i++ {
			if req_buf[i] == '\r' && req_buf[i+1] == '\n' {
				pos = pos + 1
				if pos == 1 {
					{
						for j := range req_buf[1:i] {
							param_len *= 10
							param_len += j - 48
						}
						param_idx = i + 2
					}
				}
			}
		}
		// fmt.Printf("param_idx %d \n", param_idx)
		params_str := string(req_buf[param_idx:])
		params := strings.Split(params_str, "\r\n")
		// fmt.Printf("%v", params)
		if len(params) > 0 {
			if params[1] == "COMMAND" {
				conn.Write([]byte("+OK\r\n"))
			} else if strings.ToLower(params[1]) == "set" {
				if err := cli.Put(params[3], params[5]); err != nil {
					conn.Write([]byte("-" + err.Error() + "\r\n"))
				} else {
					conn.Write([]byte("+OK\r\n"))
				}
			} else if strings.ToLower(params[1]) == "get" {
				// TODO:
				v, err := cli.Get(params[3])
				if err != nil {
					conn.Write([]byte("-" + err.Error() + "\r\n"))
				} else {
					// "$6\r\nfoobar\r\n"
					conn.Write([]byte("$" + strconv.Itoa(len(v)) + "\r\n" + v + "\r\n"))
				}
			} else {
				conn.Write([]byte("-Unknow command\r\n"))
			}
		}
	}
}
