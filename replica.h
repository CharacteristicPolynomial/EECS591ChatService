#pragma once

#include "phone.h"
#include "replicaTools.h"
#include <thread>
#include <chrono>

class Replica {
public:
    Replica(int myid);
    ~Replica();
    void run();
private:
    int view;
    int id;
    int f;
    bool leaderQ;

    Phone* phone; // general phone
    RequestList acceptLog;
    RequestList learnLog;
    ViewChange viewchange;
    Learner learner;
    vector<chrono::nanoseconds> heartBeatList;

    int time_since_last_heartbeat(int myid) {
        chrono::milliseconds ps = 
            chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()
                    - heartBeatList[myid]);
        return ps.count();
    }

    bool check_heart_beat();
    void parseLogFile();
    void acceptLog();
    void learnLog();
    void viewLog();
    void sendPromise(); // send promise to current view
    void receivePromise(); // ensures be in state WAIT_PROMISE when calling
    void enforceViewChange();
    void process_request();
    void accept_it();
};