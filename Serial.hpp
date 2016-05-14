#ifndef H_Joy2Sr
#define H_Joy2Ser

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
#include "joystick.h"
#include <boost/thread.hpp>  
#include <boost/thread/mutex.hpp>
#include <boost/date_time.hpp> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define NUM_MESSAGES 20
#define KILL 101
#define SERVOMIN 150
#define SERVOMAX 550
#define MASTER 1
#define SLAVE 0
#define BACKLOG 10

using namespace std;

class Joy2Ser
{
 private:
  boost::thread  thJS; // thread for joystick to serial 
  boost::thread  thC; // cpu control thread
  char *  queuenJS; //send to Joy2Serial         0+
  char * queuenC; // cpu control process   1000+
  const char * port; // the serial port
  const char * joyn; // joystick name
  int fdJS; // joystick file descriptor
  unsigned int baud_rate;
  boost::asio::io_service io;
  boost::asio::serial_port serial;
  char ard1[2];
  boost::asio::streambuf ard_control; 
  int listenfd, connfd;
  struct sockaddr_in serv_addr, client_addr;
  struct addrinfo hints, *res;
  
 public:
  Joy2Ser(const char * a, unsigned int b, const char * c, const char * serv);
  ~Joy2Ser();
  void synchronize_cpu(); // waits for cpu threads to complete
  void openthread();
  void messanger(js_event jse, int i);
  bool SerMessanger(js_event jse);
  void executioner(int i); // listen for messages on thread i
};


#endif
