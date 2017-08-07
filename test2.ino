// ATMEGA
#include <SoftwareSerial.h>
SoftwareSerial mySerial(5, 6); // RX, TX

void setup()
{
  Serial.begin(57600); // comms
  mySerial.begin(9600); // communicate with esp

  digitalWrite(7, LOW);
  Serial.println(F("ESP ON"));
  delay(1000); 
}

void loop() // run over and over
{
  char nbuf[10], obuf[10], ibuf[10];
  String s = "n"; s += 5; s += "x";
  Serial.println(s);
  s.toCharArray(nbuf,10);
  mySerial.write(nbuf); 
  mySerial.flush();
//   mySerial.write("helloworldxtestx"); 
//   mySerial.print("hellowworldxtestx"); 
//   mySerial.flush(); 
  if (mySerial.available()){
    delay(800); 
    Serial.println(mySerial.read()); 
    delay(800); 
    }
  else{
    Serial.println("esp not available"); 
  }
  delay(1000); 

}
