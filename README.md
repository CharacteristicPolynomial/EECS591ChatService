# EECS591ChatService
course implementation project.

## a nice picture
![momo](momo_16_9.png)

## Design Schemes
Clearly explain specific choices when implementing PAXOS.
1. Clients send the same request repeatedly until get a ACK response
2. Replicas are given a logfile which initially may not be empty. Replicates need to parse that logfile to get an initial state.
3. Leader and Acceptor on a same replica share a acceptLog and view number. 
4. Leader has three states: (thus a pair (view, leaderQ) can determine the state)
    - LEADING, state where it gets majority in his view. In the LEADING state, the process goes as a leader. 
    - WAIT_PROMISE, state where it is in his view but hasn't got the majority. When receiving client request in this state, it enfoces viewchange to others. It goes to LEADING when the majority is obtained (by setting leaderQ=true). 
    - IDLE, state where it is not in his view. When receiving client request in this state, it checks the heartbeat list, if haven't heard from the replicas between the current view and itself, it initializes the view_change structure and enters WAIT_PROMISE state by increase the view number to his next view number.

| Leader State | VIEW_PROMISE | CLIENT_REQUEST | 
| LEADING | ignore | process it | 
| WAIT_PROMISE | collect | re-enforce leadership |
| IDLE | ignore | view change when no heartbeats |

## File Structures
Organize files.

## Communiaction Protocols
Clearly state all message format, in terms of meaning and type.

## Test System
Use stream redirection and test class to control debugging information.