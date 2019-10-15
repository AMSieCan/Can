/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "application.h"
#line 1 "c:/CodeDevelopmentFolder/Can/Firmware/src/AMSieCan.ino"
/*
 * Project AMSieCan
 * Description: Program for monitoring the volume and quantity of debris collected through the apparatus and sending the sampled sensor data to the cloud database
 * Author: Daniel Howe
 * Date: 2019-08-26
 * Updates: 2019-09-19 
 *            - Cleaned up reporting logic
 *            - Improved sampling of counter
 *            -- Debounce added in case trash is inserted for long periods of time or user has slow hands
 *            - Tamper detection added
 * Updates: 2019-10-15
 *            - Added logic for 5V power regulator to enable/disable distance sensor
 */

//STATIC DEFINITIONS
// -----------------
// -- SAMPLING LOGIC
void setup();
void loop();
void logData(double p_distanceInches, int p_count, int p_tilt);
#line 18 "c:/CodeDevelopmentFolder/Can/Firmware/src/AMSieCan.ino"
const int SAMPLE_RATE_MINUTES = 1; // Minutes between saving data and sending to cloud
unsigned long nextLog; // Millisecond time that log should begin after

// -- HARDWARE
// 5 Volt power regulator enable/disable
const int V5_EXT = D4;

// Pin assignments for the HC-SR04 sensor
const int ECHO_PIN = D6;
const int TRIGGER_PIN = D2;

// Pin assignments and configurations for the tipping sensor
const int COUNTER_INT = D0;
int counter = 0; // initialize the start value
long lastCounterTime = millis(); // initialize the counter position to register state changes
unsigned long debounceCount = 1200000; // 120S wait before count gets incremented again, also note: data type is set as unsigned long, inst. int, to mitigate comp errors in compiler

//Pin assignments for the tamper mechanism
const int TAMPER_PIN = A1;
const float refVoltageLimit = 3300;
int tilt = 0;

// Function prototypes
double hcDist();
void logData(double p_distanceInches, int p_count);
int readCounter(int p_counterInt);
void enable5VPower(int p_powerPin, int p_mSexcitationTime);
void disable5VPower(int p_powerPin);

void setup()
{ 
  // Description: Runs once to setup the device into operating mode and the main loop below
  //    all inputs and outputs should be configured and initialized to properly states here.
  // Input: None
  // Output: None

  //Setup the voltage regulator and enable
  pinMode(V5_EXT, OUTPUT);

  // Setup the HC SR-04
  // Make the pins accept input, or send an output (hz)
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIGGER_PIN, OUTPUT);

  //  Setup for tipping mechanism
  pinMode(COUNTER_INT, INPUT);      //Set the pin as an input
  digitalWrite(COUNTER_INT, HIGH);  //Set the input high (normally closed switch), expect voltage through circuit e.g. 3.3vdc
  
  // Initialize sampling time to now!
  nextLog = millis();
}

void loop()
{ 
  // Description: Main loop and logic for the program
  // Input: None
  // Output: None
  
  if ( nextLog < millis() ) // Check to see if the data needs to be logged according to the current time and nextLog value
  {
    Serial.println("Logging and posting data..");
    // Setting the next time, right away
    nextLog = millis() + (SAMPLE_RATE_MINUTES * 60 * 1000);

    //Turn on 5V power for 3S
    enable5VPower(V5_EXT, 30000);
    
    // Sampling the distance sensor
    double distanceInches = hcDist();

    //Turn off 5V power
    disable5VPower(V5_EXT);

    // Store Data
    logData(distanceInches, counter, tilt);
    counter = 0;
    tilt = 0;
    Serial.println("Reset values to zero..");
  }
  else
  {
    unsigned long actualTime = nextLog - millis();
    unsigned long actualTimeSeconds = (actualTime / 1000); // Seconds
    Serial.println("Logging data in\t" + String(actualTimeSeconds / 60) +" Minutes:\t"+ String(actualTimeSeconds % 60) +" Seconds");
  }

  // Check the interupt state for an open condition of the circuit and that enough time has passed to begin counting or incrementing again
  //  Debounce is implemented here
  if( readCounter(COUNTER_INT) && ( millis() - lastCounterTime >= debounceCount ))
  {
    counter++; // Increment the stored value
    Serial.println("Trash inserted!");
    Serial.println("Count increased now: " + String(counter) );
    lastCounterTime = millis(); // We need to set the time to now for measuring the time to wait before incrementing again (debounce)
  }
  else if ( readCounter(COUNTER_INT) )
  {
    Serial.println("...zzz...");
  }

  if( !readCounter(COUNTER_INT) && ( millis() - lastCounterTime < debounceCount))
  { 
    // If the flipper returned to rest or a closed circuit again, we reset the time.  Not like a typical debounce...
    // To reset, we just set the value to the best acceptable using the variable below
    lastCounterTime = millis() - debounceCount;
  }
  float tamperDetection = analogRead(TAMPER_PIN);
  if (tamperDetection >=1900 )
  {
    Serial.println("TAMPER Detected: " + String(tamperDetection));
    tilt++;
  }
  else
  {
    // Serial.println("Current tilt reading : " + String(tamperDetection));
  }
  
  // End of the tunnel, its time to go again
  delay(1000); // Wait for next time around
}

// Function implementations
double hcDist()
{ 
  // Description: Function samples the ultrasonic HC SR-04 sensor (https://www.makerguides.com/hc-sr04-arduino-tutorial/)
  // Input: Nothing
  // Returns:  Negative value if no valid distance is found, or a positive value indicating the distance in inches

  // Prepare Sensor to get distance
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  //-- Send ultrasonic pulse trigger
  digitalWrite(TRIGGER_PIN,HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  //-- Stop

  // Read distance
  double distance = pulseIn(ECHO_PIN, HIGH) * 0.0133 / 2; //TODO : add temperature compensation!
  Serial.print("Distance Measured: ");
  Serial.println(distance);
  if ( distance <= 0 ) { return -1.0; } // Failed reading
  else { return distance; } // Good reading
}

void logData(double p_distanceInches, int p_count, int p_tilt)
{
  // Description: Function for logging data specifically the distance and counting
  // Input: Accepts the parameters as values for the p_distanceInches and p_count
  // Output: Nothing

  Particle.publish("distance", String(p_distanceInches), PRIVATE);
  Particle.publish("count", String(p_count), PRIVATE);
  Particle.publish("tilt", String(p_tilt), PRIVATE);
  return;
}

int readCounter(int p_counterInt)
{ 
  // Description: Function to read the state of an interupt and determine if there is a state change
  //  It is important to note that the reed switch used in this implementation is a normally closed
  //  until magnet moves away from apparatus..
  // Input: p_counterInt is the pin used for checking state change in the hardware
  // Output: returns int value of 1 if an event is detected, otherwise returns nothing

  if ( !digitalRead(p_counterInt) )
  {
    // Serial.println("Flipper has been moved...");
    return 1;   //  Return the value
  }
  return 0;
}

void enable5VPower(int p_powerPin, int p_mSexcitationTime)
{
  digitalWrite(p_powerPin, HIGH);
  delay(p_mSexcitationTime); //Time in mS.  Wait at least 500mS for sensor to turn on and begin recording valid data, otherwise it might be garbage.
  Serial.println("5V Power On");
  return;
}

void disable5VPower(int p_powerPin)
{
  digitalWrite(p_powerPin, LOW);
  Serial.println("5V Power Off");
  return;
}