# EECS591ChatService
## Introduction
Course implementation project- a crash-tolernt chat service which are always safe. It should be live as long as from some certain point there are f+1 correct replicas and messages are delivered synchronously.

![momo](momo_16_9.png)

## Design Elements
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

In the following sections I will describe what exactly each role will act. Note that each role has no memory apart from what I have listed above.

### Leader

| State | view | leaderQ |
| --- | --- | --- |
| LEADING | view = id (mod (2f+1)) | True |
| WAIT_PROMISE | view = id (mod (2f+1)) | False |
| IDLE | otherwise | * |

| Message | LEADING | WAIT_PROMISE | IDLE |
| --- | --- | --- | --- |
| CLIENT_REQUEST | if it is executable, reply ACK; if not, enforce all unlearned slots before that request (add NOOPs if necessary). | enforce view change | check heartBeats and set view |
| VIEW_PROMISE | ignore | add to viewchange | ignore |

### Acceptor

### Learner

## Script Tools

| Name | Usage |
| --- | --- |
| genConfig | Takes 2f+1 as argument, fill addressFile with usable addresses |
| manager | a command line program that manages the processes |

## Request Memory Track
1. VIEW_PROMISE: Phone::read_request()->TempLog::append()->(TempLog or Die)
2. VIEW_PROMISE: ViewChange -> update(), make_copy() replaces acceptLog
3. CLIENT_REQEUST: in process_request(), make_request()-> (Die or stores in acceptLog)
4. ACCEPT_IT: read_request()->(Die or store in acceptLog)
5. accepted: read_request()->(store in learner)
