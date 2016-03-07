#include "toyClust.hpp"
#include "toyClust-temp.cpp"

toyCluster::toyCluster(const char* IP, const int & PortNumber = 22, const char* user = NULL, const char* pw= NULL, const int accport = 6788, const int caccport=6798, const char * client = NULL, const int t = MASTER,list<node> fclient = (list<node>) {}, int who=0) : 
  io_serv(), io_slave() , sck_slav(io_slave) , server_port(accport) , client_port(caccport), acceptor(io_serv, ip::tcp::endpoint(ip::tcp::v4(), accport)), acceptor_sync(io_slave, ip::tcp::endpoint(ip::tcp::v4(), caccport)), server_add(IP) 
{  
  this->setConnection(IP,PortNumber,user,pw,client,t,fclient,who);

}    

toyCluster::~toyCluster(){
  io_serv.stop();
  io_slave.stop();
  sync_log<<"join the sync thread"<<endl;
  th_sync.join(); 
  slog<<"join the main thread"<<endl;
  message_queue::remove("toy_queue");
  message_queue::remove("toy_queue_sync");   
    
  if (type==MASTER){
    message_queue::remove("toy_queue_sync_end");
    for (map<int,node>::iterator itt=active_machines.begin(); itt!=active_machines.end(); itt++ ){
      th[itt->first].join();
      stringstream stri;
      stri<<"clientq"<<itt->first;
      const char* name= (stri.str()).c_str();
      message_queue::remove(name);
    }
  }
  else
    th[whoAmI].join();

  //connection_.find(whoAmI)->second->socket_.close();
  sck_slav.close();  
  slog<<"closing the session"<<endl;
  slog.close();
  sync_log<<"closing session"<<endl;
  sync_log.close();
 
}

