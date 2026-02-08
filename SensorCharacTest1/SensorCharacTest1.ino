/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

#include "driver/timer.h"


MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
volatile long irValue;

volatile int minuteCount = 1;

static bool IRAM_ATTR tell_HR (void *args){ 

  if(irValue < 50000)
    Serial.print("BPM: 0");
  else{
    Serial.print("BPM: ");
    Serial.print(beatAvg);
  }

  Serial.print(" | Movement");
  if(digitalRead(2))
    Serial.println(" Detected!");
  else
    Serial.println(" Undetected");
 
  return true;

}



static bool IRAM_ATTR printingtime(void *args){

  Serial.print("\n[Time Elapsed: ");
  Serial.print(minuteCount);
  Serial.print(" minute/s");
  Serial.println("]\n");

  minuteCount = minuteCount + 1;
  
  return true;

}


void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  //pinMode(2, INPUT);

  /*TIMER 1 SETUP*/
  timer_config_t config;
  config.alarm_en = TIMER_ALARM_DIS;
  config.counter_en = TIMER_PAUSE;
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_dir = TIMER_COUNT_UP;
  config.auto_reload = TIMER_AUTORELOAD_EN;
  config.divider = 2;

  timer_init(TIMER_GROUP_0,TIMER_0,&config);

  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 40000000);

  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);

  timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);

  timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, tell_HR, (void*)0, 0);

  timer_start(TIMER_GROUP_0, TIMER_0);

   /*TIMER 2 SETUP*/
  timer_init(TIMER_GROUP_0,TIMER_1,&config);

  timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, 2400000000);

  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);

  timer_set_alarm(TIMER_GROUP_0, TIMER_1, TIMER_ALARM_EN);

  timer_isr_callback_add(TIMER_GROUP_0, TIMER_1, printingtime, (void*)0, 0);

  timer_start(TIMER_GROUP_0, TIMER_1);
}

void loop()
{
   irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }

  }

}