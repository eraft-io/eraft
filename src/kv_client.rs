use crate::{eraft_proto};
use eraft_proto::raft_service_client::RaftServiceClient;
use eraft_proto::{CommandRequest, CommandResponse, OpType};

pub async fn send_command(target: String, op: OpType, k: &str, v: &str) -> Result<tonic::Response<CommandResponse>, Box<dyn std::error::Error>> {
    let mut cli = RaftServiceClient::connect(target).await?;
    let request = tonic::Request::new(CommandRequest {
        key: String::from(k),
        value: String::from(v),
        op_type: op as i32,
        client_id: 999,
        command_id: 999,
        context: vec![0],
        limit: 0,
        offset: 0,
    });
    let resp = cli.do_command(request).await?;
    Ok(resp)
}