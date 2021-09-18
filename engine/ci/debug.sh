docker run -it \
                --net eraft-network \
                --name eraft-engine \
                --hostname eraft-engine \
                --ip 172.19.0.18 \
                -v ${PWD}:/engine \
                -p 20160:20160 \
                mhz88/corundum:latest
 