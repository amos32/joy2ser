#include "SimpleSerial.h"

using namespace std;
using namespace boost;

Joy2Ser::Joy2Ser(string a, unsigned int b, string c)  : io(), serial(io,a){
  port=a;
  baud_rate=b;
  joyn=c;

  char * filen= new char[joyn.length()+1];
  std::strcpy(filen,joyn.c_str()); // to char*
  fdJS=open (filen, O_RDONLY | O_NONBLOCK); //open the joystick
  delete[] filen;
  
  serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));

  stringstream sti;
  string name;
   
   sti.str("");
   sti.clear();
   sti<<"Joy2Serq"<<0;
   name = sti.str();
   queuenJS= new char[name.length()+1];
   std::strcpy(queuenJS,name.c_str()); // to char*
   sti.str("");
   sti.clear();
   sti<<"CPUcontrolq"<<1;
   name = sti.str();
   queuenC= new char[name.length()+1];
   std::strcpy(queuenC,name.c_str()); // to char*
    // name the queues
   try { 
     // creating a message queue
     message_queue mq (create_only, queuenJS,NUM_MESSAGES,QUEUE_SIZE );
   } 
   catch (interprocess_exception& e) { 
     std::cout << e.what( ) << std::endl;
     cout<<"I kill the queue "<<queuenJS<<"and I remake it"<<endl;
     message_queue::remove(queuenJS);
     message_queue mq (create_only, queuenJS,NUM_MESSAGES,QUEUE_SIZE );
   }
   try { 
     // creating a message queue
     message_queue mq (create_only, queuenC,NUM_MESSAGES,QUEUE_SIZE );
   } 
   catch (interprocess_exception& e) { 
     std::cout << e.what( ) << std::endl;
     cout<<"I kill the queue "<<queuenC<<"and I remake it"<<endl;
     message_queue::remove(queuenC);
     message_queue mq (create_only, queuenC,NUM_MESSAGES,QUEUE_SIZE );
   }
}   

void Joy2Ser::openthread(){
  thJS = boost::thread(boost::bind(&Joy2Ser::executioner, this, 0));
  thC = boost::thread(boost::bind(&Joy2Ser::executioner, this, 1));
}

Joy2Ser::~Joy2Ser(){
  thJS.join();
  thC.join();// join all the threads
  cout<<"I have joined all the threads"<<endl;
  message_queue::remove(queuenJS);//remove all the queues
  message_queue::remove(queuenC);
  cout<<"I have removed the queues"<<endl;
  close(fdJS);
  cout<<"closed the file"<<endl;
}


template <class T>
void Joy2Ser::ship_result(message<T> mess){
  stringstream oss;

  boost::archive::text_oarchive oa(oss);
  oa << mess;
  string serialized_mess(oss.str());
     
  try {
    message_queue mq(open_only,queuenJS );
    mq.send(serialized_mess.data(), serialized_mess.size(), 0); 
    // cout<<"Im sending size "<<serialized_mess.size()<<" to thread "<<mess.th<<endl;
  } 
  catch (interprocess_exception& e) { 
    cout << e.what( ) <<" suca"<<endl;
  }
}

template <class T>
pair<bool,int> Joy2Ser::try_catch_result(int th, message<T> & mess){
  pair<bool,int> pp;
  size_t recvd_size;
  unsigned int priority;
  stringstream iss;
  string serialized_mess;
  serialized_mess.resize(QUEUE_SIZE);
  char * name;
  bool control=false;
  try{
    switch(th){
    case 0:
      name=queuenJS;
      break;
    case 1:
      name=queuenC;
      break;
    }
    message_queue mq(open_only, name); 
    //cout<<"thread "<<th<<"has "<<mq.get_num_msg()<<" messages"<<endl;
    if(mq.try_receive(&serialized_mess[0], QUEUE_SIZE, recvd_size, priority)){
      // cout<<"received size "<<recvd_size<<" thread "<<th<<" data "<<serialized_mess<<endl;
      control=true;
      //cout<<"we got something on thread "<<th<<endl;
      iss << serialized_mess;
      boost::archive::text_iarchive ia(iss);
      ia >> mess;
      //cout<<"mess "<<mess.name<<endl;
    }
  }
  catch (interprocess_exception& e) { 
    cout << e.what( ) <<endl;
  }

  pp.first=control;
  if (control)
    pp.second=th;
  return pp;
}

