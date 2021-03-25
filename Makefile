all:
	g++ -o test -lssl -lcrypto sslclient.cpp wsclient.cpp
