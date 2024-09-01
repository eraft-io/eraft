FROM ubuntu:22.04

RUN apt-get update

RUN apt-get install make wget -y

RUN wget https://go.dev/dl/go1.23.0.linux-arm64.tar.gz -O go.tar.gz \
 && tar -xzvf go.tar.gz -C /usr/local
