/*
 * Program:
 *  5_Wire_Read_servomotor_write_PID_calculate
 * Created by:
 *  Mohammed S. baayou
 * On date:
 *  2023-8-5
 * 
 * Description: This program controls a ball and plate system using a 5-wire touch panel,
 *              servo motors, and PID control to maintain the ball's position at the center.
 * Function:
 *      1. Detects ball placement on the plate using a 5-wire resistive touch panel.
 *      2. Reads ball position coordinates using a state machine approach.
 *      3. Filters sensor readings to reduce noise.
 *      4. Calibrates readings for accurate position determination.
 *      5. Implements PID control to calculate precise servo adjustments.
 *      6. Controls servo motors to tilt the plate and keep the ball balanced.
 *      7. Transitions between states to manage system behavior.
 *      8. Enters standby mode when the ball is not present to conserve energy.
 * Plan:
 *  Clean extraneous code
 */

//~~~~~~~~
//~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LIBRARIES
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <Filters.h>
#include <Servo.h>
#include <PID_v1.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int temp;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// State variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int state;
unsigned long state_time;
#define STATE_STANDBY 0
#define STATE_START 1


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PID Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double SetpointX, InputX, OutputX;
double SetpointY, InputY, OutputY;

float xKp = 4.4;
float xKi = 0.01;
float xKd = 0.8;

//stable
//float yKp = 2.5;
//float yKi = 0.005;
//float yKd = 1.260;
float yKp = 4.2;
float yKi = 0.005;
float yKd = 0.80;

int Ts = 15;

PID myPIDX(&InputX, &OutputX, &SetpointX, xKp, xKi, xKd,
DIRECT);
PID myPIDY(&InputY, &OutputY, &SetpointY, yKp, yKi, yKd,
DIRECT);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Touch panel variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define PIN_TR 2
#define PIN_TL 1
#define PIN_S  A1
#define PIN_SD 5
#define PIN_BL 4
#define PIN_BR 3
#define SETTLE_TIME 5


float x_pos, y_pos;
float pre_pos = 810;
unsigned long panel_time;
int standByValue = 0 ;
unsigned int noTouchCount = 0;

//Caliboration
float Caliboration_Xscale = 800/34;
float Caliboration_Yscale = 800/34;
float Xoffset = 550;
float Yoffset = 550;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// low pass filters variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 float filterFrequencyX= 5;
 float filterFrequencyY= 4;
 int InputXfiltered, InputYfiltered;

 // Initializing a one pole (RC) lowpass filter
 FilterOnePole lowpassFilterX( LOWPASS, filterFrequencyX );
 FilterOnePole lowpassFilterY( LOWPASS, filterFrequencyY );


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Servo.h Instllation 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Servo myservoX;
Servo myservoY;

double angleX;
double angleY;

double flatAngleX = 72;
double flatAngleY = 75;

int stableCount = 0;

#define PIN_X 9
#define PIN_Y 8

void setup()
{ 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Establish pin modes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Touch panel
  pinMode(PIN_TR, OUTPUT);
  pinMode(PIN_TL, OUTPUT);
  pinMode(PIN_BL, OUTPUT);
  pinMode(PIN_BR, OUTPUT);
  digitalWrite(PIN_TR, LOW);
  digitalWrite(PIN_TL, LOW);
  digitalWrite(PIN_BL, LOW);
  digitalWrite(PIN_BR, LOW);
  pinMode(PIN_S, INPUT);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize variables
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // State Machine
  state = 0;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Servo Motors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Attach Servo motors to pins
  myservoX.attach(PIN_X);
  myservoY.attach(PIN_Y);

  OutputX = flatAngleX;
  OutputY = flatAngleY;

  myservoX.write(OutputX);
  myservoY.write(OutputY);

// Setting starting parameters for PID
  myPIDX.SetMode(AUTOMATIC);
  myPIDY.SetMode(AUTOMATIC);
  InputX = 0;
  InputY = 0;

  SetpointX = 0.00;
  SetpointY = 0.00;

  // Setting limiting parameters for servo motors
  myPIDX.SetOutputLimits(flatAngleX-85, flatAngleX+85);
  myPIDY.SetOutputLimits(flatAngleY-85, flatAngleY+85);

  myPIDX.SetSampleTime(Ts);
  myPIDY.SetSampleTime(Ts);

  angleX = flatAngleX;
  angleY = flatAngleY;

  



  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Begin Serial Communication
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Serial.begin(9600);
}

