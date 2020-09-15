#include <MKRWAN.h>

String msg;
char buff;
void resetModule() { 
  Serial.println("resetting module");
  digitalWrite(LORA_BOOT0, LOW);    
  Serial2.end();
  Serial2.begin(115200);   
  delay(100);
  digitalWrite(LORA_RESET, HIGH);    
  delay(100);    
  digitalWrite(LORA_RESET, LOW);    
  delay(100);    
  digitalWrite(LORA_RESET, HIGH);
  delay(100);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200);
  while (!Serial);  
  while (!Serial2); 
  pinMode(LED_BUILTIN, OUTPUT);    
  pinMode(LORA_BOOT0, OUTPUT);    
  pinMode(LORA_RESET, OUTPUT);  
  resetModule();
  Serial.println("Listening to the LoRaMesh module");
  delay(200);
  Serial2.write("start");
}

void loop() {
  msg = Serial2.readString();
  Serial.println(msg);
}
