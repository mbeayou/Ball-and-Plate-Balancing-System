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
 *  Clean comments and extraneous code
 */

//~~~~~~~~
//~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LIBRARIES
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <Servo.h>
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


int Ts = 15;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Touch panel variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define PIN_TR 3
#define PIN_TL 2
#define PIN_S  A1
#define PIN_SD 6
#define PIN_BL 5
#define PIN_BR 4
#define SETTLE_TIME 5


float x_pos, y_pos;
float pre_pos = 810;
unsigned long panel_time;
int standByValue = 0 ;
unsigned int noTouchCount = 0;
char header;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Caliboration
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
float Caliboration_Xscale = 800/34;
float Caliboration_Yscale = 800/28;
float Xoffset = 550;
float Yoffset = 550;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Servo.h Instllation 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Servo myservoX;
Servo myservoY;
double angleX;
double angleY;
double flatAngleX = 72;
double flatAngleY = 75;
//int outputX = flatAngleX;
//int outputY = flatAngleY;
#define PIN_X 9
#define PIN_Y 8

int stableCount = 0;

typedef union{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t sendX1;
FLOATUNION_t sendY2;

FLOATUNION_t xValue;
FLOATUNION_t yValue;

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

  xValue.number = flatAngleX;
  yValue.number = flatAngleY;
  angleX = flatAngleX;
  angleY = flatAngleY;

  myservoX.write(flatAngleX);
  myservoY.write(flatAngleY);



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

      if(standByValue < 800){
        state = 1;
        myservoX.attach(PIN_X);
        myservoY.attach(PIN_Y);
        noTouchCount = 0; 
      }
      else{
        noTouchCount++;
        myservoX.write(angleX); 
        myservoY.write(angleY); 
        if(noTouchCount > 100) //if there is no ball on plate longer
        {
          myservoX.write(flatAngleX); 
          myservoY.write(flatAngleY); 
          delay(1000);   
          myservoX.detach(); //detach servos
          myservoY.detach();   
          break;          }

      }
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
    

    case 5: // packeting & sending coordinates via serial communication
    
      //----------------------------------
      //x_pos = (x_pos - Xoffset) / Caliboration_Xscale;
      //y_pos = (y_pos - Yoffset) / Caliboration_Yscale;
      //----------------------------------
      sendX1.number = x_pos;
      sendY2.number = y_pos;
      if( x_pos > 0 && y_pos > 0){

        Serial.print("A");

        for (int i=0; i<4; i++){
          Serial.write(sendX1.bytes[i]); 
        }

        for (int i=0; i<4; i++){
          Serial.write(sendY2.bytes[i]); 
        }

        Serial.print("\n");
        delay(50);

        state = 6;
        //--------------------------------
        break;
      }
      state = 0;
      break;


    case 6: // recieving servo angles
      while(Serial.available()>8){
        Serial.read();
      }
      if (Serial.available() == 8) {
        xValue.number = getFloat();
        yValue.number = getFloat();
      } 


      if(xValue.number < 170 && yValue.number < 170 && xValue.number > 10 && yValue.number > 10 ){
        angleX = xValue.number;
        angleY = yValue.number;
      }
        myservoX.write(angleX);
        myservoY.write(angleY);

      state = 7;
      break;
    
    
    case 7: // Turn off the panel
      // Turn off the panel
      digitalWrite(PIN_TR, LOW);
      digitalWrite(PIN_TL, LOW);
      digitalWrite(PIN_BL, LOW);
      digitalWrite(PIN_BR, LOW);

      // Switch to button debounce state
      state = 0;
      break;
      
    default: // Error
      Serial.println("Something has gone terribly wrong.");
      break;
    }
  }

float getFloat(){
    int cont = 0;
    FLOATUNION_t f;
    while (cont < 4 ){
        f.bytes[cont] = Serial.read() ;
        cont = cont +1;
    } 
    return f.number;
}


