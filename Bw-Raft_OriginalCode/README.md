# 1. Set up the environment 

### Install protobuf

**Method 1**

~~~shell
go get -u github.com/golang/protobuf/proto
go get -u github.com/golang/protobuf/protoc-gen-go
~~~

**Method 2:  (Recommend)**

~~~shell
git clone https://github.com/golang/protobuf.git $GOPATH/src/github.com/golang/protobuf
cd $GOPATH/src/github.com/golang/protobuf
go install ./proto
go install ./protoc-gen-go
~~~

###  Install GRPC

~~~shell
git clone https://github.com/golang/net.git $GOPATH/src/golang.org/x/net
git clone https://github.com/golang/text.git $GOPATH/src/golang.org/x/text
git clone https://github.com/google/go-genproto.git $GOPATH/src/google.golang.org/genproto
git clone https://github.com/grpc/grpc-go.git $GOPATH/src/google.golang.org/grpc
cd $GOPATH/src/
go install google.golang.org/grpc
~~~

# 2. Start a Raft and BW-raft cluster

## Raft 

**Run the following commands in the mykvraft directory to start a raft cluster **

~~~shell
 # Start a raft  cluster consisting of 3 nodes
 go run myserver.go -address Ip0:port -members  Ip1:port,Ip2:port 
 go run myserver.go -address Ip1:port -members  Ip0:port,Ip2:port
 go run myserver.go -address Ip2:port -members  Ip1:port,Ip0:port
 -delay # No need to set up on the cloud server, but need to be set up on the local server
 # Start a raft client
 go run myclient.go -servers  Ip0:port,Ip1:port,Ip2:port 
		-mode write #benchmark mode  
		-cnums 1000 # clientNums 
		-onums 1000 #option Nums
~~~

## BW-Raft

**Run the following commands in the geokvraft directory to start a bw-raft cluster** 

~~~shell
# using the same commands to start 5 raft nodes Ip0-Ip4
go run myserver.go \
-address Ip0:port \
-members  Ip1:port,Ip2:port,Ip3:port,Ip4:port \
-observers Ip5:port\
-secretaries Ip6:port\
-secmembers Ip0:port,Ip1:port # The followers whose log is replicated by the secretary
 -delay # No need to set up on the cloud server, but need to be set up on the local server
 
# Start observers, In our experiments, raft nodes that cannot be elected are usually used as observers
go run observer.go -address Ip5:port
# Start secretaries
go run mysecretary.go -address Ip6:port -members  Ip0:port,Ip1:port
# Start client
go run myclient.go -servers Ip0-Ip4:port
~~~

# 3. YSCB Benchmark 

The yscb's test code can't be found now, You can refer to the official tutorial below to complete a test interface very quickly.

https://github.com/brianfrankcooper/YCSB/wiki/Adding-a-Database





We refactored the code, if you have any questions, you can email me: yunxiao.du@zju.edu.cn