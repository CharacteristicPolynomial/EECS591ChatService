#include <sys/socket.h>
#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <errno.h>
#include <thread>
#include <chrono>
#include "configure.h"
#include "replica.h"
using namespace std;

int main(int argc, const char * argv[]) { // id
    // parse input
    if(argc < 2) {
        cerr << "Error: Missing arguments. (id)" << endl;
        return -1;
    }
    int id = atoi(argv[1]);

    Replica rep(id);
    rep.run(); // enters working loop
    
    return 0;
}