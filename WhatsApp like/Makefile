CXX=g++
CXXFLAGS = -g -Wextra -Wall -std=c++11

EX4SRC= whatsappServer.cpp whatsappClient.cpp whatsappio.h whatsappio.cpp
IOSRC = whatsappio.h whatsappio.cpp

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS=$(EX4SRC) Makefile README

all: whatsappClient whatsappServer

whatsappClient: $(IOSRC) whatsappClient.cpp 
	$(CXX) $(CXXFLAGS) whatsappio.cpp whatsappClient.cpp -o whatsappClient

whatsappServer: $(IOSRC) whatsappServer.cpp
	$(CXX) $(CXXFLAGS) whatsappio.cpp whatsappServer.cpp -o whatsappServer


clean:
	-rm -f *.o whatsappServer whatsappClient $(TARNAME)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
