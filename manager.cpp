#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "configure.h"
#include <fstream>
#include "message.h"
using namespace std;

void halt();
void pauseService();
void resumeService();
void activateReplica(int id);
void resumeReplica(int id);
// state variables
enum STATE {
    INIT, 
    RUNNING,
    END
};
STATE state;
int n; // the total number of replicas
bool pausedQ;

void checkLog() {
    cout << "checking logs" << endl;
    for(int i=0; i<n; i++) {
        cout << "chat log of replica " << i << " : ";
        string filei = get_config(CHATLOG_PREFIX) + to_string(i) + get_config(CHATLOG_SUFFIX);
        ifstream ifsi;
        ifsi.open(filei);
        if(ifsi.fail()) {
            cout << "empty." << endl;
            ifsi.close();
            continue;
        }
        
        char bufi[BUFFER_SIZE];
        int holeN = 0;
        int total = 0;
        while(1) {
            memset(bufi, 0, BUFFER_SIZE);
            ifsi.getline(bufi, BUFFER_SIZE);
            if(ifsi.eof()) {
                break;
            }
            if(strcmp(bufi, "(Empty slot)")==0) {
                holeN ++ ;
            }
            total++;
        }
        cout << "log length (" << total << ") holes (" << holeN << ")" << endl;
        ifsi.close();
    }
}

void checkConsistency() {
    // ensure all replicas are killed, because we are reading their logs
    cout << "running consistency check" << endl;

    for(int i=0; i< n; i++) {
        for(int j=i+1; j<n; j++) {
            // cout << "comparing replica " << i 
            //     << " and replica " << j << " : ";
            string filei = get_config(CHATLOG_PREFIX) + to_string(i) + get_config(CHATLOG_SUFFIX);
            string filej = get_config(CHATLOG_PREFIX) + to_string(j) + get_config(CHATLOG_SUFFIX);
            ifstream ifsi, ifsj;
            ifsi.open(filei);
            ifsj.open(filej);
            if(ifsi.fail()) {
                // cout << "chat log " << i << " is empty, automatically consistent." << endl;
                ifsi.close();
                ifsj.close();
                continue;
            }
            if(ifsj.fail()) {
                // cout << "chat log " << j << " is empty, automatically consistent." << endl;
                ifsi.close();
                ifsj.close();
                continue;
            }
            // both log i and log j exists
            char bufi[BUFFER_SIZE];
            char bufj[BUFFER_SIZE];
            int k = 0;
            while(1) {
                memset(bufi, 0, BUFFER_SIZE);
                memset(bufj, 0, BUFFER_SIZE);
                ifsi.getline(bufi, BUFFER_SIZE);
                ifsj.getline(bufj, BUFFER_SIZE);
                if(ifsi.eof()) {
                    // cout << "consistent until the end of chat log " << i << endl;
                    break;
                }
                if(ifsj.eof()) {
                    // cout << "consistent until the end of chat log " << j << endl;
                    break;
                }
                if(memcmp(bufi, bufj, BUFFER_SIZE) == 0) {
                    continue;
                } else {
                    if(strcmp(bufi, "(Empty slot)")==0)
                        continue;
                    if(strcmp(bufj, "(Empty slot)")==0)
                        continue;
                    // both are not empty and not equal!
                    // probamatic! Report the bug
                    cout << endl << "INCONSISTENCY ERROR between chat log "
                        << i << " chat log " << j << " at slot " << k << endl;
                    cout << "CHATLOG " << i << " : " << bufi << endl;
                    cout << "CHATLOG " << j << " : " << bufj << endl;
                    ifsi.close();
                    ifsj.close();
                    return;
                }
                k++;
            }
            ifsi.close();
            ifsj.close();
        }
    }
    cout << "all chat logs are consistent." << endl;
}

void init() {
    cout << "initializing" << endl;
    halt();

    string temp;
    cout << "removing address file" << endl;
    temp = "rm " + get_config(ADDRESS_FILE);
    system(temp.c_str());

    cout << "removing all logs" << endl;
    temp = "rm " + get_config(MSGLOG_PREFIX) + "*" + get_config(MSGLOG_SUFFIX);
    system(temp.c_str());
    temp = "rm " + get_config(LOGFILE_PREFIX) + "*" + get_config(LOGFILE_SUFFIX);
    system(temp.c_str());
    temp = "rm " + get_config(ACCEPTLOG_PREFIX) + "*" + get_config(ACCEPTLOG_SUFFIX);
    system(temp.c_str());
    temp = "rm " + get_config(CHATLOG_PREFIX) + "*" + get_config(CHATLOG_SUFFIX);
    system(temp.c_str());
    state = INIT;
    pausedQ = false;
}

