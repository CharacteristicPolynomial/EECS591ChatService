#pragma once

#define CONFIG_FILE "serverConfigure.config"
#define LOGFILE_PREFIX "LogFilePrefix"
#define LOGFILE_SUFFIX "LogFileSuffix"
#define ADDRESS_FILE "AddressFile"
#define MSGLOG_PREFIX "MsgLogPrefix"
#define MSGLOG_SUFFIX "MsgLogSuffix"
#define MSGLOG_ON "MsgLogOn"
#define LOSS_PROBABILITY "LossProbability"

#define RECV_TIME_OUT 100 // in milliseconds
#define HEART_BEAT_TIME 2000 // in milliseconds
#define PATIENCE_TIME 3000 // in milliseconds


#include <string>
#include <fstream>
#include <iostream>
using namespace std;

string get_config(string key);