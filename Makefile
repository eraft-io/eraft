all:
	@if [ ! -d "googletest" ]; then echo "googletest not present. Fetching googletest from internet..."; wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz; tar xzvf release-1.10.0.tar.gz; rm -f release-1.10.0.tar.gz; mv googletest-release-1.10.0 googletest; fi
