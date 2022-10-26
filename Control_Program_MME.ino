/*
  Eric Viscione
  University of Massachusetts Lowell, ACTMRL Lab
  Darshil Shah
  Professor Christopher Hansen
  Eric_Viscione@student.uml.edu
  EricJViscione@gmail.com
*/
#include <L298NX2.h>
#include <L298N.h>
//Sensor Pin Definitions
#define HallPin 2       //Pins with  hall effect
#define HallPin2 3
//Relay Pin Definitions
#define Air_1  22         //##define the pin that will have the signal for the air blast relay
#define Air2   23
#define Air3   24         // tertiary air for spreading
#define Alert   27        //Buzzer
//Button Pin Definitions
#define PauseState 44   //Pin for  pause function
#define ReversePinB 46   //pin for reversing the motor direction for emptying pumps for pump B
#define ReversePinA 45  //For reversing Pump A
#define reset_switch 53 //Pin that reset switch is attached to 
#define MFHG_Reset   52 ///will set the current state to the mfhg side 
#define MFHG_Foam_Reset 51 // sets the current state to MFHG Foam 
#define PR48_Reset   50 // will set the current state to Pr48 side
#define PR48_Foam_Reset 49
#define Prime_ResinA 48 //pin for priming a resin pump this one is usually pr48
#define Prime_ResinB 47 //secondary pump
#define Test_Reset 7
// Create one motor instance
//Motor Pin Definitions for the l298n motor controller
const unsigned int EN_A = 8;
const unsigned int IN1_A = 9;
const unsigned int IN2_A = 10;
const unsigned int IN1_B = 11;
const unsigned int IN2_B = 12;
const unsigned int EN_B = 13;
//// Variables Defintions
int Pump_VolDes = 1.25;   //volume of resin dispensed
int motor_speed = 255; //255 is the max speed 0 is min. Motors dont work below 2/3 of 255
int TriggerCount1 = 0;  // Initializes the variable that stores how many times the sensor is triggered
int TriggerCount2 = 0;
int TriggerCount = 0;
int Last_State = 0;    //I still think this var is unessacary but i keep it in to prevent any kind of double detection
int Last_State2 = 0;    //I still think this var is unessacary but i keep it in to prevent any kind of double detection
int Case_State = 0;    //this variable stores which step the printing is left on
int LayerACount = 0;   //keeps track of number of layers for material A usually PR48
int LayerBCount = 0;  //Same as Layer B
int TotLayer = 0;     //Number of total Layers counted(about half of layer a and layer b combined)
int SupportLayers = 0; //numnber of support layers in this certain print. This variable should be changed from 0 if there is support layers n
int disDebug  = 0;   //this will spit out more info if i need it. 1 is debug mode
unsigned long previousMillis = 0;        // will store last time Error was checked was updated
unsigned long previousMillisRelay = 0;        // will store last time Relay was checked was updated
int Alert_State = LOW;
int Alert_Length = 250;
int ErrorCount = 0;
// Initialize  motors
//L298N motorEmpty(ENEmpty, IN1Empty, IN2Empty);
L298NX2 motors(EN_A, IN1_A, IN2_A, EN_B, IN1_B, IN2_B);
//
void setup() {
  // initializing
  pinMode(reset_switch, INPUT_PULLUP);
  pinMode(PR48_Reset, INPUT_PULLUP);
  pinMode(MFHG_Reset, INPUT_PULLUP);
  pinMode(MFHG_Foam_Reset, INPUT_PULLUP);
  pinMode(PR48_Foam_Reset, INPUT_PULLUP);

  pinMode(ReversePinB, INPUT_PULLUP);
  pinMode(ReversePinA, INPUT_PULLUP );
  pinMode(Prime_ResinA, INPUT_PULLUP );
  pinMode(Prime_ResinB, INPUT_PULLUP );
  pinMode(PauseState, INPUT_PULLUP);
  pinMode(Test_Reset, INPUT_PULLUP);
  pinMode(Alert, OUTPUT)   ;
  pinMode(Air3, OUTPUT);
  pinMode(Air_1, OUTPUT);
  pinMode(Air2, OUTPUT);  
  Serial.begin(9600);
  motors.setSpeed(255);       //set the motor speed to max for some reason
}

