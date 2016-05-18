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

Joy2Ser::Joy2Ser(const char * a, unsigned int b, const char * p, bool ard){
  arduino=ard;
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

  if(arduino){ // if we are using arduino we need serial
    serrfd = open(a, O_RDWR | O_NOCTTY | O_SYNC);
    if (serrfd < 0)
      {
	slog<<"error opening serial "<<strerror(errno)<<endl;
	exit(1);
      }
    
    set_interface_attribs (serrfd, baud_rate, 0);  // set
  }
  else{
  
    key_t keyNA = ftok("/home/aeshma/joy2ser/serial_serv", '3'); // NO arduino queue
    msqidNA = msgget(keyNA, 0666 | IPC_CREAT);
  }

  // message queues
  
  key_t keyC = ftok("/home/aeshma/joy2ser/serial_serv", '1');
  key_t keyJ = ftok("/home/aeshma/joy2ser/serial_serv", '2');
  
  msqidC = msgget(keyC, 0666 | IPC_CREAT);
  msqidJ = msgget(keyJ, 0666 | IPC_CREAT);

  slog.open("/home/aeshma/slave.log", ios::out | ios::app); // open the log file
  slog<<"new session "<<time(NULL)<<endl;
}   

void Joy2Ser::openthread(){
  thJS = boost::thread(boost::bind(&Joy2Ser::executioner, this, 0));
  thC = boost::thread(boost::bind(&Joy2Ser::executionerC, this, 1));
  if(!arduino)
    thNA = boost::thread(boost::bind(&Joy2Ser::NoArdLoop, this, 2)); // no arduino thread 
}

Joy2Ser::~Joy2Ser(){
  thJS.join();
  thC.join();// join all the threads
  if(arduino)
    close(serrfd);
  else{
    thNA.join();
    msgctl(msqidNA, IPC_RMID, NULL);
  }
  slog<<"I have joined all the threads"<<endl;
  msgctl(msqidC, IPC_RMID, NULL);
  msgctl(msqidJ, IPC_RMID, NULL);
  slog<<"I have removed the queues"<<endl;
  close(connfd);
  close(listenfd);
  close(connfdC);
  close(listenfdC); 
 
  slog.close();
  slog<<"closed the file"<<endl;
}


int Joy2Ser::ship_message(unsigned int mess, int th, int num){
     
  struct smessage messi={num ,mess};

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
  case 2:
    if(msgsnd(msqidNA, &messi, sizeof(mess), 0)<0)
      return -1;
    else
      return 0;
    break;
  }
}


bool Joy2Ser::try_catch_message(int th, unsigned int & data, int num){
  bool control=false;
  struct smessage pmb;
  
  switch(th){
  case 0:
    if(msgrcv(msqidJ, &pmb, sizeof(unsigned int), num, IPC_NOWAIT)>0){
      control=true;
      data=pmb.mess;
    }
    break;
  case 1:
    if(msgrcv(msqidC, &pmb, sizeof(unsigned int), num, IPC_NOWAIT)>0){
      control=true;
      data=pmb.mess;
    }
    break;
  case 2:
    if(msgrcv(msqidNA, &pmb, sizeof(unsigned int), num, IPC_NOWAIT)>0){
      control=true;
      data=pmb.mess;
    }
    break;
  }
  return control;
}

