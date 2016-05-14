#include "Serial_cli.hpp"

using namespace std;

char * string2charpN(const char * name, int n){ // converts char to char+ n 
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

int Joy2Ser::connectSock(int i){  // connect to client i
  int result=-77;
  if(i==1){ // adapt it to several clients, we are connecting to the control thread

    connfdC = socket(resC->ai_family, resC->ai_socktype, resC->ai_protocol); // initialie socket
    fcntl(connfdC, F_SETFL, O_NONBLOCK); // make it non block

  // make the socket reusable
    int yes=1;
	  
    if (setsockopt(connfdC,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
      result=-66;;
    }
    FD_SET(connfdC, &masterwriteC);
    cout<<"connect sock "<<connfdC<<endl;;
    if(connfdC>fdmaxC) // update the larget fd
      fdmaxC = connfdC; // so far, it's this one
  
    result = connect(connfdC, resC->ai_addr, resC->ai_addrlen); // try to reconnect
  }
  else{ // we are connecting to the joy thread
    connfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    fcntl(connfd, F_SETFL, O_NONBLOCK); // make it non block
    
  // make the socket reusable
    int yes=1;
	  
    if (setsockopt(connfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
      result=-66;
    }
    result = connect(connfd, resC->ai_addr, resC->ai_addrlen); // try to reconnect
  }
  return result;
}

bool Joy2Ser::parse(string s){
  vector<string> SplitVec; // #2: Search for tokens
  boost::split( SplitVec, s, boost::is_any_of("\t "), boost::token_compress_on );
  for(int j=0; j<SplitVec.size(); j++)
    cout<<SplitVec[j]<<endl;
  if(SplitVec.size()==1){
    if(SplitVec[0]=="q"){
      //  control=false;
      this->ship_message(TERMINATE,0);
      sleep(3);
      return false;
    }
  }
  else
    if(SplitVec[0]=="connect"){ // at least two pieces
      int result;
      const char * addr="127.0.0.1";
      const char * ser="/dev/rfcomm0";
      int baudn =115200;
      char * addrn, * sern;
      
      for(int j=1; j<SplitVec.size()-1;j++){
	if(SplitVec[j]=="--host"){
	  addrn = new char[SplitVec[j+1].length()+1];
	  std::strcpy(addrn,SplitVec[j+1].c_str()); // to char*
	  addr=addrn;
	}
	if(SplitVec[j]=="--serial"){
	  sern = new char[SplitVec[j+1].length()+1];
	  std::strcpy(sern,SplitVec[j+1].c_str()); // to char*
	  ser=sern;
	}
	if(SplitVec[j]=="--baud"){
	  char * bauds;
	  bauds = new char[SplitVec[j+1].length()+1];
	  std::strcpy(bauds,SplitVec[j+1].c_str()); // to char*
	  baudn=atoi(bauds);
	  delete bauds;
	}
      }
      char * tcp_portC;
      tcp_portC= string2charpN(tcp_port,1);
 

  
      getaddrinfo(addr, tcp_port, &hints, &res);
      getaddrinfo(addr, tcp_portC, &hintsC, &resC);

      if(tcp_portC!=NULL) // this is not right 
	delete tcp_portC;
      
      result=this->startRemote(addr,"aeshma","qcnfnded666",ser, baudn); 
 
      if (result==0){
	cout<<"client started succesfully"<<endl;
	thJS = boost::thread(boost::bind(&Joy2Ser::executioner, this, 0));
	thJS.detach();
      }
      else
	cout<<"error starting the client"<<endl;
    }
  if(SplitVec[0]=="kill"){
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    this->ship_message(TERMINATE,0); // kill the other thread
   
  }
  return true;
}

void Joy2Ser::ExecuteCmdResponse(const char* cmd, ssh_session ss1) 
{ 
  int rc;
  channel = ssh_channel_new(ss1);
  ssh_channel_open_session(channel);
  rc = ssh_channel_request_exec(channel, cmd);
  if (rc != SSH_OK)
    {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      throw  std::runtime_error("Failed to start the client");
    }
  else{
    sleep(2);
  }
}

int Joy2Ser::startRemote(const char* name,  const char* user, const char* pw, const char * serialN, unsigned int baudN) 
{

  int rc=-11;
  int ret=66; // adapt this for several clients

  if(ssh_get_fd(ss1)==-1){ // not connected
    slog.open("/home/aeshma/master.log", ios::out | ios::app); // open the log file
    slog<<"new session "<<time(NULL)<<endl;
    sync_log.open("/home/aeshma/master_sync.log", ios::out | ios::app); // open the log file
    sync_log<<"new session "<<time(NULL)<<endl;

   
    int verbosity = SSH_LOG_PROTOCOL;
    int port = 22;
    
    ssh_options_set(ss1, SSH_OPTIONS_HOST, name); // the client
    ssh_options_set(ss1, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(ss1, SSH_OPTIONS_PORT, &port);  // the port
    ssh_options_set(ss1, SSH_OPTIONS_USER, &user); // we assume that the user is the same but we can change it
    // Connect to server
    
    rc = ssh_connect(ss1);
    
    if (rc == SSH_OK)  
      {
	rc = ssh_userauth_password(ss1, user, pw);
      }
    else{
      cout<<"we cannot connect"<<endl;
      ret=-1;
      
    }
  }
  
  if (rc == SSH_AUTH_SUCCESS || ssh_get_fd(ss1)!=-1 ){
    stringstream stri;
    string sout;
    stri<<"/home/aeshma/joy2ser/serial_serv --serial "<<serialN<<" --baud "<<baudN<<" --port "<<tcp_port;
    sout=stri.str();
    cout<<sout<<endl;
    
    const char *csout = sout.c_str();
    try{
      this->ExecuteCmdResponse(csout, ss1);
      ret=0;
      slog<<csout<<endl;
    }
    catch(std::runtime_error& err){
      cout<<"we cannot execute the remote command"<<endl;
      cout<<err.what()<<endl;
      ret=-3; 
    }
  }
  else{
    cout<<"we cannot authenticate"<<endl;
    ret=-2;
  }
  return ret;
}



Joy2Ser::Joy2Ser(const char * c, const char * serv, const char * p) {
 
  joyn=c;
  tcp_port=p;
  //  char * tcp_portC;
  //tcp_portC= string2charpN(p,1);
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  memset(&hintsC, 0, sizeof hintsC);
  hintsC.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hintsC.ai_socktype = SOCK_STREAM;
  hintsC.ai_flags = AI_PASSIVE;     // fill in my IP for me

  getaddrinfo(serv, tcp_port, &hints, &res);
 
  
  key_t keyC = ftok("/home/aeshma/joy2ser/serial_cli", '1');
  key_t keyJ = ftok("/home/aeshma/joy2ser/serial_cli", '2');

  msqidC = msgget(keyC, 0666 | IPC_CREAT);
  msqidJ = msgget(keyJ, 0666 | IPC_CREAT);
  msgctl(msqidC, IPC_RMID, NULL);
  msgctl(msqidJ, IPC_RMID, NULL);
  cout<<"I have removed the queues"<<endl;
  msqidC = msgget(keyC, 0666 | IPC_CREAT);
  msqidJ = msgget(keyJ, 0666 | IPC_CREAT);

  ss1 = ssh_new();
  
}   

void Joy2Ser::openthread(){
 
  thC = boost::thread(boost::bind(&Joy2Ser::executionerC, this));
}

Joy2Ser::~Joy2Ser(){
 
  //thJS.join();
  thC.join();// join all the threads
  cout<<"I have joined all the threads"<<endl;
  msgctl(msqidC, IPC_RMID, NULL);
  msgctl(msqidJ, IPC_RMID, NULL);
  cout<<"I have removed the queues"<<endl;

  close(connfdC);
  cout<<"closed connfdc"<<endl;
  if(ssh_channel_is_closed(channel)==0){
    cout<<"close ssh channel"<<endl;
    ssh_channel_close(channel);
    ssh_channel_free(channel);
  }
  ssh_disconnect(ss1);
  ssh_free(ss1);
  slog.close();
  cout<<"closed the file"<<endl;
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


bool Joy2Ser::readFds(int th){

  fd_set read_fds;  // temp file descriptor list for select()
  fd_set write_fds;
  struct timeval twait;
  int fdm;
  string s;
  bool control=true;
  twait.tv_sec=1;
  twait.tv_usec=0;

  FD_ZERO(&write_fds);    // clear the master and temp sets
  FD_ZERO(&read_fds);
  cout<<"Im read fds"<<endl;
  if(th==1){
    read_fds=masterreadC;
    fdm=fdmaxC;
    cout<<"fdm "<<fdm<<endl;
  }
  else{
    read_fds=masterread;
    fdm=fdmax;
  }
    
  if (select(fdm+1, &read_fds, NULL, NULL, &twait) == -1) {
    cout<<"error select"<<endl;
    control=false;
  }

  cout<<"we passed select"<<endl;
  if(FD_ISSET(0, &read_fds) && th==1){

    cout<<"ready for reading"<<endl;
    getline(cin,s);
    
    control=this->parse(s);
  }

  if(FD_ISSET(fdJS, &read_fds) && th!=1){ // this should also be a function
    cout<<"ready for reading"<<endl;
    while (read (fdJS, &jse, sizeof(jse)) > 0) {
      if(jse.type == JS_EVENT_AXIS){
	int num=0;
	cout<<"axis event"<<endl;
	if((num=send(connfd,&jse,sizeof(jse),MSG_NOSIGNAL))>0) // with no msg nosignal broken pipe will kill you
	  cout<<"we wrote "<<num<<" bytes"<<endl;
	else{ // the other side is down
	  cout<<"error error "<<strerror(errno)<<endl;
	  control=false; // close the thread
	  break;
	    //result=connect(connfd, res->ai_addr, res->ai_addrlen);
	}
      }
      else if(jse.type == JS_EVENT_BUTTON){
	cout<<"button event "<<(int) jse.number<<endl;
	if((int) jse.value==1 && (int) jse.number==4)
	  this->ship_message(SHIT,1);
      }
    }
    if (errno != EAGAIN && errno!=0){
      cout<<"joy read error "<<strerror(errno)<<endl;
      close(fdJS);
      sleep(2);
      fdJS=open (joyn, O_RDONLY | O_NONBLOCK); //open the joystick
    }
  }
  return control;
}

void Joy2Ser::executioner(int i){
  bool control=true;
  int   result;        // maximum file descriptor number

  fdJS=open(joyn, O_RDONLY | O_NONBLOCK); //open the joystick
  connfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  fcntl(connfd, F_SETFL, O_NONBLOCK); // make it non block

  // make the socket reusable
  int yes=1;
  if (setsockopt(connfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
    cout<<"error setsockopt"<<endl;
    control=false;
  }

   
  FD_ZERO(&masterwrite);    // clear the master and temp sets
  FD_ZERO(&masterread);
  // add the listener to the master set
  FD_SET(connfd, &masterwrite);
  FD_SET(fdJS, &masterread);
  fdmax = connfd; // so far, it's this one
  if(fdJS > fdmax)
    fdmax=fdJS;
  result = connect(connfd, res->ai_addr, res->ai_addrlen);
  this->ship_message(CONNECT,1);// ask control thread to connect, control thread shouldnt ty to connect until the client is up
  
  while (control){

    /******************************************************************************************************************************************/
    control=this->isConnected(i, result);

    /**********************************************************************************************************************************************/
    control=this->readFds(i);
   
    /********************************************************************************************************************/

    control=this->internalMess(i, result);

  }
  close(connfd);
  close(fdJS);
}



bool Joy2Ser::internalMess(int i, int & result){

  unsigned int mess0;
  if(this->Joy2Ser::try_catch_message(i, mess0)){  // this should also be a function
    cout<<"this is what I got "<<mess0<<endl;
    switch (mess0){
    case TERMINATE:
      {
	cout<<"I have been hit"<<endl;
	int merda=KILL;
	int num;
	if((num=send(connfdC,&merda,sizeof(merda),MSG_NOSIGNAL))>0) // with no msg nosignal broken pipe will kill you
	  cout<<"we kill the remote machine"<<endl;
	else{
	  cout<<"error sending message"<<endl;
	  if(connfd>2)
	    result = this->connectSock(i); // try to reconnect
	}
	return false;
      }
    case SHIT:
      {
	cout<<"we got shit"<<endl;
	int merda=25;
	int num;
	if((num=send(connfdC,&merda,sizeof(merda),MSG_NOSIGNAL))>0) // with no msg nosignal broken pipe will kill you
	  cout<<"we wrote CONTROL"<<num<<" bytes"<<endl;
	else{
	  cout<<"error sending message"<<endl;
	  if(connfdC>2)
	    close(connfdC);
	  
	  result = this->connectSock(1);
	  
	  cout<<"this is result "<<result<<" errno "<<strerror(errno)<<endl;
	}
	break;
      }
    case CONNECT:
      if(connfdC>2) // careful not to close stdin or stdout
	close(connfdC);
      result = this->connectSock(1);
      cout<<"this is result "<<result<<" errno "<<strerror(errno)<<endl;
    }
  }

  return true;
}




bool Joy2Ser::isConnected(int th, int & result){

  fd_set read_fds;  // temp file descriptor list for select()
  fd_set write_fds;
  struct timeval twait;
  string s;
  int fdm;
  
  if(result<0){
    cout<<"control "<<strerror(errno)<<endl;
    if(errno == EINPROGRESS){
      cout<<"connect CONTROL in progress"<<endl;
      do{
	twait.tv_sec=1;
	twait.tv_usec=1;
	if (th==1){ // control thread
	  read_fds=masterreadC;
	  write_fds=masterwriteC;
	  fdm=fdmaxC;
	}
	else{
	  read_fds=masterread;
	  write_fds=masterwrite;
	  fdm=fdmax;
	}
	
	if (select(fdm+1, &read_fds, &write_fds, NULL, &twait) == -1) {
	  perror("select");
	  return false;
	}
	else{
	  if(FD_ISSET(0, &read_fds) && th==1){
	    cout<<"ready for reading"<<endl;
	    getline(cin,s);
	    
	    if(!this->parse(s))
	      return false;
	    
	  }
	  else if(FD_ISSET(connfdC, &write_fds)){ 
	    int so_error;
	    socklen_t len = sizeof so_error;
	    
	    if(getsockopt(connfdC, SOL_SOCKET, SO_ERROR, &so_error, &len)<0){
	      cout<<"we have an error CONTROL"<<endl;
	    }
	    else
	      if(so_error >0)
		cout<<"so error error "<<strerror(so_error)<<endl;
	      else{
		cout<<"we should be connected CONTROL "<<strerror(so_error)<<endl;
		  result=0;
		  break;
		}
	  }
	  else if(FD_ISSET(connfd, &write_fds)){ 
	    int so_error;
	    socklen_t len = sizeof so_error;
	    
	    if(getsockopt(connfd, SOL_SOCKET, SO_ERROR, &so_error, &len)<0){
	      cout<<"we have an error CONTROL"<<endl;
	    }
	    else
	      if(so_error >0)
		cout<<"so error error "<<strerror(so_error)<<endl;
	      else{
		cout<<"we should be connected "<<strerror(so_error)<<endl;
		result=0;
		break;
	      }
	  }
	}
      }while(1);
    }
  }
  return true;
}


void Joy2Ser::executionerC(){
  bool control=true;
  int result=0;        // maximum file descriptor number

  cout<<"executioner C"<<endl;
   
  FD_ZERO(&masterwriteC);    // clear the master and temp sets
  FD_ZERO(&masterreadC);
  FD_SET(0, &masterreadC);
  fdmaxC = 0; // so far, it's this one
  

  
  while (control){

    control=this->isConnected(1, result);

    cout<<"passed connected"<<endl;
    
    // check for messages from other thread
    /**************************************************************************************************/
    control=this->internalMess(1, result);

    cout<<"passed internak ness"<<endl;
    control=this->readFds(1);
    cout<<"passed read"<<endl;
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

  Joy2Ser * joy = new Joy2Ser("/dev/input/js0",server_name,port);
  joy->openthread();
  delete joy;
  return 0;
}
/* g++  serial.cpp -o serial -lboost_system -lboost_thread -lpthread */
// g++ serial_clin.cpp  -o serial_cli -lboost_system -lboost_thread -lpthread -lrt
