wget https://studygolang.com/dl/golang/go1.15.4.linux-amd64.tar.gz
sudo tar -zxvf go1.15.4.linux-amd64.tar.gz -C /opt
mkdir ~/Go
echo "export GOPATH=~/Go" >>  ~/.zshrc 
echo "export GOROOT=/opt/go" >> ~/.zshrc 
echo "export GOTOOLS=$GOROOT/pkg/tool" >> ~/.zshrc
echo "export PATH=$PATH:$GOROOT/bin:$GOPATH/bin" >> ~/.zshrc
source ~/.zshrc


cd ~/Go
git clone https://github.com/yunxiao3/MyRaft.git
