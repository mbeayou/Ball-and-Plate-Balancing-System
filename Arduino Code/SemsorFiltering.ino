/*
 * Program:
 *  5_Wire_Read_05
 * Created by:
 *  Jonathon Weeks
 * On date:
 *  2023-8-5
 * 
 * Description:
 *  This program is a state machine that waits for a button-press
 *  before reading a coordinate pair from a 5-wire resisitve touch
 *  panel. The coordinate pair is then sent to the computer via
 *  Serial.
 * 
 * Function:
 *  Upon button press:
 *    LED turns on, and a message is sent through Serial, indicating
 *      that the process has started.
 *    X and Y coordinates are read from the panel, with pauses while
 *      the panel's voltages settle.
 *    X and Y coordinates are sent to the computer through Serial.
 *    The panel is turned off.
 *    Time is checked to allow enough time for button debouncing.
 *    The LED turns off, and a message is sent through Serial,
 *      indicating that the process has finished.
 * Plan:
 *  Clean extraneous code
 */

//~~~~~~~~
//~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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


int stableCount = 0;

typedef union{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;
FLOATUNION_t sendX1;
FLOATUNION_t sendY2;
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
        noTouchCount = 0; 
      }
      else{
        noTouchCount++;

        if(noTouchCount > 100) //if there is no ball on plate longer
        {
          break;  
        }

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
      sendX1.number = x_pos;
      sendY2.number = y_pos;

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


    case 6: // Turn off the panel
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

