cd $GOPATH/src/


scp root@116.62.139.165:/root/Go/src/github.com.tar.gz ./
scp root@116.62.139.165:/root/Go/src/google.golang.org.tar.gz ./
scp root@116.62.139.165:/root/Go/src/golang.org.tar.gz ./
tar -xvf github.com.tar.gz  

tar -xvf golang.org.tar.gz
tar -xvf google.golang.org.tar.gz

go install google.golang.org/grpc

