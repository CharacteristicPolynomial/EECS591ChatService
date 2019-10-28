all: makeReplica makeClient genConfig

makeReplica: makeReplica.cpp replica.cpp replicaTools.cpp phone.cpp message.cpp debugLog.cpp configure.cpp
	g++ -pthread -Wall makeReplica.cpp replica.cpp replicaTools.cpp phone.cpp message.cpp debugLog.cpp configure.cpp -o makeReplica

makeClient: makeClient.cpp configure.cpp
	g++ makeClient.cpp configure.cpp -o makeClient

genConfig: genConfig.cpp configure.cpp
	g++ genConfig.cpp configure.cpp -o genConfig

clean:
	rm makeReplica genConfig makeClient replica_*.log replica_*.msglog