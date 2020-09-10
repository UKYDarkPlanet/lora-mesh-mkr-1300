#include <MKRWAN.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200);
  /*
  Serial1.begin(9600);
  Serial2.begin(9600);
  */
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(20);
  char buff;
  while(buff != '\n') {
    buff = Serial2.read();
    Serial.print(buff);
  }
  
}
