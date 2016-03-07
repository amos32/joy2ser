#ifndef H_clust
#define H_clust

#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <libssh2.h>
#include <libssh/libssh.h>
#include <ostream>
#include <stdexcept>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/thread.hpp>  
#include <boost/thread/mutex.hpp>
#include <boost/date_time.hpp> 
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#define MASTER 1
#define SLAVE 0
#define QUEUE_SIZE 1024
#define NUM_MESSAGES 20

#define SETMOMENTA 1
#define SIMULATE 2
#ifndef ENDED
#define ENDED 3
#endif

#define MAXM 128

using namespace std;
using namespace boost::asio;
using namespace boost::interprocess;

enum RES {CPU,TOCPU}; // message to main from gpu, to main from cpu thread, to GPU, tp CPU thread


typedef pair<int,int> pint;
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> AcceptPtr;
typedef boost::shared_ptr<boost::asio::io_service> IoPtr;


class connection : public boost::enable_shared_from_this<connection>
{
 public:
  connection(int port, io_service& serv) :   socket_(serv){}; //,  accept_(serv, ip::tcp::endpoint(ip::tcp::v4(),port)){};
  ip::tcp::socket socket_;
  ip::tcp::socket& socket(){
    return socket_;
  }
};

template <class T>
class message
{   

 public:
  int th;
  RES type; // cpu or gpu
  string name; // type of T
  int action; // the function that is shipping the message
  vector<T> data;
  message(){};
  message(int i, RES t, string n, int a, vector<T> d){
    th=i;
    type=t;
    name=n;
    action=a;
    data=d;
  };
 private:
  friend class boost::serialization::access;

  template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {  
      ar & th;
      ar & type;
      ar & name;
      ar & action;
      ar & data;
    }
};

struct node{
  node(string s, int a, int b) : address(s) , sshport(a), id(b){};
  string address;
  int sshport;
  int id;
};

//typedef pair<int,node> Anode; // active node, we use the integer to identify it
typedef boost::shared_ptr<connection> connection_ptr;


class toyCluster : public boost::noncopyable // makes it impossible to copy or assign
{
  typedef std::runtime_error re;
  
 protected:
  io_service io_serv; // the server
  io_service io_slave; // the client

  ip::tcp::socket sck_slav;
  
  ip::tcp::acceptor acceptor, acceptor_sync; //, acceptor; // acceptor of connections, the second is used to synchronize the cluster

  map<int,connection_ptr> connection_; // sockets on server port
  map<int,connection_ptr> connection_sync; // sockets on client port

  int type; // master or slave
  const char * server_add;
  int server_port, client_port;
  boost::thread th[MAXM]; // put them in a map as well, wrapped in a shared pointer
  boost::thread th_sync; // the second one is used to synchronize the cluster
  boost::asio::streambuf response, response_sync;
  
  ofstream slog, sync_log;
  list<pint> active_nodes;//what tasks the nodes are running, used for synchronization
  int whoAmI; // server is 0
  
  map<int,node> active_machines; // active machines in the cluster excluding the server

  inline string UnloadBuff() throw(boost::archive::archive_exception);
  inline string UnloadBuffSync() throw(boost::archive::archive_exception);
  virtual void cluster_executioner(); // service that runs on main program of slave
  
  virtual void receiverTh();
  virtual void receiverTh_sync();
  virtual void handle_connection(const boost::system::error_code& error); // CLIENT
  virtual void handle_accept(boost::system::error_code& error, int ith); // SERVER
  virtual void handle_connection_sync(const boost::system::error_code& error);
  void start_accept(int ith);
private:
  void ExecuteCmdResponse(const char* cmd, ssh_session ss1); 
  void startMessanger(); //SERVER
  virtual void startListener(); //CLIENT
  void acceptorTh(int ith){ // SERVER  
    this->start_accept(ith);
    io_serv.run();
  };
  void acceptorTh_sync(){ // SERVER
    cout<<"th sync"<<endl;
    io_slave.run(); // infinite loop
  };
 
  //void start_accept_sync(){ // SERVER
    // acceptor_sync.async_accept(sck_slav, boost::bind(&toyCluster::handle_accept_sync, this )); 
  //};
  void read_handler(const boost::system::error_code& error); // SERVER
  void write_handler(); // SERVER
  void handle_accept_sync(int ith); // server
  void handle_sync(int ith);
  void swrite_handler(string mess);
  void sread_handler();
 public:
  toyCluster(const char* IP, const int & PortNumber, const char* user, const char* pw, const int accport, const int caccport, const char * client, const int t,list<node> fclient,int who);
 toyCluster(const int accport = 6788, const int caccport=6798) :  io_serv(), io_slave(), sck_slav(io_slave),   server_port(accport), client_port(caccport), acceptor(io_serv, ip::tcp::endpoint(ip::tcp::v4(), accport)),  acceptor_sync(io_slave, ip::tcp::endpoint(ip::tcp::v4(), caccport)){}; //, connection_(new connection(accport,io_serv)){};
 virtual ~toyCluster();    
 void setConnection(const char* IP, const int & PortNumber, const char* user, const char* pw ,  const char * client , const int t,list<node> fclient, int who);

 bool IamMaster(){return type;};
 void openCluster(); 

 
  ///// SLAVE PART

 // put the messaging in a separate object
 void Message(string mess);
 void Message();
 void Broadcast(string mess);
 template <class T>
   void ship_message(message<T> mess);
 template <class T>
   message<T> catch_message(int th);
 template <class T>
   bool try_catch_message(int th, message<T> & mess);
 template <class T>
   void Broadcast(message<T> mess);
 void synchronize_cluster();
 void terminate_cluster();
 void handle_null(){};  
};


#endif