vector<pid_t> clients;
unordered_map<int, pid_t> replicas; // format: replicaID -> process id


void resumeService() {
    if (pausedQ == false) {
        cout << "you cannot resume a non-paused service" << endl;
        return;
    }
    cout << "resuming the servers" << endl;
    pausedQ = false;
    for(auto p : replicas) {
        resumeReplica(p.first);
    }
}

void resumeReplica(int id) {
    kill(replicas[id], SIGCONT);
}

void pauseReplica(int id) {
    kill(replicas[id], SIGSTOP);
}

void pauseService() {
    if (pausedQ == true) {
        cout << "it has been paused already" << endl;
        return;
    }
    cout << "pausing the servers" << endl;
    pausedQ = true;
    for(auto p : replicas) {
        pauseReplica(p.first);
    }
}

void closeService() {
    cout << "closing the servers" << endl;
    for(auto p : replicas) {
        kill(p.second, SIGKILL);
    }
    replicas.clear();
}

void halt() {
    cout << "killing all processes" << endl;
    int k = 0;
    for(auto p : clients) {
        // cout << "killing client " << k++ << endl;
        kill(p, SIGKILL);
    }
    clients.clear();
    closeService();
}

void addRobotClient() {
    pid_t pid = fork();
    if(pid == 0) {
        // children process
        string temp = to_string(clients.size());
        execl("./makeClient", "makeClient", "-a", temp.c_str(), NULL);
    } else {
        // parent process
        // do nothing
        cout << "client " << clients.size() << " added" << endl;
        clients.push_back(pid);
    }
}

void activateReplica(int id) {
    if (id >= n) {
        cout << "activate replica " << id << " failed (you only have " << n << " replicas)" << endl;
        return;
    } 
    if (replicas.find(id) != replicas.end()) {
        cout << "activate replica " << id << " failed (replica " << id << " has been activated)" << endl;
        return;
    }
    pid_t pid = fork();
    if(pid == 0) {
        // children process
        string temp;
        temp = to_string(id);
        execl("./makeReplica", "makeReplica", temp.c_str(), NULL);
    } else {
        // parent process
        // do nothing
        replicas[id] = pid;
        if (pausedQ == true) {
            pauseReplica(id);
            return;
        }
        // cout << "Replica " << id << " activated" << endl;
    }
}

void killReplica(int id) {
    if (id >= n) {
        cout << "kill replica " << id << " failed (you only have " << n << " replicas" << endl;
        return;
    } 
    if (replicas.find(id) == replicas.end()) {
        cout << "kill replica " << id << " failed (replica " << id << " hasn't been activated)" << endl;
        return;
    }
    kill(replicas[id], SIGKILL);
    replicas.erase(id);
    // cout << "Replica " << id << " killed" << endl;
}

int genAddr(int k) {
    string temp;
    temp = "./genConfig " + to_string(k);
    system(temp.c_str());
}

void status() {
    cout << "manager is in ";
    if(state == INIT) {
        cout << "initial state." << endl;
        return;
    } else if(state == RUNNING) {
        cout << "running state." << endl;
        cout << "pausedQ=" << pausedQ << endl;
        cout << "total replicas: " << n << endl;
        cout << "number of robot clients: " << clients.size() << endl;
        cout << "living replicas: " << replicas.size() << endl;
    } else {
        // END state
        cout << "end state." << endl;
        cout << "total replicas: " << n << endl;
    }
}


void printHelp() {
    cout << "help(help) : " << "print help" << endl;
    cout << "status(s) : " << "print manager status" << endl;
    cout << "init : " << "initializes all the things" << endl;
    cout << "halt : " <<  "stop all processes (without removing the logs)" << endl;
    cout << "exit : " << "halt and exit the manager" << endl;
    cout << "genAddr(ga) [n] : " << "generate n addresses to the configured address file and start the service" << endl;
    cout << "addRobotClient(arc) : " << "add a robot client" << endl;
    cout << "addRobotClientN(arcn) [n] : " << "add n robot clients" << endl;
    cout << "activateReplica(ar) [n] : " << "activate replica n" << endl;            
    cout << "activateReplicaRange(arr) [b] [e] : " << "activate replicas b, b+1, ..., e-1" << endl;
    cout << "resumeService(rs) : " << "resume all paused replicas" << endl;
    cout << "killReplica(kr) [n] : " << "kill replica n" << endl;
    cout << "killReplicaRange(krr) [b] [e] : " << "kill replicas b, b+1, ..., e-1" << endl;
    cout << "pauseService(ps) : " << "pause all living replicas" << endl;
    cout << "checkConsistency(cc) : " << "check whether the chat logs are consistent" << endl;
    cout << "checkLog(cl) : " << "print information of chat logs" << endl;
}

