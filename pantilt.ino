
#include <Servo.h>

Servo myservo, myservo1;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 90;    // variable to store the servo position
int pos1 = 90;
int step=0;
int step1=0;
int temp=90;
int temp1=90;
short throttle=0;
char control[2];
short int controli=777;

void setup() {
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo1.attach(10);
  Serial.begin(115200);      // turn on Serial Port
  //Serial.begin(115200);
  control[0] = (controli >> 8) & 0xFF;
  control[1] = controli & 0xFF;  
  //control[3] = 'n';  
  Serial.write(control); //we get it started
}

void loop() {
  if (Serial.available()>2) {         
  byte check; 
  byte axis;
  char instring[3];
  check=Serial.readBytes(instring,3);                 //Read user input into myName

 // if (instring[0]=='c'){ // communication from comp no data, make new request
   //Serial.write(control);// make new request    
    //}
  //else{
  axis=instring[0];
  throttle = (instring[1] << 8) | instring[2];

  if(axis==0 && throttle!=0 && abs(throttle)<=25){
    if (throttle>0)
    step=1;
    else
    step=-1;
    }
    else
      step=0;
  if(axis==1 && throttle!=0 && abs(throttle)<=25){
    if(throttle>0)
    step1=-1;
    else
    step1=1;
    }
  else
      step1=0; 
  }
  
  temp=pos+step;
  temp1=pos1+step1;
  
  if(temp>0 && temp<180){
    pos=temp;
    myservo.write(pos); 
    }
    else if(temp>180){
      pos=179;
      }
      else if(temp<0){
        pos=1;
        }
   
    if(temp1>0 && temp1<180){
    pos1=temp1;
    myservo1.write(pos1); 
    }
    else if(temp1>180){
      pos1=179;
      }
      else if(temp1<0){
        pos1=1;
        }
   if(throttle!=0 && abs(throttle)<=25){
   short del=abs(throttle);
   del=25-del;
   delay(del);
   }
  
}

