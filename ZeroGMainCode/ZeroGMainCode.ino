//libraries
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <AccelStepper.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

//CS for SD shield SPI
const int chipSelect = 10;
String dataString = "";
File dataFile;

//control pins for pump 1
const int pot11 = A0;
const int pot12 = A1;
const int pot13 = A2;
const int pot14 = A3;
const int switch1 = 2;

//green on/off button
const int onOffButton = 3;

//control pins for pump 2 (i think A4 and A5 are used for i2c for the shields)
const int pot21 = A6;
const int pot22 = A7;
const int pot23 = A8;
const int pot24 = A9;
const int switch2 = 4;

//control values for pump 1
int pushRate1 = 0;
int pushTarget1 = 0;
int pullRate1 = 0;
int pullTarget1 = 0;
bool pumpOn1 = 0;

//control values for pump 2
int pushRate2 = 0;
int pushTarget2 = 0;
int pullRate2 = 0;
int pullTarget2 = 0;
bool pumpOn2 = 0;

//green indicator led
const int LED = 5;

//variables to hold information about the pumps
bool buttonPressed = 0;
int stepCounter1 = 0;
int stepCounter2 = 0;

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
  pinMode(pot11, INPUT);
  pinMode(pot12, INPUT);
  pinMode(pot13, INPUT);
  pinMode(pot14, INPUT);
  pinMode(switch1, INPUT);
  //on off button for the whole system
  pinMode(onOffButton, INPUT);
  //initate pots and switch for pump 2
  pinMode(pot21, INPUT);
  pinMode(pot22, INPUT);
  pinMode(pot23, INPUT);
  pinMode(pot24, INPUT);
  pinMode(switch2, INPUT);

//set up CS as output
  //pinMode(SS, OUTPUT);
pinMode(10, OUTPUT);

  //initate indication LED light
  pinMode(LED, OUTPUT);

  //setup the sd card
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  // Open up the file we're going to log to!
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog.txt");
    // Wait forever since we cant write data
    while (1) ;
  }
  dataFile.println(pushRate1, pushTarget1, pullRate1, pullTarget1, pushRate2, pushTarget2, pullRate2, pullTarget2, pumpOn1, pumpOn2);
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
      pushRate1 = map(analogRead(pot11), 0, 1023, 0, 10); //pushing speed
      Serial.println(pushRate1);
      pushTarget1 = map(analogRead(pot12), 0, 1023, 0, 1000);//pushing target
      Serial.println(pushTarget1);
      pullRate1 = map(analogRead(pot13), 0, 1023, 0, 10);//pulling speed
      Serial.println(pullRate1);
      pullTarget1 = map(analogRead(pot14), 0, 1023, 0, 1000 ); //pulling target
      Serial.println(pullTarget1);
      pumpOn1 = digitalRead(switch1);//switch checking if pump is on

      //set values for pump 2
      pushRate2 = map(analogRead(pot21), 0, 1023, 0, 10); //pushing speed
      Serial.println(pushRate2);
      pushTarget2 = map(analogRead(pot22), 0, 1023, 0, 1000);//pushing target
      Serial.println(pushTarget2);
      pullRate2 = map(analogRead(pot23), 0, 1023, 0, 10);//pulling speed
      Serial.println(pullRate2);
      pullTarget2 = map(analogRead(pot24), 0, 1023, 0, 1000 ); //pulling target
      Serial.println(pullTarget2);
      pumpOn2 = digitalRead(switch2);

      // read pots and append to the string to save
      for (int potPin = 0; potPin < 8; potPin++) {
        int potReading = analogRead(potPin);
        dataString += String(potReading);
        if (potPin < 7) {
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
      dataFile.println(dataString);
      dataFile.flush();

      if (pumpOn1 == true && pushRate1 > 0 || pumpOn2 == true && pullRate2 > 0) { 
        digitalWrite(LED, HIGH);//indicate pump is activity
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
      if (pushTarget1 != 0 ) {
        Stepper1->setSpeed(pushRate1 * 10); //set step speed based on push rate1
        Stepper1->step(2, FORWARD, SINGLE); //move stepper one step in desired speed
        stepCounter1 += 1; //count steps for pump 1
      }
      if (pushTarget2 != 0 ) {
        Stepper2->setSpeed(pushRate2 * 10); //set step speed based on push rate2
        Stepper2->step(2, FORWARD, SINGLE); //move stepper one step in desired speed
        stepCounter2 += 1; //count steps for pump 2

      }
      if (stepCounter1 >= pushTarget1 && stepCounter2 >= pushTarget2) {
        stepCounter1 = 0;//zero the step counter towards pulling phase
        stepCounter2 = 0;
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
      //move stepper1 one step in desired speed
      if (pullTarget1 != 0 && stepCounter1 >= pullTarget1 && pumpOn1 == true) {
        Stepper1->setSpeed(pullRate1 * 10); //set step speed based on push rate
        Stepper1->step(2, BACKWARD, SINGLE); //move stepper one step
        stepCounter1+=1; //count steps for pump 2
      }
      //move stepper2 one step in desired speed
      if (pullTarget2 != 0 && stepCounter2 >= pullTarget2 && pumpOn2 == true) {
        Stepper1->setSpeed(pullRate2 * 10); //set step speed based on push rate
        Stepper1->step(2, BACKWARD, SINGLE); //move stepper one step
        stepCounter2 += 1; //count steps for pump 2
      }

      if (stepCounter1 >= pullTarget1 && stepCounter2 >= pullTarget2) {
        pump_state = STATE_STOP;
        break;
      }
      break;

    case STATE_STOP:
      Serial.println("case stop");
      stepCounter1 = 0;
      stepCounter2 = 0;
      
      digitalWrite(LED, LOW);
      delay(10);
      pump_state = STATE_IDLE;
      break;

    case STATE_BUTTON_PRESSED://button was pressed a second time to stop extruding and restart.
      Serial.println("case button pressed");
      stepCounter1 = 0;//zero the step counter
      stepCounter2 = 0;
      buttonPressed = 0;
      digitalWrite(LED, LOW);
      delay(500);
      pump_state = STATE_IDLE;
      break;
  }
}





