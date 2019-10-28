#include "phone.h"

void Phone::init(int myid) {
    cout << "initilizing phone" << endl;
    id = myid;
    
    // initialize according to the config file
    lossp = stof(get_config(LOSS_PROBABILITY));

    msglog.init(id);
    logFile = get_config(LOGFILE_PREFIX) + to_string(id) + get_config(LOGFILE_SUFFIX);

    recvBuffer = new char[BUFFER_SIZE];
    sendBuffer = new char[BUFFER_SIZE];

    parseAddressFile();
    create_socket();
}

Phone::~Phone() {
    cout << "destructing phone" << endl;
    delete[] recvBuffer;
    delete[] sendBuffer;
}

void Phone::parseAddressFile() {
    ifstream ifs;
    string addrFile = get_config(ADDRESS_FILE);
    ifs.open(addrFile);
    if(ifs.fail()) {
        cerr << "Error: Openning file " << addrFile << " fails." << endl;
        ifs.close();
        exit(-1);
    }
    string ip;
    unsigned short port;
    n = 0;
    while(ifs >> ip >> port) {
        struct sockaddr_in temp;
        memset((char *)&temp, 0, sizeof(temp));
        temp.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &temp.sin_addr);
        temp.sin_port = htons(port);
        replicaAddrs.push_back(temp);
        if(n == id) {
            myAddr = temp;
        }
        ++n;
    }
    ifs.close();
}

void Phone::create_socket() {
    // create socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    cerr << "replicate " << id << " cannot create socket" << endl;
	    exit(-1);
    }

    // name socket
    if (bind(fd, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
	    cerr << "replicate " << id << " bind() fails." << endl;
	    exit(-1);
    }
    
    // set recvfrom time-out to RECV_TIME_OUT milliseconds
    struct timeval tv;
    tv.tv_sec = RECV_TIME_OUT / 1000;
    tv.tv_usec = 1000 * (RECV_TIME_OUT % 1000);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "Error: cannot set time out." << endl;
        exit(-1);
    }
}

HEADER Phone::phone_pickup() {
    // acts the recv function
    msglog.logRecv();
    readCurser = recvBuffer;
    socklen_t addrLen = sizeof(struct sockaddr_in); 
    recvLen = recvfrom(fd, recvBuffer, BUFFER_SIZE, 0, 
        (struct sockaddr*)&comingAddr, &addrLen);
    HEADER header;
    if(recvLen == -1) {
        // recv error
        if(errno == 11) {
            // time out 
            header = NO_MESSAGE;
        } else {
            // error
            cerr << "Error: socket recvfrom() error. " << strerror(errno) << endl;
            exit(-1);
        }
    } else {
        // received a datagram
        header = *(HEADER*) readCurser;
        readCurser += sizeof(HEADER); // read a HEADER
    }
    msglog.logHeader(header);
    return header;
}

int Phone::read_int() {
    int temp = *(int*) readCurser;
    readCurser += sizeof(int);
    msglog.logInt(temp);
    return temp;
}

struct sockaddr_in Phone::read_addr() {
    struct sockaddr_in temp = *(struct sockaddr_in*) readCurser;
    readCurser += sizeof(struct sockaddr_in);
    return temp;
}

char* Phone::read_text(int len) {
    char* buf = new char[len];
    memcpy(buf, readCurser, len);
    readCurser += len;
    return buf;
}

Request Phone::read_request() {
    Request temp;
    // recall that the format is
    // view pos clientAddr seq textLen text
    temp.view = read_int();
    temp.position = read_int();
    temp.clientAddr = read_addr();
    temp.seq = read_int();
    temp.textLen = read_int();
    temp.text = read_text(temp.textLen); // this allocates memory!
    msglog.logRequest(temp);
    return temp;
}

Request Phone::make_request(int view, int position) {
    // make a request out of a client request
    Request temp;
    // recall that the format is
    // seq textlen text
    temp.view = view;
    temp.position = position;
    temp.clientAddr = comingAddr;
    temp.seq = read_int();
    temp.textLen = read_int();
    temp.text = read_text(temp.textLen);
    msglog.logRequest(temp);
    return temp;
}

void Phone::write_int(int k) {
    memcpy(writeCurser, &k, sizeof(int)); // do it
    msglog.logInt(k); // log it
    writeCurser += sizeof(int); // move cursor
    sendLen += sizeof(int); // increase send length
}

void Phone::write_request(Request r) {
    // recall that the format is
    // view pos clientAddr seq textLen text
    msglog.logRequest(r); // log it
    
    memcpy(writeCurser, &r.view, sizeof(int)); // do it
    writeCurser += sizeof(int); // move cursor
    sendLen += sizeof(int); // increase send length
    
    memcpy(writeCurser, &r.position, sizeof(int)); // do it
    writeCurser += sizeof(int); // move cursor
    sendLen += sizeof(int); // increase send length
    
    memcpy(writeCurser, &r.clientAddr, sizeof(struct sockaddr_in)); // do it
    writeCurser += sizeof(struct sockaddr_in); // move cursor
    sendLen += sizeof(struct sockaddr_in); // increase send length
    
    memcpy(writeCurser, &r.seq, sizeof(int)); // do it
    writeCurser += sizeof(int); // move cursor
    sendLen += sizeof(int); // increase send length
    
    memcpy(writeCurser, &r.textLen, sizeof(int)); // do it
    writeCurser += sizeof(int); // move cursor
    sendLen += sizeof(int); // increase send length

    memcpy(writeCurser, r.text, r.textLen); // do it
    writeCurser += r.textLen; // move cursor
    sendLen += r.textLen; // increase send length
}

void Phone::phone_call(HEADER header) {
    // initiates a phone call
    msglog.logSend();
    writeCurser = sendBuffer;
    sendLen = 0;

    memcpy(writeCurser, &header, sizeof(HEADER)); // do it
    msglog.logHeader(header); // log it
    writeCurser += sizeof(HEADER); // move cursor
    sendLen += sizeof(HEADER); // increase send length
}

void Phone::send_to(int target) {
    msglog.log_send_to(target); // log it
    if (loss()) 
        return;
    if (sendto(fd, sendBuffer,sendLen, 0, 
        (struct sockaddr*) &replicaAddrs[target], sizeof(struct sockaddr_in)) < 0) {
        cout << "sendto failed: " << strerror(errno) << endl;
        exit(-1);
    }
}

void Phone::broadcast() {
    msglog.log_broadcast(); // log it
    if (loss()) 
        return;
    for(int target=0; target<n; target++) {
        if (sendto(fd, sendBuffer,sendLen, 0, 
            (struct sockaddr*) &replicaAddrs[target], sizeof(struct sockaddr_in)) < 0) {
            cout << "sendto failed: " << strerror(errno) << endl;
            exit(-1);
        }
    }
}

void Phone::reply() {
    msglog.log_reply();
    if (loss()) 
        return;
    if (sendto(fd, sendBuffer,sendLen, 0, 
            (struct sockaddr*) &comingAddr, sizeof(struct sockaddr_in)) < 0) {
            cout << "sendto failed: " << strerror(errno) << endl;
            exit(-1);
        }
}

void Phone::log() {
    msglog.log_log();
    ofstream ofs;
    ofs.open(logFile, ios_base::out | ios_base::app);
    ofs.write(sendBuffer,sendLen);
    ofs.close();
}