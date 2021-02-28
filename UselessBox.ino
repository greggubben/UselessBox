/*
 * This code is adapted from grafalex
 * https://www.thingiverse.com/thing:1258082
 * 
 * Box printed from designs above and modified by Xrew
 * https://www.thingiverse.com/thing:2847024
 * 
 * Adapted the code and design for a LOLIN/WEMOS D1 Mini Pro
 * so I can take advantage of the LiPo Battery Manager and
 * the built in I2C connector. No soldering to the board required.
 * 
 * 
 */

#include <Servo.h> 

// Define the IO Pins that will be used
// I used the 4 pin connector provided on the LOLIN D1 Mini Pro:
const uint8_t SwitchPin = 4; // GPIO4 aka SDA
const uint8_t ServoPin = 5;  // GPIO5 aka SCL

// Define the Servo parameters
Servo turnSwitchOffServo;

// Servo limits
const int ServoMinPos = 10;
const int ServoMaxPos = 138;
const int SlamDistance = 20;

// Range to use when in teasing mode
const int TeaseMinPos = 90;
const int TeaseMaxPos = 120;

// Tease constants
const int FastClickThreshold = 2000; //2 seconds
const int ClicksForTeaseMode = 4;
const int TeaseThreshold = 4000; // 5 seconds after the last click

// Wakeup constants
const unsigned long WakeupMinMillis = 40 * 60 * 1000;   // Wake up Min 40 minutes
const unsigned long WakeupMaxMillis = 55 * 60 * 1000;   // Wake up Max 55 minutes
unsigned long wakeup_time = WakeupMinMillis;            // Start with the Min


// Click Status
unsigned long last_click_timestamp = 0;
int fast_clicks = 0;
bool tease_mode = false;



void setup() 
{ 
  Serial.begin(115200);

//  turnSwitchOffServo.attach(ServoPin, ServoMinPos, ServoMaxPos);
  turnSwitchOffServo.attach(ServoPin);
  turnSwitchOffServo.write(ServoMinPos);
  
  pinMode(SwitchPin, INPUT);

  randomSeed(analogRead(A0));

  last_click_timestamp = millis();

  Serial.println();
  Serial.println("Setup done");
  Serial.println();
} 


void loop() 
{
  // Get a new random speed range each loop for variability
  int speedTurnOff = random(1, 3);
  int speedReturn = random(2, 8);

  if(should_tease()) {
    tease();
  }

  //TODO Sleep while not turned on
  // Wait for click
  if(digitalRead(SwitchPin)) {
    
    Serial.println("Switch turned ON");
    
    check_for_bothering();
  
    // Move forward until switch is off
    turnOffSwitch(speedTurnOff);
    Serial.println("Switch turned OFF");
    
    // Now move backward until min pos or user clicked switch again
    servoHome(speedReturn, true);
    Serial.println("Switch action END");
    Serial.println();
  }
} 


/**************************
 * Tease Functions
 **************************/

bool should_tease()
{

  unsigned long cur_timestamp = millis();

  if (cur_timestamp - last_click_timestamp > wakeup_time) {
    Serial.println("  Wake Up!");
    wakeup_time = random(WakeupMinMillis, WakeupMaxMillis);
//    tease_mode = true;
    last_click_timestamp = cur_timestamp;
    return true;
  }

  // Check whether there was no action during last few seconds so we may tease
  if(tease_mode && (cur_timestamp - last_click_timestamp > TeaseThreshold)) {
    return true;
  }

  return false;
}

void tease()
{

  Serial.println("  Teasing!");
  
  move_servo(TeaseMinPos, random(4, 6), false);
  delay(200 + random(2000));

  int taunts = random(2,6);
  Serial.print("Taunts ");
  Serial.println(taunts);
  for (int t=0; t<taunts; t++) {
    Serial.print("  Tease loop! ");
    Serial.println(t);

    if(digitalRead(SwitchPin)) // User again clicked the switch
      return;
    
    move_servo(TeaseMaxPos, random(2, 6), false);
    delay(random(1000)); 
    move_servo(TeaseMinPos, random(4, 6), false);
    delay(200 + random(2000));  
  }

  if(digitalRead(SwitchPin)) // User again clicked the switch
    return;
  
  Serial.println("  Tease Exiting loop!");
  servoHome(random(4, 6));
  
  // No longer tease
  tease_mode = false;
}



/**************************
 * Switch Functions
 **************************/

void check_for_bothering()
{

  unsigned long cur_timestamp = millis();
  
  // Check whether previous click was not too far ago
  if(cur_timestamp - last_click_timestamp > FastClickThreshold)
  {
    // If last click was to far back (normal user) - reset the clicks counter
    Serial.println("  Check Slow Click!");
    fast_clicks = 0;
  }
  else
  {
    // Our user is nerd of clicking
    Serial.println("  Check Fast Click!");
    fast_clicks++;
  }
  
  last_click_timestamp = cur_timestamp;
  
  // If user bother us with clicking - enter tease mode
  if(fast_clicks > ClicksForTeaseMode)
  {
    Serial.println("  Check Tease mode");
    tease_mode = true;
  }
}


/**************************
 * Servo Functions
 **************************/

// Turn the switch off and add a little slam at the end to make sure
bool turnOffSwitch(int step_delay) {
    move_servo(ServoMaxPos - SlamDistance, step_delay, true);
    turnSwitchOffServo.write(ServoMaxPos);   // Slam the switch off
    delay(100);
}

// Move servo to home position
void servoHome(int step_delay) {
  servoHome(step_delay, false);
}
void servoHome(int step_delay, bool look_for_switch) {
  move_servo(ServoMinPos, step_delay, look_for_switch);
}


// Move the servo 
bool move_servo(int end_pos, int step_delay, bool look_for_switch)
{
  int cur_pos = turnSwitchOffServo.read();

  /*
  Serial.print("    Move to ");
  Serial.print(end_pos);
  Serial.print(" from ");
  Serial.print(cur_pos);
  Serial.print(" with delay ");
  Serial.print(step_delay);
  Serial.print(" looking for switch ");
  Serial.println(look_for_switch);
  */

  // Check if we are already there
  if(cur_pos == end_pos)
  {
    Serial.println("    Move Already there");
    return true;
  }

  // Move from current position to the end position  
  int servo_step = cur_pos < end_pos ? 1 : -1;
  int switching_state = (cur_pos >= end_pos);
  //Serial.print("    Move step ");
  //Serial.print(servo_step);
  //Serial.print(" with switching state ");
  //Serial.println(switching_state);
  
  for(; cur_pos != end_pos; cur_pos += servo_step)
  {                                
    turnSwitchOffServo.write(cur_pos);
    delay(step_delay);

    // If servo is moving forward - expect for switch off
    // if moving backward - switch on will break the loop
    if(look_for_switch && (digitalRead(SwitchPin) == switching_state))
    {
      //Serial.print("    Move Switch change detected to ");
      //Serial.println(digitalRead(SwitchPin));
      return false;
    }
  } 
  
  //Serial.println("    Move Done");
  return true;
}

 
