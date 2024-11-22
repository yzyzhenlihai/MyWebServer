CXX=g++
FLAG= -std=c++14  -Wall -g

TARGET=./bin/main

SRC=./code/http/*.cpp ./code/server/*.cpp ./code/main.cpp \
	./code/buffer/*.cpp ./code/pool/*.cpp ./code/log/*.cpp \
	./code/timer/*cpp

$(TARGET) : $(SRC)
	$(CXX) $(SRC) -o $(TARGET) $(FLAG) -lmysqlclient

.PHONY:clean
clean:
	rm -rf $(TARGET)