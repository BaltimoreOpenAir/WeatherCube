// Trying to send message to ESP

#include <SoftwareSerial.h>

#define WIFI_EN 8
SoftwareSerial mySerial(5, 6); // RX, TX
char* test; 

void setup()
{  pinMode(WIFI_EN, OUTPUT);
  digitalWrite(WIFI_EN, HIGH);
  Serial.begin(57600);
  mySerial.begin(9600);
}

void loop() // run over and over
{test = {"t103h217x"};
//test = {"hello"}; 
  if (mySerial.available()){
    while (mySerial.available()){
      Serial.write(mySerial.read());
    }
  }
//  if (Serial.available()){
//    while(Serial.available()){
//      mySerial.write(Serial.read());
//      mySerial.write(test);   
//    }
//  }

  long L = millis();
  Serial.println(L);
  char cbuf[25];
  //String s = "hello";
  String s = ""; 
  //s += L;
  //s += "x";
  for (int i=0; i< 25; i++){ s+= test[i];}
  s.toCharArray(cbuf,25); 
  mySerial.write(cbuf);
  mySerial.flush();
  delay(800);

  // instead, 

 
}

