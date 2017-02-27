CC  = g++
CFLAGS = -Wall -g -W
objs = main.o fileType.o httpFileServer.o UrlCode.o httpCommon.o
httpServer: $(objs)
	$(CC) $(CFLAGS) -o httpServer $(objs) -lpthread -L/usr/local/lib -llog -llog4cplus

httpFileServer.o: httpFileServer.cpp httpCommon.h
	$(CC) $(CFLAGS) -c httpFileServer.cpp
main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp
fileType.o: fileType.cpp
	$(CC) $(CFLAGS) -c fileType.cpp
UrlCode.o: UrlCode.cpp UrlCode.h
	$(CC) $(CFLAGS) -c UrlCode.cpp
httpCommon.o: httpCommon.cpp httpCommon.h
	$(CC) $(CFLAGS) -c httpCommon.cpp

clean:
	rm httpServer $(objs)

dist:
	tar -jcvf httpServer_1.0.1.tar.bz2 *.cpp *.h Makefile

distclean:
	rm ./httpServer ./httpServer_1.0.1.tar.bz2 $(objs)
