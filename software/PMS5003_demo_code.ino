#include <SoftwareSerial.h>

SoftwareSerial mySerial(3,7); // RX, TX
char readchar;
long data[12];

void setup()
{
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  Serial.println("working");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
}

void loop() // run over and over
{
  int cutoff = -2000;
  if (mySerial.available()){
    char c1, c2;
    c1= mySerial.read();
    if(c1 == 0x42){
      delay(10);
      c1 = mySerial.read();  delay(10);
      c1 = mySerial.read();  delay(10);
      c1 = mySerial.read();  delay(10);
      for(int i = 0; i < 12; i++){
        c1 = mySerial.read();  delay(10);
        c2 = mySerial.read();  delay(10);
        int i1, i2;
        i1 = c1;
        i2 = c2;
        
        //these two lines are speculative but should help address the overflow problem
        if(i1 < -5) i1 += 256;
        if(i2 < -5) i2 += 256;
        data[i] = i1 * 256 + i2;
      }

      Serial.print(millis());
      Serial.print('\t');
      for(int i = 0; i < 12; i++){
        Serial.print(data[i]);
        Serial.print('\t');
      }
      Serial.println();
    }
  }
}

