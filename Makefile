deps:
	@if [ ! -d "googletest" ]; then echo "googletest not present. Fetching googletest from internet..."; wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz; tar xzvf release-1.10.0.tar.gz; rm -f release-1.10.0.tar.gz; mv googletest-release-1.10.0 googletest; fi
	@if [ ! -d "rocksdb" ]; then echo "rocksdb not present. Fetching rocksdb v6.3.6 from internet..."; curl -s -L -O https://github.com/facebook/rocksdb/archive/v6.3.6.tar.gz; tar xzvf v6.3.6.tar.gz; rm -f v6.3.6.tar.gz; mv rocksdb-6.3.6 rocksdb; fi
	cd rocksdb && make static_lib && cd -
