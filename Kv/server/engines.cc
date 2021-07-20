#include <Kv/engines.h>
#include <Kv/file.h>


namespace kvserver
{

    Engines::Engines(std::string kvPath, std::string raftPath) {
        rocksdb::Options opts;
        opts.create_if_missing = true;
        rocksdb::Status s1 = rocksdb::DB::Open(opts, kvPath, &kvDB_);
        assert(s1.ok());
        rocksdb::Options opts1;
        opts1.create_if_missing = true;
        rocksdb::Status s2 = rocksdb::DB::Open(opts1, raftPath, &raftDB_);
        assert(s2.ok());
        this->kvPath_ = kvPath;
        this->raftPath_ = raftPath;
    }

    Engines::~Engines() {
        // this->Close();
    }

    bool Engines::Close() {
        delete kvDB_;
        delete raftDB_;     
    }

    bool Engines::Destory() {
        File::DeleteDirectory(this->kvPath_);
        File::DeleteDirectory(this->raftPath_);
    }

    bool Engines::WriteKV(rocksdb::WriteBatch& batch) {
        rocksdb::Status s = kvDB_->Write(rocksdb::WriteOptions(), &batch);
        assert(s.ok());
    }

    bool Engines::WriteRaft(rocksdb::WriteBatch& batch) {
        rocksdb::Status s = raftDB_->Write(rocksdb::WriteOptions(), &batch);
        assert(s.ok());
    }

} // namespace kvserver
