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

use lazy_static::lazy_static;
use std::sync::{Mutex};

use crate::eraft_proto::{Entry, EntryType};

lazy_static! {
    static ref RAFT_LOG_LOCK: Mutex<u8> = Mutex::new(6);
}


#[derive(Default, Debug, Clone)]
pub struct RaftMemLog {
   ents:  Vec<Entry>,
}

//
// build a new raft log
//
fn build_raft_log() -> RaftMemLog {
    let mut log_ents : Vec<Entry> = Vec::new();
    let empty_ent = Entry{
        entry_type: EntryType::EntryNormal as i32,
        term: 0,
        index: 0,
        data: vec![],
    }; 
    log_ents.push(empty_ent);
    RaftMemLog { 
        ents:     log_ents, 
    }
}

impl RaftMemLog {
    
    pub fn new() -> RaftMemLog {
        build_raft_log()
    }
    
    pub fn get_first(&self) -> &Entry {
        let ent = self.ents.get(0).unwrap();
        ent
    }

    pub fn size(&self) -> usize  {
        self.ents.len()
    }

    pub fn erase_before(&self, idx: i64) -> Vec<Entry> {
        self.ents.as_slice()[idx as usize..].to_vec()
    }

    pub fn erase_after(&self, idx: i64) -> Vec<Entry> {
        self.ents.as_slice()[..idx as usize].to_vec()
    }
 
    pub fn get_range(&self, start: usize, end: usize) -> Vec<Entry> {
        self.ents.as_slice()[start..end].to_vec()
    }

    pub fn append(&mut self, new_ent: Entry) {
        self.ents.push(new_ent)
    }

    pub fn get_entry(&self, idx: i64) -> &Entry {
        let ent = self.ents.get(idx as usize).unwrap();
        ent
    }

    pub fn get_last(&self) -> &Entry {
        let ent = self.ents.get(self.ents.len() - 1).unwrap();
        ent
    }

}

#[cfg(test)]
mod raftlog_tests {
    use crate::eraft_proto::{EntryType, Entry};

    use super::RaftMemLog;

    #[test]
    fn test_build_raft_log() {
        let raft_log = RaftMemLog::new();
        let frist_log = raft_log.get_first();
        assert_eq!(frist_log.index, 0);
        assert_eq!(frist_log.entry_type, EntryType::EntryNormal as i32);
        assert_eq!(raft_log.size(), 1);
    }

    #[test]
    fn test_append_and_get_log() {
        let ent = Entry{
            index: 1,
            term : 1,
            entry_type: EntryType::EntryNormal as i32,
            data: vec![],
        };
        let mut raft_log = RaftMemLog::new();
        // test append a log
        raft_log.append(ent);
        assert_eq!(raft_log.size(), 2);
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
        raft_log.append(ent1);
        assert_eq!(raft_log.size(), 3);
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
        let mut raft_log = RaftMemLog::new();
        // test append a log
        raft_log.append(ent);
        raft_log.append(ent1);
        raft_log.append(ent2);
        // 
        // now four logs with ids [0, 1, 2, 3] in raft_log
        //
        
        // test erase before
        let after_erase_before_ents = raft_log.erase_before(2);
        assert_eq!(after_erase_before_ents[0].index, 2);
        assert_eq!(after_erase_before_ents[0].term, 1);

        // test erase after
        let after_erase_after_ents =  raft_log.erase_after(2);
        assert_eq!(after_erase_after_ents[after_erase_after_ents.len()-1].index, 1);
        assert_eq!(after_erase_after_ents[after_erase_after_ents.len()-1].term, 1);

    }


}