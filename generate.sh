#! zsh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}
CXX=${CXX:-g++}

mkdir -p $BUILD_DIR/ && cd $BUILD_DIR && cmake $SOURCE_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=1 && make $* -j4

if [[ $? -eq 0 ]];then 
    figlet Comliler Successfully
    sleep 1s && clear
fi