bin=HttpdServer
cc=g++
LDFLAGS=-lpthread

$(bin):HttpdServer.cc
	$(cc) -o  $@ $^ $(LDFLAGS) -std=c++11


.PHONY:clean
clean:
	rm -f $(bin) Cal
