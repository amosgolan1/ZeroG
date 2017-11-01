//to do:
//add timestepmps for actions. add IMU data with time stemp
//callibrate pumps 
//create indicatoions on poteniometers to know where are the differnt stops
//make the stops of potentiometers whole numbers beteen 0 and 900 (100,200,300,400...)

//libraries
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <AccelStepper.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
//
//CS for SD shield SPI
const int chipSelect = 53;//was 10
String dataString = "";
File dataFile;

//control pins for pump 1
const int pot1 = A0;
const int pot2 = A1;
const int pot3= A2;
const int pot4 = A3;

const int switch1 = 2;
const int switch2 = 3;

//green on/off button
const int onOffButton = 4;

//control values for pumps. (i think A4 and A5 are used for i2c for the shields)
int pushRate = 0;
int pushTarget = 0;
int pullRate = 0;
int pullTarget = 0;
bool pumpOn1 = 0;
bool pumpOn2 = 0;
//green indicator led
//const int LED = 5;

//variables to hold information about the pumps
bool buttonPressed = 0;
int stepCounter = 0;

//state machine
const int STATE_IDLE = 0;
const int STATE_SETVALUE = 1;
const int STATE_PUSH = 2;
const int STATE_PULL = 3;
const int STATE_STOP = 4;
const int STATE_BUTTON_PRESSED = 5;

//initial state
int pump_state = STATE_IDLE;

//create stepper objects
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *Stepper1 = AFMS.getStepper(200, 1);
Adafruit_StepperMotor *Stepper2 = AFMS.getStepper(200, 2);

void setup() {
  Serial.begin(9600);
  Serial.println("PUMP IT ON");
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  AFMS.begin();  // create with the default frequency 1.6KHz
  //initate pots and switch for pump 1
  pinMode(pot1, INPUT);
  pinMode(pot2, INPUT);
  pinMode(pot3, INPUT);
  pinMode(pot4, INPUT);
  
  //on off button for the whole system
  pinMode(onOffButton, INPUT);
  
  //initate switches for pumps
  pinMode(switch1, INPUT);
  pinMode(switch2, INPUT);

//set up CS as output
  //pinMode(SS, OUTPUT);
pinMode(10, OUTPUT);

  //initate indication LED light
 // pinMode(LED, OUTPUT);

  //setup the sd card
  Serial.print("Initializing SD card...");
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
////  // Open up the file we're going to log to!
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog.txt");
    // Wait forever since we cant write data
    while (1) ;
  }
  dataFile.println("pushRate, pushTarget, pullRate, pullTarget");
    dataFile.flush();

}

void loop() {
  switch (pump_state) {
    case STATE_IDLE:    // waiting for a button press
      Serial.println("case idle");
      buttonPressed = digitalRead(onOffButton);
      if (buttonPressed == true) { //button pressed for the first time
        pump_state = STATE_SETVALUE;
        delay(50);
      }
      break;

    case STATE_SETVALUE:
      Serial.println("case set values");
      //set values for pump 1
      pushRate = map(analogRead(pot1), 0, 1023, 0, 10); //pushing speed
      Serial.println(pushRate);
      pushTarget = map(analogRead(pot2), 0, 1023, 0, 1000);//pushing target
      Serial.println(pushTarget);
      pullRate = map(analogRead(pot3), 0, 1023, 0, 10);//pulling speed
      Serial.println(pullRate);
      pullTarget = map(analogRead(pot4), 0, 1023, 0, 1000 ); //pulling target
      Serial.println(pullTarget);
      
       pumpOn1 = digitalRead(switch1);//switch checking if pump 1 is on
       pumpOn2 = digitalRead(switch2);//switch checking if pump 2 is on
      
      // read pots and append to the string to save
      for (int potPin = 0; potPin < 4; potPin++) {
        int potReading = analogRead(potPin);
        dataString += String(potReading);
        if (potPin <=3) {
          dataString += ",";
        }
      }

      //append the switch state of the two pumps
      for (int switchPin = 0; switchPin < 2; switchPin++) {
        int switchReading = digitalRead(switchPin);
        dataString += String(switchReading);
        if (switchPin < 1) {
          dataString += ",";
        } 
      }
      dataString += "\n";
      //dataFile.println(dataString);
      Serial.println(dataString);
      //dataFile.flush();//for some reason flush dosn't work here..
      dataString = "";//clear the datastring by asigning a new clear string to it
  
      

      if (pumpOn1 == true && pushRate > 0 || pumpOn2 == true && pushRate > 0) { 
//        digitalWrite(LED, HIGH);//indicate pump is active
        delay(250);
        pump_state = STATE_PUSH;//pump is On, go to pushing stage
        break;
      }
      pump_state = STATE_IDLE;//pump was off, go back to idle
      break;


    case STATE_PUSH:   // stepper moves one turn to push syringe.
      Serial.println("case push");
      buttonPressed = digitalRead(onOffButton);
      if (buttonPressed == true) { //button pressed for the second time to stop pushing
        Serial.println("button pressed again");
        pump_state = STATE_BUTTON_PRESSED;
        break;
      }
      
      if (pushTarget != 0 && stepCounter <= pullTarget){
        Serial.println ("pumps state");
        Serial.println (pumpOn1);
        Serial.println (pumpOn2);
      if (pumpOn1 == true) {
        Stepper1->setSpeed(pushRate * 10); //set step speed based on push rate1
        Stepper1->step(2, FORWARD, SINGLE); //move stepper one step in desired speed
      }
      
      if (pumpOn2 == true) {
        Stepper2->setSpeed(pushRate * 10); //set step speed based on push rate2
        Stepper2->step(2, FORWARD, SINGLE); //move stepper one step in desired speed 
      }  
      stepCounter += 1; //count steps 
      }
      if (stepCounter >= pushTarget) {
        stepCounter = 0;//zero the step counter towards pulling phase
        pump_state = STATE_PULL;//finished pushing, go to pulling
        break;
      }
      break;


    case STATE_PULL: // stepper moves one turn to pull syringe.
      Serial.println("case pull");
      buttonPressed = digitalRead(onOffButton);
      if (buttonPressed == true) { //button pressed for the second time to stop pushing
        Serial.println("button pressed again");
        pump_state = STATE_BUTTON_PRESSED;
        break;
      }
    
      if (pullTarget != 0 && stepCounter <= pullTarget){
      if (pumpOn1 == true){
        Stepper1->setSpeed(pullRate * 10); //set step speed based on push rate
        Stepper1->step(2, BACKWARD, SINGLE); //move stepper one step
      }
      if (pumpOn2 == true){
        Stepper2->setSpeed(pullRate * 10); //set step speed based on push rate
        Stepper2->step(2, BACKWARD, SINGLE); //move stepper one step
      
      }
        stepCounter += 1; //count steps 
      }
      
      if (stepCounter >= pullTarget) {
        pump_state = STATE_STOP;
        break;
      }
      break;

    case STATE_STOP:
      Serial.println("case stop");
      stepCounter = 0;
      
//      digitalWrite(LED, LOW);
      delay(10);
      pump_state = STATE_IDLE;
      break;

    case STATE_BUTTON_PRESSED://button was pressed a second time to stop extruding and restart.
      Serial.println("case button pressed");
      stepCounter = 0;//zero the step counter
      buttonPressed = 0;
//      digitalWrite(LED, LOW);
      delay(500);
      pump_state = STATE_IDLE;
      break;
  }
}




