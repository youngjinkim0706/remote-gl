CXX=g++
CXXFLAGS=-std=c++11 -Wno-deprecated-declarations

LIBS=-L/usr/lib/ -lzmq `pkg-config --libs glew` `pkg-config --libs glfw3` -lpthread
STREAMER_LIBS=-L/usr/lib/ -lzmq `pkg-config --libs gstreamer-1.0` `pkg-config --libs gstreamer-app-1.0` `pkg-config --libs gstreamer-rtsp-server-1.0` -lpthread
STREAMER_INCLUDES=-I/usr/include/ -I/usr/include/gstreamer-1.0 -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/

INCLUDES=-I/usr/include/ 
SERVER_OBJS=server.o
CLIENT_OBJS=client.o

all: gl_server gl_client streamer

server.o: ./glremote_server/glremote_server.cpp ./glremote_server/glremote_server.h 
	$(CXX) -c -g -o server.o $(CXXFLAGS) $(INCLUDES) ./glremote_server/glremote_server.cpp

gl_server: main.cpp server.o
	$(CXX) -g -o gl_server $(CXXFLAGS) $(INCLUDES) main.cpp $(SERVER_OBJS) $(LIBS)

client.o: ./glremote/glremote.cpp ./glremote/glremote/glremote_client.h ./glremote/gl_commands.h
	$(CXX) -c -g -o client.o $(CXXFLAGS) $(INCLUDES) ./glremote/glremote.cpp 

gl_client: client.cpp client.o
	$(CXX) -g -o gl_client $(CXXFLAGS) $(INCLUDES) client.cpp $(CLIENT_OBJS) $(LIBS)

streamer: streamer.cpp
	$(CXX) -g -o stremaer $(CXXFLAGS) $(STREAMER_INCLUDES) streamer.cpp $(STREAMER_LIBS)

test: test.cpp
	$(CXX) -g -o test $(CXXFLAGS) $(INCLUDES) test.cpp $(LIBS)

clean:
	rm gl_server gl_client streamer *.o 
