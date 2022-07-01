use crate::{eraft_proto};
use eraft_proto::raft_service_client::{RaftServiceClient};
use eraft_proto::{CommandRequest, CommandResponse, OpType};

#[tokio::main]
pub async fn send_command() -> Result<(), Box<dyn std::error::Error>> {
    let mut client = RaftServiceClient::connect("http://[::1]:8088").await?;
    let request = tonic::Request::new(CommandRequest {
        key: "testkey".into(),
        value: "testvalue".into(),
        op_type: OpType::OpPut as i32,
        client_id: 999,
        command_id: 999,
        context: vec![0],
    });
    println!("Sending request to gRPC Server...");
    let response = client.do_command(request).await?;
    println!("RESPONSE={:?}", response);
    Ok(())
}