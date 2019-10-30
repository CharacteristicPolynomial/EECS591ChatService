# EECS591ChatService
## Introduction
Course implementation project- a crash-tolernt chat service which are always safe. It should be live as long as from some certain point there are f+1 correct replicas and messages are delivered synchronously.

## Usage
### Executables
manager, genConfig, makeClient, makeReplica.

### Complie/Execute
| actions | commands |
| --- | --- |
| compile executables | `make all` |
| remove executables and logs | `make clean` |
| run manager | `./manager` |
| manually start service | `./genConfig (2f+1)` |
| manually run client | `./makeClient -m` |
| manually run replicas | `./makeReplica (id)` |

### Manager (used for managing replica processes and organizing robot clients)
Manager is a c++ program which manages a whole service session.

| command(alias) | usage |
| --- | --- |
| help(help) | print help |
| status(s) | print manager status |
| init | initializes all the things (and remove all logs) |
| halt | stop all processes (without removing the logs) |
| exit | halt and exit the manager |
| genAddr(ga) [n] | this will call init, generate n addresses to the configured address file and start the service (though there is no living replica and client at this point) |
| addRobotClient(arc) | add a robot client that will send "robot message (seq)" for increasing seq |
| addRobotClientN(arcn) [n] | add n robot clients |
| activateReplica(ar) [n] | activate replica n |
| activateReplicaRange(arr) [b] [e] | activate replicas b, b+1, ..., e-1 |
| resumeService(rs) | resume all paused replicas (send SIGCONT to replicas) |
| killReplica(kr) [n] | kill replica n |
| killReplicaRange(krr) [b] [e] | kill replicas b, b+1, ..., e-1 |
| pauseService(ps) | pause all living replicas (send SIGSTOP to replicas) |
| checkConsistency(cc) | check whether the chat logs are consistent |
| checkLog(cl) | print information of chat logs |

Below is a possible command sequence.

    >   ga 101          # creates a service with 101 replicas
    >   arr 50 101      # activate replicas 50, 51, ..., 100
    >   arcn 100        # add 100 robot clients
    ...                 # start making progress
    >   ps              # pause the service
    >   cl              # check chat log information
    >   cc              # check chat log consistency
    >   krr 50 101      # kill replicas 50, 51, ..., 100
    >   arr 0 50        # activate replicas 0, 1, ..., 49
    >   rs              # resume the service
    ...                 # no progress will be make (only 50 living replicas)
    >   ar 100          # activate replica 100
    ...                 # service becomes alive
    >   halt            # kill all replicas and clients (and cannot be resumed)
    >   cl              # check chat log information
    >   cc              # check chat log consistency
    >   exit            # exit manager

**Note:** 
1. There will be some information going in the manager's terminal, which is used to reassure that the system is keeping live.
As a result, you may want to type `ps` and enter to pause the service.
Note that, the replicas are using system time clock. Therefore, when you type `ps`, you are not really freezing the time of the replicas. You are just holding them from executing. 
As a side effect, there could appear unnecessary view change sessions after a long pause. Since all the replicas look at the system clock and find that it has been a long (system) time since the last time I received the heartbeat from the leader.
2. By calling `exit` in `manager` will not automatically delete any log file. You can either call `init` in `manager` before calling `exit`. Or you can use the command `make cleanLog` to remove the logs.
3. You can of course open another terminal (could be on a difference device) to create manual/automatic mode clients. You need to get the address file of the replicas before running clients on other devices.
4. The program `manager` don't support having servers deployed on different device (in fact, it is not supported even in different directories on the same machine). However, the replicas support this. Thus it is just a matter of some tricks to have replicas deplyed on different locations. For example, you can do the following.
    - Run `manager` on machine A with 101 replicas.
    - Run `manager` on machine B with 101 replicas.
    - Before activating and replicas, replace address files on both machiens with (first 50 addresses of A) || (last 51 addresses of B).
    - A's `manager` only governs the first 50 replicas and B's `manager` governs the rest.
    - Do whatever you want from now on.