void toyCluster::setConnection(const char* IP, const int & PortNumber, const char* user, const char* pw, const char * client, const int t,list<node> fclient, int who) 
{

  server_add=IP;
  type =t;
  whoAmI=who; // number used by the server to recognize the client; the server itself is 0
  
  switch(type){
  case MASTER: // if I am master I start the slave on a remote computer 
    {
      cout<<"Iam master"<<endl;
      slog.open("/home/aeshma/master.log", ios::out | ios::app); // open the log file
      slog<<"new session "<<time(NULL)<<endl;
      sync_log.open("/home/aeshma/master_sync.log", ios::out | ios::app); // open the log file
      sync_log<<"new session "<<time(NULL)<<endl;

      int rc;
   

      int verbosity = SSH_LOG_PROTOCOL;
      int port = 22;
   
      for (list<node>::iterator itt=fclient.begin(); itt!=fclient.end(); itt++){
	ssh_session ss1;
	ss1 = ssh_new();
	slog<<"address "<< itt->address<<":"<< itt->sshport<<endl;
	const char * name = itt->address.c_str();
	ssh_options_set(ss1, SSH_OPTIONS_HOST, name); // the client
	ssh_options_set(ss1, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
	ssh_options_set(ss1, SSH_OPTIONS_PORT, & itt->sshport);  // the port
	ssh_options_set(ss1, SSH_OPTIONS_USER, &user); // we assume that the user is the same but we can change it
	// Connect to server
	
	rc = ssh_connect(ss1);
	
	if (rc == SSH_OK)  
	  {
	    rc = ssh_userauth_password(ss1, user, pw);  
	    if (rc == SSH_AUTH_SUCCESS){
	      stringstream stri;
	      string sout;
	      stri<<"/home/nironi/BBB/CUDA/NZC/Zig --client --host "<<IP<<" --whoAmI "<<(itt->id)<<" 2> /home/nironi/error.txt 1> /home/nironi/output.txt"; // We pass our own address and we tell the client it's own number
	      sout=stri.str();
	      cout<<sout<<endl;

	      const char *csout = sout.c_str();
	      try{
		string outp;
		this->ExecuteCmdResponse(csout, ss1);
		active_machines.insert(pair<int,node>(itt->id,*itt)); // the node is active so we store it
		slog<<csout<<endl;
		slog<<outp<<endl;
	      }
	      catch(std::runtime_error& err){
		slog<<"we cannot execute the remote command"<<endl;
		slog<<err.what()<<endl;
	      }
	    }
	    else
	      slog<<"we cannot authenticate"<<endl;
	  }
	else
	  slog<<"we cannot connect"<<endl;   
	sleep(2);
	ssh_disconnect(ss1);
	ssh_free(ss1); 
      }
 
      try { 
	// creating a message queue
	message_queue mq (create_only,"toy_queue",NUM_MESSAGES,QUEUE_SIZE );
      } 
      catch (interprocess_exception& e) { 
	std::cout << e.what( ) << std::endl;
	message_queue::remove("toy_queue");
	message_queue mq (create_only,"toy_queue",NUM_MESSAGES,QUEUE_SIZE );
      }
      try { 
	// creating a message queue
	message_queue mq (create_only,"toy_queue_sync",NUM_MESSAGES,QUEUE_SIZE );
      } 
      catch (interprocess_exception& e) { 
	std::cout << e.what( ) << std::endl;
	message_queue::remove("toy_queue_sync");
	message_queue mq (create_only,"toy_queue_sync",NUM_MESSAGES,QUEUE_SIZE );
      }
      try { // used to terminate the cluster
	// creating a message queue
	message_queue mq (create_only,"toy_queue_sync_end",NUM_MESSAGES,QUEUE_SIZE );
      } 
      catch (interprocess_exception& e) { 
	std::cout << e.what( ) << std::endl;
	message_queue::remove("toy_queue_sync_end");
	message_queue mq (create_only,"toy_queue_sync_end",NUM_MESSAGES,QUEUE_SIZE );
      }
 
      break;
      
    }
  case SLAVE:
    {
      slog.open("/home/nironi/slave.log", ios::out | ios::app); // open the log file
      slog<<"new session "<<time(NULL)<<endl;
      slog<<"Iam slave"<<endl;
      sync_log.open("/home/nironi/slave_sync.log", ios::out | ios::app); // open the log file
      sync_log<<"new session "<<time(NULL)<<endl;
      sync_log<<"Iam slave sync"<<endl;
      try { 
	// creating a message queue
	message_queue mq (create_only,"toy_queue",NUM_MESSAGES,QUEUE_SIZE );
      } 
      catch (interprocess_exception& e) { 
	slog << e.what( ) << endl;
	message_queue::remove("toy_queue");
	message_queue mq (create_only,"toy_queue",NUM_MESSAGES,QUEUE_SIZE );
      }
       try { 
	// creating a message queue
	message_queue mq (create_only,"toy_queue_sync",NUM_MESSAGES,QUEUE_SIZE );
      } 
      catch (interprocess_exception& e) { 
	slog << e.what( ) <<endl;
	message_queue::remove("toy_queue_sync");
	message_queue mq (create_only,"toy_queue_sync",NUM_MESSAGES,QUEUE_SIZE );
      }

      break;
    }
  }
}

void toyCluster::openCluster(){
  switch(type){
  case MASTER:
    this->startMessanger();
    break;
  case SLAVE:
    this->startListener(); // YOU ARE SUPPOSED TO OVERLOAD THE SLAVE SIDE WITH YOUR OWN IMPLEMENTATION
    break;
  }
}  

void toyCluster::ExecuteCmdResponse(const char* cmd, ssh_session ss1) 
{ 
  ssh_channel channel;
  int rc;
  channel = ssh_channel_new(ss1);
  ssh_channel_open_session(channel);
  rc = ssh_channel_request_exec(channel, cmd);
  if (rc != SSH_OK)
    {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      throw re("Failed to start the client");
    }
  else{
    /*char buffer[256];
    unsigned int nbytes;
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    int tot=nbytes;
    while (nbytes > 0 && tot<256) 
      {
	nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
	slog<<buffer;
	tot+=nbytes;
      }*/
    // stringstream stri;
    // stri<<buffer;
    //stri>> (*outp);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
  }
}

void toyCluster::receiverTh(){ // CLIENT
  boost::system::error_code error;
  ip::tcp::endpoint endpoint(ip::address::from_string(server_add), server_port);  
  sck_slav.async_connect(endpoint, boost::bind(&toyCluster::handle_connection, this, boost::asio::placeholders::error));
  slog<<"Im receiver"<<endl;
  io_slave.run();
}

void toyCluster::receiverTh_sync(){ // CLIENT
  ip::tcp::endpoint endpoint(ip::address::from_string(server_add), client_port);  
  connection_.find(whoAmI)->second->socket_.async_connect(endpoint, boost::bind(&toyCluster::handle_connection_sync, this, boost::asio::placeholders::error)); // you should connect only if you have something to say an disconnect immediately
  sync_log<<"Im receiver"<<endl; // we just start the io service on this thread. we handle connections where needed
  io_serv.run();
}

void toyCluster::start_accept(int ith){// server
  boost::system::error_code error, error2;  
  connection_ptr aaa=connection_.find(ith)->second;
  connection_ptr bbb=connection_sync.find(ith)->second;
  acceptor.async_accept(aaa->socket_, boost::bind(&toyCluster::handle_accept, this, error, ith));
  acceptor_sync.async_accept(bbb->socket_, boost::bind(&toyCluster::handle_accept_sync, this, ith));
}

void toyCluster::startMessanger(){ // SERVER
 
  for (map<int,node>::iterator itt=active_machines.begin(); itt!=active_machines.end(); itt++ ){
    connection_ptr aaa(new connection(server_port,io_serv));
    connection_ptr bbb(new connection(client_port,io_slave));
    connection_.insert(make_pair(itt->first,aaa));
    connection_sync.insert(make_pair(itt->first,bbb));
    stringstream stri;
    stri<<"clientq"<<itt->first;
    string sname=stri.str();
    char * name = new char[sname.length()+1];
    std::strcpy(name,sname.c_str()); // to char*
    cout<<name<<endl;
    try { 
      message_queue mq (create_only, name,NUM_MESSAGES,QUEUE_SIZE );
    } 
    catch (interprocess_exception& e) { 
      std::cout << e.what( ) << std::endl;
      message_queue::remove(name);
      message_queue mq (create_only, name,NUM_MESSAGES,QUEUE_SIZE );
    }
    delete name;
    th[itt->first]=boost::thread(boost::bind(&toyCluster::acceptorTh, this, itt->first));
  }
  th_sync=boost::thread(boost::bind(&toyCluster::acceptorTh_sync, this));
}

void toyCluster::startListener(){ // CLIENT
  connection_ptr aaa(new connection(client_port,io_serv));  
  connection_.insert(make_pair(whoAmI,aaa));
  th[whoAmI]=boost::thread(boost::bind(&toyCluster::receiverTh, this));
  th_sync=boost::thread(boost::bind(&toyCluster::receiverTh_sync, this));
}

void toyCluster::read_handler(const boost::system::error_code& error) // OVERLOADED
{
  string mess;
  string messs;
  stringstream stri;
  message<string> messS;
  if (!error)
    {
      mess=this->UnloadBuff();
      stri<<"this is what we got from server: "<<mess<<endl;
      messs=stri.str();	
      cout<<messs;
      if(mess=="END"){
	cout<<"we close the acceptor"<<endl;
	messS.th=0; // main
	messS.data.push_back(mess);
	io_serv.stop();
	this->toyCluster::ship_message(messS);
      }
      else{
	cout<<"ready to accept a new connection"<<endl;
	this->start_accept(0);  // fix it
      }		  
    }
}

void toyCluster::write_handler() // OVERLOADED
{
  cout<<connection_.find(whoAmI)->second->socket_.remote_endpoint()<<endl;	
  
  boost::asio::async_read_until(connection_.find(whoAmI)->second->socket_,response,'\n',boost::bind(&toyCluster::read_handler, this, boost::asio::placeholders::error));
}

void toyCluster::handle_accept(boost::system::error_code& error, int ith) // SERVER SIDE // overloaded
{
  if(!error){
  cout<<"we have a connection"<<endl;
  
  message<string> messS;
  messS = this->toyCluster::catch_message<string>(ith+2);// blocks
  string messg = NullTerm(messS.data[0]);
  cout<<"request from main: "<<messg;
  connection_ptr machine=connection_.find(ith)->second;
  if (messS.data[0]=="END"){
    slog<<" we received END"<<endl;
    message<string> messS;
    messS.data.push_back("END");
    messS.th=0;
    boost::asio::write(machine->socket_,boost::asio::buffer(messg)); // tell the client to terminate
    this->toyCluster::ship_message(messS); // we terminate ouserlves
    io_serv.stop();
  }
  else{
    cout<<"ready to accept a new connection"<<endl;
    this->start_accept(ith); // fix it
  }
  }
  else
    {
      cout<<"error error error"<<endl;
      this->start_accept(ith);
    }
}

void toyCluster::handle_accept_sync(int ith) // SERVER SIDE // not overloaded
{
  cout<<"conection"<<endl;
  connection_ptr bbb=connection_sync.find(ith)->second;
  boost::asio::async_read_until(bbb->socket_, response_sync, '\n',boost::bind(&toyCluster::handle_sync, this, ith));   // when we stop we might need to cancel the read    
  
}


void toyCluster::handle_sync(int ith){ // SERVER SIDE not overloaded, we expect to receive a pair of ints
  // we receive a message from client and we forward it as is to queue 2 (typically a client has completed a task)
  sync_log<<"we got a message from client"<<endl;
  
  string temp = this->UnloadBuffSync();
 
  sync_log<<temp<<endl;
  if (temp!=""){ 
    message<string> messS;
    messS.th=2;
    messS.data.push_back(temp);
    this->toyCluster::ship_message(messS);
  }
  this->handle_accept_sync(ith);
}



void toyCluster::swrite_handler(string mess)
{
  slog<<mess; 
  if(mess=="END"){
    slog<<" we received END"<<endl;
    message<string> messS;
    messS.data.push_back("END");
    messS.th=0;
    this->toyCluster::ship_message(messS);
    io_slave.stop();
  }	
  else{
    boost::system::error_code ignored_error;
    slog<<" we try to get another message"<<endl;
    this->handle_connection(ignored_error);
  }
}



 
void toyCluster::sread_handler()
{
  string mess;
  string messg;
  istream is(&response);
  getline(is,mess); // remember that >> stops at white spaces and getline doesn't include endl
  messg=NullTerm(mess);

  boost::asio::async_write(sck_slav, boost::asio::buffer(messg), boost::bind(&toyCluster::swrite_handler , this , mess));
}

void toyCluster::handle_connection(const boost::system::error_code& error)
{
  if (!error){
    slog<<"trying to read"<<endl;
    boost::asio::async_read_until(sck_slav, response, '\n',boost::bind(&toyCluster::sread_handler, this));  
  } 
  else
    slog<<"error in handle connection"<<endl;
}

void toyCluster::handle_connection_sync(const boost::system::error_code& error)
{
  if (!error){
    sync_log<<"connected"<<endl;

  } 
  else
    sync_log<<"error in handle connection"<<endl;
}

void toyCluster::Message(string mess)
{
  message<string>  messS;
  messS.name="string";
  vector<string> vs;
  vs.push_back(mess);
  messS.data = vs;
  messS.th=1;
  this->toyCluster::ship_message(messS);
}

void toyCluster::Message()
{
  message<string>  messS;
  messS.name="string";
  vector<string> vs;
  string mess;
  cout<<"Insert a string"<<endl;
  cin>>mess;
  vs.push_back(mess);
  messS.data = vs;
  messS.th=1;
  this->toyCluster::ship_message(messS);
}


void toyCluster::Broadcast(string mess){
  for (map<int,node>::iterator itt=active_machines.begin(); itt!=active_machines.end(); itt++){
    char * name;
    stringstream stri;
    stri<<"clientq"<<itt->first;
    string cname= stri.str();
    name = new char [cname.length()+1];
    std::strcpy (name, cname.c_str());

    try{
      message_queue mq(open_only,name );
      mq.send(mess.data(), mess.size(), 0); 
      cout<<"Broadcast Im sending size "<<mess.size()<<" to thread "<<itt->first<<endl;
    } 
    catch (interprocess_exception& e) { 
      cout << e.what( ) <<" suca"<<endl;
    }
  }
}

void toyCluster::synchronize_cluster()
{
  message<string> mess;
   pint dat;      

  while(true){
    if (this->toyCluster::try_catch_message(2,mess)){
    
      stringstream stri; 
      stri<<mess.data[0];
      //      sync_log<<mess.data[0]<<endl;
      try{
	boost::archive::text_iarchive ia(stri);
	ia>>dat;
	list<pint>::iterator itt;
	itt = find(active_nodes.begin(),active_nodes.end(),dat);
	if (itt != active_nodes.end()){
	  sync_log<<"we found it and erased it"<<endl;
	  active_nodes.erase(itt);
	}
	if (active_nodes.size()==0){
	  sync_log<<"cluster synchronized"<<endl;
	  break;
	}
      }
      catch(boost::archive::archive_exception & err2){
	//	sync_log<< err2.what()<<endl;
	//sync_log<<"we probably got some garbage on the socket"<<endl;
      }
    }
    else if(active_nodes.size()==0){
      sync_log<<"cluster synchronized"<<endl;
      break;
    }
  }
}

void toyCluster::terminate_cluster(){
  message<string> mess;
  //  mess.th=0;
  mess.data.push_back("END");
  this->toyCluster::Broadcast(mess);
}

void toyCluster::cluster_executioner(){ // CLIENT overloaded
  message<string> mess;
  bool control=true;
  while(control){
    if(this->toyCluster::try_catch_message<string>(1,mess)){
      switch(mess.action){
      case ENDED:
	control=false;
	break;
      }
    }
  }
}
