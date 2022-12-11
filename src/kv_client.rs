//  MIT License

//  Copyright (c) 2022 eraft dev group

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:

//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.

//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

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
