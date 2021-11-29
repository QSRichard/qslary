#include <iostream>
#include <fstream>

using namespace std;

int main(){

    ofstream of;
    of.open("./log.txt");
    of<<"Hello";
    of.close();
    return 0;
}