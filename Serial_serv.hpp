#ifndef H_Joy2Sr
#define H_Joy2Ser

#include <sys/msg.h>
#include <stddef.h>
#include <boost/utility.hpp>
#include <ostream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include "joystick.h"
#include <boost/thread.hpp>  
#include <boost/thread/mutex.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define QUEUE_SIZE 1024
#define NUM_MESSAGES 20
#define KILL 101
#define SERVOMIN 150
#define SERVOMAX 550
#define MASTER 1
#define SLAVE 0
#define BACKLOG 10
#define TERMINATE 66

using namespace std;



struct smessage{
  long mtype;
  unsigned int mess;
};


class Joy2Ser
{
 private:
  boost::thread  thJS; // thread for joystick to serial 
  boost::thread  thC; // cpu control thread
  const char * port; // the serial port
  const char * joyn; // joystick name
  int fdJS; // joystick file descriptor
  unsigned int baud_rate; 
  int listenfd, connfd, serrfd, connfdC, listenfdC;
  ofstream slog;
  const char * tcp_port;
  struct sockaddr_in serv_addr, client_addr;
  struct addrinfo hints, hintsC, *res, *resC;
  int msqidC, msqidJ;
  
 public:
  Joy2Ser(const char * a, unsigned int b, const char * p);
  ~Joy2Ser();
  void synchronize_cpu(); // waits for cpu threads to complete
  void openthread();
  bool try_catch_message(int th, unsigned int & mess);
  int ship_message(unsigned int mess, int th);
  bool SerMessanger(js_event jse);
  void executioner(int i); // listen for messages on thread 
};


#endif
