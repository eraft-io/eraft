#include <Kv/engines.h>
#include <Kv/file.h>


namespace kvserver
{

    Engines::Engines(std::string kvPath, std::string raftPath) {
        leveldb::Options opts;
        opts.create_if_missing = true;
        leveldb::Status s1 = leveldb::DB::Open(opts, kvPath, &kvDB_);
        assert(s1.ok());
        leveldb::Options opts1;
        opts1.create_if_missing = true;
        leveldb::Status s2 = leveldb::DB::Open(opts1, raftPath, &raftDB_);
        assert(s2.ok());
        this->kvPath_ = kvPath;
        this->raftPath_ = raftPath;
    }

    Engines::~Engines() {
        this->Close();
    }

    bool Engines::Close() {
        delete kvDB_;
        delete raftDB_;        
    }

    bool Engines::Destory() {
        File::DeleteDirectory(this->kvPath_);
        File::DeleteDirectory(this->raftPath_);
    }

    bool Engines::WriteKV(leveldb::WriteBatch& batch) {
        leveldb::Status s = kvDB_->Write(leveldb::WriteOptions(), &batch);
        assert(s.ok());
    }

    bool Engines::WriteRaft(leveldb::WriteBatch& batch) {
        leveldb::Status s = raftDB_->Write(leveldb::WriteOptions(), &batch);
        assert(s.ok());
    }

} // namespace kvserver
