#pragma once

#include "phone.h"
#include "replicaTools.h"
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

    Phone* phone;
    RequestList acceptLog;
    RequestList learnLog;
    Learner learner;
    vector<chrono::nanoseconds> heartBeatList;
};