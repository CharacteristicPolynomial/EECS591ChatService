#pragma once

#define SOCKADDR_LENGTH sizeof(struct sockaddr_in)
#define BUFFER_SIZE 1024

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
using namespace std;


string getIP(struct sockaddr_in addr);

unsigned short getPort(struct sockaddr_in addr);

enum HEADER {
    NO_MESSAGE,
    VIEW_CHANGE,
    VIEW_PROMISE,
    CLIENT_REQUEST,
    HEARTBEAT, 
    ACCEPT_IT,
    ACCEPTED,
    ACK,
    VIEW_LOG,
    ACCEPT_LOG,
    LEARN_LOG,
    HEADERSIZE
};

class Request {
public:
    void make_hole(int v, int pos) {
        if (text != NULL ) {
            cerr << "making non-empty request a hole" << endl;
            exit(-1);
        }
        char* temp = new char[20];
        strcpy(temp, "NOOP");
        text = temp;
        position = pos;
        textLen = strlen(temp);
        view = v;
    }

    void freeText() {
        // one should explicitly free the memory
        delete[] text;
    }

    bool operator==(const Request& r) const
    { 
        // we assume client always send the same message with the same seq number
        return r.view == view && 
        r.position == position && 
        r.seq == seq && 
        getIP(r.clientAddr) == getIP(clientAddr) && 
        getPort(r.clientAddr) == getPort(clientAddr);
    } 


    bool contentPositionMatch(const Request& r) const
    { 
        // we assume client always send the same message with the same seq number
        return r.position == position && 
        r.seq == seq && 
        getIP(r.clientAddr) == getIP(clientAddr) && 
        getPort(r.clientAddr) == getPort(clientAddr);
    } 
    bool contentMatch(const Request& r) const
    { 
        // we assume client always send the same message with the same seq number
        return r.seq == seq && 
        getIP(r.clientAddr) == getIP(clientAddr) && 
        getPort(r.clientAddr) == getPort(clientAddr);
    } 

    Request make_copy() {
        // allocates memory!
        Request temp;
        temp.view = view;
        temp.position = position;
        temp.seq = seq;
        temp.textLen = textLen;
        temp.clientAddr = clientAddr;
        char* buf = new char[textLen];
        memcpy(buf, text, textLen);
        temp.text = buf;
        return temp;
    }

    int view, position, seq, textLen;
    struct sockaddr_in clientAddr;
    const char* text = NULL;
};
