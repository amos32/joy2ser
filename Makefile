CC=g++

LIBS=-lboost_system -lboost_thread -lpthread -lrt -lssh -lPCA9685 -lwiringPi

all: serial_cli serial_serv

serial_servn.o: serial_servn.cpp Serial_serv.hpp
	$(CC) -c -o serial_servn.o serial_servn.cpp

serial_clin.o: serial_clin.cpp Serial_cli.hpp
	$(CC) -c -o serial_clin.o serial_clin.cpp

serial_serv: serial_servn.o
	$(CC) -o serial_serv serial_servn.o $(LIBS)

serial_cli: serial_clin.o
	$(CC) -o serial_cli serial_clin.o $(LIBS)

.PHONY: clean

clean:
	rm -f ./*.o  
