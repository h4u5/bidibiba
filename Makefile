CXX=CC
CXXFLAGS= -std=c++11 -O0 -g -pedantic -Wno-deprecated -Wall
LDFLAGS=

BINARY= bidibiba_ss

all: slingshot_bidibiba.cpp
	$(CXX) $(CXXFLAGS) -o $(BINARY) slingshot_bidibiba.cpp

clean:
	rm -f $(BINARY)
