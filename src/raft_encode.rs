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

use prost::bytes::BufMut;

use crate::eraft_proto::{Entry};
use crate::raft_persis_state;
use crate::consts;


pub enum EncodeMode {
    Json,
}

pub fn encode_entry (ent: Entry, mode: EncodeMode) -> Vec<u8> {
    let serialized_entry_meta = serde_json::to_vec(&ent).unwrap();
    serialized_entry_meta
}

pub fn decode_entry (data : Vec<u8>) -> Entry {
    let ent = serde_json::from_slice(&data).unwrap();
    ent
}

pub fn encode_raft_persisten_state(pst: raft_persis_state::RaftPersistenState, mode: EncodeMode) -> Vec<u8> {
    let serialized_pst = serde_json::to_vec(&pst).unwrap();
    serialized_pst
}

pub fn decode_raft_persisten_state (data : Vec<u8>) -> raft_persis_state::RaftPersistenState {
    let pst = serde_json::from_slice(&data).unwrap();
    pst
}

pub fn encode_raftlog_persisten_state(lpst: raft_persis_state::RaftLogPersistenState, mode: EncodeMode) -> Vec<u8> {
    let serialized_lpst = serde_json::to_vec(&lpst).unwrap();
    serialized_lpst
}

pub fn decode_raftlog_persisten_state (data : Vec<u8>) -> raft_persis_state::RaftLogPersistenState {
    let lpst = serde_json::from_slice(&data).unwrap();
    lpst
}

pub fn encode_raftlog_entry_key(idx: u64) -> Vec<u8> {
    let mut seq: Vec<u8> = vec![];
    for b in &*consts::RAFT_LOG_PREFIX {
        seq.push(*b);
    }
    seq.put_u64(idx);
    seq
}


pub fn decode_raftlog_entry_key(seq: Vec<u8>) -> u64 {
    u64::from_be_bytes(seq[4..].try_into().unwrap())
}

#[cfg(test)]
mod raft_encode_tests {

    use crate::eraft_proto::{Entry, EntryType};
    use super::{EncodeMode, encode_entry, decode_entry, encode_raftlog_entry_key, decode_raftlog_entry_key};

    #[test]
    fn test_encode_entry() {
        let ent = Entry{
            index: 1,
            term : 1,
            entry_type: EntryType::EntryNormal as i32,
            data: vec![],
        };
        let seq = encode_entry(ent, EncodeMode::Json);
        assert_eq!(vec![123, 34, 101, 110, 116, 114, 121, 95, 116, 121, 112, 101, 34, 58, 48, 44, 34, 116, 101, 114, 109, 34, 58, 49, 44, 34, 105, 110, 100, 101, 120, 34, 58, 49, 44, 34, 100, 97, 116, 97, 34, 58, 91, 93, 125], seq);
    }

    #[test]
    fn test_decode_entry() {
        let seq = vec![123, 34, 101, 110, 116, 114, 121, 95, 116, 121, 112, 101, 34, 58, 48, 44, 34, 116, 101, 114, 109, 34, 58, 49, 44, 34, 105, 110, 100, 101, 120, 34, 58, 49, 44, 34, 100, 97, 116, 97, 34, 58, 91, 93, 125];
        let ent = decode_entry(seq);
        assert_eq!(ent.index, 1);
        assert_eq!(ent.term, 1);
        assert_eq!(ent.entry_type, EntryType::EntryNormal as i32);
    }

    #[test]
    fn test_encode_raftlog_key() {
        assert_eq!(vec![17, 17, 136, 136, 0, 0, 0, 0, 0, 0, 0, 64], encode_raftlog_entry_key(64));
    }

    #[test]
    fn test_decode_raftlog_key() {
        assert_eq!(64, decode_raftlog_entry_key(vec![17, 17, 136, 136, 0, 0, 0, 0, 0, 0, 0, 64]));
    }


}