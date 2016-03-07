inline string toyCluster::UnloadBuff() throw(boost::archive::archive_exception){
  string res;

  try{
    istream iss(&response);
  
    getline(iss,res); //if you don't do this the buffer response doesn't get unloaded 
  }
  catch(boost::archive::archive_exception & err2){
    throw(err2);
  }

  return res;
}

inline string toyCluster::UnloadBuffSync() throw(boost::archive::archive_exception){
  string res;

  try{
    istream iss(&response_sync);
  
    getline(iss,res); //if you don't do this the buffer response doesn't get unloaded 
  }
  catch(boost::archive::archive_exception & err2){
    throw(err2);
  }

  return res;
}

template <class T>
inline string SerializeN(T mp){ // serialize and null terminate it; the serialization must be implemented somewhere
    stringstream oss;
    boost::archive::text_oarchive oa(oss);
    oa << mp; // we ship online the data !
    oss<<endl; // add an end of line
    string serialized_mess(oss.str());
    return serialized_mess;
}

template <class T>
inline string Serialize(T mp){ // serialize and null terminate it; the serialization must be implemented somewhere
    stringstream oss;
    boost::archive::text_oarchive oa(oss);
    oa << mp; // we ship online the data !
    string serialized_mess(oss.str());
    return serialized_mess;
}

template <class T>
inline T DeSerialize(string response){
  T dat;
  stringstream stri;
  stri<<response;
  boost::archive::text_iarchive ia(stri);
  ia>>dat;
  return dat;
}

inline string NullTerm(const string S){ // add /n at the end of the string
  
  stringstream stri;
  stri<<S<<endl;
  string res=stri.str();
  return res;
}

template <class T>
message<T> toyCluster::catch_message(int th){
  message<T> mess;
  size_t recvd_size;
  unsigned int priority;
  stringstream iss;
  string serialized_mess;
  serialized_mess.resize(QUEUE_SIZE);  
  char * name;
  if (th==1)
    name="toy_queue";
  else if (th==0)
     name="toy_queue_sync";   
  else if (th==2)  
   name="toy_queue_sync_end";
  else if (th>2){
    stringstream stri;
    stri<<"clientq"<<th-2;
    string cname= stri.str();
    name = new char [cname.length()+1];
    std::strcpy (name, cname.c_str());
  }
  message_queue mq(open_only, name);
  try{
    mq.receive(&serialized_mess[0], QUEUE_SIZE, recvd_size, priority);
    // slog<<"received object "<<serialized_mess<<" thread "<<endl;
    iss << serialized_mess;
    boost::archive::text_iarchive ia(iss);
    ia >> mess;
  }
  catch (interprocess_exception& e) { 
    cout << e.what( ) <<endl;
  }

  return mess;
}

template <class T>
void toyCluster::Broadcast(message<T> mess){
  for (map<int,node>::iterator itt=active_machines.begin(); itt!=active_machines.end(); itt++){
    char * name;
    stringstream stri;
    stri<<"clientq"<<itt->first;
    string cname= stri.str();
    name = new char [cname.length()+1];
    std::strcpy (name, cname.c_str());
    string serialized_mess=Serialize<message<T> >(mess);
    try{
      message_queue mq(open_only,name );
      mq.send(serialized_mess.data(), serialized_mess.size(), 0); 
      //cout<<"Broadcast Im sending "<<serialized_mess<<" to thread "<<itt->first<<endl;
    } 
    catch (interprocess_exception& e) { 
      cout << e.what( ) <<" an exception"<<endl;
    }
  }
}

template <class T>
void toyCluster::ship_message(message<T> mess){ // T must be serializable
  stringstream oss;
  boost::archive::text_oarchive oa(oss);
  oa << mess;
  char * name;
  string serialized_mess(oss.str());
  if (mess.th==1)
    name="toy_queue";
  else if (mess.th==0)
    name="toy_queue_sync";
  else if (mess.th==2)
    name="toy_queue_sync_end";
  else if (mess.th>2){
    stringstream stri;
    stri<<"clientq"<<th-2;
    string cname= stri.str();
    name = new char [cname.length()+1];
    std::strcpy (name, cname.c_str());
  }  
  try{
    message_queue mq(open_only,name );
    mq.send(serialized_mess.data(), serialized_mess.size(), 0); 
    // slog<<"Im sending size "<<serialized_mess.size()<<" to thread "<<mess.th<<endl;
  } 
  catch (interprocess_exception& e) { 
    cout << e.what( ) <<" exception"<<endl;
  }
}


template <class T>
bool toyCluster::try_catch_message(int th, message<T> & mess){
  size_t recvd_size;
  unsigned int priority;
  stringstream iss;
  string serialized_mess;
  serialized_mess.resize(QUEUE_SIZE);
  char * name;
  bool control=false; // we have not received anything
  // ii is the thread of the graphic card !!!! we are sending everything to the same thread !!!!
  try { 
    switch(th){
    case 0:
      name="toy_queue_sync";
      break;
    case 1:
      name="toy_queue";
      break;
    case 2:
      name="toy_queue_sync_end";
      break;
    default:
      {
	stringstream stri;
	stri<<"clientq"<<th-2;
	string cname= stri.str();
	name = new char [cname.length()+1];
	std::strcpy (name, cname.c_str());
	break;
      }
   }
    message_queue mq(open_only, name); 
    if(mq.try_receive(&serialized_mess[0], QUEUE_SIZE, recvd_size, priority)){
      // cout<<"received size "<<recvd_size<<" thread "<<th<<" data "<<serialized_mess<<endl;
      control=true;
      //cout<<"we got something on thread "<<th<<endl;
      iss << serialized_mess;
      boost::archive::text_iarchive ia(iss);
      ia>>mess;
    }
  }
  catch (interprocess_exception& e) { 
    cout << e.what( ) <<endl;
  }

  return control;
}

