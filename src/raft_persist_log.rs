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

use rocksdb::DB;
use crate::eraft_proto::{Entry, EntryType};
use crate::raft_encode::{encode_raftlog_entry_key, decode_entry};
use crate::{raft_encode, consts, raft_persis_state};


pub struct RaftPersistLog {
    first_idx: u64,
    last_idx: u64,
    snapshot_idx: u64,
    db_ptr: Box<DB>,
}

fn build_persist_raft_log(node_id: i64, db_name: String) -> RaftPersistLog {
    let db = Box::new(DB::open_default(format!("./{}_{}", db_name, node_id)).unwrap());
    let empty_ent = Entry {
        entry_type: EntryType::EntryNormal as i32,
        term: 0,
        index: 0,
        data: vec![],
    };
    let empty_ent_encoded = raft_encode::encode_entry(empty_ent, raft_encode::EncodeMode::Json);
    let empty_log_key_encoded = raft_encode::encode_raftlog_entry_key(consts::INIT_RAFTLOG_IDX);
    db.put(empty_log_key_encoded, empty_ent_encoded).unwrap();
    RaftPersistLog { first_idx: 0, last_idx: 0, snapshot_idx: 0, db_ptr: (db) }
}

impl RaftPersistLog {

    pub fn new(node_id: i64, db_name: String) -> RaftPersistLog {
        build_persist_raft_log(node_id, db_name)
    }

    pub fn get_first(&self) -> Entry {
        let encode_log_key = encode_raftlog_entry_key(self.first_idx);
        let encode_val = self.db_ptr.get(encode_log_key).unwrap();
        decode_entry(encode_val.unwrap())
    }

    pub fn get_first_idx(&self) -> u64 {
        self.first_idx
    }

    pub fn item_count(&self) -> u64 {
        self.last_idx - self.first_idx + 1
    }

    pub fn erase_before(&mut self, split_idx: u64) {
        let pre_first_idx = self.first_idx;
        self.first_idx = split_idx;
        for idx in pre_first_idx..split_idx {
            let encode_log_key = encode_raftlog_entry_key(idx);
            self.db_ptr.delete(encode_log_key).unwrap();
        }
    }

    pub fn erase_range(&mut self, start_idx: u64, end_idx: u64) {
        for idx in start_idx..end_idx {
            let encode_log_key = encode_raftlog_entry_key(idx);
            self.db_ptr.delete(encode_log_key).unwrap();
        }
    }

    pub fn erase_after(&mut self, split_idx: u64) {
        for idx in split_idx..self.last_idx+1 {
            let encode_log_key = encode_raftlog_entry_key(idx);
            self.db_ptr.delete(encode_log_key).unwrap();
        }
        self.last_idx = split_idx - 1
    }

    pub fn get_entry(&self, idx: u64) -> Box<Entry> {
        let encode_log_key = encode_raftlog_entry_key(idx);
        let encode_val = self.db_ptr.get(encode_log_key).unwrap();
        Box::new(decode_entry(encode_val.unwrap()))
    }

    pub fn get_range(&self, lo: u64, hi: u64) -> Vec<Box<Entry>> {
        let mut ents: Vec<Box<Entry>> = vec![];
        for idx in lo..hi+1 {
            ents.push(self.get_entry(idx))
        }
        ents
    }

    pub fn get_before(&self, split_idx: u64) -> Vec<Box<Entry>> {
        self.get_range(self.first_idx, split_idx)
    }

    pub fn get_after(&self, split_idx: u64) -> Vec<Box<Entry>> {
        self.get_range(split_idx, self.last_idx)
    }

    pub fn append(&mut self, new_ent: Box<Entry>) {
        let ent_index = new_ent.index as u64;
        let encode_log_key = encode_raftlog_entry_key(ent_index);
        let empty_ent_encoded = raft_encode::encode_entry(*new_ent, raft_encode::EncodeMode::Json);
        self.db_ptr.put(encode_log_key, empty_ent_encoded).unwrap();
        self.last_idx = ent_index;
    }

    pub fn get_last(&self) -> Box<Entry> {
        self.get_entry(self.last_idx)
    }

    pub fn get_snapshot_entry(&self) -> Box<Entry> {
        self.get_entry(self.snapshot_idx)
    }

