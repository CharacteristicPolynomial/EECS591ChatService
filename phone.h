#pragma once

#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>
#include <cstring>
#include <iostream>
#include "configure.h"
#include "message.h"
#include "debugLog.h"
using namespace std;

class Phone {
public:
    Phone(int myid);
    ~Phone();

    int get_f() {
        return n/2;
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
    void reply(); // reply the current sendBuffer to the client, (call this only when the previous message is a client request)
    void log(); // log the composed message to the log file
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