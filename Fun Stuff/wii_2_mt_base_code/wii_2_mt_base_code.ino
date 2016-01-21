
#include <Servo.h>
Servo servo1; 
Servo servo2;
#include <Wire.h>


#define ZEROX 530  
#define ZEROY 530
#define ZEROZ 530

#define PIN_C 10
#define PIN_Z 9

#define WII_NUNCHUK_I2C_ADDRESS 0x52



int counter;


uint8_t data[6];

void setup() 
{ 
  
  servo1.attach(11);
  servo2.attach(12);

 
  Wire.begin();

  Wire.beginTransmission(WII_NUNCHUK_I2C_ADDRESS);
  Wire.write(0xF0);
  Wire.write(0x55);
  Wire.endTransmission();

  Wire.beginTransmission(WII_NUNCHUK_I2C_ADDRESS);
  Wire.write(0xFB);
  Wire.write(0x00);
  Wire.endTransmission();

  // Set up the LED's as well
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_Z, OUTPUT);
} 

void loop() 
{ 
  
    Wire.requestFrom(WII_NUNCHUK_I2C_ADDRESS, 6);

    counter = 0;
   
    while(Wire.available())
    {
   
      data[counter++] = Wire.read();
    }

    
    Wire.beginTransmission(WII_NUNCHUK_I2C_ADDRESS);
    Wire.write(0x00);
    Wire.endTransmission();

    if(counter >= 5)
    {
      
      double accelX = ((data[2] << 2) + ((data[5] >> 2) & 0x03) - ZEROX);
      double accelY = ((data[3] << 2) + ((data[5] >> 4) & 0x03) - ZEROY);
      double accelZ = ((data[4] << 2) + ((data[5] >> 6) & 0x03) - ZEROZ);

      int value = constrain(accelY, -180, 180);
     
      value = map(value, -180, 180, 0, 180);
     
      servo1.write(value);
      
     
      value = constrain(accelX, -180, 180);
      value = map(value, -180, 180, 0, 180); 
      servo2.write(value);

      // New code, read C & Z pins
      bool   c_button = (data[5] & 0x02) == 0;
      bool   z_button = (data[5] & 0x01) == 0;
      digitalWrite(PIN_C, c_button);
      digitalWrite(PIN_Z, z_button);
      
    
      delay(20);
    }
}
