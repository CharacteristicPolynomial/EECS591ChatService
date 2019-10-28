# EECS591ChatService
course implementation project.

## a nice picture
![momo](momo_16_9.png)

## Design Schemes
### Philosophy
Since this project focuses on the correctness part, I will focus on simplification and will not do any optimization. 

All parts may lazily act in the sense that, "if at any point I can ensure both safety and liveness withtout doing certain action, then I may not do that. "

On the other hand, all parts may act brainlessly in the sense that, "I will not refrain to do things even if it is unnecessary as long as both safety and liveness are still guaranteed."

Following the philosophy of laziness and brainlessness, roles will seem very dumb. But the whole point have their acts defined in a simple stated manner.

### Choices
1. Clients send the same request repeatedly until get a ACK response. This is to ensure liveness. 
2. Replicas are given a logfile which initially may not be empty. Replicates need to parse that logfile to get an initial state. Each replicate can be reconstructed from the triple (AcceptorLog, LearnerLog, View).
3. Leader and Acceptor on a same replica share a same acceptLog and a same view number. 
4. Leader has three states: (the pair (view, leaderQ) can determine the state)
    - LEADING, state where it gets majority in his view. In the LEADING state, the process goes as a leader. 
    - WAIT_PROMISE, state where it is in his view but hasn't got the majority. When receiving client request in this state, it enfoces viewchange to others. It goes to LEADING when the majority is obtained (by setting leaderQ=true). 
    - IDLE, state where it is not in his view. When receiving client request in this state, it checks the heartbeat list, if haven't heard from the replicas between the current view and itself, it initializes the view_change structure and enters WAIT_PROMISE state by increase the view number to his next view number.
5. Acceptor only has one working mode. When receiving VIEW_CHANGE, change view if it has a higher view and send all the VIEW_PROMISE packets and increases the view. If the coming view is the same as my view, send all the VIEW_PROMISE packets without changing the view. When receiving ACCEPT_IT, if the view is higher than my view, accept it and broadcast ACCEPTED messages.
6. Learner only has one working mode. Whenever comes an ACCEPTED message, add it into some data structure, which will learn a (value, slot) once get a majority of that of a same view.

### Message digest
| Role | Receiving requests | Sending requests |
| --- | --- | --- |
| Replica | HEARTBEAT | HEARTBEAT |
| Leader (LEADING) | CLIENT_REQUEST | ACCEPT_IT, ACK |
| Leader (WAIT_PROMISE) | VIEW_PROMISE, CLIENT_REQUEST | VIEW_CHANGE |
| Leader (IDLE) | CLIENT_REQUEST |  |
| Acceptor | VIEW_CHANGE, ACCEPT_IT | VIEW_PROMISE, ACCEPTED |
| Learner | ACCEPTED | |
| Client | ACK | CLIENT_REQUEST |

### Log
1. Acceptor logs whenever change the view.
2. Acceptor logs whenever accepts an ACCEPT_IT message.
3. Learner logs whenever learned a slot.

## Correctness
### Argument of liveness
Each time a client resends its request, this pushes the system to either
1. Response to the client if the leader is correct and the request is learned and executable.
2. Enforce others to accept all the slots in order to get a response if the leader is correct and if it is not executable or even not learned.
3. Tries to become a leader if the leader is not believed to be correct.

Eventually the client will get a response as long as f+1 replica are alive.

### Argument of safety
By the safety of Paxos, once a slot is learned, any newly learned request at that slot will be the same. Every leader will only propose a single value for each slot.

## Format
### Classes/Enum

#### enum HEADER
We have a HEADER enum type for headers. 
    VIEW_CHANGE,
    VIEW_PROMISE,
    CLIENT_REQUEST,
    HEARTBEAT, 
    ACCEPT_IT,
    ACCEPTED,
    ACK, 
    VIEW_LOG,
    ACCEPT_LOG,
    LEARN_LOG.

#### class Request
Also we define a Request class as follows. Note that, for the Request class, it has two forms, the decoded form (Request) and the encoded form (void*)
| Member of *Request* |  |
| --- | --- |
| (int) view | the view that sends this request with ACCEPT_IT header |
| (int) position | the position of the slot |
| (struct sockaddr_in) clientAddr | the address of the client that initiates this requests |
| (int) seq | the client sequence number of this request |
| (int) textLen | the length of the text |
| (char*) text | the pointer to the head of the text |

| Member functions of *Request*   |  |
| --- | --- |
| Request(const char* buf, int buflen) | constructor, decodes buf |
| ~Request() | destructor |
| int encode(char* buf) | encode itself to buf, return the number of usd bytes |

### Communication protocols
| Request | Format |
| --- | --- |
| VIEW_CHANGE | VIEW_CHANGE (HEADER), view (int) | 
| VIEW_PROMISE | VIEW_PROMISE (HEADER), view (int), replicaID (int), totalLogLen (int), request (Request)|
| CLIENT_REQUEST | CLIENT_REQUEST (HEADER), seq (int), textlen (int), text (no-type) |
| ACCEPT_IT | ACCEPT_IT (HEADER), view (int), request (Request) |
| ACCEPTED |  ACCEPTED (HEADER), view (int), replicaID (int), request (Request) |
| ACK | ACK (HEADER), seq (int) |
| HEARTBEAT | HEARBEAT (HEADER), replicaID (int) |

### Log file
To increase readabiliity of the log, we use English style log rather than binary log. Thus the log entries are "printed in ASCII" instead of being binary coded.
| Log entry | Format |
| --- | --- |
| view change | VIEW_LOG (HEADER), view (int) |
| acceptor log | ACCEPT_LOG (HEADER), request (Request)  |
| learner log | LEARN_LOG (HEADER), request (Request) |

## Test System
There are a few features I should implement for debugging and simulating message loss.
### class Phone
This class wraps all communications, so that we can simulate different channel condition.
| Member of *Phone* |  |
| --- | --- |
| int id | my replica ID |
| int fd | my socket file descriptor |
| vector\<struct sockaddr_in\> replicaAddrs | addresses of all replicas |
| int* recvLen | |
| char* recvBuffer | |
| int* sendLen | |
| char* sendBuffer | |
| struct sockaddr_in comingAddr | the sending address of the last message |

| Member functions of *Phone*   |  |
| --- | --- |
| Phone(const char* addrFile, int myid, int myfd) | constructor |
| check_phone() | receive message |
| send_to(int replicaID) | send message to a certain replica |
| broadcast() | send messages to all replica | 
| reply() | send ACK to the client |
| editMessage() | edit the sendBuffer |

### class DebugLog
This class wraps all debugging logs.
| Member of *DebugLog* |  |
| --- | --- |
| bool vLogQ |  |
| ofstream vLog | the most verbose log |
| bool LogQ |  |
| ofstream Log | log 1 |
| bool Log2Q |  |
| ofstream Log2 | log 2 |

| Member functions of *DebugLog*   |  |
| --- | --- |
| DebugLog(int replicaID) | constructor |
| log() | log states |

## Structures
    class Replica {
        Phone phone {
            int id;
        };
        DebugLog debugLog;
        int view;
        RequestList acceptorLog, learnerLog {

        };
        Learner {
            
        };
    }

## Tools
| Name | Usage |
| --- | --- |
| genConfig | Takes 2f+1 as argument, fill addressFile with usable addresses |