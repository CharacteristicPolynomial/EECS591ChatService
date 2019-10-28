#pragma once
#include "message.h"
#include "phone.h"
#include "replica.h"
#include <unordered_map>
#include <unordered_set>
using namespace std;

class RequestList {
    // organize list of requests
public:
    void print() {
        // print log contents to cout
        cout << endl << "Learn Log Updated" << endl;
        int k = 0;
        for(auto entry : slots) {
            cout << "Slot " << k++ << " : ";
            if (entry.second == true) {
                Request r = entry.first;
                cout << "Request(view=" << r.view
                    << ", pos="<< r.position
                    << ", seq="<< r.seq
                    << ", clientIP="<< getIP(r.clientAddr)
                    << ", clientPort="<< getPort(r.clientAddr)
                    << ", content=";
                cout.write(r.text, r.textLen);
                cout << ")" << endl;
            } else {
                cout << "(Empty slot)" << endl;
            }
        }
    }
    void add(const Request r) { // store r here
        // replace previous request
        int pos = r.position;
        if(pos >= (int)slots.size()) {
            slots.resize(pos+1);
        }
        if(slots[pos].second) {
            // free previous request
            slots[pos].first.freeText();
            slots[pos].first = r;
        } else {
            slots[pos].first = r;
            slots[pos].second = true;
            length ++;
        }
    }
    void update(Request r) { // copy r here
        // update the request if incoming has higher view number
                cout << "Updating Request(view=" << r.view
                    << ", pos="<< r.position
                    << ", seq="<< r.seq
                    << ", clientIP="<< getIP(r.clientAddr)
                    << ", clientPort="<< getPort(r.clientAddr)
                    << ", content=";
                cout.write(r.text, r.textLen);
                cout << ")" << endl;
        int pos = r.position;
        if(pos >= (int)slots.size()) {
            slots.resize(pos+1);
        }
        if(slots[pos].second) {
            if(slots[pos].first.view < r.view) {
                slots[pos].first.freeText();
                slots[pos].first = r.make_copy();
            }
        } else {
            slots[pos].first = r.make_copy();;
            slots[pos].second = true;
            length ++;
        }

    }
    void setView(int view) {
        for(auto& entry : slots) {
            if(entry.second) {
                entry.first.view = view;
            }
        }
    }
    vector<pair<Request, bool>> slots; // (request, occupiedQ)
    int length=0;
};

class LearnerSlot {
    unordered_map<int, pair<Request, unordered_set<int>>> history;
    // format: view_number->(request, (list of replicas))
public:
    ~LearnerSlot() {
        for(auto hentry : history) {
            hentry.second.first.freeText();
        }
    }
    int add(Request r, int ri) {
        // return the size of accepted of this request
        Request& myr = history[r.view].first;
        unordered_set<int>& myset = history[r.view].second;
        if(myset.size() == 0) {
            // the first message of this view
            myr = r;
        } else {
            // error detection, request of the same view should be the same
            if(!(myr == r)) {
                cerr << "Error: inconsistent request of a same view at a same slot" << endl;
                exit(-1);
            }
            // free the myr to save space
            myr.freeText();
            myr = r;
        }
        myset.insert(ri);
        return myset.size();
    }
};

class Learner {
    // data structure for learn a value
public:
    Phone* phone;
    RequestList learnLog;
    void init(Phone* p) {
        phone = p;
    }
    bool response(Request r) {
        // return false if it is not executable
        // return true if it is executable
        for(auto entry : learnLog.slots) {
            if(entry.second == false)
                return false;
            if(entry.first.contentMatch(r))
                return true;
        }
        return false;
    }
    void accepted() {
    // format: request, replicaID
        Request r = phone->read_request();
        int ri = phone->read_int();
        if( lss[r.position].add(r, ri) > phone->get_f() ) {
            // the request r is learned
            learnLog.add(r.make_copy());
            // log it
            phone->write_learnLog(r);
            learnLog.print();
        }
    }

    unordered_map<int, LearnerSlot> lss;
    //format: position -> learnerslot
};

class TempLog {
public:
    ~TempLog() {
        for(auto logEntry : logset) {
            logEntry.second.freeText();
        }
    }
    unordered_map<int, Request> logset; // (position, request)
    int append(Request r) {
        // append one log entry
        // and return the number of entries received
        if(logset.find(r.position) == logset.end()) {
            logset[r.position] = r;
        } else {
            r.freeText();
        }
        return logset.size();
    }
};

class ViewChange {
public:
    Phone* phone;
    void init(Phone* p) {
        phone = p;
        viewchange_view = 0;
    }
    void start(int view) {
        viewchange_view = view;
        // clear data
        viewchange_data.clear();
        completeList.clear();
    }
    unordered_map<int, TempLog> viewchange_data; // (id, packets)
    unordered_set<int> completeList; // (id)
    int viewchange_view;

    int add_data() {
    // format: view_number, replicaID, totalLogLen, request
    // return the number of completed replicas
        int hisview = phone->read_int();
        if(viewchange_view > hisview) {
            return completeList.size();
        }
        if(hisview > viewchange_view) {
            // error detection: it is impossible to receive promise from a higher view
            // NO, IT IS VERY POSSIBLE
            cerr << "Error: leader receives higher view promise" << endl;
            exit(-1);
        }
        // proceed only when tempview == view
        int replicaID = phone->read_int();
        int totalLogLen = phone->read_int();
        if(totalLogLen == 0) {
            completeList.insert(replicaID);
        } else {
            Request r = phone->read_request();
            TempLog& mylog = viewchange_data[replicaID]; // a new templog will be created if needed
            if(mylog.append(r) == totalLogLen) {
                completeList.insert(replicaID);
            }
        }
        return completeList.size();
    }
};