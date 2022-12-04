///
/// raft basic request vote  definition
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct RequestVoteRequest {
    #[prost(int64, tag="1")]
    pub term: i64,
    #[prost(int64, tag="2")]
    pub candidate_id: i64,
    #[prost(int64, tag="3")]
    pub last_log_index: i64,
    #[prost(int64, tag="4")]
    pub last_log_term: i64,
}
///
/// raft basic request vote response
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct RequestVoteResponse {
    #[prost(int64, tag="1")]
    pub term: i64,
    #[prost(bool, tag="2")]
    pub vote_granted: bool,
}
///
/// raft basic log entry definition
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct Entry {
    #[prost(enumeration="EntryType", tag="1")]
    pub entry_type: i32,
    #[prost(uint64, tag="2")]
    pub term: u64,
    #[prost(int64, tag="3")]
    pub index: i64,
    #[prost(bytes="vec", tag="4")]
    pub data: ::prost::alloc::vec::Vec<u8>,
}
/// 
/// raft basic append entries request definition
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct AppendEntriesRequest {
    #[prost(int64, tag="1")]
    pub term: i64,
    #[prost(int64, tag="2")]
    pub leader_id: i64,
    #[prost(int64, tag="3")]
    pub prev_log_index: i64,
    #[prost(int64, tag="4")]
    pub prev_log_term: i64,
    #[prost(int64, tag="5")]
    pub leader_commit: i64,
    #[prost(message, repeated, tag="6")]
    pub entries: ::prost::alloc::vec::Vec<Entry>,
}
///
/// raft basic append entries response definition
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct AppendEntriesResponse {
    #[prost(int64, tag="1")]
    pub term: i64,
    #[prost(bool, tag="2")]
    pub success: bool,
    #[prost(int64, tag="3")]
    pub conflict_index: i64,
    #[prost(int64, tag="4")]
    pub conflict_term: i64,
}
///
/// apply message definition
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct ApplyMsg {
    #[prost(bool, tag="1")]
    pub command_valid: bool,
    #[prost(bytes="vec", tag="2")]
    pub command: ::prost::alloc::vec::Vec<u8>,
    #[prost(int64, tag="3")]
    pub command_term: i64,
    #[prost(int64, tag="4")]
    pub command_index: i64,
    #[prost(bool, tag="5")]
    pub snapshot_valid: bool,
    #[prost(bytes="vec", tag="6")]
    pub snapshot: ::prost::alloc::vec::Vec<u8>,
    #[prost(int64, tag="7")]
    pub snapshot_term: i64,
    #[prost(int64, tag="8")]
    pub snapshot_index: i64,
}
///
/// client command request
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct CommandRequest {
    #[prost(string, tag="1")]
    pub key: ::prost::alloc::string::String,
    #[prost(string, tag="2")]
    pub value: ::prost::alloc::string::String,
    #[prost(enumeration="OpType", tag="3")]
    pub op_type: i32,
    #[prost(int64, tag="4")]
    pub client_id: i64,
    #[prost(int64, tag="5")]
    pub command_id: i64,
    #[prost(bytes="vec", tag="6")]
    pub context: ::prost::alloc::vec::Vec<u8>,
    #[prost(int64, tag="7")]
    pub offset: i64,
    #[prost(int64, tag="8")]
    pub limit: i64,
}
///
/// client command response
///
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct CommandResponse {
    #[prost(string, tag="1")]
    pub value: ::prost::alloc::string::String,
    #[prost(int64, tag="2")]
    pub leader_id: i64,
    #[prost(int64, tag="3")]
    pub err_code: i64,
    #[prost(string, repeated, tag="4")]
    pub match_keys: ::prost::alloc::vec::Vec<::prost::alloc::string::String>,
    #[prost(string, tag="5")]
    pub context: ::prost::alloc::string::String,
}
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct InstallSnapshotRequest {
    #[prost(int64, tag="1")]
    pub term: i64,
    #[prost(int64, tag="2")]
    pub leader_id: i64,
    #[prost(int64, tag="3")]
    pub last_included_index: i64,
    #[prost(int64, tag="4")]
    pub last_included_term: i64,
    #[prost(bytes="vec", tag="5")]
    pub data: ::prost::alloc::vec::Vec<u8>,
}
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct InstallSnapshotResponse {
    #[prost(int64, tag="1")]
    pub term: i64,
}
///
/// the log entry type
/// 1.normal -> like put, get key
/// 2.conf change -> cluster config change
///
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord, ::prost::Enumeration)]
#[repr(i32)]
pub enum EntryType {
    EntryNormal = 0,
    EntryConfChange = 1,
}
///
/// client op type
///
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord, ::prost::Enumeration)]
#[repr(i32)]
pub enum OpType {
    OpPut = 0,
    OpAppend = 1,
    OpGet = 2,
    OpScan = 3,
}
/// Generated client implementations.
pub mod raft_service_client {
    #![allow(unused_variables, dead_code, missing_docs, clippy::let_unit_value)]
    use tonic::codegen::*;
    #[derive(Debug, Clone)]
    pub struct RaftServiceClient<T> {
        inner: tonic::client::Grpc<T>,
    }
    impl RaftServiceClient<tonic::transport::Channel> {
        /// Attempt to create a new client by connecting to a given endpoint.
        pub async fn connect<D>(dst: D) -> Result<Self, tonic::transport::Error>
        where
            D: std::convert::TryInto<tonic::transport::Endpoint>,
            D::Error: Into<StdError>,
        {
            let conn = tonic::transport::Endpoint::new(dst)?.connect().await?;
            Ok(Self::new(conn))
        }
    }
    impl<T> RaftServiceClient<T>
    where
        T: tonic::client::GrpcService<tonic::body::BoxBody>,
        T::Error: Into<StdError>,
        T::ResponseBody: Body<Data = Bytes> + Send + 'static,
        <T::ResponseBody as Body>::Error: Into<StdError> + Send,
    {
        pub fn new(inner: T) -> Self {
            let inner = tonic::client::Grpc::new(inner);
            Self { inner }
        }
        pub fn with_interceptor<F>(
            inner: T,
            interceptor: F,
        ) -> RaftServiceClient<InterceptedService<T, F>>
        where
            F: tonic::service::Interceptor,
            T::ResponseBody: Default,
            T: tonic::codegen::Service<
                http::Request<tonic::body::BoxBody>,
                Response = http::Response<
                    <T as tonic::client::GrpcService<tonic::body::BoxBody>>::ResponseBody,
                >,
            >,
            <T as tonic::codegen::Service<
                http::Request<tonic::body::BoxBody>,
            >>::Error: Into<StdError> + Send + Sync,
        {
            RaftServiceClient::new(InterceptedService::new(inner, interceptor))
        }
        /// Compress requests with `gzip`.
        ///
        /// This requires the server to support it otherwise it might respond with an
        /// error.
        #[must_use]
        pub fn send_gzip(mut self) -> Self {
            self.inner = self.inner.send_gzip();
            self
        }
        /// Enable decompressing responses with `gzip`.
        #[must_use]
        pub fn accept_gzip(mut self) -> Self {
            self.inner = self.inner.accept_gzip();
            self
        }
        pub async fn request_vote(
            &mut self,
            request: impl tonic::IntoRequest<super::RequestVoteRequest>,
        ) -> Result<tonic::Response<super::RequestVoteResponse>, tonic::Status> {
            self.inner
                .ready()
                .await
                .map_err(|e| {
                    tonic::Status::new(
                        tonic::Code::Unknown,
                        format!("Service was not ready: {}", e.into()),
                    )
                })?;
            let codec = tonic::codec::ProstCodec::default();
            let path = http::uri::PathAndQuery::from_static(
                "/eraft_proto.RaftService/RequestVote",
            );
            self.inner.unary(request.into_request(), path, codec).await
        }
        pub async fn append_entries(
            &mut self,
            request: impl tonic::IntoRequest<super::AppendEntriesRequest>,
        ) -> Result<tonic::Response<super::AppendEntriesResponse>, tonic::Status> {
            self.inner
                .ready()
                .await
                .map_err(|e| {
                    tonic::Status::new(
                        tonic::Code::Unknown,
                        format!("Service was not ready: {}", e.into()),
                    )
                })?;
            let codec = tonic::codec::ProstCodec::default();
            let path = http::uri::PathAndQuery::from_static(
                "/eraft_proto.RaftService/AppendEntries",
            );
            self.inner.unary(request.into_request(), path, codec).await
        }
        pub async fn do_command(
            &mut self,
            request: impl tonic::IntoRequest<super::CommandRequest>,
        ) -> Result<tonic::Response<super::CommandResponse>, tonic::Status> {
            self.inner
                .ready()
                .await
                .map_err(|e| {
                    tonic::Status::new(
                        tonic::Code::Unknown,
                        format!("Service was not ready: {}", e.into()),
                    )
                })?;
            let codec = tonic::codec::ProstCodec::default();
            let path = http::uri::PathAndQuery::from_static(
                "/eraft_proto.RaftService/DoCommand",
            );
            self.inner.unary(request.into_request(), path, codec).await
        }
    }
}
/// Generated server implementations.
pub mod raft_service_server {
    #![allow(unused_variables, dead_code, missing_docs, clippy::let_unit_value)]
    use tonic::codegen::*;
    ///Generated trait containing gRPC methods that should be implemented for use with RaftServiceServer.
    #[async_trait]
    pub trait RaftService: Send + Sync + 'static {
        async fn request_vote(
            &self,
            request: tonic::Request<super::RequestVoteRequest>,
        ) -> Result<tonic::Response<super::RequestVoteResponse>, tonic::Status>;
        async fn append_entries(
            &self,
            request: tonic::Request<super::AppendEntriesRequest>,
        ) -> Result<tonic::Response<super::AppendEntriesResponse>, tonic::Status>;
        async fn do_command(
            &self,
            request: tonic::Request<super::CommandRequest>,
        ) -> Result<tonic::Response<super::CommandResponse>, tonic::Status>;
    }
    #[derive(Debug)]
    pub struct RaftServiceServer<T: RaftService> {
        inner: _Inner<T>,
        accept_compression_encodings: (),
        send_compression_encodings: (),
    }
    struct _Inner<T>(Arc<T>);
    impl<T: RaftService> RaftServiceServer<T> {
        pub fn new(inner: T) -> Self {
            Self::from_arc(Arc::new(inner))
        }
        pub fn from_arc(inner: Arc<T>) -> Self {
            let inner = _Inner(inner);
            Self {
                inner,
                accept_compression_encodings: Default::default(),
                send_compression_encodings: Default::default(),
            }
        }
        pub fn with_interceptor<F>(
            inner: T,
            interceptor: F,
        ) -> InterceptedService<Self, F>
        where
            F: tonic::service::Interceptor,
        {
            InterceptedService::new(Self::new(inner), interceptor)
        }
    }
    impl<T, B> tonic::codegen::Service<http::Request<B>> for RaftServiceServer<T>
    where
        T: RaftService,
        B: Body + Send + 'static,
        B::Error: Into<StdError> + Send + 'static,
    {
        type Response = http::Response<tonic::body::BoxBody>;
        type Error = std::convert::Infallible;
        type Future = BoxFuture<Self::Response, Self::Error>;
        fn poll_ready(
            &mut self,
            _cx: &mut Context<'_>,
        ) -> Poll<Result<(), Self::Error>> {
            Poll::Ready(Ok(()))
        }
        fn call(&mut self, req: http::Request<B>) -> Self::Future {
            let inner = self.inner.clone();
            match req.uri().path() {
                "/eraft_proto.RaftService/RequestVote" => {
                    #[allow(non_camel_case_types)]
                    struct RequestVoteSvc<T: RaftService>(pub Arc<T>);
                    impl<
                        T: RaftService,
                    > tonic::server::UnaryService<super::RequestVoteRequest>
                    for RequestVoteSvc<T> {
                        type Response = super::RequestVoteResponse;
                        type Future = BoxFuture<
                            tonic::Response<Self::Response>,
                            tonic::Status,
                        >;
                        fn call(
                            &mut self,
                            request: tonic::Request<super::RequestVoteRequest>,
                        ) -> Self::Future {
                            let inner = self.0.clone();
                            let fut = async move {
                                (*inner).request_vote(request).await
                            };
                            Box::pin(fut)
                        }
                    }
                    let accept_compression_encodings = self.accept_compression_encodings;
                    let send_compression_encodings = self.send_compression_encodings;
                    let inner = self.inner.clone();
                    let fut = async move {
                        let inner = inner.0;
                        let method = RequestVoteSvc(inner);
                        let codec = tonic::codec::ProstCodec::default();
                        let mut grpc = tonic::server::Grpc::new(codec)
                            .apply_compression_config(
                                accept_compression_encodings,
                                send_compression_encodings,
                            );
                        let res = grpc.unary(method, req).await;
                        Ok(res)
                    };
                    Box::pin(fut)
                }
                "/eraft_proto.RaftService/AppendEntries" => {
                    #[allow(non_camel_case_types)]
                    struct AppendEntriesSvc<T: RaftService>(pub Arc<T>);
                    impl<
                        T: RaftService,
                    > tonic::server::UnaryService<super::AppendEntriesRequest>
                    for AppendEntriesSvc<T> {
                        type Response = super::AppendEntriesResponse;
                        type Future = BoxFuture<
                            tonic::Response<Self::Response>,
                            tonic::Status,
                        >;
                        fn call(
                            &mut self,
                            request: tonic::Request<super::AppendEntriesRequest>,
                        ) -> Self::Future {
                            let inner = self.0.clone();
                            let fut = async move {
                                (*inner).append_entries(request).await
                            };
                            Box::pin(fut)
                        }
                    }
                    let accept_compression_encodings = self.accept_compression_encodings;
                    let send_compression_encodings = self.send_compression_encodings;
                    let inner = self.inner.clone();
                    let fut = async move {
                        let inner = inner.0;
                        let method = AppendEntriesSvc(inner);
                        let codec = tonic::codec::ProstCodec::default();
                        let mut grpc = tonic::server::Grpc::new(codec)
                            .apply_compression_config(
                                accept_compression_encodings,
                                send_compression_encodings,
                            );
                        let res = grpc.unary(method, req).await;
                        Ok(res)
                    };
                    Box::pin(fut)
                }
                "/eraft_proto.RaftService/DoCommand" => {
                    #[allow(non_camel_case_types)]
                    struct DoCommandSvc<T: RaftService>(pub Arc<T>);
                    impl<
                        T: RaftService,
                    > tonic::server::UnaryService<super::CommandRequest>
                    for DoCommandSvc<T> {
                        type Response = super::CommandResponse;
                        type Future = BoxFuture<
                            tonic::Response<Self::Response>,
                            tonic::Status,
                        >;
                        fn call(
                            &mut self,
                            request: tonic::Request<super::CommandRequest>,
                        ) -> Self::Future {
                            let inner = self.0.clone();
                            let fut = async move { (*inner).do_command(request).await };
                            Box::pin(fut)
                        }
                    }
                    let accept_compression_encodings = self.accept_compression_encodings;
                    let send_compression_encodings = self.send_compression_encodings;
                    let inner = self.inner.clone();
                    let fut = async move {
                        let inner = inner.0;
                        let method = DoCommandSvc(inner);
                        let codec = tonic::codec::ProstCodec::default();
                        let mut grpc = tonic::server::Grpc::new(codec)
                            .apply_compression_config(
                                accept_compression_encodings,
                                send_compression_encodings,
                            );
                        let res = grpc.unary(method, req).await;
                        Ok(res)
                    };
                    Box::pin(fut)
                }
                _ => {
                    Box::pin(async move {
                        Ok(
                            http::Response::builder()
                                .status(200)
                                .header("grpc-status", "12")
                                .header("content-type", "application/grpc")
                                .body(empty_body())
                                .unwrap(),
                        )
                    })
                }
            }
        }
    }
    impl<T: RaftService> Clone for RaftServiceServer<T> {
        fn clone(&self) -> Self {
            let inner = self.inner.clone();
            Self {
                inner,
                accept_compression_encodings: self.accept_compression_encodings,
                send_compression_encodings: self.send_compression_encodings,
            }
        }
    }
    impl<T: RaftService> Clone for _Inner<T> {
        fn clone(&self) -> Self {
            Self(self.0.clone())
        }
    }
    impl<T: std::fmt::Debug> std::fmt::Debug for _Inner<T> {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(f, "{:?}", self.0)
        }
    }
    impl<T: RaftService> tonic::transport::NamedService for RaftServiceServer<T> {
        const NAME: &'static str = "eraft_proto.RaftService";
    }
}
