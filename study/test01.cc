#include <stdio.h>

// 测试__sync_fetch_and_add等gcc特有函数能否使用clang编译

/* 命令行 
          g++ test01.cc -o test01 
          clang++ test01.cc -o test01 
    都编译成功且结果正常

*/
int main(){

    int n=1;
    int t=__sync_fetch_and_add(&n,2);
    printf("%d\n",t);
}