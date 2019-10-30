#pragma once

#include "phone.h"
#include "replicaTools.h"
#include "timer.h"
#include <thread>
#include <chrono>
#include <iostream>
using namespace std;

class Replica {
public:
    Replica(int myid);
    void run();
private:
    int view;
    int id;
    int f;
    bool leaderQ;

    Phone phone; // general phone
    RequestList acceptLog; // store accept_it requests
    string acceptLogFile;
    ViewChange viewchange; // store viewpromise requests
    Timer viewchangeTimer;
    Timer requestTimer;
    Learner learner; // store accepted requests
    vector<chrono::nanoseconds> heartBeatList;
    chrono::nanoseconds bornTime;

    int time_since_last_heartbeat(int myid) {
        chrono::milliseconds ps = 
            chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()
                    - heartBeatList[myid]);
        return ps.count();
    }

    bool check_heart_beat();
    void parseLogFile() {
        string logFile = get_config(LOGFILE_PREFIX) + to_string(id) + get_config(LOGFILE_SUFFIX);
        ifstream ifs;
        ifs.open(logFile);
        char buf[BUFFER_SIZE];
        while(ifs.read(buf, sizeof(int))) {
            int len = *(int*) buf;
            char* cursor = buf+sizeof(int);
            Request r;
            if(ifs.read(cursor, len)) {
                switch (phone.phone_pickup(cursor, len)) {
                    case VIEW_LOG:
                        view = phone.read_int();
                        break;
                    case ACCEPT_LOG:
                        r = phone.read_request();
                        acceptLog.add(r);
                        break;
                    case LEARN_LOG:
                        r = phone.read_request();
                        learner.learnLog.add(r);
                        break;
                    default:
                        cerr << "Replica Error: unknown header (Parsing file)" << endl;
                        exit(-1);
                }
            } else {
                cerr << "Error when parsing log file" << endl;
                exit(-1);
            }
        }
        ifs.close();
    }
    RoundRobin rb;
    
    void sendPromise() { // send promise to current view
        // format
        // header, view, replicaID, totalLogLen, request
        if (acceptLog.length ==0) {
            // if it has no entry
            phone.phone_call(VIEW_PROMISE);
            phone.write_int(view);
            phone.write_int(id);
            phone.write_int(acceptLog.length);
            phone.reply();
        } else {
            rb.next_round();
            int k = acceptLog.slots.size();
            for(auto slot : acceptLog.slots) {
                if(!rb.filter(k))
                    continue;
                if(slot.second) {
                    // if it is not empty
                    phone.phone_call(VIEW_PROMISE);
                    phone.write_int(view);
                    phone.write_int(id);
                    phone.write_int(acceptLog.length);
                    phone.write_request(slot.first);
                    phone.reply();
                }
            }
        }
    }
    void enforceViewChange() {
        // broadcast view change
        // format
        // header, view
        phone.phone_call(VIEW_CHANGE);
        phone.write_int(view);
        phone.broadcastExcept(viewchange.completeList);
    }
    void process_request() {
        // process the request as a leading leader
        Request r = phone.make_request(view, 0);
        if(learner.response(r)) {
            // executable
            phone.phone_call(ACK);
            phone.write_int(r.seq);
            phone.reply();
            r.freeText();
        } else {
            int k = 0;
            int msgn = 0;
            learner.updateExecN(id);
            for(auto& entry : acceptLog.slots) {
                if (! learner.filledQ(k)) {
                    // only enforce unlearned slots
                    if (entry.second == false) {
                        // if it is a hole
                        // fill it with no-op
                        entry.first.make_hole(view, k);
                        entry.second = true;
                        phone.write_acceptLog(entry.first);
                    }
                    phone.phone_call(ACCEPT_IT);
                    phone.write_request(entry.first);
                    phone.broadcast();
                    msgn++;
                    if(msgn > ROUNDROBIN) {
                        r.freeText();
                        return;
                    }
                }
                r.position = k;
                if(entry.first.contentPositionMatch(r)) {
                    r.freeText();
                    return;
                } 
                k++;
            }
            r.position = k;
            // not accepted yet, add this to acceptLog and log it!
            acceptLog.add(r);
            phone.write_acceptLog(r);
        }
    }
    void accept_it() {
        Request r = phone.read_request();
        int sender_id;
        sender_id = r.view % (2*f +1);
        heartBeatList[sender_id] = chrono::system_clock::now().time_since_epoch();
        if (r.view > view) {
            view = r.view;
            phone.write_viewLog(view);
        }
        if (r.view >= view) {
            // accept it, log and broadcast it
            acceptLog.add(r);
            acceptLog.saveAccept(acceptLogFile);
            phone.write_acceptLog(r);
            // format
            // header request replicaID
            phone.phone_call(ACCEPTED);
            phone.write_request(r);
            phone.write_int(id);
            phone.broadcast();
        } else {
            r.freeText();
        }
    }
    void process_view_change() {
        // select the most acceptslot 
        for(int k : viewchange.completeList) {
            for(auto data : viewchange.viewchange_data[k].logset) {
                // format (position, templog)
                Request r = data.second;
                acceptLog.update(r);
            }
        }
        acceptLog.setView(view);
    }
};