void loop()
{
  switch(state)
  {
    case 0: //standby mode
      pinMode(PIN_TR, OUTPUT);
      pinMode(PIN_TL, OUTPUT);
      pinMode(PIN_BL, OUTPUT);
      pinMode(PIN_BR, OUTPUT);
      digitalWrite(PIN_TR, LOW);
      digitalWrite(PIN_TL, LOW);
      digitalWrite(PIN_BL, LOW);
      digitalWrite(PIN_BR, LOW);

      pinMode(PIN_S, INPUT_PULLUP );
      delay(10);

      standByValue = analogRead(PIN_S);
      //Serial.println(noTouchCount);
      //Serial.print("\t");
      if(standByValue < 800){
        state = 1;
        myservoX.attach(PIN_X);
        myservoY.attach(PIN_Y);
        noTouchCount = 0; 
      }
      else{
        noTouchCount++;

          myservoX.write(OutputX); 
          myservoY.write(OutputY); 
        if(noTouchCount > 100) //if there is no ball on plate longer
        {

          OutputX=flatAngleX; //make plate flat
          OutputY=flatAngleY;
          myservoX.write(OutputX); 
          myservoY.write(OutputY); 
          delay(1000);   
          myservoX.detach(); //detach servos
          myservoY.detach();   
          break;  
        }

      }
      //state=1;
      break;


    case 1: // Set up X read mode
      //~~~~~~~~~~~~~~~~
      // Set up X read
      //~~~~~~~~~~~~~~~~
      // Set TR and BR high
      digitalWrite(PIN_TR, LOW);
      digitalWrite(PIN_BR, LOW);
      // Set TL and BL low
      digitalWrite(PIN_TL, HIGH);
      digitalWrite(PIN_BL, HIGH);

      //~~~~~~~~~~~~~~~~
      // Move to next state
      //~~~~~~~~~~~~~~~~
      // Record the time
      panel_time = millis();
      // Switch to waiting for the panel to settle
      state = 2;
      break;
    
    case 2: // Reading X position mode
      // How long has it been since the panel changed configuration?
      temp = millis() - panel_time;
      // Has it been long enough?
      if (temp >= SETTLE_TIME)
      { // If it has...
        pre_pos = x_pos; 
        x_pos = analogRead(PIN_S);
        //filtering no touch spikes
        if (x_pos > 900){
          x_pos = pre_pos;
        }
        // Switch to the Read X state
        state = 3;
      } // Otherwise, do nothing.
      break;

    case 3: // Set up Y read mode
      //~~~~~~~~~~~~~~~~
      // Set up Y read
      //~~~~~~~~~~~~~~~~
      // Set TR and TL high
      digitalWrite(PIN_TR, HIGH);
      digitalWrite(PIN_TL, HIGH);
      // Set BL and BR low
      digitalWrite(PIN_BL, LOW);
      digitalWrite(PIN_BR, LOW);

      //~~~~~~~~~~~~~~~~
      // Move to next state
      //~~~~~~~~~~~~~~~~
      // Record the time
      panel_time = millis();
      // Switch to waiting for the panel to settle
      state = 4;

      break;
      
    case 4: // Reading Y position mode
      // How long has it been since the panel changed configuration?
      temp = millis() - panel_time;
      
      // Has it been long enough?
      if (temp >= SETTLE_TIME)
      { // If it has...
        // Read the sensor voltage (Y position)
        pre_pos = y_pos; 
        y_pos = analogRead(PIN_S);
        if (y_pos > 800){
          y_pos = pre_pos;
        }
          state = 5;
      } // Otherwise, do nothing.
      break;
    

    case 5: // testing & filtering coordinates mode
      //----------------------------------
      //Serial.print(x_pos);
      //Serial.print("\t");
      //Serial.print(y_pos);
      //Serial.print("\t");
      
      // Filtering touchpanel signal
      x_pos = lowpassFilterX.input(x_pos);
      y_pos = lowpassFilterY.input(y_pos);

      // Write the coordinates to Serial for debuging
      //Serial.print("filtered coordinates  ");
      //Serial.print(x_pos);
      //Serial.print("\t");
      //Serial.print(y_pos);
      //Serial.print("\n");

      state = 6;
      //--------------------------------
      break;

    case 6: // Caliboration & checking if the ball in the center
      InputX = (x_pos - Xoffset) / Caliboration_Xscale;
      InputY = (y_pos - Yoffset) / Caliboration_Yscale;
      //Serial.print("Caliborated position  ");
      Serial.print("\t");
      Serial.print(InputX);
      Serial.print("\t");
      Serial.print(InputY);
      //Serial.print("\n");

      if(InputX <= SetpointX + 0.85 && InputX >= SetpointX - 0.85 && InputY <= SetpointY + 1.25 && InputY >= SetpointY - 1.25 ){
        stableCount++;
      }
      else{
        stableCount =- 2;
      }
        
      if(stableCount > 150){
        myservoX.write(flatAngleX); 
        myservoY.write(flatAngleY);
        //delay(1000);
        state = 8;
        break;
      }
      state = 7;
      break;

    case 7: // setting PID outputs


      myPIDX.Compute();  //action control X compute
      myPIDY.Compute();  //   action control  Y compute  

      myservoX.write(OutputX); 
      myservoY.write(OutputY);

      //Serial.print("servos angle  ");
      //Serial.print("\t");
      //Serial.print(OutputX);
      //Serial.print("\t");
      //Serial.print(OutputY);
      Serial.println("\n");

      state = 8;
      break;


    case 8: // Turn off the panel
      // Turn off the panel
      digitalWrite(PIN_TR, LOW);
      digitalWrite(PIN_TL, LOW);
      digitalWrite(PIN_BL, LOW);
      digitalWrite(PIN_BR, LOW);

      // Switch to button debounce state
      state = 9;
      break;


    case 9: // End state
      
      // Send message through Serial
      //Serial.println("Panel read finished");
      delay(20);

      // Switch to the waiting state
      state = 0;
      break;
      
    default: // Error
      Serial.println("Something has gone terribly wrong.");
      break;
    }
  }

