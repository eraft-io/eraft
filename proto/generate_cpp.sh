#!/bin/bash

protoc -I=proto --cpp_out=output proto/*
