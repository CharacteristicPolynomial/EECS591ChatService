#pragma once

#define SOCKADDR_LENGTH sizeof(struct sockaddr_in)
#define BUFFER_SIZE 1024

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string>
using namespace std;

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
    Request() {
        text = NULL;
    }
    ~Request() {
        delete[] text;
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
    const char* text;
};

string getIP(struct sockaddr_in addr) {
    return string(inet_ntoa(addr.sin_addr));
}

unsigned short getPort(struct sockaddr_in addr) {
    return ntohs(addr.sin_port);
}