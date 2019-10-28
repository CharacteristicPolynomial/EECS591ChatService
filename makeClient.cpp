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

using namespace std;

#include "configure.h"
#include "message.h"

vector<struct sockaddr_in> serverAddr;

void parseAddr(const char* addrFile) {
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
        // cout << port << endl;
        serverAddr.push_back(temp);
    }
    ifs.close();
}

int manual(int fd) {
    int seq = 0;
    char msg[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    while(1) {
        string request;
        cout << "wait for request (no spaces allowed): ";
        cin >> request;
        while(1) {
            for(auto it=serverAddr.begin(); it!=serverAddr.end(); it++) {
                struct sockaddr_in temp = *it;
                // format: header, seq, len, text
                HEADER header = CLIENT_REQUEST;
                memcpy(msg, &header, sizeof(HEADER));
                memcpy(msg+sizeof(HEADER), &seq, sizeof(int));
                int k = request.size();
                memcpy(msg+sizeof(HEADER)+sizeof(int), &k, sizeof(int));
                memcpy(msg+sizeof(HEADER)+2*sizeof(int), request.c_str(), k);
                if (sendto(fd, msg, sizeof(HEADER)+k+2*sizeof(int), 0, (struct sockaddr*) &temp, sizeof(temp)) < 0) {
                    cout << "sendto failed: " << strerror(errno) << endl;
                    exit(-1);
                }
            }

            int dgramsize;
            dgramsize = recvfrom(fd, buffer, BUFFER_SIZE, 0, 0, 0);
            if(dgramsize == -1) {
                // recv error
                if(errno == 11) {
                    // time out 
                } else {
                    // error
                    cerr << "Error: socket recvfrom() error." << endl;
                    exit(-1);
                }
            } else {
                // received a datagram
                // format: ACK, ID
                HEADER header = *(HEADER*) buffer;
                int tempseq = *(int*) (buffer + sizeof(HEADER));
                if (header != ACK) {
                    // error dectection
                    cerr << "Error: client received non-ack message." << endl;
                    exit(-1);       
                }
                if (tempseq > seq) {
                    // error dectection
                    cerr << "Error: client received unexpected sequence number." << endl;
                    exit(-1);                    
                } else if (tempseq == seq) {
                    // the request is processed
                    break;
                }
            }
            cout << "no response, resending message number " << seq  << endl;
        }
        seq ++;
    }
}

int main(int argc, const char * argv[]) {
    // -m, manual mode
    // addrFile
    if(argc < 2) {
        cerr << "missing arguments: [-m] " << endl;
        exit(-1);
    }
    parseAddr(get_config(ADDRESS_FILE).c_str());

    // create socket 
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    cerr << "client cannot create socket" << endl;
	    return -1;
    }

    // name socket
    struct sockaddr_in myaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
	    cerr << "client bind() fails" << endl;
	    return -1;
    }

    // set time outs to 
    struct timeval tv;
    tv.tv_sec = RECV_TIME_OUT / 1000;
    tv.tv_usec = 1000 * (RECV_TIME_OUT % 1000);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error: cannot set time out");
    }

    if(strcmp(argv[1], "-m")==0) {
        return manual(fd);
    }
}