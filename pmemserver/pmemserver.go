package pmemserver

import (
	"context"

	pmem_redis "github.com/eraft-io/eraft/go-redis-pmem/redis"
	pb "github.com/eraft-io/eraft/raftpb"
)

type PMemServer struct {
	db *pmem_redis.PMemDictStore
	pb.UnimplementedRaftServiceServer
}

func MakePMemServer(path string) *PMemServer {
	pmemSvr, err := pmem_redis.MakePMemDictStore(path)
	if err != nil {
		panic(err)
	}
	return &PMemServer{db: pmemSvr}
}

func (s *PMemServer) DoCommand(ctx context.Context, req *pb.CommandRequest) (*pb.CommandResponse, error) {
	cmdResp := &pb.CommandResponse{}
	// raftcore.PrintDebugLog(fmt.Sprintf("do cmd %s", req.String()))
	switch req.OpType {
	case pb.OpType_OpPut:
		if err := s.db.Put(req.Key, req.Value); err != nil {
			return nil, err
		}
	case pb.OpType_OpGet:
		err, val := s.db.Get(req.Key)
		if err != nil {
			return nil, err
		}
		cmdResp.Value = val
	}
	return cmdResp, nil
}

func (s *PMemServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	return nil, nil
}

func (s *PMemServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {

	return nil, nil
}
