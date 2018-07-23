CXX=g++
CXXFLAGS = -c -g -Wextra -Wall -std=c++11 -pthread

LIBSRC= MapReduceFramework.cpp Barrier.h Barrier.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
TARSRCS=$(LIBSRC) Makefile README

MAPREDUCELIB=libMapReduceFramework.a

all: libMapReduceFramework.a

libMapReduceFramework.a: $(LIBOBJ)
	ar rcs $(MAPREDUCELIB) $(LIBOBJ)

MapReduceFramework.o: MapReduceFramework.cpp 
	$(CXX) $(CXXFLAGS) MapReduceFramework.cpp -o MapReduceFramework.o

Barrier.o: Barrier.h Barrier.cpp
	$(CXX) $(CXXFLAGS) Barrier.cpp -o Barrier.o

clean:
	-rm -f *.o $(MAPREDUCELIB) $(TARNAME)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
