CXX=CC
CXXFLAGS= -std=c++11 -O0 -g -pedantic -Wno-deprecated -Wall
LDFLAGS=

BINARY= bidibiba

all: aries_bidibiba.cpp
	$(CXX) $(CXXFLAGS) -o $(BINARY) aries_bidibiba.cpp

clean:
	rm -f $(BINARY)
