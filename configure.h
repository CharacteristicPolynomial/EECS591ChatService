#pragma once

#define CONFIG_FILE "serverConfigure.config"
#define LOGFILE_PREFIX "LogFilePrefix"
#define LOGFILE_SUFFIX "LogFileSuffix"
#define ADDRESS_FILE "AddressFile"
#define MSGLOG_PREFIX "MsgLogPrefix"
#define MSGLOG_SUFFIX "MsgLogSuffix"
#define MSGLOG_ON "MsgLogOn"

#define RECV_TIME_OUT 2000 // in milliseconds
#define HEART_BEAT_TIME 1000 // in milliseconds

#include <string>
#include <fstream>
#include <iostream>
using namespace std;

string get_config(string key) {
    ifstream ifs;
    ifs.open(CONFIG_FILE);
    if(ifs.fail()) {
        cerr << "Error: cannot open config file '" << CONFIG_FILE << "'" << endl;
        ifs.close();
        exit(-1);
    }
    string entry, value;
    while(ifs >> entry >> value) {
        if(entry == key) {
            ifs.close();
            return value;
        }
    }
    ifs.close();
    cerr << "Config error: cannot find " << key << endl;
    exit(-1);
}