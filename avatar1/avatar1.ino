
//#include <Servo.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
// you can also call it with a different address you want
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x41);

// Depending on your servo make, the pulse width min and max may vary, you 
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to tweak them as necessary to match the servos you
// have!
#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  550 // this is the 'maximum' pu

//Servo myservo, myservo1;  // create servo object to control a servo
// twelve servo objects can be created on most boards

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int MAXDC=1000;
 byte axis;
short throttle=0;
short throttle1=0;
short throttle0=0;
short throttlea=0;
short res=25;
float mstep=1.0/((float) res);
short ref=SERVOMIN+(SERVOMIN+SERVOMAX)/2;
short throttle3=ref;
float pos = ref;    // variable to store the servo position
float pos1 = ref;
float step=0;
float step1=0;
float temp=ref;
float temp1=ref;
unsigned char instring[4];// buffer

void setup() {
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  //myservo.attach(6);  // attaches the servo on pin 9 to the servo object
  //myservo1.attach(11);
  Serial.begin(115200);      // turn on Serial Port
  Serial1.begin(115200);  // debug
  pwm.begin();
  pwm1.begin();
  
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  pwm1.setPWMFreq(1500);  // dc motor  
  pinMode(2,OUTPUT);  
  pinMode(4,OUTPUT);
  //pinMode(3,OUTPUT);  
  //pinMode(5,OUTPUT);
 
  pwm.setPWM(0, 0, ref);
  pwm.setPWM(1, 0, ref);
  pwm.setPWM(2, 0, ref);
  pwm1.setPWM(0, 0, 0);
  pwm1.setPWM(1, 0, 0);
}

void loop() {
/*  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)*/
  //Serial.print("AcX = "); Serial.print(AcX);
  //Serial.print(" | AcY = "); Serial.print(AcY);
  //Serial.print(" | AcZ = "); Serial.print(AcZ);
  //Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);  //equation for temperature in degrees C from datasheet
  //Serial.print(" | GyX = "); Serial.print(GyX);
  //Serial.print(" | GyY = "); Serial.print(GyY);
  //Serial.print(" | GyZ = "); Serial.println(GyZ);
 
  while (Serial1.available()>3) {        
  byte check; 
  
  check=Serial1.readBytes(instring,4);   
   axis=instring[0];
   if (axis==1)
    throttle1 = (instring[1] << 8) | instring[2]; // pan
   else  if (axis==0)
    throttle0 = (instring[1] << 8) | instring[2]; // tilt
   else if (axis==2 || axis==5)
     throttlea = (instring[1] )<<8 | (instring[2]); // dc motor
    else if (axis==3)
     throttle3 = (instring[1] )<<8 | (instring[2]); // steering
    //Serial.print("axis ");
    //Serial.println(axis);
    //Serial.print("throttle ");
    //if(axis==0) 
     //Serial.println(throttle0);
    //else if(axis==1)
    // Serial.println(throttle1);
    //else if (axis==3)
    // Serial.println(throttle3);
    //else
     //Serial.println(throttlea);
 
    if(axis==0) // tilt
    step=mstep*(float) throttle0;
  
  if(axis==1)
    step1=mstep*(float) throttle1;

  if(axis==3)
     pwm.setPWM(2, 0, throttle3);
   
   if(axis==2){
        digitalWrite(2,LOW); //controls the direction the motor
        digitalWrite(4,LOW);
        //analogWrite(3,map(throttle,-32767,32767,0,155));
        //analogWrite(5,map(throttle,-32767,32767,0,155));
        pwm1.setPWM(0, 0, throttlea);
        pwm1.setPWM(1, 0, throttlea);
        }

   if(axis==5){
        digitalWrite(2,HIGH); //controls the direction the motor
        digitalWrite(4,HIGH);
        //analogWrite(3,map(throttle,-32767,32767,0,155));
        //analogWrite(5,map(throttle,-32767,32767,0,155));
        pwm1.setPWM(0, 0, throttlea);
        pwm1.setPWM(1, 0, throttlea);
        }
    temp=(float) pos+step;
  temp1=(float) pos1+step1;

  if(temp>=SERVOMIN && temp<=SERVOMAX){
    pos= temp;
   //myservo.write(pos);
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
    //myservo1.write(pos1); 
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
   //myservo.write(pos);
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
    //myservo1.write(pos1); 
    pwm.setPWM(1, 0, (short) pos1); 
    }
    else if(temp1>SERVOMAX){
      pos1=SERVOMAX;
      }
      else if(temp1<SERVOMIN){
        pos1=SERVOMIN;
        }
   
}

