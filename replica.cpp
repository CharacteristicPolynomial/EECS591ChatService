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
#define RECV_TIME_OUT 500 // in milliseconds
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
            addrs.push_back(temp);
        }
        ifs.close();

        // set sockAddr and fd
        memcpy(&addr, sockAddr, SOCKADDR_LENGTH);
        fd = fd_;
    }
    void broadcast(const char* msg) { // ensures that msg is a cstring
        for(auto it=addrs.begin(); it!=addrs.end(); it++) {
            if (sendto(fd, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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

class Request {
public:
    struct sockaddr_in clientAddr;
    string request;
    int requestID; // given by the client
};

#include "rolePlay.seg"
#include "messages.seg"
class Replica { // used for role play (leader/proposer, acceptor, learner)
public:
    Replica(int f_, int id_, int fd_, const struct sockaddr_in *addr, Speaker* sp) {
        f = f_;
        id = id_;
        fd = fd_;
        speaker = sp;
        memcpy(&myaddr, addr, SOCKADDR_LENGTH);
        view = 0;
    }
    void run() {
        thread hp(&Replica::heartBeat, this); // create heart beat thread
        char buffer[BUFFER_SIZE];
        while(1) {
            int dgramsize;
            dgramsize = recvfrom(fd, buffer, BUFFER_SIZE, 0, 0, 0);
            if(dgramsize == -1) {
                // recv error
                if(errno == 11) {
                    // time out 
                    time_out();
                } else {
                    // error
                    cerr << "Error: socket recvfrom() error." << endl;
                    exit(-1);
                }
            } else {
                // received a datagram
                parsemsg(buffer, dgramsize);
            }
        }
    }
private:
    int f, id, fd;
    int view;
    struct sockaddr_in myaddr;
    Speaker* speaker;

    Leader leader;
    Acceptor acceptor;
    Learner learner;

    vector<Request> chatLog;

    void time_out() {

    }
    void parsemsg(char* buf, int buflen) {

    }
    void heartBeat() {
        int k = 0;
        char heartBuf[BUFFER_SIZE];
        while(1) {
            this_thread::sleep_for(chrono::milliseconds(HEART_BEAT));
            encodeHeartBeat(id, heartBuf);
            speaker->broadcast(heartBuf);
        }
    }
};

int main(int argc, const char * argv[]) { // f, id, myaddr, myport, addrFile
    // parse input
    if(argc < 6) {
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
    Replica rep(f, id, fd, &myaddr, myspeaker);
    rep.run(); // enters working loop
    
    return 0;
}