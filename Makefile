CXX = g++
CFLAGS = -std=c++14 -O2 -Wall

TARGET = ./bin/server
OBJS = ./src/*.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -lpthread -lmysqlclient

clean:
	rm -rf $(OBJS) $(TARGET)