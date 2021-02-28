#include <Servo.h> 

// Define the IO Pins that will be used
const uint8_t switchPin = 4; // GPIO4 aka SDA
const uint8_t servoPin = 5;  // GPIO5 aka SCL

// Define the Servo parameters
Servo myservo;

// Servo limits
const int servoMinPos = 10;
const int servoMaxPos = 138;
const int slamDistance = 20;

// Range to use when in teasing mode
const int teaseMinPos = 90;
const int teaseMaxPos = 120;

// Click constants
const int fastClickThreshold = 2000; //2 seconds
const int clicksForTeaseMode = 4;
const int teaseThreshold = 4000; // 5 seconds after the last click


// Click Status
unsigned long last_click_timestamp = 0;
int fast_clicks = 0;
bool tease_mode = false;



void setup() 
{ 
  Serial.begin(115200);

  myservo.attach(servoPin);
  myservo.write(servoMinPos);
  
  pinMode(switchPin, INPUT);

  randomSeed(analogRead(A0));

  last_click_timestamp = millis();

  Serial.println();
  Serial.println("Setup done");
  Serial.println();
} 

bool move_servo(int end_pos, int step_delay, bool look_for_switch)
{
  int cur_pos = myservo.read();
  
  Serial.print("    Move to ");
  Serial.print(end_pos);
  Serial.print(" from ");
  Serial.print(cur_pos);
  Serial.print(" with delay ");
  Serial.print(step_delay);
  Serial.print(" looking for switch ");
  Serial.println(look_for_switch);

  // Check if we are already there
  if(cur_pos == end_pos)
  {
    Serial.println("    Move Already there");
    return true;
  }

  // Move from current position to the end position  
  int servo_step = cur_pos < end_pos ? 1 : -1;
  int switching_state = (cur_pos >= end_pos);
  Serial.print("    Move step ");
  Serial.print(servo_step);
  Serial.print(" with switching state ");
  Serial.println(switching_state);
  for(; cur_pos != end_pos; cur_pos += servo_step)
  {                                
    myservo.write(cur_pos);
    delay(step_delay);

    // If servo is moving forward - expect for switch off
    // if moving backward - switch on will break the loop
    if(look_for_switch && (digitalRead(switchPin) == switching_state))
    {
      Serial.print("    Move Switch change detected to ");
      Serial.println(digitalRead(switchPin));
      return false;
    }
  } 
  
  Serial.println("    Move Done");
  return true;
}

void tease()
{

  Serial.println("  Teasing!");
  
  move_servo(teaseMinPos, random(4, 6), false);
  delay(200 + random(2000));
  
  do
  {
    Serial.println("  Tease loop!");

    if(digitalRead(switchPin)) // User again clicked the switch
      return;
    
    move_servo(teaseMaxPos, random(2, 6), false);
    delay(random(1000)); 
    move_servo(teaseMinPos, random(4, 6), false);
    delay(200 + random(2000));  
  }
  while(random(5) > 2);

  if(digitalRead(switchPin)) // User again clicked the switch
    return;
  
  Serial.println("  Tease Exiting loop!");
  move_servo(servoMinPos, random(4, 6), false);
  
  // No longer tease
  if(random(5) > 2)
  {
    Serial.println("  Tease No longer!");
    tease_mode = false;
  }
}

void check_for_bothering()
{

  unsigned long cur_timestamp = millis();
  
  // Check whether previous click was not too far ago
  if(cur_timestamp - last_click_timestamp > fastClickThreshold)
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
  if(fast_clicks > clicksForTeaseMode)
  {
    Serial.println("  Check Tease mode");
    tease_mode = true;
  }
}

bool should_tease()
{

  unsigned long cur_timestamp = millis();

  // Check whether there was no action during last few seconds so we may tease
  if(tease_mode && (cur_timestamp - last_click_timestamp > teaseThreshold))
    return true;
    
  return false;
}

void loop() 
{
  // Get a new random speed range each loop for variability
  int speedTurnOff = random(1, 3);
  int speedReturn = random(2, 8);

  if(should_tease())
    tease();

  //TODO Sleep while not turned on
  // Wait for click
  if(digitalRead(switchPin)) {
    
    Serial.println("Switch turned ON");
    Serial.println(digitalRead(switchPin));
    
    check_for_bothering();
  
    // Move forward until switch is off  
    move_servo(servoMaxPos - slamDistance, speedTurnOff, true);
    Serial.println(digitalRead(switchPin));
    myservo.write(servoMaxPos);   // Slam the switch off
    delay(200);
    Serial.println(digitalRead(switchPin));

    Serial.println("Switch turned OFF");
    
    // Now move backward until min pos or user clicked switch again
    move_servo(servoMinPos, speedReturn, true);
    Serial.println("Switch action END");
    Serial.println();
  }
} 
