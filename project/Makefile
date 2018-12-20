bin=HttpdServer
cc=g++
LDFLAGS=-lpthread

.PHONY:all
all:$(bin) cgi cal

$(bin):HttpdServer.cc
	$(cc) -o  $@ $^ $(LDFLAGS) -std=c++11

.PHONY:cgi
cgi:
	g++ -o testcgi testcgi.cc
.PHONY:cal
cal:
	g++ -o cal cal.cc
.PHONY:clean
clean:
	rm -f $(bin) Cal testcgi cal
