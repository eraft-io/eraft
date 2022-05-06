docker run -it \
                --net eraft-network \
                --name eraft-1 \
                --hostname eraft-1 \
                --ip 172.19.0.11 \
                -v ${PWD}:/eraft \
                -w /eraft \
                -p 20160:20160 \
                hub.docker.com/eraftio/eraft:latest 
                
                /eraft/build_/cmd/kvserver/kv_svr 172.19.0.11:20160 /tmp/db1 1

docker run -it \
                --net eraft-network \
                --name eraft-2 \
                --hostname eraft-2 \
                --ip 172.19.0.12 \
                -v ${PWD}:/eraft \
                -w /eraft \
                -p 20161:20161 \
                hub.docker.com/eraftio/eraft:latest 

                /eraft/build_/cmd/kvserver/kv_svr 172.19.0.12:20161 /tmp/db2 2

docker run -it \
                --net eraft-network \
                --name eraft-3 \
                --hostname eraft-3 \
                --ip 172.19.0.13 \
                -v ${PWD}:/eraft \
                -w /eraft \
                -p 20162:20162 \
                hub.docker.com/eraftio/eraft:latest 

                /eraft/build_/cmd/kvserver/kv_svr 172.19.0.13:20162 /tmp/db3 3


docker run -it \
                --net eraft-network \
                --name eraft-4 \
                --hostname eraft-4 \
                --ip 172.19.0.14 \
                -v ${PWD}:/eraft \
                -w /eraft \
                -p 20163:20163 \
                hub.docker.com/eraftio/eraft:latest 

                /eraft/build_/cmd/kvserver/kv_svr 172.19.0.14:20163 /tmp/db4 4

./kv_cli  172.19.0.11:20160 put test_cf test_key test_value 1

./kv_cli  172.19.0.11:20160 get test_cf test_key
