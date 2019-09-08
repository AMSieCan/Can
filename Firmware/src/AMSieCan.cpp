#include "application.h"
#line 1 "/Users/danielhowe/CodeDevelopmentFolder/TrashBinThnkg/AMSieCan/AMSieCan/src/AMSieCan.ino"
/*
 * Project AMSieCan
 * Description: Program for monitoring the volume and quantity of debris collected through the apparatus and sending the sampled sensor data to the cloud database
 * Author: Daniel Howe
 * Date: 2019-08-26
 */


//Static definitions
// -- Sampling Logic
double hcDist();
void logData(double p_distanceInches, int p_count, int p_debug);
void setup();
void loop();
#line 11 "/Users/danielhowe/CodeDevelopmentFolder/TrashBinThnkg/AMSieCan/AMSieCan/src/AMSieCan.ino"
const int SAMPLE_RATE_MINUTES = 15; // Minutes between saving data and sending to cloud
unsigned long nextLog; // Millisecond time that log should begin after

// -- Hardware
// Pin assignments for the HC-SR04
const int ECHO_PIN = D6;
const int TRIGGER_PIN = D2;

double hcDist()
{ // Returns:  negative value if no valid distance is found, or a positive value indicating the distance in inches
  // Prepare sensor to get distance
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  //-- Send ultrasonic pulse trigger
  digitalWrite(TRIGGER_PIN,HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  //-- Stop

  // Read distance
  double distance = pulseIn(ECHO_PIN, HIGH) * 0.0133 / 2;
  Serial.print("Distance Measured: ");
  Serial.println(distance);
  if (distance <= 0) return -1.0; // Failed reading
  else { return distance; } // Good reading
}

void logData(double p_distanceInches, int p_count, int p_debug)
{
  Particle.publish("distance", String(p_distanceInches), PRIVATE);
  Particle.publish("count", String(p_count), PRIVATE);
  Particle.publish("debug", String(p_debug), PRIVATE);
}

void setup()
{ // Input: None
  // Output: None
  // Description: 
  // Runs once to setup the device into operating mode and the main loop below
  // Make the pins for the ultrasonic and counter accept input, or send output (hz)
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIGGER_PIN, OUTPUT);

  // Initialize sampling time to now!
  nextLog = millis();
}

void loop()
{ // Input: None
  // Output: None
  // Description: Main loop and logic for the program

  

  if (nextLog < millis() ) // Check to see if the data needs to be logged according to the current time and nextLog value
  {
    Serial.print("Logging and posting data..");
    // Setting the next time, right away
    nextLog = millis() + (SAMPLE_RATE_MINUTES * 60 * 1000);

    // Sampling the distance sensor
    double distanceInches = hcDist();

    // Logging other values
    int counter = 0; //Arbitrary for now
    bool debugState = true; //Arbitrary for now

    // Store Data
    logData(distanceInches, counter, debugState);
  }
  else
  {
    unsigned long actualTime = nextLog - millis();
    unsigned long actualTimeSeconds = (actualTime / 1000); // Seconds
    Serial.println("Logging in (min:sec)" + String(actualTimeSeconds / 60) +":"+ String(actualTimeSeconds % 60));
  }
  delay(100);
}