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
	rm -rf $(bin)  testcgi cal output 

.PHONY:output
output:
	mkdir output
	 cp $(bin) output 
	 cp -rf mywwwroot output 
	 cp cal output/mywwwroot
	 cp start.sh output 
	 
