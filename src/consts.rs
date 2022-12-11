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

pub static DBS_KEY_PREFIX: &'static str = "/catalog/db/";

pub static TBS_KEY_PREFIX: &'static str = "/catalog/tab/";

pub static INIT_RAFTLOG_IDX: u64 = 0;

use lazy_static::lazy_static;
lazy_static! {
   pub static ref LOG_META_KEY: Vec<u8> = vec![0x77, 0x77];
   pub static ref FIRST_LOG_IDX_KEY: Vec<u8> =  vec![0x88, 0x88];
   pub static ref LAST_LOG_IDX_KEY: Vec<u8> = vec![0x99, 0x99];
   pub static ref RAFT_STATE_KEY: Vec<u8> = vec![0x20, 0x22];
   pub static ref RAFT_LOG_PREFIX: Vec<u8> = vec![0x11, 0x11, 0x88, 0x88];
}


pub enum ErrorCode {
    ErrorNone,
    ErrorWrongLeader,
    ErrorTimeOut,
}