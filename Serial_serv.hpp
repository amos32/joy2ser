#ifndef H_Joy2Sr
#define H_Joy2Ser

#include "PCA9685.h"
#include <wiringPi.h>
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
#include <sys/poll.h>

#define QUEUE_SIZE 1024
#define NUM_MESSAGES 20
#define KILL 100
#define MASTER 1
#define SLAVE 0
#define BACKLOG 10
#define TERMINATE 66
#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pu
#define MIDP SERVOMIN+(SERVOMIN+SERVOMAX)/2
#define MSPEED 1000
#define RES 25
#define I2CB 1 // odroid xu4
#define SHIT 67

using namespace std;



struct smessage{
  long mtype;
  unsigned int mess;
};

struct jmessage{
  long mtype;
  unsigned char inputstr[4];
};


class Joy2Ser
{
 private:
  boost::thread  thJS; // thread for joystick to serial 
  boost::thread  thC; // cpu control thread
  boost::thread  thNA; // no arduino thread
  struct  js_event jse;
  const char * port; // the serial port
  const char * joyn; // joystick name
  int fdJS; // joystick file descriptor
  unsigned int baud_rate; 
  int listenfd, connfd, serrfd, connfdC, listenfdC;
  ofstream slog;
  const char * tcp_port;
  struct sockaddr_in serv_addr, client_addr;
  struct addrinfo hints, hintsC, *res, *resC;
  int msqidC, msqidJ, msqidNA;
  int fdmax, fdmaxC;
  fd_set masterreadC, masterwriteC, masterread, masterwrite;
  bool arduino;
  PCA9685 pwm, pwm1;
  struct pollfd ufds[2];
  struct pollfd ufdsC[2];  
  
 public:
  Joy2Ser(const char * a, unsigned int b, const char * p, bool ard);
  ~Joy2Ser();
  void synchronize_cpu(); // waits for cpu threads to complete
  void openthread();
  bool try_catch_message(int th, unsigned int & mess, int num);
  int ship_message(unsigned int mess, int th, int num);
  bool SerMessanger();
  void NoArdLoop(int i);
  bool NoArdMessanger();
  void executioner(int i); // listen for messages on thread
  void executionerC(int i);
  bool internalMess(int i);
  bool readFds(int i);
  void processJoy();
};


#endif