    pub fn set_snapshot_idx(&mut self, snap_idx: u64) {
        self.snapshot_idx = snap_idx;
    }

    pub fn write_raftlog_persisten_state(&self, commit: u64, apply_idx_: u64) {
        let raft_log_state = raft_persis_state::RaftLogPersistenState {
            commit_idx: commit,
            apply_idx: apply_idx_,
            first_idx: self.first_idx,
            last_idx: self.last_idx,
            snap_idx: self.snapshot_idx,
        };
        let log_state_encoded = raft_encode::encode_raftlog_persisten_state(raft_log_state, raft_encode::EncodeMode::Json);
        let log_key_encoded: Vec<u8> =(*consts::LOG_META_KEY).clone();
        self.db_ptr.put(log_key_encoded, log_state_encoded).unwrap();
    }

    //
    //
    // return (commit_idx, apply_idx)
    pub fn read_raftlog_persisten_state(&mut self) -> (u64, u64) {
        let log_key_encoded: Vec<u8> =(*consts::LOG_META_KEY).clone();
        let seq = self.db_ptr.get(log_key_encoded).unwrap();
        let lpst = raft_encode::decode_raftlog_persisten_state(seq.unwrap());
        self.first_idx = lpst.first_idx;
        self.last_idx = lpst.last_idx;
        self.snapshot_idx = lpst.snap_idx;
        (lpst.commit_idx, lpst.apply_idx)
    }

}

#[cfg(test)]
mod raft_persistlog_tests {
    use crate::eraft_proto::{EntryType, Entry};
    use super::RaftPersistLog;

    #[test]
    fn test_build_raft_log() {
        let raft_log = RaftPersistLog::new(0, String::from("LOG_DB"));
        let frist_log = raft_log.get_first();
        assert_eq!(frist_log.index, 0);
        assert_eq!(frist_log.entry_type, EntryType::EntryNormal as i32);
        assert_eq!(raft_log.item_count(), 1);
    }

    #[test]
    fn test_append_and_get_log() {
        let ent = Entry{
            index: 1,
            term : 1,
            entry_type: EntryType::EntryNormal as i32,
            data: vec![],
        };
        let mut raft_log = RaftPersistLog::new(0, String::from("LOG_DB"));
        // test append a log
        raft_log.append(Box::new(ent));
        assert_eq!(raft_log.item_count(), 2);
        let first_log = raft_log.get_first();
        assert_eq!(first_log.index, 0);
        assert_eq!(first_log.term, 0);
        let last_log = raft_log.get_last();
        assert_eq!(last_log.index, 1);
        assert_eq!(last_log.term, 1);
        // test get a entry
        let log_with_id_1 = raft_log.get_entry(1);
        assert_eq!(log_with_id_1.index, 1);
        assert_eq!(log_with_id_1.term, 1);
        // test get range
        let ent1 = Entry{
            index: 2,
            term : 1,
            entry_type: EntryType::EntryConfChange as i32,
            data: vec![],
        };
        raft_log.append(Box::new(ent1));
        assert_eq!(raft_log.item_count(), 3);
        let range_ents = raft_log.get_range(1, 2);
        assert_eq!(range_ents[0].index, 1);
        assert_eq!(range_ents[0].term, 1);
    }

    #[test]
    fn test_erase_before_or_after() {
        let ent = Entry{
            index: 1,
            term : 1,
            entry_type: EntryType::EntryNormal as i32,
            data: vec![],
        };
        let ent1 = Entry{
            index: 2,
            term : 1,
            entry_type: EntryType::EntryNormal as i32,
            data: vec![],
        };
        let ent2 = Entry{
            index: 3,
            term : 1,
            entry_type: EntryType::EntryConfChange as i32,
            data: vec![],
        };
        let mut raft_log = RaftPersistLog::new(0, String::from("LOG_DB"));
        // test append a log
        raft_log.append(Box::new(ent));
        raft_log.append(Box::new(ent1));
        raft_log.append(Box::new(ent2));
        // 
        // now four logs with ids [0, 1, 2, 3] in raft_log
        //

        // test erase before
        raft_log.erase_before(2);
        assert_eq!(2, raft_log.get_first().index);

        // test erase after
        raft_log.erase_after(3);
        assert_eq!(2, raft_log.get_last().index);
        
    }

}