all: makeReplica makeClient genConfig

makeReplica: makeReplica.cpp replica.cpp replicaTools.cpp phone.cpp message.cpp debugLog.cpp
	g++ makeReplica.cpp replica.cpp replicaTools.cpp phone.cpp message.cpp debugLog.cpp -o makeReplica

makeClient: 
	ls

genConfig: genConfig.cpp
	g++ genConfig.cpp -o genConfig