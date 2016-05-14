#include "Serial_serv.hpp"

using namespace std;



char * string2charpN(const char * name, int n){
  char * charp;
  stringstream sti;
  string namep;
  sti.str("");
  sti.clear();
  int temp=atoi(name)+n;
  sti<<temp;
  namep = sti.str();
  charp = new char[namep.length()+1];
  std::strcpy(charp,namep.c_str()); // to char*
  return charp;
}



int set_interface_attribs (int fd, int speed, int parity){
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
    {
      cout<<"error from tcgetattr "<<errno<<endl;;
      return -1;
    }

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);
  
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
  
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
  
  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;
  
  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
      cout<<"error from tcsetattr "<<strerror(errno)<<endl;
      return -1;
    }
  return 0;
}

Joy2Ser::Joy2Ser(const char * a, unsigned int b, const char * p){
  port=a;
  baud_rate=b;
  tcp_port=p;
  char * tcp_portC;

  tcp_portC = string2charpN(p,1);
  memset(&hints, 0, sizeof hints);
  memset(&hintsC, 0, sizeof hintsC);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  hintsC.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hintsC.ai_socktype = SOCK_STREAM;
  hintsC.ai_flags = AI_PASSIVE;     // fill in my IP for me

  getaddrinfo(NULL, p, &hints, &res);
  getaddrinfo(NULL,tcp_portC, &hintsC, &resC);
  // make a socket:

  if(tcp_portC!=NULL)
    delete tcp_portC;
   
  connfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  connfdC = socket(resC->ai_family, resC->ai_socktype, resC->ai_protocol);
  // bind it to the port we passed in to getaddrinfo():

  // make the socket reusable
  int yes=1;
  if (setsockopt(connfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  } 

  yes=1;
  if (setsockopt(connfdC,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  
  bind(connfd, res->ai_addr, res->ai_addrlen);
  listen(connfd, BACKLOG);

  bind(connfdC, resC->ai_addr, resC->ai_addrlen);
  listen(connfdC, BACKLOG);
  
  serrfd = open(a, O_RDWR | O_NOCTTY | O_SYNC);
  if (serrfd < 0)
    {
      cout<<"error opening serial "<<strerror(errno)<<endl;
      exit(1);
    }
  
  set_interface_attribs (serrfd, baud_rate, 0);  // set

  key_t keyC = ftok("/home/aeshma/joy2ser/serial_serv", '1');
  key_t keyJ = ftok("/home/aeshma/joy2ser/serial_serv", '2');
  msqidC = msgget(keyC, 0666 | IPC_CREAT);
  msqidJ = msgget(keyJ, 0666 | IPC_CREAT);

  slog.open("/home/aeshma/slave.log", ios::out | ios::app); // open the log file
  slog<<"new session "<<time(NULL)<<endl;
}   

void Joy2Ser::openthread(){
  thJS = boost::thread(boost::bind(&Joy2Ser::executioner, this, 0));
  thC = boost::thread(boost::bind(&Joy2Ser::executioner, this, 1));
}

Joy2Ser::~Joy2Ser(){
  thJS.join();
  thC.join();// join all the threads
  slog<<"I have joined all the threads"<<endl;
  msgctl(msqidC, IPC_RMID, NULL);
  msgctl(msqidJ, IPC_RMID, NULL);
  slog<<"I have removed the queues"<<endl;
  close(connfd);
  close(listenfd);
  close(connfdC);
  close(listenfdC); 
  close(serrfd);
  slog.close();
  slog<<"closed the file"<<endl;
}


int Joy2Ser::ship_message(unsigned int mess, int th){
     
  struct smessage messi={2,mess};

  switch(th){
  case 0:
    if(msgsnd(msqidJ, &messi, sizeof(mess), 0)<0)
      return -1;
    else
      return 0;
    break;
  case 1:
    if(msgsnd(msqidC, &messi, sizeof(mess), 0)<0)
      return -1;
    else
      return 0;
    break;
  }
}


bool Joy2Ser::try_catch_message(int th, unsigned int & data){
  bool control=false;
  struct smessage pmb;
  
  switch(th){
  case 0:
    if(msgrcv(msqidJ, &pmb, sizeof(unsigned int), 2, IPC_NOWAIT)>0){
      control=true;
      data=pmb.mess;
    }
    break;
  case 1:
    if(msgrcv(msqidC, &pmb, sizeof(unsigned int), 2, IPC_NOWAIT)>0){
      control=true;
      data=pmb.mess;
    }
    break;
  }
  return control;
}

bool Joy2Ser::SerMessanger(js_event jse){
  
  stringstream sti;
  string name;
  __s16 val;
  unsigned char inputstr[4]; // do not forget unsigned or it will make a huge mess

  if(jse.number==2 || jse.number==5)
    val = (__s16)(((float)(jse.value)/32767.0)*500.0+500.0);
  else if(jse.number==3)
    val = (__s16)(((float)(jse.value+32767)/(2.0*32767.0))*(SERVOMAX-SERVOMIN)+SERVOMIN);
  else
    val = (__s16)(((float)(jse.value)/32767.0)*25.0);
  //val = (__s16)(((float)(jse.value)));
  inputstr[0] = jse.number;
  inputstr[1] = (val >> 8) & 0xFF;
  inputstr[2] = val & 0xFF;
  inputstr[3] = 0xFF; // pad with zero, we want a divisor of buffer size
  slog<<"axis "<< (int) jse.number<<" this is what we got "<<val<<endl;
  // try{
  int num;
  if((num=write(serrfd,inputstr,sizeof(inputstr)))>0){ // with no msg nosignal broken pipe will kill you
    slog<<"we wrote "<<num<<" bytes"<<endl;
    return true;
  }
  else{
    slog<<"error error "<<strerror(errno)<<endl;
    return false;
  }
 
  return true;
}




void Joy2Ser::executioner(int i){
  bool control=true;
  bool control1=true;
   
  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number
  struct timeval twait;
  int rv;
  switch (i){
  case 0: // joystick thread
   
   
    
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
      // add the listener to the master set
    FD_SET(connfd, &master);
   
    // keep track of the biggest file descriptor
    fdmax = connfd; // so far, it's this one
   
    while (control){
      socklen_t addr_size;
      struct  js_event jse;
    
      read_fds = master; // copy it
     
      twait.tv_sec=1;
      twait.tv_usec=1;
      
      if (select(fdmax+1, &read_fds, NULL, NULL, &twait) == -1) {
	perror("select");
	control=false;
      }

        // run through the existing connections looking for data to read
      for(int ii = 0; ii <= fdmax; ii++) {
	if (FD_ISSET(ii, &read_fds)) { // we got one!!
	  if (ii == connfd) {
	    addr_size = sizeof client_addr;
	    listenfd = accept(connfd, (struct sockaddr *)&client_addr, &addr_size);
	    slog<<"we have a connection"<<endl;
	    fcntl(listenfd, F_SETFL, O_NONBLOCK); // make it non block
	    FD_SET(listenfd, &master); // add to master set
	    if (listenfd > fdmax) {    // keep track of the max
	      fdmax = listenfd;
	    }
	  }
	  else{
	    char test;
	    if (recv(ii, &test, sizeof(test), MSG_PEEK | MSG_DONTWAIT) == 0) {
	      slog<<"they hung up"<<endl;
	      close(ii);
	      FD_CLR(ii, &master); // remove from master set
	    }
	    else{
	      bool control1=true;
	      while(control1){
		int num;
		num = read(ii, &jse,sizeof(jse));
		slog<<"num bytes read "<<num<<endl;
		if(num>0){ // read as much as you can
		  if(jse.type == JS_EVENT_AXIS){
		    control1= this->SerMessanger(jse); //we send it over serial line
		    if(!control1){
		      slog<<"serial is down"<<endl;
		      close(serrfd);
		      sleep(1);
		      slog<<"reopen serial"<<endl;
		      serrfd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
		      if (serrfd < 0)
			{
			  slog<<"error opening serial "<<strerror(errno)<<endl;
			  exit(1);
			}
		      sleep(1); //required to make flush work, for some reason
		      tcflush(serrfd,TCIOFLUSH);
		      set_interface_attribs (serrfd, baud_rate, 0);  // set
		    }
		  }
		}
		else if (num<= 0 && (errno == EAGAIN || errno==0))
		  control1=false;
		else if (num<0 && errno!=EAGAIN && errno!=0){
		  slog<<"error reading from socket "<<strerror(errno)<<endl;
		  close(ii);
		  FD_CLR(ii, &master); // remove from master set
		  /* do something interesting with processed events */
		  control1=false;
		}
	      }
	    }
	  }
	}
      }
      unsigned int mess0;
      if(this->Joy2Ser::try_catch_message(i, mess0)){
	slog<<"this is what I got "<<mess0<<endl;
	switch (mess0){
	case TERMINATE:
	  slog<<"I have been hit"<<endl;
	  control=false;
	  break;
	}
      }    
    }
    break;
  case 1: //this is the control thread
    string s;
       
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    // add the listener to the master set
    FD_SET(connfdC, &master);
    //    FD_SET(0,&master);
    // keep track of the biggest file descriptor
    fdmax = connfdC; // so far, it's this one
  
    while (control){
      socklen_t addr_size;
      struct  js_event jse;
      
      read_fds = master; // copy it
      
      twait.tv_sec=1;
      twait.tv_usec=1;
      
      if (select(fdmax+1, &read_fds, NULL, NULL, &twait) == -1) {
	perror("select");
	control=false;
      }
      
      // run through the existing connections looking for data to read
      for(int ii = 0; ii <= fdmax; ii++) {
	if (FD_ISSET(ii, &read_fds)) { // we got one!!
	  if (ii == connfdC) {
	    addr_size = sizeof client_addr;
	    listenfdC = accept(connfdC, (struct sockaddr *)&client_addr, &addr_size);
	    slog<<"we have a connection"<<endl;
	    fcntl(listenfdC, F_SETFL, O_NONBLOCK); // make it non block
	    FD_SET(listenfdC, &master); // add to master set
	    if (listenfdC > fdmax) {    // keep track of the max
	      fdmax = listenfdC;
	    }
	  }
	  else if(ii==0){
	    slog<<"ready for reading"<<endl;
	    cin>>s;
	    if(s=="q"){
	      control=false;
	      this->ship_message(TERMINATE,0);
	      break;
	    }
	  }
	  else{
	    int merda;
	    int num;
	    if ((num=recv(ii, &merda, sizeof(merda), 0)) <= 0) {
	      if (num==0)
		slog<<"control they hung up"<<endl;
	      else
		slog<<"control error"<<endl;
	      close(ii);
	      FD_CLR(ii, &master); // remove from master set
	    }
	    else{
	      slog<<"we received this CONTROL "<<merda<<endl;
	      if (merda==KILL){
		control=false;
		this->ship_message(TERMINATE,0);
		break;
	      }
	    }
	  }
	}
      }
    }
    break;
  }
}

int main(int argc, char* argv[])
{
  int type;
  const char * port="8786";
  const char * server_name="192.168.1.9";
  const char * serial_name="/dev/ttyUSB0"; //default to ttyUSB0
  int baud_rate=115200; // default baud rate
  const char * client="192.168.1.79"; // replace it with a list of clients
  string figa;
  
  for (int i=0; i<argc; i++){
    cout<< argv[i]<<endl;
    figa=argv[i];
    if(figa=="--client"){
      type=SLAVE;
    }
    else if(figa=="--master"){
      type=MASTER; 
    }
    else if(figa=="--host")
      server_name=argv[i+1]; // must be an IP address
    else if(figa=="--serial")
      serial_name=argv[i+1];
    else if(figa=="--baud")
      baud_rate=atoi(argv[i+1]);
    else if(figa=="--port")
      port=argv[i+1];
  }

  Joy2Ser * joy = new Joy2Ser(serial_name,baud_rate,port);
  joy->openthread();
  delete joy;
  return 0;
}
/* g++  serial.cpp -o serial -lboost_system -lboost_thread -lpthread */
// g++ serial_servn.cpp  -o serial_serv -lboost_system -lboost_thread -lpthread -lrt
