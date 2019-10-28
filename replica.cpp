#include "replica.h"
#include "configure.h"

Replica::Replica(int myid) {
    id = myid;
    view = 0;

    phone = new Phone(myid); // create phone
    f = phone->get_f();
    heartBeatList = vector<chrono::nanoseconds>(2*f + 1); // if n is even, the last slot is unused
}

Replica::~Replica() {
    delete phone;
}

void Replica::run() {
    int sender_id;
    int sender_view_number;
    while(1) {
        switch (phone->phone_pickup())
        {
            case HEARTBEAT: 
                sender_id = phone->read_int();
                heartBeatList[sender_id] = chrono::system_clock::now().time_since_epoch();
                break;
            case VIEW_CHANGE: 
                sender_view_number = phone->read_int();
                if (sender_view_number >= view) {
                    view = sender_view_number;
                    if(sender_view_number > view) {
                        // write log
                        viewlog();
                        // leader acts
                        newLeader();
                    }

                    // acceptor acts
                    sendPromise(senderAddr);

                }
                break;
            case VIEW_PROMISE:
                cout << "[MESSAGE TYPE: VIEW_PROMISE] ";
                // leader acts
                receivePromise(msg.content);
                break;
            case CLIENT_REQUEST: 
                cout << "[MESSAGE TYPE: CLIENT_REQUEST] ";
                receiveRequest(msg.content, senderAddr);
                break;
            case ACCEPT_IT: 
                cout << "[MESSAGE TYPE: ACCEPT_IT] ";
                // acceptor acts
                acceptit(msg.content);
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