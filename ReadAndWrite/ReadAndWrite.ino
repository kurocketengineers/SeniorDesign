#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "Adafruit_FRAM_I2C.h"
#define buffer8 0x000000ff

union Flip
    {
         float input;   // assumes sizeof(float) == sizeof(int)
         int   output;
    };
   
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();
uint16_t          framAddr = 0;
uint16_t          a = 0;
boolean           startProgram = 0;

void setup() {
  Serial.begin(9600); 
  Wire.begin();
  delay(15000);
  if(!bmp.begin())
    {
      /* There was a problem detecting the BMP085 ... check your connections */
      Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
      while(1);
    }
    
   if (fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C FRAM");
    } else {
      Serial.println("I2C FRAM not identified ... check your connections?\r\n");
      Serial.println("Will continue in case this processor doesn't support repeated start\r\n");
   }

  // Read the first byte
  uint8_t test = fram.read8(0x0);
  Serial.print("Restarted "); Serial.print(test); Serial.println(" times");
  // Test write ++
  fram.write8(0x0, test+1);
   


}

void loop() {

/* Get a new sensor event */ 
    
  int entry = 0;

  Flip pressure; 

   //write
   while (entry < 30){
      sensors_event_t event;
      bmp.getEvent(&event);
      // Display the results (barometric pressure is measure in hPa) 
      if (event.pressure)
      {
        //event.pressure returns float value from baro
        pressure.input = event.pressure;
        // Display atmospheric pressue in hPa 
        Serial.print("Pressure:    ");
        Serial.print(pressure.input);
        Serial.println(" hPa");
        fram.write8(0 + (entry *4), (pressure.output ));
        fram.write8(1 + (entry *4), (pressure.output >> 8 ));
        fram.write8(2 + (entry *4), (pressure.output >> 16 ));
        fram.write8(3 + (entry *4), (pressure.output >> 24 ));   
        
        entry++;
        delay(1000);
      }
      else
      {
        Serial.println("Sensor error");
      }

   }

        //Read
   for(int i = 0; i < 30; i++){
    Flip readPressure;
    readPressure.output = 0;
    uint8_t value;
    
      for (uint16_t a = 0; a < 4; a++) {
        value = fram.read8( ( i * 4 ) + (3-a));
        readPressure.output = (readPressure.output << 8 ) + value;
      }
      Serial.println("");
      Serial.println(readPressure.input);
      delay(1000);    
   }

  
}