void Joy2Ser::SerMessanger(js_event jse){
  
  stringstream sti;
  string name;
   
   sti.str("");
   sti.clear();
   sti<<6666;//beginning
   name = sti.str();
   boost::asio::write(serial,boost::asio::buffer(name.c_str(),name.size()));
   sti.str("");
   sti.clear();
   sti<<jse.time;
   name = sti.str();
   boost::asio::write(serial,boost::asio::buffer(name.c_str(),name.size()));
   sti.str("");
   sti.clear();
   sti<<jse.value;
   name = sti.str();
   boost::asio::write(serial,boost::asio::buffer(name.c_str(),name.size()));
    sti.str("");
   sti.clear();
   sti<<jse.type;
   name = sti.str();
   boost::asio::write(serial,boost::asio::buffer(name.c_str(),name.size()));
   sti.str("");
   sti.clear();
   sti<<jse.number;
   name = sti.str();
   boost::asio::write(serial,boost::asio::buffer(name.c_str(),name.size()));
   sti.str("");
   sti.clear();
   sti<<7777;//terminate
   name = sti.str();
   boost::asio::write(serial,boost::asio::buffer(name.c_str(),name.size()));
}

void Joy2Ser::messanger(js_event jse, int i){ // sends a joystick event
  vector<int> message;
  message.push_back(jse.time);
  message.push_back(jse.value);
  message.push_back(jse.type);
  message.push_back(jse.number);
  switch (i){
  case 0: 
    try { 
      message_queue mq (open_only, queuenJS);
      int ms=message.size();
      mq.send(&ms, sizeof(int), 0); // the first integer is the length of the message
      for (int k=0; k<ms; k++)
	mq.send(&message[k], sizeof(int), 0);
    } 
    catch (interprocess_exception& e) { 
      cout << e.what( ) <<endl;
    }
    break;
  case 1: 
    try { 
      message_queue mq (open_only, queuenC);
      int ms=message.size();
      mq.send(&ms, sizeof(int), 0); // the first integer is the length of the message
      for (int k=0; k<ms; k++)
	mq.send(&message[k], sizeof(int), 0);
    } 
    catch (interprocess_exception& e) { 
      cout << e.what( ) <<endl;
    }
    break;
  }
}

void Joy2Ser::executioner(int i){
  bool control=true;
  switch (i){
  case 0: // joystick thread
    while (control){
      message<int> mess;
      pair<bool, int> pp;
      pp=this->Joy2Ser::try_catch_result<int>(i, mess); //try to read from queue without blocking
      if(pp.first){
	cout<<"this is what I got "<<mess.data[0]<<endl;
	switch (mess.data[0]){
	case KILL:
	  cout<<"I have been hit"<<endl;
	  control=false;
	  break;
	}
      }
      else{ // since we didnt get anything lets read from joystick
	struct  js_event jse;
	while (read (fdJS, &jse, sizeof(jse)) > 0) {
	  cout<<jse.type<<endl;
	  this->SerMessanger(jse); //we send it over serial line
	  /* EAGAIN is returned when the queue is empty */
	}
	if (errno != EAGAIN){
	  cout<<"joy read error"<<endl;
	  /* do something interesting with processed events */
	}
	else
	  cout<<"empty joy queue"<<endl;
      }
    }
    break;
  case 1: //this is the control thread
    string s;
    while(control){
      cin>>s;
      if(s=="q"){
	control=false;
	message<int> mess2js;
	vector<int> chunk;
	mess2js.name="int";
	mess2js.action=KILL;
	mess2js.th=0;
	chunk.push_back(KILL);
	mess2js.data=chunk;
	this->ship_result(mess2js);
      }
    }
    break;
  }
}

int main(int argc, char* argv[])
{
  Joy2Ser * joy = new Joy2Ser("/dev/ttySAC2",115200,"/dev/input/js0");
  joy->openthread();
  delete joy;
  return 0;
}
/* g++  serial.cpp -o serial -lboost_system -lboost_thread -lpthread */
// g++ serial.cpp  -o serial -lboost_system -lboost_thread -lpthread -lboost_serialization -lrt
