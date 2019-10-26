# EECS591ChatService
course implementation project.

## a nice picture
![momo](momo_16_9.png)

## Specifications
### Requirements
1. Safety: All replicas agree on the chat log at all time.
2. Liveness: The service should remain available despite the failure of f replicas given synchrony.
### Design Schemes
1. There are at least two c++ programs- one for replicas and one for clients. There may be other programs and scripts to help testing.
2. Clients send requests in form of (request, requestID) to all replicas. Replicas handle the requests in form of (clientIP, request, requestID). Replicas send responses in form of (response, requestID) to the corresponding clientIP.
3. Clients repeatedly with time CLIENT_RETRY_TIME_OUT send a (request, requestID) until get the response with the matching requestID.
4. There needs an initialization phase for the replicas, which allow them to establish full connections.
5. Clients needs to connect to at least f+1 replicas to start enjoying service.
6. 
### Arguments
- Replicas
    1. f
    2. a unique replica ID
    3. list of replicas' (IP, port) 
### Parameters
- Replicas
- Clients
    1. CLIENT_RETRY_TIME_OUT: time in seconds before resending the request
    2. 
- Config
    1. f, number of tolerated failures
    2. number of replicas
    3. list of replicas' IP address
    4. several test modes
### Assumptions
We will do a list of innocent assumptions to simplify the project without reducing its core challenges. 
1. We assume a user request/message is a successive visible characters, i.e. can be read by the stream operate >> into a string. (e.g. "sdfsdf2314123_+" is ok, but "sdf sfw" is not). The request/message size is bound by BUFFER_SIZE.
2. All processes use IPv4 addresses and UDP communication.
3. View number start from 2f+1. Acceptor slot with view number that is less than 2f+1 is considered to be a hole.

## Implementation
### all replicas
1. Heartbeat periodically.
### leader/proposer
1. Wait until (wait state)
    - no heartbeat from the current leader
    - no heartbeat from all replicas between leader and itself
2. Decides to be a leader.
    - broadcast (VIEW_CHANGE, view_number)
    - if within some time period, it gets majority and promises (VIEW_PROMISE, view_number, chatLog), it becomes a leader, initilize a private pointer p to the smallest unoccupied slot, broadcast noops on such positions
    - if it doesn't get majority before timing out, back to (wait state with its next view_number)
3. Becomes a leader.
    - Keep a private pointer p to the current list position.
    - Receive (CLIENT_REQUEST, (request, requestID)): send (ACCEPT_IT, p, (clientIP, request, requestID)) to all learners, ++p.
4. Quit a leader.
    - Receive (VIEW_CHANGE, view_number) that is larger than its own view_number, quit the leader, enter (wait state)
### acceptor
1. ignore messages with lower view_number
2. Receive (VIEW_CHANGE, view_number) that is larger than its current view_number, send (VIEW_PROMISE, view_number, chatLog) back. update its view_number.
3. Receive (ACCEPT_IT, (clientIP, request, requestID), position, view_number), log it, broadcast (ACCEPTED, (clientIP, request, requestID), position, view_number)
### learner
1. upon received f+1 ACCEPTED messages with the same content, Log (clientIP, request, requestID) at the corresponding position
### Tracing one request
1. sent from one client to all replicas
2. whenever a replica receives a request, first let the learner role check whether the request has been in the log or not. If yes, if it is not executable, ignore the message; if it is executable send (ACK, requestID, content) back to the client directly; if not in the log, pass the message to the leader role
    - if the replica is not the current leader, ignore the message
    - if the replica is the current leader, it will broadcast the ACCEPT_IT messages to all replicas
3. if the process above failed at any point, the client will hear nothing back but note that it can retry this after some time.
### Recovery
1. Each replica must be provided with a stable memory and in this case is a file. Initially replica has to parse its stable memory to return back to the previous state.
2. Replica must note down both the memory log and the value-view log
3. Before doing things, log them in some format.
### How to send log?
Log could potentially be arbitrarily long and thus we have to send log entries one by one.
Procedudre:
- we just send (VIEW_PROMISE, view_number, totalLogLen, replicaID, acceptLogEntry) for each acceptLogEntry
## Interface
1. a server-simulation terminal program to manage automatic replicas
2. a client-simulation terminal program to manage automatic clients
3. manual version replica
4. manual version client


## Compile command
- replica> `g++ -pthread replica.cpp -o replica`