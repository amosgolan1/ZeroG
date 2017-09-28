#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <AccelStepper.h>
//control pins
const int pot1 = A0;
const int pot2 = A1;
const int pot3 = A2;
const int pot4 = A3;
const int switch1 = 2;
const int onOffButton = 3;

//control values for pump 1
int pushRate = 0;
int pushTarget = 0;
int pullRate = 0;
int pullTarget = 0;
bool pumpOn = 0;

//variables to hold information about the pump
bool buttonPressed = 0;
int stepCounter = 0;

//state machine
const int STATE_IDLE = 0;
const int STATE_SETVALUE = 1;
const int STATE_PUSH = 2;
const int STATE_PULL = 3;
const int STATE_STOP = 4;
const int STATE_BUTTON_PRESSED = 5;

int pump_state = STATE_IDLE;

Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_StepperMotor *Stepper1 = AFMS.getStepper(200, 1);




void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("PUMP IT ON");
  AFMS.begin();  // create with the default frequency 1.6KHz
 pinMode(pot1, INPUT);
 pinMode(pot2, INPUT);
 pinMode(pot3, INPUT);
 pinMode(pot4, INPUT);
 pinMode(switch1, INPUT);
 pinMode(onOffButton, INPUT);
}

void loop() {
  switch (pump_state) {
    case STATE_IDLE:    // waiting for a button press
      Serial.println("case idle"); 
      buttonPressed = digitalRead(onOffButton);
      //Serial.println("button pressed value is");
      Serial.println(buttonPressed);
      
      if(buttonPressed == true){//button pressed for the first time      
        pump_state = STATE_SETVALUE;
        delay(50);
      }
      break;
    
    case STATE_SETVALUE:
      Serial.println("case set values"); 
      pushRate = map(analogRead(pot1), 0, 1023, 0, 10); //pushing speed
      Serial.println(pushRate);
      pushTarget = map(analogRead(pot2), 0, 1023, 0, 1000);//pushing target
       Serial.println(pushTarget);
      pullRate = map(analogRead(pot3), 0, 1023, 0, 10);//pulling speed
       Serial.println(pullRate);
      pullTarget = map(analogRead(pot4), 0, 1023, 0,1000 );//pulling target
       Serial.println(pullTarget);
      pumpOn = digitalRead(switch1);//sitch checking if pump is on     
      if (pumpOn == true && pushRate>0 ||pullRate>0){
      delay(250);
      pump_state = STATE_PUSH;//pump is On, go to pushing stage
      break;
      }
      pump_state = STATE_IDLE;//pump was off, go back to idle
      break;
      
    
    case STATE_PUSH:   // stepper moves one turn to push syringe.
   Serial.println("case push");
       buttonPressed = digitalRead(onOffButton);
    if(buttonPressed == true){//button pressed for the second time to stop pushing
     Serial.println("button pressed again");
     pump_state = STATE_BUTTON_PRESSED;
      break;
      }
       if(pushTarget!=0){
       Stepper1->setSpeed(pushRate*100);  //set step speed based on push rate
       Stepper1->step(2, FORWARD, DOUBLE); //move stepper one step in desired speed
       
       
       //delay(pushRate*10);
       stepCounter+=1;//count steps
       }
    if (stepCounter>=pushTarget){
      stepCounter = 0;//zero the step counter towards pulling phase
      pump_state = STATE_PULL;//finished pushing, go to pulling
      break;
    } 
    break;
    
    case STATE_PULL: // stepper moves one turn to pull syringe.
     Serial.println("case pull");
     buttonPressed = digitalRead(onOffButton);
      if(buttonPressed == true){//button pressed for the second time to stop 
      pump_state = STATE_BUTTON_PRESSED;
      break;
      }
      //move stepper one step in desired speed
       //Serial.println("pulling...");
       if(pullTarget!=0){
       Stepper1->setSpeed(pullRate*100);  //set step speed based on push rate
       Stepper1->step(2, BACKWARD, DOUBLE); //move stepper one step
       stepCounter+=1;//count steps
       }
    if (stepCounter>=pullTarget){
      pump_state=STATE_STOP;
      break;
    } 
    break;
  
    case STATE_STOP: 
      Serial.println("case stop");
      stepCounter = 0;
      delay(10);
      pump_state=STATE_IDLE;
      break;
  
    case STATE_BUTTON_PRESSED://button was pressed a second time to stop extruding and restart.
      Serial.println("case button pressed");
      stepCounter = 0;//zero the step counter
      buttonPressed = 0;
      delay(500);
      pump_state=STATE_IDLE;
      break;
  }
}





