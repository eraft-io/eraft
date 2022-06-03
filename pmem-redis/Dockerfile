# FROM pmem/pmemkv:1.4-ubuntu-20.04
FROM registry.hub.docker.com/library/ubuntu:18.04
MAINTAINER colin@eraft.cn

# Set required environment variables
ENV OS ubuntu
ENV OS_VER 18.04
ENV PACKAGE_MANAGER deb
ENV NOTTY 1

COPY install-pmem-redis.sh install-pmem-redis.sh
RUN ./install-pmem-redis.sh
