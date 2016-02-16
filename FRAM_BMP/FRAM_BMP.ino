#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "Adafruit_FRAM_I2C.h"
   
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();
uint16_t          framAddr = 0;
uint16_t          a = 0;
boolean           startProgram = 0;

void setup() {
  Serial.begin(9600); 
  Wire.begin();
  pinMode(3, OUTPUT);
  
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
    sensors_event_t event;
    bmp.getEvent(&event);
   
    /* Display the results (barometric pressure is measure in hPa) */
    if (event.pressure)
    {
      /* Display atmospheric pressue in hPa */
      Serial.print("Pressure:    ");
      Serial.print(event.pressure);
      Serial.println(" hPa");
      
      /* Calculating altitude with reasonable accuracy requires pressure    *
       * sea level pressure for your position at the moment the data is     *
       * converted, as well as the ambient temperature in degress           *
       * celcius.  If you don't have these values, a 'generic' value of     *
       * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
       * in sensors.h), but this isn't ideal and will give variable         *
       * results from one day to the next.                                  *
       *                                                                    *
       * You can usually find the current SLP value by looking at weather   *
       * websites or from environmental information centers near any major  *
       * airport.                                                           *
       *                                                                    *
       * For example, for Paris, France you can check the current mean      *
       * pressure and sea level at: http://bit.ly/16Au8ol                   */
       
      /* First we get the current temperature from the BMP085 */
      float temperature;
      bmp.getTemperature(&temperature);
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" C");
   
      /* Then convert the atmospheric pressure, SLP and temp to altitude    */
      /* Update this next line with the current SLP for better results      */
      float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
      Serial.print("Altitude:    "); 
      Serial.print(bmp.pressureToAltitude(seaLevelPressure,
                                          event.pressure,
                                          temperature)); 
      Serial.println(" m");
      Serial.println("");
    }
    else
    {
      Serial.println("Sensor error");
    }
  
    //FRAM part
    uint8_t value;
    for (uint16_t a = 0; a < 16; a++) {
      value = fram.read8(a);
      if ((a % 16) == 0) {
          Serial.print("\n 0x"); 
          Serial.print(a, HEX); 
          Serial.print(": ");
        }
        Serial.print("0x"); 
        if (value < 0x10){ 
          Serial.print('0');
        }
        Serial.print(value, HEX); 
        Serial.print(" ");
    }
    digitalWrite(3, 1);
    Serial.println("");
    delay(1000);

}
