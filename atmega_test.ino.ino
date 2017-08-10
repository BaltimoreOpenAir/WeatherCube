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

//int loop_counter = 0;
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
  delay(1000); 
  Serial.begin(57600);
  mySerial.begin(9600);
  Wire.begin();
  // make dummy data to send
  long input[6] = {145.2, 3.45, 5.2, 350.0, 405.0};
  for (int i = 0; i < 6; i++) {
    writeEEPROMdouble(EEP0, 64 + 2 * i, input[i]);
  }
  Serial.println("setup completed");


}

void loop() // run over and over
{
  ////-----------take readings------------------


  ////-----------save readings------------------

  // add: if loop_counter > X...
  ////-----------send readings------------------
  //// first pull from EEPROM
  Serial.println("EEPROM says...");
  long one_day_array[EEPROM_BLOCKSIZE];
  for (int i = 0; i < loop_counter * EEPROM_BLOCKSIZE; i = i + 2) {
    one_day_array[i] = readEEPROMdouble(EEP0, 64 + i);
  }
  
  //// check serial messages
  if (mySerial.available()) {
    while (mySerial.available()) {
      Serial.write(mySerial.read());
    }
  }
    //// send data to AWS

  long L = millis();
  //Serial.println(L);
  char cbuf[25];
  //String s = "hello";
  String s = "";
  //s += L;
  for (int i = i; i < 5; i++) {
    s += String(one_day_array[i]);
  }
  s += "x";
  s.toCharArray(cbuf, 25);
  Serial.println(s);
  mySerial.write(cbuf);
  mySerial.flush();
  delay(800);

  
  loop_counter ++;

  // add: wifi low
  Serial.println("Looping"); 
}

//----------------FUNCTIONS----------------------
long writeEEPROMdouble(int deviceaddress, unsigned int eeaddress, int value)
{
  //    int value = input[i];
  //    int address = 64 + 2 * i;
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  // write 2nd bit
  Wire.write(two);
  // write first byte
  Wire.write(one);
  Wire.endTransmission();
  delay(50);
}

byte readEEPROMdouble(int deviceaddress, unsigned int eeaddress )
{
  long two = readEEPROM(deviceaddress, eeaddress);
  long one = readEEPROM(deviceaddress, eeaddress + 1);
  //    // append bit-shifted data
  int output =  ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
  return output;
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
