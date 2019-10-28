#include "replica.h"
#include "configure.h"

Replica::Replica(int myid) {
    id = myid;
    view = 0;

    phone = new Phone(myid); // create phone
    f = phone->get_f();
    heartBeatList = vector<chrono::nanoseconds>(2*f + 1); // if n is even, the last slot is unused
    leaderQ = false;
    parseLogFile();
}

Replica::~Replica() {
    delete phone;
}

void Replica::parseLogFile() {

}

void Replica::run() {
    // periodically send heartbeats
    thread hp(&Phone::heartBeat, phone); // create heart beat thread

    int sender_id;
    int sender_view_number;
    while(1) {
        // pickup the phone!
        switch (phone->phone_pickup())
        {
            case HEARTBEAT: 
                sender_id = phone->read_int();
                heartBeatList[sender_id] = chrono::system_clock::now().time_since_epoch();
                break;
            case VIEW_CHANGE: 
                sender_view_number = phone->read_int();
                // acceptor acts
                if(view < sender_view_number) {
                    // change view
                    view = sender_view_number;
                    viewLog();
                    // send promise
                    sendPromise();
                } else if (view == sender_view_number) {
                    sendPromise();
                }
                break;
            case VIEW_PROMISE:
                // leader in state WAIT_PROMISE acts
                if (view%(2*f+1) == id && !leaderQ) {
                    receivePromise();
                }
                break;
            case CLIENT_REQUEST: 
                // leader acts
                int cl = view%(2*f+1);
                if (cl != id) {
                    // state IDLE
                    if (!check_heart_beat()) {
                        // change view
                        if(id < cl) {
                            view +=  id + 2*f+1  - cl;
                        } else {
                            view += id - cl;
                        }
                        viewLog();
                        // initiate view change
                        viewchange.init();
                    }
                } else if (!leaderQ) {
                    // state WAIT_PROMISE
                    enforceViewChange();
                } else {
                    // state LEADING
                    process_request();
                }
                break;
            case ACCEPT_IT: 
                // acceptor acts
                sender_view_number = phone->read_int(); 
                Request r = phone->read_request();
                if(view <= sender_view_number) {
                    accept_it(sender_view_number, r);
                }
                break;
            case ACCEPTED: 
                cout << "[MESSAGE TYPE: ACCEPTED] ";
                // learner acts
                learner.accepted(msg.content, chatLog);
                break;
            default:
                cerr << "Replica Error: unknown header" << endl;
                exit(-1);
        }
    }
}

bool Replica::check_heart_beat() {
    // ENSURE id != view%(2*f+1)
    int cl = view%(2*f+1);
    if(id > cl) {
        for(int k=cl; k< id; k++) {
            if(time_since_last_heartbeat(k) <= PATIENCE_TIME) {
                // there are living processes between the view leader and me
                return true;
            }
        }
    } else {
        // we have id < cl
        for(int k=cl; k< 2*f+1; k++) {
            if(time_since_last_heartbeat(k) <= PATIENCE_TIME) {
                // there are living processes between the view leader and me
                return true;
            }
        }
        for(int k=0; k< id; k++) {
            if(time_since_last_heartbeat(k) <= PATIENCE_TIME) {
                // there are living processes between the view leader and me
                return true;
            }
        }
    }
    return false;
}