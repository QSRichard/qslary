#ifndef __TEST_H
#define __TEST_H
#include <vector>
#include <map>
#include <iostream>
#include <stdio.h>
class Test{

public:
    static std::map<int,int> mp;
    static int test_insert(int a,int b){
        std::cout<<"before"<<std::endl;
        mp[a]=b;
        std::cout << "later" << std::endl;
        return 0;
    };
};

// static std::vector<int> v;

#endif