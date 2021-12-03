#! zsh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install-cpp11}
CXX=${CXX:-g++}

mkdir -p $BUILD_DIR/ && cd $BUILD_DIR && cmake $SOURCE_DIR && make $* -j4

if [[ $? -eq 0 ]];then 
    figlet Comliler Successfully
    sleep 1s && clear
fi