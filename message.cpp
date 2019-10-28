#include "message.h"


string getIP(struct sockaddr_in addr) {
    return string(inet_ntoa(addr.sin_addr));
}

unsigned short getPort(struct sockaddr_in addr) {
    return ntohs(addr.sin_port);
}