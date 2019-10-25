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