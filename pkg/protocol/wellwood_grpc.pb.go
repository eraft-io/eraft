// Code generated by protoc-gen-go-grpc. DO NOT EDIT.
// versions:
// - protoc-gen-go-grpc v1.2.0
// - protoc             v3.19.4
// source: wellwood.proto

package protocol

import (
	context "context"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
)

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
// Requires gRPC-Go v1.32.0 or later.
const _ = grpc.SupportPackageIsVersion7

// MetaServiceClient is the client API for MetaService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://pkg.go.dev/google.golang.org/grpc/?tab=doc#ClientConn.NewStream.
type MetaServiceClient interface {
	// for multi-raft server group config manage
	ServerGroupMeta(ctx context.Context, in *ServerGroupMetaConfigRequest, opts ...grpc.CallOption) (*ServerGroupMetaConfigResponse, error)
}

type metaServiceClient struct {
	cc grpc.ClientConnInterface
}

func NewMetaServiceClient(cc grpc.ClientConnInterface) MetaServiceClient {
	return &metaServiceClient{cc}
}

func (c *metaServiceClient) ServerGroupMeta(ctx context.Context, in *ServerGroupMetaConfigRequest, opts ...grpc.CallOption) (*ServerGroupMetaConfigResponse, error) {
	out := new(ServerGroupMetaConfigResponse)
	err := c.cc.Invoke(ctx, "/protocol.MetaService/ServerGroupMeta", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// MetaServiceServer is the server API for MetaService service.
// All implementations must embed UnimplementedMetaServiceServer
// for forward compatibility
type MetaServiceServer interface {
	// for multi-raft server group config manage
	ServerGroupMeta(context.Context, *ServerGroupMetaConfigRequest) (*ServerGroupMetaConfigResponse, error)
	mustEmbedUnimplementedMetaServiceServer()
}

// UnimplementedMetaServiceServer must be embedded to have forward compatible implementations.
type UnimplementedMetaServiceServer struct {
}

func (UnimplementedMetaServiceServer) ServerGroupMeta(context.Context, *ServerGroupMetaConfigRequest) (*ServerGroupMetaConfigResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method ServerGroupMeta not implemented")
}
func (UnimplementedMetaServiceServer) mustEmbedUnimplementedMetaServiceServer() {}

// UnsafeMetaServiceServer may be embedded to opt out of forward compatibility for this service.
// Use of this interface is not recommended, as added methods to MetaServiceServer will
// result in compilation errors.
type UnsafeMetaServiceServer interface {
	mustEmbedUnimplementedMetaServiceServer()
}

func RegisterMetaServiceServer(s grpc.ServiceRegistrar, srv MetaServiceServer) {
	s.RegisterService(&MetaService_ServiceDesc, srv)
}

func _MetaService_ServerGroupMeta_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ServerGroupMetaConfigRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetaServiceServer).ServerGroupMeta(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/protocol.MetaService/ServerGroupMeta",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetaServiceServer).ServerGroupMeta(ctx, req.(*ServerGroupMetaConfigRequest))
	}
	return interceptor(ctx, in, info, handler)
}

// MetaService_ServiceDesc is the grpc.ServiceDesc for MetaService service.
// It's only intended for direct use with grpc.RegisterService,
// and not to be introspected or modified (even as a copy)
var MetaService_ServiceDesc = grpc.ServiceDesc{
	ServiceName: "protocol.MetaService",
	HandlerType: (*MetaServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "ServerGroupMeta",
			Handler:    _MetaService_ServerGroupMeta_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "wellwood.proto",
}

// FileBlockServiceClient is the client API for FileBlockService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://pkg.go.dev/google.golang.org/grpc/?tab=doc#ClientConn.NewStream.
type FileBlockServiceClient interface {
	FileBlockOp(ctx context.Context, in *FileBlockOpRequest, opts ...grpc.CallOption) (*FileBlockOpResponse, error)
}

type fileBlockServiceClient struct {
	cc grpc.ClientConnInterface
}

func NewFileBlockServiceClient(cc grpc.ClientConnInterface) FileBlockServiceClient {
	return &fileBlockServiceClient{cc}
}

func (c *fileBlockServiceClient) FileBlockOp(ctx context.Context, in *FileBlockOpRequest, opts ...grpc.CallOption) (*FileBlockOpResponse, error) {
	out := new(FileBlockOpResponse)
	err := c.cc.Invoke(ctx, "/protocol.FileBlockService/FileBlockOp", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// FileBlockServiceServer is the server API for FileBlockService service.
// All implementations must embed UnimplementedFileBlockServiceServer
// for forward compatibility
type FileBlockServiceServer interface {
	FileBlockOp(context.Context, *FileBlockOpRequest) (*FileBlockOpResponse, error)
	mustEmbedUnimplementedFileBlockServiceServer()
}

// UnimplementedFileBlockServiceServer must be embedded to have forward compatible implementations.
type UnimplementedFileBlockServiceServer struct {
}

func (UnimplementedFileBlockServiceServer) FileBlockOp(context.Context, *FileBlockOpRequest) (*FileBlockOpResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method FileBlockOp not implemented")
}
func (UnimplementedFileBlockServiceServer) mustEmbedUnimplementedFileBlockServiceServer() {}

// UnsafeFileBlockServiceServer may be embedded to opt out of forward compatibility for this service.
// Use of this interface is not recommended, as added methods to FileBlockServiceServer will
// result in compilation errors.
type UnsafeFileBlockServiceServer interface {
	mustEmbedUnimplementedFileBlockServiceServer()
}

func RegisterFileBlockServiceServer(s grpc.ServiceRegistrar, srv FileBlockServiceServer) {
	s.RegisterService(&FileBlockService_ServiceDesc, srv)
}

func _FileBlockService_FileBlockOp_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(FileBlockOpRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(FileBlockServiceServer).FileBlockOp(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/protocol.FileBlockService/FileBlockOp",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(FileBlockServiceServer).FileBlockOp(ctx, req.(*FileBlockOpRequest))
	}
	return interceptor(ctx, in, info, handler)
}

// FileBlockService_ServiceDesc is the grpc.ServiceDesc for FileBlockService service.
// It's only intended for direct use with grpc.RegisterService,
// and not to be introspected or modified (even as a copy)
var FileBlockService_ServiceDesc = grpc.ServiceDesc{
	ServiceName: "protocol.FileBlockService",
	HandlerType: (*FileBlockServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "FileBlockOp",
			Handler:    _FileBlockService_FileBlockOp_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "wellwood.proto",
}
