// ATMEGA
#include <SoftwareSerial.h>
SoftwareSerial mySerial(5, 6); // RX, TX

void setup()
{
  Serial.begin(57600); // comms
  mySerial.begin(9600); // communicate with esp
  pinMode(8, OUTPUT); 
  digitalWrite(8, HIGH);
  Serial.println(F("ESP ON"));
  delay(1000);
}

void loop() // run over and over
{
  char nbuf[10];
  String s = "n"; s += 5; s += "x";
  Serial.println(s);
  s.toCharArray(nbuf, 10);
  mySerial.print(nbuf);
  //mySerial.write(nbuf);
  mySerial.flush();
  mySerial.write("helloworldxtestx");
  //mySerial.print("hellowworldxtestx");
  mySerial.flush();
  // try to connect
  int i = 0;
  while (~mySerial.available() &&  i < 20) {
    //    Serial.println(mySerial.read());
    //i++;
    if (mySerial.available()) {
      delay(1000);
      Serial.println("Success!"); 
      Serial.println(mySerial.read());
      delay(800);
    }

    //Serial.println("trying again"); 
    delay(1000); 
    i++; 
    //  else{
    //    ;//Serial.println("esp not available");
  }
    delay(1000);
  }
