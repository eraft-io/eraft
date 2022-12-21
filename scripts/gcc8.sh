#!/bin/bash

yum install centos-release-scl
yum install devtoolset-8

scl enable devtoolset-8 -- bash
source /opt/rh/devtoolset-8/enable 

