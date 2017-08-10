// Trying to send message to ESP

#include <SoftwareSerial.h>

#define WIFI_EN 8
SoftwareSerial mySerial(5, 6); // RX, TX
char* test;


// adding after working
int loop_counter = 0;
#define SERIAL_ID 0
#define DEBUG_MODE 1
// # define INITIAL_DATE
//----------------libraries-----------------
#include <Statistic.h>
//#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include "LMP91000.h"
#include "ClosedCube_HDC1080.h"
#include "Adafruit_SHT31.h"
#include <EEPROM.h>

//--------------- definitions-----------------
#define FAN_INTERVAL 6000
#define READ_INTERVAL 5000
#define RTC_ADDR 0x6F
#define RTC_TS_BITS 7
#define TIME_REG 0x00
#define EEP0 0x50    //Address of 24LC256 eeprom chip
#define EEP1 0x51    //Address of 24LC256 eeprom chip

#define ID_LOCATION 0 // where we save the id
#define CHECK_SETUP_INDEX 2 // where we save whether or not we've run the do_once setup functions, including setting id and setting clock
#define CHECK_SEND_LOCATION 4// where we save if the data successfully send 
#define DAYS_NOT_SENT_LOCATION 5// where we log number of days not sent, may be redundant 
#define START_WRITE_LOCATION 64 // 2nd page 
#define MAX_MESSAGE_LENGTH 25


#define VDIV 5.02    //voltage divider: (1 + 4.02) / 1
#define VREF 1.1

#define VREF_EN 4
#define WIFI_EN 8
#define FAN_EN 9

#define VOLT A0
// numeric code to index the array LMP91000
#define CO    0
#define EtOH  1
#define H2S   2
#define SO2   3
#define NO2   4
#define O3    5
#define IAQ   6
#define RESP  7

#define HDC_ADDRESS 0x40

#define DATA_ARRAY_SIZE 14
#define EEPROM_BLOCKSIZE 32
#define TOTAL_MEASUREMENTS


void setup()
{ //----------------pin stuff-----------------
  // define which way pins work and turn on/off
  analogReference(INTERNAL);
  pinMode(VOLT, INPUT);

  pinMode(VREF_EN, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(WIFI_EN, OUTPUT);
  pinMode(FAN_EN, OUTPUT);

  digitalWrite(VREF_EN, LOW);
  //digitalWrite(WIFI_EN, LOW);
  digitalWrite(FAN_EN, LOW);
  pinMode(WIFI_EN, OUTPUT);
  digitalWrite(WIFI_EN, HIGH);
  Serial.begin(57600);
  mySerial.begin(9600);
  Wire.begin();
  // make dummy data to send
  for (int i = 64; i < 64 * 5; i++) {
    writeEEPROM(EEP0, i, 5 );
  }
  Serial.println("setup completed");
}

void loop() // run over and over
{
  Serial.println("EEPROM says...");
  long day_array[MAX_MESSAGE_LENGTH];
  for (int i = 64; i < MAX_MESSAGE_LENGTH + 64; i++) {
    //  Serial.print(readEEPROM(EEP0, i ));
    day_array[i - 64] = readEEPROM(EEP0, i );
  }

  //test = {"t103h217x"};
  //test = {"hello"};
  if (mySerial.available()) {
    while (mySerial.available()) {
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
  s += L;
  for (int i = i; i < 5; i++) {
    s += String(day_array[i]);
  }
  s += "x";
  // form data to be sent here

  // // int day_array[loop_counter * EEPROM_BLOCKSIZE];
  //  char day_array[loop_counter * EEPROM_BLOCKSIZE];
  //  Serial.println("day array is : ");
  //  for (int i = 0; i <64*5; i = i + 2) {
  //    day_array[i] = readEEPROM(EEP0, 64+i );
  //    Serial.print(day_array[i], DEC);
  //  }


  //  for (int i = 0; i < 25; i++) {
  //    s += test[i];
  //    //s += day_array[i];
  //  }
  s.toCharArray(cbuf, 25);
  Serial.println(s);
  mySerial.write(cbuf);
  mySerial.flush();
  delay(800);

  // instead,


}

void read_double() {
  int day_array[loop_counter * EEPROM_BLOCKSIZE];
  for (int i = 0; i < loop_counter * EEPROM_BLOCKSIZE; i = i + 2) {
    long two = readEEPROM(EEP0, 64 + i);
    long one = readEEPROM(EEP0, 64 + i + 1);
    // append bit-shifted data
    day_array[i] =  ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
  }
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress )
{
  byte rdata = 0xFF;

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, 1);

  if (Wire.available()) rdata = Wire.read();

  return rdata;
}

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data )
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();

  delay(10);
}
