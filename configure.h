#pragma once

#define CONFIG_FILE "serverConfigure.config"
#define LOGFILE_PREFIX "LogFilePrefix"
#define LOGFILE_SUFFIX "LogFileSuffix"
#define ADDRESS_FILE "AddressFile"
#define MSGLOG_PREFIX "MsgLogPrefix"
#define MSGLOG_SUFFIX "MsgLogSuffix"
#define MSGLOG_ON "MsgLogOn"
#define LOSS_PROBABILITY "LossProbability"
#define CHATLOG_PREFIX "ChatLogPrefix"
#define CHATLOG_SUFFIX "ChatLogSuffix"
#define ACCEPTLOG_PREFIX "AcceptLogPrefix"
#define ACCEPTLOG_SUFFIX "AcceptLogSuffix"

#define RECV_TIME_OUT 1000 // in milliseconds
#define HEART_BEAT_TIME 500 // in milliseconds
#define VIEW_CHANGE_TIME 200 // in milliseconds
#define REQUEST_TIME 100 // in milliseconds
#define PATIENCE_TIME 5000 // in milliseconds
#define PATIENCE_TIME_INCREMENT 100 // in milliseconds
#define CLIENT_RETRY_TIME 100 // in milliseconds

#define ROUNDROBIN 10

#include <string>
#include <fstream>
#include <iostream>
using namespace std;

string get_config(string key);