### Files

| files | usage | type |
| --- | --- | --- |
| AddressFile | list of all replicas' addresses | Text |
| LogFile | stable memory for replicas | binary |
| ChatLog | log for all learned slots of replicas (for consitency checking) | English |
| MessageLog | log for all messages of replicas (for debug) | English |
| AcceptLog | log for all accepted slots of replicas (for debug) | English |

- Set configurations in serverConfigure.config (no recompilation needed).
- Tune more parameters in configure.h (recompliation needed).

## Assumptions
- The size of a single client request is at most BUFFER_SIZE(=1024).



## Design Elements
![momo](momo_16_9.png)
### Enum *HEADER*

    enum HEADER {
        VIEW_CHANGE,
        VIEW_PROMISE,
        CLIENT_REQUEST,
        HEARTBEAT, 
        ACCEPT_IT,
        ACCEPTED,
        ACK, 
        VIEW_LOG,
        ACCEPT_LOG,
        LEARN_LOG
    };

### Class *Request*

| Member of *Request* |  |
| --- | --- |
| (int) view | the view of this request |
| (int) position | the position of its slot |
| (struct sockaddr_in) clientAddr | the address of the client that initiates this request |
| (int) seq | the client sequence number of this request |
| (int) textLen | the length of the text |
| (char*) text | the pointer to the head of the text |

### Log Format

| Log entry | Format |
| --- | --- |
| view change | entryLen (int), VIEW_LOG (HEADER), view (int) |
| acceptor log | entryLen (int), ACCEPT_LOG (HEADER), request (Request)  |
| learner log | entryLen (int), LEARN_LOG (HEADER), request (Request) |

### Communication Protocols

| Request | Format |
| --- | --- |
| VIEW_CHANGE | VIEW_CHANGE (HEADER), view (int) | 
| VIEW_PROMISE | VIEW_PROMISE (HEADER), view (int), replicaID (int), totalLogLen (int), request (Request)|
| CLIENT_REQUEST | CLIENT_REQUEST (HEADER), seq (int), textlen (int), text (no-type) |
| ACCEPT_IT | ACCEPT_IT (HEADER), request (Request) |
| ACCEPTED |  ACCEPTED (HEADER), request (Request), replicaID (int) |
| ACK | ACK (HEADER), seq (int) |
| HEARTBEAT | HEARBEAT (HEADER), replicaID (int) |

## Logic Design

### Message Digest

| Role | Receiving requests | Sending requests |
| --- | --- | --- |
| Replica | HEARTBEAT | HEARTBEAT |
| Leader (LEADING) | CLIENT_REQUEST | ACCEPT_IT, ACK |
| Leader (WAIT_PROMISE) | VIEW_PROMISE, CLIENT_REQUEST | VIEW_CHANGE |
| Leader (IDLE) | CLIENT_REQUEST |  |
| Acceptor | VIEW_CHANGE, ACCEPT_IT | VIEW_PROMISE, ACCEPTED |
| Learner | ACCEPTED | |
| Client | ACK | CLIENT_REQUEST |

### Clients
Repeatedly send a same request until a corresponding ACK is received. The requests are ordered, a client can only send the next request after receiving ACK of its current request.

### Replicas
A replica has the following members

    vector<Request> acceptLog;  // need to be logged to stable memory
    vector<Request> learnLog;   // need to be logged to stable memory
    int view;                   // need to be logged to stable memory

    int id; // configured initially
    int f;  // configured initially

    bool leaderQ; // state variable
    vector<chrono::nanoseconds> heartBeatList; // list store heartBeats

    Phone phone; // communication interface
    ViewChange viewchange; // view change data interface
    Learner learner; // learner interface

## Script Tools

| Name | Usage |
| --- | --- |
| genConfig | Takes 2f+1 as argument, fill addressFile with usable addresses |
| manager | a command line program that manages the processes |
