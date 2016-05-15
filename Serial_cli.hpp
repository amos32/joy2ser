#ifndef H_Joy2Sr
#define H_Joy2Ser

#include <boost/algorithm/string.hpp>
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
#include "joystick.h"
#include <boost/thread.hpp>  
#include <boost/thread/mutex.hpp> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libssh/libssh.h>


#define NUM_MESSAGES 20
#define KILL 101
#define SERVOMIN 150
#define SERVOMAX 550
#define MASTER 1
#define SLAVE 0
#define BACKLOG 10
#define TERMINATE 66
#define SHIT 67
#define CONNECT 68

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
  int listenfd, connfd, connfdC;
  struct sockaddr_in serv_addr, client_addr;
  struct addrinfo hints, hintsC, *res, *resC;
  struct js_event jse;
  const char * tcp_port;
  int msqidC, msqidJ;
  ofstream slog, sync_log;
  ssh_channel channel;
  ssh_session ss1;
  fd_set masterreadC, masterwriteC, masterread, masterwrite;
  int fdmaxC, fdmax;
  
 public:
  Joy2Ser(const char * c, const char * serv, const char * p);
  ~Joy2Ser();
  void synchronize_cpu(); // waits for cpu threads to complete
  void openthread();
  bool try_catch_message(int th, unsigned int & mess);
  int ship_message(unsigned int mess, int th);
  void executioner(int i); // listen for messages on thread i
  void executionerC();
  int startRemote(const char* name,  const char* user, const char* pw, const char * serialN, unsigned int baudN, bool ar) ;
  void ExecuteCmdResponse(const char* cmd, ssh_session ss1); 
  bool parse(string ss);
  bool isConnected(int th, int & result);
  bool internalMess(int i, int & result);
  bool readFds(int th);
  int connectSock(int i);
};


#endif
