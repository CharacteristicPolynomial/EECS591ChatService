#pragma once

#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>
#include <cstring>
#include <iostream>
#include "configure.h"
#include "message.h"
#include "debugLog.h"
#include <chrono>
#include <thread>
#include <cstdlib>
using namespace std;

class Phone {
public:
    void init(int myid);
    ~Phone();

    int get_f() {
        return n/2;
    }
    float lossp;
    bool loss() {
        return (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) )
            < lossp;
    }

    void heartBeat() {
        // we use independent buffers from normal phone usage
        // we don't log heart beat sending
        // writing some replicated code here
        char heartBuf[BUFFER_SIZE];
        HEADER header = HEARTBEAT;
        int msglen = sizeof(HEADER) + sizeof(int);
        while(1) {
            this_thread::sleep_for(chrono::milliseconds(HEART_BEAT_TIME));
                // broadcast
                for(auto it=replicaAddrs.begin(); it!=replicaAddrs.end(); it++) {
                    struct sockaddr_in temp = *it;
                    memcpy(heartBuf, &header, sizeof(HEADER));
                    memcpy(heartBuf+sizeof(HEADER), &id, sizeof(id));
                    if (sendto(fd, heartBuf, msglen, 0, (struct sockaddr*) &temp, sizeof(temp)) < 0) {
                        cout << "sendto failed: " << strerror(errno) << endl;
                        exit(-1);
                }
            }
        }
    }

    // read and write the buffers
    HEADER phone_pickup(); // initiates a message reading
    int read_int(); // read an int and move the cursor
    Request read_request(); // read a Request and move the cursor
    struct sockaddr_in read_addr(); // read a sockaddr_in and move the cursor
    char* read_text(int len); // read a text, allocate some memory and move the cursor
    Request make_request(int view, int position); // make a request and move the cursor
    void write_int(int k); 
    void write_request(Request r);
    void phone_call(HEADER header); // initiates a message writing

    // communication
    void send_to(int target); // send the current sendBuffer to replica #target
    void broadcast(); // broadcast the current sendBuffer to all replicas (including itself)
    void reply(); // reply the current sendBuffer to the sender
    void log(); // log the composed message to the log file

    // log
    HEADER phone_pickup(const char* buf, int len) {
        // initiates a log reading
        msglog.logReadLog();
        readCurser = recvBuffer;
        recvLen = len;
        memcpy(recvBuffer, buf, len);
        HEADER header;
        header = *(HEADER*) readCurser;
        readCurser += sizeof(HEADER); // read a HEADER
        msglog.logHeader(header);
        return header;
    }
    void write_acceptLog(const Request r) {
        ofstream ofs;
        ofs.open(logFile, ios_base::out | ios_base::app);
        phone_call(ACCEPT_LOG);
        write_request(r);
        ofs.write((char*)&sendLen, sizeof(int));
        ofs.write(sendBuffer, sendLen);
        ofs.close();
    }
    void write_learnLog(const Request r) {
        ofstream ofs;
        ofs.open(logFile, ios_base::out | ios_base::app);
        phone_call(LEARN_LOG);
        write_request(r);
        ofs.write((char*)&sendLen, sizeof(int));
        ofs.write(sendBuffer, sendLen);
        ofs.close();
    }
    void write_viewLog(int v) {
        ofstream ofs;
        ofs.open(logFile, ios_base::out | ios_base::app);
        phone_call(VIEW_LOG);
        write_int(v);
        ofs.write((char*)&sendLen, sizeof(int));
        ofs.write(sendBuffer, sendLen);
        ofs.close();
    }
private:
    int id, fd, n;
    // id (replicaID), fd (socket file descriptor), n (total number of replicas)
    vector<struct sockaddr_in> replicaAddrs;
    int recvLen, sendLen;
    char *recvBuffer;
    const char *readCurser; 
    char *sendBuffer;
    char *writeCurser;
    struct sockaddr_in comingAddr;
    struct sockaddr_in myAddr;
    MsgLog msglog;
    string logFile;

    void parseAddressFile();
    void create_socket();
};