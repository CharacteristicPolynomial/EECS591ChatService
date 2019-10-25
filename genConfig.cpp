#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
#define SOCKADDR_LENGTH sizeof(struct sockaddr_in)
#define PORT_LOW 20000
#define PORT_HIGH 65535

int getIPv4Addr(struct sockaddr_in * publicAddr);
string spellAddr(struct sockaddr_in* addr);
void scanPorts(vector<unsigned int>& ports, struct sockaddr_in* addr, int n);

int main (int argc, const char * argv[]) { // arguments: (number of replicas) (write file name)
    // Parse args
    if (argc <3) {
        cerr << "Error: Missing arguments. (number of replicas) (write file name)" << endl;
        return -1;
    }
    int replicaN = atoi(argv[1]);


    // obtain IP address
    struct sockaddr_in publicAddr;
    if(getIPv4Addr(&publicAddr)) {
        cerr << "Error: Cannot find public IPv4 address." << endl;
        return -1;
    }
    cout << "public IPv4 address obtained: " << spellAddr(&publicAddr) << endl;
    vector<unsigned int> ports;
    scanPorts(ports, &publicAddr, replicaN);
    if(ports.size() < replicaN) {
        cerr << "Error: Only " << ports.size() << " ports are available." << endl;
        return -1;
    }

    ofstream ofs;
    ofs.open(argv[2], ios_base::app);
    if(ofs.fail()) {
        cerr << "Error: Writing to file " << argv[2] << " fails." << endl;
        ofs.close();
        return -1;
    }
    for(auto it=ports.begin(); it!=ports.end(); it++) {
        ofs << spellAddr(&publicAddr) << " " << *it << endl;
    }
    ofs.close();
    return 0;
}

void scanPorts(vector<unsigned int>& ports, struct sockaddr_in* addr, int n) {
    // returns the number of available ports from PORT_LOW to PORT_HIGH
    // open check socket
    int checkfd;

    struct sockaddr_in probeAddr;
    memcpy(&probeAddr, addr, SOCKADDR_LENGTH);
    
    for(unsigned short i=PORT_LOW; i<PORT_HIGH; i++) {
        if(ports.size()>=n)
            break;
        probeAddr.sin_port = htons(i);
        if((checkfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cerr << "Error: Cannot create check socket." << endl;
            exit(-1);
        }
        cout << "port " << i << " (checked by socket " << checkfd << ")" ;
        if(bind(checkfd, (struct sockaddr*) &probeAddr, SOCKADDR_LENGTH) < 0) {
            cout << " is closed" << endl;
        } else {
            cout << " is open" << endl;
            ports.push_back(i);
        }
        if(close(checkfd)) {
            cerr << "Error: Cannot close check socket." << endl;
            exit(-1);
        }
    }
    return;
}

string spellAddr(struct sockaddr_in* addr) {
    char* buffer = new char[1000];
    inet_ntop(AF_INET, & addr->sin_addr, buffer, 1000);
    string temp(buffer);
    delete[] buffer;
    return temp;
}

int getIPv4Addr(struct sockaddr_in* publicAddr) {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    int flag = -1;
    getifaddrs(&ifAddrStruct);
    char* buffer = new char[1000];

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            // check whether it is 127.0.0.1
            inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, buffer, 1000);
            if(strcmp("127.0.0.1", buffer)) {
                memcpy(publicAddr, ifa->ifa_addr, SOCKADDR_LENGTH);
                flag = 0;
            }
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    delete[] buffer;
    return flag;
}