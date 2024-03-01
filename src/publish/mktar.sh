#!/bin/sh

vn=`cat ../comm/spp_version.h | grep spp| awk -F': ' '{print $2}'|awk -F '"' '{print $1}'`

echo "spp_version: $vn"

function make_pkg() 
{
    echo "param num: $#, $1 $vn"
    arch_local=all
    if [ $1 -eq 32 ]; then
        arch_local=all32
    elif [ $1 -eq 64 ]; then
        arch_local=all
    else
        echo "invalid parameter"
        echo "usage: $0 [32|64]"
        exit -1
    fi
 
    tar_name="${vn}_${1}bit.tar.gz"
    
    cd ../../src/
    make clean $arch_local
    cd -
    cd ../../src/module/
    make clean $arch_local;make clean
    cd -
    rm -rf spp 
    mkdir -p ./spp/bin
    cp ../../bin/* ./spp/bin/ -r
    cp ../module ./spp/ -r
    cp ../../etc ./spp/ -r

    find ./spp/ -name ".svn" |xargs rm -rf

    tar czf ../../dist/${tar_name} spp
    rm -rf spp
}

if [ $# -eq 0 ]; then
    make_pkg 64
    make_pkg 32
elif [ $1 -eq 32 ]; then
    make_pkg 32
elif [ $1 -eq 64 ]; then
    make_pkg 64
else
    echo "invalid parameter"
    echo "usage: $0 [32|64]"
    exit -1
fi

md5sum ../../dist/*$vn*.gz > ../../dist/md5sum
