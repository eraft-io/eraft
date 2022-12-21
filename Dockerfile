FROM ubuntu:18.04

RUN apt update -y \
    && apt install -y cmake ccache libssl-dev libcrypto++-dev \
        libglib2.0-dev libltdl-dev libicu-dev libmysql++-dev \
        libreadline-dev libmysqlclient-dev unixodbc-dev \
        unixodbc-dev devscripts dupload fakeroot debhelper \
        gcc-7 g++-7 unixodbc-dev devscripts dupload fakeroot debhelper \
        liblld-5.0-dev libclang-5.0-dev liblld-5.0 \
        build-essential autoconf libtool pkg-config \
        libgflags-dev libgtest-dev -y

RUN apt install bison flex -y

RUN apt install mariadb-server -y
