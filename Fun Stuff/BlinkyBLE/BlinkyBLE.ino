#include <Wire.h>

bool led = true;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  
  pinMode(3, OUTPUT);

}

void loop() {

  
  digitalWrite(3, !led);
  delay(1000);
}
