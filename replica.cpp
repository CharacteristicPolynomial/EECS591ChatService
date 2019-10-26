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
#define SOCKADDR_LENGTH sizeof(struct sockaddr_in)
#define RECV_TIME_OUT 2000 // in milliseconds
#define HEART_BEAT 1000 // in milliseconds
#define BUFFER_SIZE 1024
using namespace std;

class Speaker { // used for broadcasting
public:
    Speaker(const char* addrFile, int fd_, const struct sockaddr_in * sockAddr) {
        // fill addrs
        ifstream ifs;
        ifs.open(addrFile);
        if(ifs.fail()) {
            cerr << "Error: Openning file " << addrFile << " fails." << endl;
            ifs.close();
            exit(-1);
        }
        string ip;
        unsigned short port;
        while(ifs >> ip >> port) {
            struct sockaddr_in temp;
            memset((char *)&temp, 0, sizeof(temp));
            temp.sin_family = AF_INET;
            inet_pton(AF_INET, ip.c_str(), &temp.sin_addr);
            temp.sin_port = htons(port);
            cout << port << endl;
            addrs.push_back(temp);
        }
        ifs.close();

        // set sockAddr and fd
        memcpy(&addr, sockAddr, SOCKADDR_LENGTH);
        fd = fd_;
    }
    void broadcast(const char* msg, int msglen) { // ensures that msg is a cstring
        for(auto it=addrs.begin(); it!=addrs.end(); it++) {
            struct sockaddr_in temp = *it;
            if (sendto(fd, msg, msglen, 0, (struct sockaddr*) &temp, sizeof(temp)) < 0) {
                cout << "sendto failed: " << strerror(errno) << endl;
                exit(-1);
            }
        }
    }
private:
    void list() {
        for(auto it=addrs.begin(); it!=addrs.end(); it++) {
            cout << ntohs(it->sin_port) << endl;
        }
    }
    int fd;
    struct sockaddr_in addr;
    vector<struct sockaddr_in> addrs;
};

#include "messages.seg"
#include "rolePlay.seg"

int main(int argc, const char * argv[]) { // f, id, myaddr, myport, addrFile, logFile
    // parse input
    if(argc < 7) {
        cerr << "Error: Missing arguments. (f, id, myaddr, myport, addrFile)" << endl;
        return -1;
    }
    int f = atoi(argv[1]);
    int id = atoi(argv[2]);

    // create socket
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    cerr << "replicate " << id << " cannot create socket" << endl;
	    return -1;
    }

    // name socket
    struct sockaddr_in myaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[3], &myaddr.sin_addr);
    myaddr.sin_port = htons(atoi(argv[4]));
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
	    cerr << "replicate " << id << " bind() fails (" << argv[3] << ", " << argv[4] << ")" << endl;
	    return -1;
    }

    // create speaker
    Speaker* myspeaker;
    myspeaker = new Speaker(argv[5], fd, &myaddr);
    
    // set time outs to 
    struct timeval tv;
    tv.tv_sec = RECV_TIME_OUT / 1000;
    tv.tv_usec = 1000 * (RECV_TIME_OUT % 1000);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error: cannot set time out");
    }
    Replica rep(f, id, fd, &myaddr, myspeaker, argv[6]);
    rep.run(); // enters working loop
    
    return 0;
}