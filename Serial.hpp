#ifndef H_Joy2Sr
#define H_Joy2Ser

#include <boost/asio.hpp>
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

#define QUEUE_SIZE 1024
#define NUM_MESSAGES 20
#define KILL 101

using namespace std;
using namespace boost::interprocess;

template <class T>
class message
{   
 public:
  int th;
  string name; // type of T
  int action; // the function that is shipping the message
  vector<T> data;
  message(){};
  message(int i, string n, int a, vector<T> d){
    th=i;
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
      ar & name;
      ar & action;
      ar & data;
    }
};

class Joy2Ser
{
 private:
  boost::thread  thJS; // thread for joystick to serial 
  boost::thread  thC; // cpu control thread
  char *  queuenJS; //send to Joy2Serial         0+
  char * queuenC; // cpu control process   1000+
  string port; // the serial port
  string joyn; // joystick name
  int fdJS; // joystick file descriptor
  unsigned int baud_rate;
  boost::asio::io_service io;
  boost::asio::serial_port serial;
  char ard1[2];
  boost::asio::streambuf ard_control; 
  
 public:
  Joy2Ser(string a, unsigned int b, string c);
  ~Joy2Ser();
  void synchronize_cpu(); // waits for cpu threads to complete
  void openthread();
  template <class T>
    void ship_result(message<T> res);
  template <class T>
    pair<bool,int> try_catch_result(int th, message<T> & mess);
  void messanger(js_event jse, int i);
  void SerMessanger(js_event jse);
  void SerMessanger();
  void executioner(int i); // listen for messages on thread i
  void sread_handler(bool& data, const boost::system::error_code& error);
};

class SimpleSerial
{
public:
    /**
     * Constructor.
     * \param port device name, example "/dev/ttyUSB0" or "COM4"
     * \param baud_rate communication speed, example 9600 or 115200
     * \throws boost::system::system_error if cannot open the
     * serial device
     */
    SimpleSerial(std::string port, unsigned int baud_rate)
    : io(), serial(io,port)
    {
        serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
    }

    /**
     * Write a string to the serial device.
     * \param s string to write
     * \throws boost::system::system_error on failure
     */
    void writeString(std::string s)
    {
        boost::asio::write(serial,boost::asio::buffer(s.c_str(),s.size()));
    }

    void write2serial() //synchronous
    {
      string letter;
      cin>>letter;
      boost::asio::write(serial,boost::asio::buffer(letter.c_str(),letter.size()));
    }
    /**
     * Blocks until a line is received from the serial device.
     * Eventual '\n' or '\r\n' characters at the end of the string are removed.
     * \return a string containing the received line
     * \throws boost::system::system_error on failure
     */
    std::string readLine()
    {
        //Reading data char by char, code is optimized for simplicity, not speed
        using namespace boost;
        char c;
        std::string result;
        for(;;)
        {
            asio::read(serial,asio::buffer(&c,1));
            switch(c)
            {
                case '\r':
                    break;
                case '\n':
                    return result;
                default:
                    result+=c;
            }
        }
    }

private:
    boost::asio::io_service io;
    boost::asio::serial_port serial;
};

#endif
