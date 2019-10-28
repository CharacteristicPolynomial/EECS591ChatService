#pragma once

#include <string>
#include <fstream>
#include "message.h"
#include "configure.h"
#include <chrono>
#include <ctime>

std::string getTimeStr();
using namespace std;

// enum LOG_MODE {
//     NO_LOG,
//     FILE_LOG,
//     LIVE_LOG,
//     LOG_MODE_SIZE
// };

// class DebugLog {
// public:
//     DebugLog(int id, LOG_MODE mod);
//     ~DebugLog();
//     void logRequest(HEADER header, Request r); 
//     // header should be either ACCEPT_LOG OR VIEW_LOG
// private:
//     string logFile;
// };

class MsgLog {
public:
    void init(int id);
    void logSend(); // automatically start a new line
    void logRecv(); // automatically start a new line
    void logReadLog() {
        if (msglog_on == false)
            return;
        ofstream ofs;
        ofs.open(logfile, ios_base::out | ios_base::app);
        ofs << endl << "Read Log ";
        ofs.close();
    }
    void logHeader(HEADER head); 
    void logInt(int k);
    void logRequest(Request r);
    void log_send_to(int target);
    void log_broadcast();
    void log_reply();
    void log_log();
private:
    string logfile;
    bool msglog_on=false;
    
};