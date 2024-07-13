#!/bin/bash

original_path=$PATH

export PATH=$(pwd):$PATH

echo "start compile"
make clean
make
ln -sf subServer setIp 
ln -sf subServer setMask
ln -sf subServer setGateway
ln -sf subServer outPutVersion
#test

# setIp 192.168.31.66 1
# setMask 255.255.255.0 2
# setGateway 192.168.31.1 3
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion 
outPutVersion
export PATH=$original_path