void loop() {
  motors.setSpeed(motor_speed);     //sets the motor speed again
  PrimeResin();                     //check the prime resin function to see if either button is pressed so we can prime the pump
  ResetCheck();                     //checks the reset button to see if all the layer and state info should be reset to 0
  ReverseCheck();                   //bascally same as prime resin function but its the reverse
  int  Pump_VolAct = Pump_VolDes * 1.33;  //adjustment for the actual volume of resin dispensed
  int  Pump_Time = (Pump_VolAct / .00266);  //calculation for pump time on based on pumps flowrate

  int Air_Time1 = 1200;  //timer for air jets
  int AirMod = 0;
  if (digitalRead(PauseState) == 0) { //pauses the printer
    Serial.println("Paused");
    delay(100);
  }
  else {
    switch (LayerCount()) {
      case 0:
        if (Case_State == 0) {
          Case_State = Case_State + 1;    //increments the state
          LayerACount = LayerACount + 1;  //increments layer counts
          TotLayer = TotLayer + 1;
          Serial.println("Exposing/Parked at window 1, MFHG");
          Serial.print("Number of MFHG layers: ");
          Serial.println(LayerACount);
          Serial.print("Layer Number: ");
          Serial.println(TotLayer);
          Serial.print("Current Print Height: ")  ;
          Serial.println(TotLayer * .025);
          if (TotLayer >= SupportLayers - 1) {                //disables second resin for supports
            FoamClean(disDebug, 1, Air_Time1);
            MotorCleanupA(Pump_Time * 1.5);
          }
          else {
            Serial.print("Support Layer: ");
            Serial.print(TotLayer);
            Serial.print(" / ");
            Serial.println(SupportLayers);
          }
          if (disDebug == 1) {
            Serial.println(Case_State);
          }
          Serial.println();
        }
        break;
      case 1:
        if (Case_State == 1) {
          Serial.println("Seperating From MFHG");
          Serial.println();
          digitalWrite(Air3, HIGH); //turns on air jets 
          digitalWrite(Air2, HIGH);
          Case_State = Case_State + 1;
        }
        break;
      case 2:
        //Maybe dispese some air
        if (Case_State == 2) {
          digitalWrite(Air3, LOW);  //turn off this air jet
          Serial.println("Lifting at Tray 1");
          Serial.println();
          Case_State = Case_State + 1;
        }
        break;
      case 3:
        if (Case_State == 3) {
          delay(750);
          digitalWrite(Air2, LOW); //leave this air jet on for spreading of resin
          Serial.println("Approaching MFHG Foam Clean");
          Serial.println();
          Case_State = Case_State + 1;
        }
        break;
      case 4:
        if (Case_State == 4) {
          Serial.println("Foam Press Layer MFHG");
          Serial.println("Being Cleaned of MFHG");
          Serial.println();
          Case_State = Case_State + 1;
        }
        break;
      case 5:
        if (Case_State == 5) {
          Serial.println("Approaching Window PR48");
          Serial.println();
          delay(250);
          digitalWrite(Air3, HIGH);  //spreads resin
          delay(100);
          digitalWrite(Air3, LOW);
          Case_State = Case_State + 1;
          //Maybe dispense air
        }
        break;
      case 6:
        if (Case_State == 6) {  //the steps repeat again
          FoamClean(disDebug, 1, Air_Time1);
          MotorCleanupB(Pump_Time * 3);
          motors.stop();
          LayerBCount = LayerBCount + 1;
          Serial.println("Exposing/Parked at window PR48");
          Serial.print("Number of PR48 layers: ");
          Serial.println(LayerBCount);
          Serial.println();
          Case_State = Case_State + 1;
        }
        break;
      case 7:
        if (Case_State == 7) {
          Serial.println("Seperating at Window PR48");
          Serial.println();
          digitalWrite(Air3, HIGH);
          digitalWrite(Air2, HIGH);
          Case_State = Case_State + 1;
        }
        break;
      case 8:
        if (Case_State == 8) {
          Serial.println("Lifting at Tray 2");
          Serial.println();
          digitalWrite(Air3, LOW);
          Case_State = Case_State + 1;
        }
        break;
      case 9:
        if (Case_State == 9) {
          delay(1000);
          digitalWrite(Air2, LOW);
          Serial.println("Approaching Foam Press 2");
          Serial.println();
          Case_State = Case_State + 1;
        }
        break;
      case 10:
        if (Case_State == 10) {
          Serial.println("Pressing at Foam 2");
          Serial.println();
          Case_State = Case_State + 1;
        }
        break;     
      case 11:
        //if (Case_State == 11) {
          Serial.println("Approaching Window MFHG");
          Serial.println();
          Case_State = 0; //Needs tobe reset at the end of this sequence

        break;
    }
  }
}
int LayerCount()
{
  int State = digitalRead(HallPin);  //read the pin that the hall effect sensor is connected to
  int State2 = digitalRead(HallPin2);
  if (State != Last_State) {          //With the statea variable and last_state i check whether or not the sensor has already been triggered/updated
    if (disDebug == 1) {
      Serial.print("TriggerCount1: ");
      Serial.println(TriggerCount);
    }
    TriggerCount = TriggerCount + 1;
    Last_State = State;
  }
  if (State2 != Last_State2) {          //With the statea variable and last_state i check whether or not the sensor has already been triggered/updated
    if (disDebug == 1) {
      Serial.print("TriggerCount2: ");
      Serial.println(TriggerCount2);
    }
    TriggerCount2 = TriggerCount2 + 1;
    Last_State2 = State2;
  }
  if (TriggerCount != TriggerCount2)
  {
      Serial.println("ERROR");  
  }
  else {
    int Step = TriggerCount % 12;        //if you divide the trigger count by 12 and check the remainder you get a value 0-11 and those correspond to each step in the printing process
    if (disDebug == 1) {
      Serial.print("Current Step: ");
      Serial.println(Step);
    }
    ErrorCount = 0;
    return (Step);
  }
}
//returns the step back from this function
void MotorCleanupA(int Pump_Time)     // this function controls motor A pumping
{
  motors.forwardA();            //these are functions from the l298xn library
  delay(Pump_Time);
  motors.stop();
}
int MotorCleanupB(int Pump_Time)   // this function controls motor B pumping
{
  motors.forwardB();            //these are functions from the l298xn library
  // motorEmpty.forward();
  delay(Pump_Time);
  motors.stop();
  //motorEmpty.stop();
}
void FoamClean(int disDebug, int Select, int Time) {                    //when this is used it dispenses a bit of air
  if (Select == 1) {
    digitalWrite(Air_1, HIGH);
    delay(Time);
    digitalWrite(Air_1, LOW);
  }
  if (Select == 2) {
    digitalWrite(Air2 , HIGH);
    delay(Time);
    digitalWrite(Air2 , LOW);
  }
  if (disDebug == 1) {
    Serial.println("air dispensed");
  }
}
void PrimeResin() {
  int Pump;
  int  A = digitalRead(Prime_ResinA);  //read the buttons
  int B = digitalRead(Prime_ResinB);
  if (A == 0 && B == 1) {  //weird check i did to control each motor.
    Pump = 0;
  }
  if (A == 1 && B  == 0) {
    Pump = 1;
  }
  if (A == 1 && B == 1) {
    Pump = 2;
  }
  motors.setSpeed(255);
  switch (Pump) { //pumps resin based on l298n library
    case 0:  
      motors.forwardA();
      delay(50);
      motors.stop();
      Serial.println("Priming Pump A");
      break;
    case 1:
      motors.forwardB();
      //motorEmpty.forward();
      delay(50);
      motors.stop();
      //motorEmpty.stop();
      Serial.println("Priming Pump B");
    case 2:
      motors.stop();
      //motorEmpty.stop();
  }
  motors.setSpeed(255);
}
int ResetCheck()
{
  int Reset_Check =  digitalRead(reset_switch); //read status of the reset pin
  if (digitalRead(MFHG_Reset) == 0) {   //all of these check to see if the program should be reset to a new location for debugging purposes
    Case_State = 0;
    TriggerCount  = 0;
    TriggerCount2 = 0   ;
    Serial.println("The printer is exposing MFHG currently")  ;
  }
  if (digitalRead(MFHG_Foam_Reset) == 0) {
    Case_State = 4;
    TriggerCount  = 4;
    TriggerCount2 = 4;

    Serial.println("The printer is Cleaning From MFHG currently")  ;
    delay(120);
  }
  if (digitalRead(PR48_Reset) == 0) {
    Case_State = 6;
    TriggerCount  = 6;
    TriggerCount2 = 6;

    Serial.println("The printer is exposing PR48 currently");
  }
  if (digitalRead(PR48_Foam_Reset) == 0) {
    Case_State = 10;
    TriggerCount  = 10;
    TriggerCount2 = 10  ;

    Serial.println("The printer is Cleaning from PR48 currently")  ;
    delay(50);

  }
  if(digitalRead(Test_Reset) == 0){
    Case_State = 1;
    TriggerCount = 1;
    TriggerCount2 = 1;
    Serial.println("Test Button Case 1 Currently")  ;
    delay(50); 

  }
  if (Reset_Check == 0) {            //If the reset pin is pulled high(Switched on) reset the layer count
    TriggerCount = 0;
    TriggerCount2 = 0 ;

    Case_State = 0; ///resets the variable that prevents looping of each position

    Serial.println("The Layer count has been reset: ");
    // Serial.println(TriggerCount);
    LayerACount = 0;
    LayerBCount = 0;
    TotLayer = 0;
    digitalWrite(Air2, LOW);
    return (LayerACount, LayerBCount, TotLayer)  ;
  }
}
void Pause() {
  while (digitalRead(PauseState) == 0) {
    Serial.println("Paused");
    delay(100);
  }
}
void ReverseCheck() {  //nearly the same as the prime resin functions but reverses resin out
  int Pump;
  int  A = digitalRead(ReversePinA);
  int B = digitalRead(ReversePinB);
  if (A == 0 && B == 1) {   //these have been changed to use internal pullups
    Pump = 0;
  }
  if (A == 1 && B  == 0) {
    Pump = 1;
  }
  if (A == 1 && B == 1) {
    Pump = 2;
  }
  motors.setSpeed(255);
  switch (Pump) {
    case 0:
      motors.backwardA();
      delay(50);
      motors.stop();
      Serial.println("Unloading Pump 1");
      break;
    case 1:
      motors.backwardB();
      delay(50);
      motors.stop();
      Serial.println("Unloading Pump 2");
    case 2:
      motors.stop();
  }
  motors.setSpeed(motor_speed);
}