bool Joy2Ser::SerMessanger(){
  
  stringstream sti;
  string name;
  __s16 val;
  unsigned char inputstr[4]; // do not forget unsigned or it will make a huge mess

  if(jse.number==2 || jse.number==5)
    val = (__s16)(((float)(jse.value)/32767.0)*(MSPEED/2.0)+(MSPEED/2.0));
  else if(jse.number==3)
    val = (__s16)(((float)(jse.value+32767)/(2.0*32767.0))*(SERVOMAX-SERVOMIN)+SERVOMIN);
  else
    val = (__s16)(((float)(jse.value)/32767.0)*RES);
  //val = (__s16)(((float)(jse.value)));
  inputstr[0] = jse.number;
  inputstr[1] = (val >> 8) & 0xFF;
  inputstr[2] = val & 0xFF;
  inputstr[3] = 0xFF; // pad with zero, we want a divisor of buffer size
  slog<<"axis "<< (int) jse.number<<" this is what we got "<<val<<endl;
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

bool Joy2Ser::NoArdMessanger(){

  __s16 val;
  struct jmessage joymess;

  if(jse.number==2 || jse.number==5)
    val = (__s16)(((float)(jse.value)/32767.0)*(MSPEED/2.0)+(MSPEED/2.0));
  else if(jse.number==3)
    val = (__s16)(((float)(jse.value+32767)/(2.0*32767.0))*(SERVOMAX-SERVOMIN)+SERVOMIN);
  else
    val = (__s16)(((float)(jse.value)/32767.0)*RES);
  //val = (__s16)(((float)(jse.value)));
  joymess.inputstr[0] = jse.number;
  joymess.inputstr[1] = (val >> 8) & 0xFF;
  joymess.inputstr[2] = val & 0xFF;
  joymess.inputstr[3] = 0xFF; // pad with zero, we want a divisor of buffer size
  slog<<"noard axis "<< (int) jse.number<<" this is what we got "<<val<<endl;

  joymess.mtype=3; // data is type 3 and control is type 2
  if(msgsnd(msqidNA, &joymess, 4* sizeof(unsigned char), 0)<=0)
    return false;
  else
    return true;
  
}



void Joy2Ser::NoArdLoop(int i){
 __u8 axis;
  __s16 throttle=0;
  __s16 throttle1=0;
  __s16 throttle0=0;
  __s16 throttlea=0;
  // __s16 res=25;
  float mstep=1.0/((float) RES);
  __s16 throttle3=MIDP;
  float pos = MIDP;    // variable to store the servo position
  float pos1 = MIDP;
  float step=0;
  float step1=0;
  float temp=MIDP;
  float temp1=MIDP;
  bool control=true;

  struct jmessage joym;
  pwm.init(I2CB,0x43); // odroid xu4
  pwm.setPWMFreq (60);
  pwm1.init(I2CB,0x42);
  pwm1.setPWMFreq (1500);
  pwm.setPWM(0,0,MIDP); // center the three servos. Pan tilt steering
  pwm.setPWM(1,0,MIDP);
  pwm.setPWM(2,0,MIDP);
  pwm1.setPWM(0,0,0); // zero the two DC motors
  pwm1.setPWM(1,0,0);
  wiringPiSetup () ;
  pinMode (0, OUTPUT);
  pinMode (2,OUTPUT);  

  while(control){

    control=this->internalMess(i);
      unsigned int mess0;
      /*if(this->Joy2Ser::try_catch_message(i, mess0,2)){
      slog<<" NOARD this is what I got "<<mess0<<endl;
      switch (mess0){
      case TERMINATE:
	slog<<"I have been hit"<<endl;
	control=false;
	break;
      }
    }*/

    if(msgrcv(msqidNA, &joym, 4*sizeof(unsigned char), 3, IPC_NOWAIT)>0){    

      axis=joym.inputstr[0];
      slog<<"noard axis "<<(int) axis<<endl;
      if (axis==1)
	throttle1 = (joym.inputstr[1] << 8) | joym.inputstr[2]; // pan
      else  if (axis==0)
	throttle0 = (joym.inputstr[1] << 8) | joym.inputstr[2]; // tilt
      else if (axis==2 || axis==5)
	throttlea = (joym.inputstr[1] )<<8 | (joym.inputstr[2]); // dc motor
      else if (axis==3)
	throttle3 = (joym.inputstr[1] )<<8 | (joym.inputstr[2]); // steering



      if(axis==0) // tilt
	step=mstep*(float) throttle0;
  
      if(axis==1)
	step1=mstep*(float) throttle1;

      if(axis==3)
	pwm.setPWM(2, 0, throttle3);
   
      if(axis==2){
        digitalWrite(0,LOW); //controls the direction the motor
        digitalWrite(2,LOW);
        //analogWrite(3,map(throttle,-32767,32767,0,155));
        //analogWrite(5,map(throttle,-32767,32767,0,155));
        pwm1.setPWM(0, 0, throttlea);
        pwm1.setPWM(1, 0, throttlea);
      }

      if(axis==5){
        digitalWrite(0,HIGH); //controls the direction the motor
        digitalWrite(2,HIGH);
        //analogWrite(3,map(throttle,-32767,32767,0,155));
        //analogWrite(5,map(throttle,-32767,32767,0,155));
        pwm1.setPWM(0, 0, throttlea);
        pwm1.setPWM(1, 0, throttlea);
      }
      temp=(float) pos+step;
      temp1=(float) pos1+step1;

      if(temp>=SERVOMIN && temp<=SERVOMAX){
	pos= temp;
	pwm.setPWM(0, 0, (short) pos); 
      }
      else if(temp>SERVOMAX){
	pos=SERVOMAX;
      }
      else if(temp<SERVOMIN){
        pos=SERVOMIN;
      }

      if(temp1>=SERVOMIN && temp1<=SERVOMAX){
	pos1=temp1; 
	pwm.setPWM(1, 0, (short) pos1); 
      }
      else if(temp1>SERVOMAX){
	pos1=SERVOMAX;
      }
      else if(temp1<SERVOMIN){
        pos1=SERVOMIN;
      }
    }

    temp=(float) pos+step;
    temp1=(float) pos1+step1;

    if(temp>=SERVOMIN && temp<=SERVOMAX){
      pos=temp;
      pwm.setPWM(0, 0, (short) pos); 
    }
    else if(temp>SERVOMAX){
      pos=SERVOMAX;
    }
    else if(temp<SERVOMIN){
      pos=SERVOMIN;
    }

    if(temp1>=SERVOMIN && temp1<=SERVOMAX){
      pos1=temp1;
      pwm.setPWM(1, 0, (short) pos1); 
    }
    else if(temp1>SERVOMAX){
      pos1=SERVOMAX;
    }
    else if(temp1<SERVOMIN){
      pos1=SERVOMIN;
    }
  }
}

bool Joy2Ser::readFds(int th){

  fd_set read_fds;  // temp file descriptor list for select()
  //fd_set write_fds;
  struct timeval twait;
  int fdm=3;
  string s;
  bool control=true;
  twait.tv_sec=1;
  twait.tv_usec=0;
  socklen_t addr_size;


 
  //FD_ZERO(&write_fds);    // clear the master and temp sets
  //  FD_ZERO(&read_fds);

  // if(th==1){
    //   read_fds=masterreadC;
  // fdm=fdmaxC;
  // }
  //else{
    //read_fds=masterread;
    //fdm=fdmax;
  //}
    
  // if (select(fdm+1, &read_fds, NULL, NULL, &twait) == -1) {
  // cout<<"error select"<<endl;
  // control=false;
    //}

  if(th!=1){
    
    poll(ufds, 2, 10000);

    for(int ii = 0; ii <= 1; ii++) {
      if (ufds[ii].revents & POLLIN) {
      // if (FD_ISSET(ii, &read_fds)) { 
	// if(th!=1){
	if(ufds[ii].fd==connfd){
	//if(ii==connfd){ // we have a connection
	  addr_size = sizeof client_addr;
	  listenfd = accept(connfd, (struct sockaddr *)&client_addr, &addr_size);
	  slog<<"we have a connection"<<endl;
	  fcntl(listenfd, F_SETFL, O_NONBLOCK); // make it non block
	  //  FD_SET(listenfd, &masterread); // add to master set
	  ufds[1].fd=listenfd;
	  ufds[1].events=POLLIN;
	  //  fdmax=1;
	  // if (listenfd > fdmax) {    // keep track of the max
	  // fdmax = listenfd;
	  // }
	}
	else{ // we can read from a socket
	  char test;
	  //  if (recv(ii, &test, sizeof(test), MSG_PEEK | MSG_DONTWAIT) == 0) { // they hung up
	  if (recv(ufds[ii].fd, &test, sizeof(test), MSG_PEEK | MSG_DONTWAIT) == 0) {
	    slog<<"they hung up"<<endl;
	    //  close(ii);
	    close(ufds[ii].fd);
	    //fdmax=0;
	    //  FD_CLR(ii, &masterread); // remove from master set
	  }
	  else{ // we are ready to read
	    bool control1=true;
	    while(control1){
	      int num;
	      //num = read(ii, &jse,sizeof(jse));
	      num = read(ufds[ii].fd, &jse,sizeof(jse));
	      slog<<"num bytes read "<<num<<endl;
	      if(num>0){ // read as much as you can
		if(jse.type == JS_EVENT_AXIS){
		  if(arduino){
		    control1= this->SerMessanger(); //we send it over serial line
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
		  else{
		    //slog<<"no arduino, I have not been implemented yet"<<endl;
		    control1=this->NoArdMessanger();
		    if(control1)
		      slog<<"noard data sent succesfully"<<endl;
		    // else
		    // slog<<"error sending data to noard"<<endl;
		  }
		}
	      }
	      else if (num<= 0 && (errno == EAGAIN || errno==0))
		control1=false;
	      else if (num<0 && errno!=EAGAIN && errno!=0){
		slog<<"error reading from socket "<<strerror(errno)<<endl;
		close(ufds[ii].fd);
		//	fdmax=0;
		//	close(ii);
		// FD_CLR(ii, &masterread); // remove from master set
		/* do something interesting with processed events */
		control1=false;
	      }
	    }
	  }
	}
      }
    }
  }
  else if(th==1){

    poll(ufdsC, 2, 10000);
    
    for(int ii = 0; ii <= 1; ii++) {    
      if (ufdsC[ii].revents & POLLIN) {
	if (ufdsC[ii].fd == connfdC) {
	  addr_size = sizeof client_addr;
	  listenfdC = accept(connfdC, (struct sockaddr *)&client_addr, &addr_size);
	  slog<<"we have a connection"<<endl;
	  fcntl(listenfdC, F_SETFL, O_NONBLOCK); // make it non block
	  ufdsC[1].fd=listenfdC;
	  ufdsC[1].events=POLLIN;
	  //	FD_SET(listenfdC, &masterreadC); // add to master set
	  //  if (listenfdC > fdmaxC) {    // keep track of the max
	  //  fdmaxC = listenfdC;
	  // }
	}
	/*else if(ii==0){
	  slog<<"ready for reading"<<endl;
	  cin>>s;
	  if(s=="q"){
	  control=false;
	  this->ship_message(TERMINATE,0,2);
	  if(!arduino)
	  this->ship_message(TERMINATE,2,2);
	  break;
	  }
	  }*/
	else{
	  int merda;
	  int num;
	  if ((num=recv(ufdsC[ii].fd, &merda, sizeof(merda), 0)) <= 0) {
	    if (num==0)
	      slog<<"control they hung up"<<endl;
	    else
	      slog<<"control error"<<endl;
	    close(ufdsC[ii].fd);
	    // FD_CLR(ii, &masterreadC); // remove from master set
	  }
	  else{
	    slog<<"we received this CONTROL "<<merda<<endl;
	    if (merda==KILL){
	      control=false;
	      this->ship_message(TERMINATE,0,2);
	      if(!arduino)
		this->ship_message(TERMINATE,2,2);
	      break;
	    }
	  }
	}
      }
      
    }
  }

  
  return control;
}


void Joy2Ser::executioner(int i){
  bool control=true;
  //  bool control1=true;
   
  // fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  // int fdmax;        // maximum file descriptor number
  struct timeval twait;
  //int rv;
  //switch (i){
  //case 0: // joystick thread
   
  ufds[0].fd = connfd;
  ufds[0].events = POLLIN; // check for normal or out-of-band   
    
  // FD_ZERO(&masterread);    // clear the master and temp sets
  // FD_ZERO(&read_fds);
      // add the listener to the master set
  // FD_SET(connfd, &masterread);
   
    // keep track of the biggest file descriptor
  // fdmax = connfd; // so far, it's this one
  //  fdmax=0;
  
  while (control){

    control=this->readFds(i);

      /*      socklen_t addr_size;
      //struct  js_event jse;
    
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
		    if(arduino){
		      control1= this->SerMessanger(); //we send it over serial line
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
		    else{
		      //slog<<"no arduino, I have not been implemented yet"<<endl;
		      if(this->NoArdMessanger())
			slog<<"noard data sent succesfully"<<endl;
		      else
			slog<<"error sending data to noard"<<endl;
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
      /*	  control1=false;
		}
	      }
	    }
	    
	  }
	}
	
      } */

    control=this->internalMess(i);

      /*
      unsigned int mess0;
      if(this->Joy2Ser::try_catch_message(i, mess0,2)){
	slog<<"this is what I got "<<mess0<<endl;
	switch (mess0){
	case TERMINATE:
	  slog<<"I have been hit"<<endl;
	  control=false;
	  break;
	}
      }*/    
  }
}


void Joy2Ser::executionerC(int i){
  bool control=true;
  //  bool control1=true;
   
  // fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  // int fdmax;        // maximum file descriptor number
  struct timeval twait;
  //int rv;
  //switch (i){
  //case 0: // joystick thread
   
   
    
  //  FD_ZERO(&masterreadC);    // clear the master and temp sets
  // FD_ZERO(&read_fds);
      // add the listener to the master set
  // FD_SET(connfdC, &masterreadC);
   
    // keep track of the biggest file descriptor
  // fdmax = connfd; // so far, it's this one
   
  //case 1: //this is the control thread
  //string s;
       
  //FD_ZERO(&masterreadC);    // clear the master and temp sets
  //FD_ZERO(&read_fds);
    // add the listener to the master set
  // FD_SET(connfdC, &masterreadC);
    //    FD_SET(0,&master);
    // keep track of the biggest file descriptor
  //fdmaxC = connfdC; // so far, it's this one

  ufdsC[0].fd=connfdC;
  ufdsC[0].events=POLLIN;
  
  while (control){

    control=this->readFds(1);

      /*
      socklen_t addr_size;
      //struct  js_event jse;
      
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
	  /*else if(ii==0){
	    slog<<"ready for reading"<<endl;
	    cin>>s;
	    if(s=="q"){
	      control=false;
	      this->ship_message(TERMINATE,0,2);
	      if(!arduino)
		this->ship_message(TERMINATE,2,2);
	      break;
	    }
	  }*/
      /*	  else{
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
		this->ship_message(TERMINATE,0,2);
		if(!arduino)
		  this->ship_message(TERMINATE,2,2);
		break;
	      }
	    }
	  }
	}
      }
    }*/
  }
}


bool Joy2Ser::internalMess(int i){

  unsigned int mess0;
  if(this->Joy2Ser::try_catch_message(i, mess0,2)){
    slog<<"this is what I got "<<mess0<<endl;
    switch (mess0){
    case TERMINATE:
      slog<<"I have been hit"<<endl;
      return false;
      break;
    }
  }
  return true;
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
  bool ard=true; // the default is to use arduino
  
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
    else if(figa=="--noard")
      ard=false;
  }

  Joy2Ser * joy = new Joy2Ser(serial_name,baud_rate,port,ard);
  joy->openthread();
  delete joy;
  return 0;
}
/* g++  serial.cpp -o serial -lboost_system -lboost_thread -lpthread */
// g++ serial_servn.cpp  -o serial_serv -lboost_system -lboost_thread -lpthread -lrt -lssh -lPCA9685
