all: makeReplica makeClient genConfig manager

makeReplica: makeReplica.cpp replica.cpp replicaTools.cpp timer.cpp phone.cpp message.cpp debugLog.cpp configure.cpp
	g++ -pthread -Wall makeReplica.cpp replica.cpp replicaTools.cpp timer.cpp phone.cpp message.cpp debugLog.cpp configure.cpp -o makeReplica

makeClient: makeClient.cpp configure.cpp
	g++ makeClient.cpp configure.cpp -o makeClient

genConfig: genConfig.cpp configure.cpp
	g++ genConfig.cpp configure.cpp -o genConfig

manager: manager.cpp configure.cpp
	g++ manager.cpp configure.cpp -o manager

clean:
	rm makeReplica genConfig makeClient manager