int main() {
    state = INIT;
    pausedQ = false;
    cout << "Welcome to the chat servcie manager" << endl;
    cout << "-----------------------------------" << endl;
    printHelp();
    cout << "-----------------------------------" << endl;
    while(1) {
        cout << ">  ";
        string command;
        cin >> command;
        if (command == "init") {
            init();
            state = INIT;
        } else if(command == "halt") {
            if(state != RUNNING) {
                cout << "service is not running" << endl;
                continue;
            }
            halt();
            state = END;
        } else if(command == "exit") {
            halt();
            return 0;
        } else if(command == "genAddr" || command == "ga") {
            int k;
            if(cin >> k) {
                if (state == RUNNING) {
                    cout << "you cannot generate addresses in a running state" << endl;
                } else if (state == END) {
                    cout << "initialize before generating new address file" << endl;
                } else {
                    init();
                    genAddr(k);
                    cout << "the service is running now (with 2f+1=" << k << ")" << endl;
                    state = RUNNING;
                    n = k;
                }
            } else {
                cout << "genAddr (the number of addresses)" << endl;
                cin.clear();
            }
        } else if(command == "addRobotClient" || command == "arc") {
            if(state != RUNNING) {
                cout << "you cannot add client since service is not running" << endl;
            } else addRobotClient();
        } else if(command == "addRobotClientN" || command == "arcn") {
            int k;
            if(cin >> k) {
                if(state != RUNNING) {
                    cout << "you cannot add clients since service is not running" << endl;
                } else {
                    for(int i=0; i<k; i++)
                        addRobotClient();
                }
            } else {
                cout << "addRobotClientN (number of clients)" << endl;
                cin.clear();
            }   
        } else if(command == "activateReplica" || command == "ar") {
            int k;
            if(cin >> k) {
                if(state != RUNNING) {
                    cout << "you cannot activate replica since service is not running" << endl;
                } else {
                    activateReplica(k);
                }
            } else {
                cout << "activateReplica (replicaID)" << endl;
                cin.clear();
            }            
        } else if(command == "activateReplicaRange" || command == "arr") {
            int k1, k2;
            if(cin >> k1 >> k2) {
                if(state != RUNNING) {
                    cout << "you cannot activate replicas since service is not running" << endl;
                } else {
                    for (int i=k1; i<k2; i++)
                        activateReplica(i);
                }
            } else {
                cout << "activateReplica (start) (end)" << endl;
                cin.clear();
            }            
        } else if(command == "resumeService" || command == "rs") {
            if(state != RUNNING) {
                cout << "you cannot resume replicas since service is not running" << endl;
            } else {
                resumeService();
            }
        } else if(command == "killReplica" || command == "kr") {
            int k;
            if(cin >> k) {
                if(state != RUNNING) {
                    cout << "you cannot kill replica since service is not running" << endl;
                } else {
                    killReplica(k);
                }
            } else {
                cout << "killReplica (replicaID)" << endl;
                cin.clear();
            }            
        } else if(command == "killReplicaRange" || command == "krr") {
            int k1, k2;
            if(cin >> k1 >> k2) {
                if(state != RUNNING) {
                    cout << "you cannot kill replicas since service is not running" << endl;
                } else {
                    for (int i=k1; i<k2; i++)
                        killReplica(i);
                }
            } else {
                cout << "killReplica (start) (end)" << endl;
                cin.clear();
            }            
        } else if(command == "pauseService" || command == "ps") {
            if(state != RUNNING) {
                cout << "you cannot pause replicas since service is not running" << endl;
            } else {
                pauseService();
            }
        } else if (command == "checkConsistency" || command == "cc") {
            if(state == INIT) {
                cout << "you haven't started the service" << endl;
                continue;
            }
            if(pausedQ == false) {
                cout << "you may only check consistency when the service is paused" << endl;
                continue;
            }
            checkConsistency();
        } else if (command == "checkLog" || command == "cl") {
            if(state == INIT) {
                cout << "you haven't started the service" << endl;
                continue;
            }
            if(pausedQ == false) {
                cout << "you may only check logs when the service is paused" << endl;
                continue;
            }
            checkLog();
        } else if (command == "status" || command == "s") {
            status();
        } else if (command == "help") {
            printHelp();
        } else {
            cout << "invalid command" << endl;
        }
    }
    return 0;
}
