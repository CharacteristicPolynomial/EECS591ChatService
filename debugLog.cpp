#include "debugLog.h"

void MsgLog::init (int id) {
    string ans = get_config(MSGLOG_ON);
    msglog_on = (ans == "True");
    if (msglog_on == true) {
        logfile = get_config(MSGLOG_PREFIX) + to_string(id) + get_config(MSGLOG_SUFFIX);
        ofstream ofs;
        ofs.open(logfile, ios_base::out | ios_base::app);
        ofs << endl << endl << endl << "Message log of replica " << id << endl;
        ofs.close();
    }
}

void MsgLog::logSend() {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << endl << "Send ";
    ofs.close();
}

void MsgLog::logRecv() {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << endl << "Recv ";
    ofs.close();
}

void MsgLog::logHeader(HEADER head) {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    switch (head) {
        case NO_MESSAGE:
            ofs << "[NO_MESSAGE] ";
            break;
        case VIEW_CHANGE:
            ofs << "[VIEW_CHANGE] ";
            break;
        case VIEW_PROMISE:
            ofs << "[VIEW_PROMISE] ";
            break;
        case CLIENT_REQUEST:
            ofs << "[CLIENT_REQUEST] ";
            break;
        case HEARTBEAT:
            ofs << "[HEARTBEAT] ";
            break;
        case ACCEPT_IT:
            ofs << "[ACCEPT_IT] ";
            break;
        case ACCEPTED:
            ofs << "[ACCEPTED] ";
            break;
        case VIEW_LOG:
            ofs << "[VIEW_LOG] ";
            break;
        case ACCEPT_LOG:
            ofs << "[ACCEPT_LOG] ";
            break;
        case LEARN_LOG:
            ofs << "[LEARN_LOG] ";
            break;
        case ACK:
            ofs << "[ACK] ";
            break;
    }
    ofs.close();
}

void MsgLog::logInt(int k) {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << k << " ";
    ofs.close();
}

void MsgLog::logRequest(Request r) {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << "Request(view=" << r.view
        << ", pos="<< r.position
        << ", seq="<< r.seq
        << ", clientIP="<< getIP(r.clientAddr)
        << ", clientPort="<< getPort(r.clientAddr)
        << ") ";
    ofs.close();
}

void MsgLog::log_send_to(int target) {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << "(Send to replica " << target << ") ";
    ofs.close();
}

void MsgLog::log_broadcast() {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << "(Broadcast to all replicas) ";
    ofs.close();
}

void MsgLog::log_reply() {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << "(Reply to client) ";
    ofs.close();
}

void MsgLog::log_log() {
    if (msglog_on == false)
        return;
    ofstream ofs;
    ofs.open(logfile, ios_base::out | ios_base::app);
    ofs << "(Write in log) ";
    ofs.close();
}
