#include "replica.h"
#include "configure.h"

Replica::Replica(int myid) {
    // cout << "initilizing replica" << endl;
    id = myid;
    view = 0;
    phone.init(id); // initiate phone
    viewchange.init(&phone);
    learner.init(&phone, id);

    viewchangeTimer.init(chrono::duration_cast<chrono::nanoseconds>(
                chrono::milliseconds(VIEW_CHANGE_TIME)));
    requestTimer.init(chrono::duration_cast<chrono::nanoseconds>(
                chrono::milliseconds(REQUEST_TIME)));
    bornTime = chrono::system_clock::now().time_since_epoch();
    

    acceptLogFile = get_config(ACCEPTLOG_PREFIX) + to_string(id) + get_config(ACCEPTLOG_SUFFIX);

    f = phone.get_f();
    // cout << "id " << id << " view " << view << " f " << f << endl;
    heartBeatList = vector<chrono::nanoseconds>(2*f + 1, bornTime); // if n is even, the last slot is unused
    leaderQ = false;
    parseLogFile();

    // if the replica is in his view, then start a view change
    if( view%(2*f+1) == id) {
        viewchange.start(view);
    }
}

void Replica::run() {
    // periodically send heartbeats
    thread hp(&Phone::heartBeat, &phone); // create heart beat thread

    int sender_id;
    int sender_view_number;
    int cl;
    while(1) {
        // pickup the phone!
        switch (phone.phone_pickup())
        {
            case NO_MESSAGE:
            
                break;
            case HEARTBEAT: 
                sender_id = phone.read_int();
                heartBeatList[sender_id] = chrono::system_clock::now().time_since_epoch();
                // if(sender_id == id)
                //     break;
                // if ((view%(2*f+1) == id) && (!leaderQ)) {
                //     if (viewchange.completeList.find(sender_id) == viewchange.completeList.end()) {
                //         viewchangeTimer.use();
                //     }
                // }
                break;
            case VIEW_CHANGE: 
                sender_view_number = phone.read_int();
                sender_id = sender_view_number % (2*f +1);
                heartBeatList[sender_id] = chrono::system_clock::now().time_since_epoch();
                // cout << "view change " <<  sender_view_number << endl;
                // acceptor acts
                if(view < sender_view_number) {
                    // change view
                    view = sender_view_number;
                    // quit the leader
                    leaderQ = false;
                    phone.write_viewLog(view);
                    // send promise
                    sendPromise();
                } else if (view == sender_view_number) {
                    sendPromise();
                }
                break;
            case VIEW_PROMISE:
                // leader in state WAIT_PROMISE acts
                if ((view%(2*f+1) == id) && (!leaderQ)) {
                    viewchangeTimer.use();
                    if(viewchange.add_data() >= f+1) {
                        // majority got!
                        leaderQ = true;
                        cout << "replica " << id << " receives majority's promises "
                         << "and becomes the leader in view " << view << endl;
                        // process view change data
                        process_view_change();
                    }
                }
                break;
            case CLIENT_REQUEST: 
                // leader acts
                cl = view%(2*f+1);
                if (cl != id) {
                    // state IDLE
                    if (!check_heart_beat()) {
                        // change view
                        int temp = view;
                        if(id < cl) {
                            view +=  id + 2*f+1  - cl;
                        } else {
                            view += id - cl;
                        }
                        cout << "replica " << id << " goes out of patience in view "
                        << temp << " and initiates a view change to "
                         << view << endl;
                        phone.write_viewLog(view);
                        // initiate view change
                        viewchange.start(view);
                        // enforce view change
                        if (viewchangeTimer.coolDown()) {
                            enforceViewChange();
                        }
                    }
                } else if (!leaderQ) {
                    // state WAIT_PROMISE
                    if (viewchangeTimer.coolDown()) {
                        cout << "replica " << id << " enforces view change to "
                         << view << " again" << endl;
                        viewchange.printProgress();
                        enforceViewChange();
                    }
                } else {
                    // state LEADING
                    process_request();
                }
                break;
            case ACCEPT_IT: 
                // acceptor acts
                accept_it();
                break;
            case ACCEPTED: 
                // learner acts
                learner.accepted();
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
    int dis = 0;
    if (id > cl) {
        dis = id-cl;
    } else {
        dis = 2*f+1+id-cl;
    }
    return time_since_last_heartbeat(cl) <= (PATIENCE_TIME + PATIENCE_TIME_INCREMENT * dis);
}