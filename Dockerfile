# FROM pmem/pmemkv:1.4-ubuntu-20.04
FROM registry.hub.docker.com/library/ubuntu:18.04
MAINTAINER eraft_group@eraft.cn

# Set required environment variables
ENV OS ubuntu
ENV OS_VER 18.04
ENV PACKAGE_MANAGER deb
ENV NOTTY 1

# Additional parameters to build docker without building components
ARG SKIP_VALGRIND_BUILD
ARG SKIP_PMDK_BUILD
ARG SKIP_LIBPMEMOBJCPP_BUILD

# Base development packages
ARG BASE_DEPS="\
	cmake \
	build-essential \
	git"

# Dependencies for compiling pmemkv project
ARG PMEMKV_DEPS="\
	libtbb-dev \
	rapidjson-dev"

# ndctl's dependencies
ARG NDCTL_DEPS="\
	automake \
	bash-completion \
	ca-certificates \
	libkeyutils-dev \
	libkmod-dev \
	libjson-c-dev \
	libudev-dev \
	pkg-config \
	systemd \
	uuid-dev"

# PMDK's dependencies
ARG PMDK_DEPS="\
	autoconf \
	automake \
	debhelper \
	devscripts \
	gdb \
	pandoc \
	python3"

# libpmemobj-cpp's dependencies
ARG LIBPMEMOBJ_CPP_DEPS="\
	libatomic1 \
	libtbb-dev"

# memkind's dependencies
ARG MEMKIND_DEPS="\
	autoconf \
	automake \
	numactl \
	libnuma-dev"

# pmem's Valgrind dependencies (optional; valgrind package may be used instead)
ARG VALGRIND_DEPS="\
	autoconf \
	automake"

# Examples (optional)
ARG EXAMPLES_DEPS="\
	libncurses5-dev \
	libsfml-dev"

# Documentation (optional)
ARG DOC_DEPS="\
	doxygen \
	graphviz"

# Tests (optional)
ARG TESTS_DEPS="\
	gdb \
	libc6-dbg \
	libunwind-dev"

# Misc for our builds/CI (optional)
ARG MISC_DEPS="\
	clang \
	libtext-diff-perl \
	pkg-config \
	sudo \
	whois"

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update

# Update the apt cache and install basic tools
RUN apt-get install -y \
	${BASE_DEPS} \
	${PMEMKV_DEPS} \
	${NDCTL_DEPS} \
	${PMDK_DEPS} \
	${LIBPMEMOBJ_CPP_DEPS} \
	${MEMKIND_DEPS} \
	${VALGRIND_DEPS} \
	${EXAMPLES_DEPS} \
	${DOC_DEPS} \
	${TESTS_DEPS} \
	${MISC_DEPS} \
 && rm -rf /var/lib/apt/lists/*

# Install libndctl
COPY scripts/install-libndctl.sh install-libndctl.sh
RUN ./install-libndctl.sh ubuntu

# Install valgrind
COPY scripts/install-valgrind.sh install-valgrind.sh
RUN ./install-valgrind.sh

# Install pmdk from sources (because there are no ndctl packages)
COPY scripts/install-pmdk.sh install-pmdk.sh
RUN ./install-pmdk.sh

# Install pmdk c++ bindings (also from sources)
COPY scripts/install-libpmemobjcpp.sh install-libpmemobjcpp.sh
RUN ./install-libpmemobjcpp.sh

# Install memkind
COPY scripts/install-memkind.sh install-memkind.sh
RUN ./install-memkind.sh

RUN apt update -y && \
    apt install -y cmake protobuf-compiler git gcc g++

RUN apt install -y libprotobuf-dev

RUN git clone --branch v1.9.2 https://github.com/gabime/spdlog.git && cd spdlog && mkdir build && cd build \
       && cmake .. && make -j && make install
