//----------------PREAMBLE-----------------
//----------------big, important definitions-----------------
#define SERIAL_ID 15
#define DEBUG_MODE 0
#define FREQUENCY 2000// sampling frequency in ms
#define PM true
#define SD_CS   10
#define HW_BAUD 57600
#define SW_BAUD 9600
#define GAIN_INDEX 3
#define MAX_MESSAGE_LENGTH 120

int start_second = 0;
int start_minute = 23;
int start_hour = 13;
int start_day_of_week = 3; // sunday is 0; 
int start_day = 21;
int start_month = 3;
int start_year = 18;
bool reset_rtc = false; // whether to reset eeprom; always rerun

//----------------libraries-----------------
#include <Statistic.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include "LMP91000.h"
#include "ClosedCube_HDC1080.h"
#include "Adafruit_SHT31.h"
#include <EEPROM.h>
#include "LowPower.h"
#include <SPI.h>
#include <SD.h>

//--------------- setup for saving on the SD card 
//#NEWCODE

char cbuf[MAX_MESSAGE_LENGTH];
bool logfile_error = false;
int yr = 18, mon = 1, dy = 1; // change these to be read

//--------------- definitions-----------------
#define SEND_HOUR 13
#define FAN_INTERVAL 30000//30000
//#define READ_INTERVAL 180//180000//60000 // change for debug
//#define SLEEP_INTERVAL 600 //600000
#define RTC_ADDR 0x6F
#define RTC_TS_BITS 7
#define TIME_REG 0x00
#define EEP0 0x50    //Address of 24LC256 eeprom chip
#define EEP1 0x51    //Address of 24LC256 eeprom chip

#define ID_LOCATION 0 // where we save the id
#define CHECK_SETUP_INDEX 2 // where we save whether or not we've run the do_once setup functions, including setting id and setting clock
#define CHECK_SEND_LOCATION 4// where we save if the data successfully send 
//#define DAYS_NOT_SENT_LOCATION 5// where we log number of days not sent, may be redundant
#define LOOP_COUNTER_LOCATION 5 // where we log the number of data points not sent, or loop_counter
#define EEP_WRITE_LOCATION_INDEX 6// where we log where the counter is; keep rolling it over to conserve lifetime of eeprom memory
#define START_WRITE_LOCATION 64 // 2nd page 

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
#define DATA_ARRAY_SIZE 12 //For the non-PM data points for each read record
#define EEPROM_BLOCKSIZE 36
//#define TOTAL_MEASUREMENTS 15
#define TOTAL_MEASUREMENT_TIME 3 // In minutes
#define SLEEP_MINUTES 12//112
#define MAX_POST_TRIES 10

adsGain_t gain[6] = {GAIN_TWOTHIRDS, GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT, GAIN_SIXTEEN};
adsGain_t adc_pga_gain;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SHT31 sht32 = Adafruit_SHT31();
ClosedCube_HDC1080 hdc1080;
LMP91000 lmp91000;
Adafruit_ADS1115 ads(0x49);

// initialize esp and atmega talking
// software serial mimicking hardware serial
// atmega has only two lines for communications
//SoftwareSerial mySerial(5, 6); // RX, TX // for ESP communication

SoftwareSerial mySerial(3, 7); // RX, TX

//From D-ULPSM Rev 0.3 LMP91000 Settings //need to set TIACN register, REFCN register, MODECN register
// this is a 2-d array because the settings need to be split between different bytes
int LMP91000_settings[8][2] = {
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_1PCT},  //CO
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_4PCT}, //EtOH
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_0PCT},  //H2S
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_10PCT}, //SO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}, //NO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_1PCT},  //O3
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_8PCT},  //IAQ
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}//,  //RESP
};
int MODECN_DEFAULT = LMP91000_FET_SHORT_DISABLED | LMP91000_OP_MODE_AMPEROMETRIC;

long post_index = 0;
bool running_fan = true;
bool debug_set_rtc = false; // run the clock debug mode, sets clock and prints it
bool fan_on;
long data[12];
long read_interval, fan_interval;
int gain_index;
bool immediate_run = false;
bool debug_run = false;
//int reading_location = 10;
int data_array[DATA_ARRAY_SIZE];
int loop_counter = 0;
//int loop_minimum = 96;
int eeprom_write_location = 64;
int device;
byte integer_time[RTC_TS_BITS]; // what we'll store time array in
//char types[] = {"oanbschdtrefgivHmN"};
//char readchar;

void setup(){
//----------------wire and serial-----------------    
  Serial.begin(HW_BAUD);
  mySerial.begin(SW_BAUD);
  Wire.begin();
  delay(100);
  Serial.println(F("Starting setup..."));

  //----------------pin stuff-----------------
  analogReference(INTERNAL);
  pinMode(VOLT, INPUT);
  pinMode(VREF_EN, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(WIFI_EN, OUTPUT);
  pinMode(FAN_EN, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);
  digitalWrite(VREF_EN, HIGH);
  digitalWrite(FAN_EN, LOW);
  pinMode(WIFI_EN, OUTPUT);
  digitalWrite(WIFI_EN, HIGH);
  delay(100);
 //----------------SD card-------------------------
  if (!SD.begin(SD_CS)) {
    logfile_error = true;
    Serial.println(F("init failed!"));
  }
  else{Serial.println(F("init done."));}
  delay(100);

  //write RTC stamp if needed
  if(reset_rtc){rtc_write_date(start_second, start_minute, start_hour, start_day_of_week, start_day, start_month, start_year);}

  // to reset...
  //if (reset== true){
  //  writeEEPROM(EEP0, CHECK_SETUP_INDEX, 0);
  //}
  //writeEEPROM(EEP0, CHECK_SETUP_INDEX, 0);
  Serial.println(F("finished pin setup"));
  Serial.println(readEEPROM(EEP0, CHECK_SETUP_INDEX));
  int check_variable = readEEPROM(EEP0, CHECK_SETUP_INDEX);

  if (check_variable != 57) {
    Serial.println(F("Doing initial setup..."));
    do_once();
    writeEEPROM(EEP0, CHECK_SETUP_INDEX, 57);
    writeEEPROM(EEP0, EEP_WRITE_LOCATION_INDEX, 64); 
    writeEEPROM(EEP0, LOOP_COUNTER_LOCATION, 0); 
  }
  else {
    Serial.println(F("Initial setup already completed"));
    loop_counter = readEEPROM(EEP1, LOOP_COUNTER_LOCATION);
    eeprom_write_location = readEEPROM(EEP1, EEP_WRITE_LOCATION_INDEX);
  }
  Serial.println(F("check variable completed"));


  //----------------set adc/gain stuff-----------------
  //float mv_adc_ratio = 0.03125;
  set_adc_gain(GAIN_INDEX);

  ads.begin();
  set_afe(10, NO2, 1); // (pin number, sensor type, index#)
  set_afe(11, CO, 2);
  set_afe(12, IAQ, 3);
  set_afe(13, RESP, 4);

  // set clock
  delay(50);
//    if (SET_CLOCK==1){ 
//      rtc_write_date(0, 37, 16, 6, 5, 3, 18);
//      Serial.println("Clock is set"); }
  //rtc_write_date(0, 24, 0, 2, 29, 8, 17); // note: rename function to in order of the time registers in the memory of the rtc
  // second, minute, hour, day of the week, day, month, year
  rtc_read_timestamp(1);
  Serial.println(F("Time is..."));
  for (int i = 0; i < RTC_TS_BITS; i++) {
    Serial.print(integer_time[i]);
    Serial.print(',');
  }

  //turn on thermometers
  sht31.begin(0x44); delay(10);
  sht32.begin(0x45); delay(10);
  hdc1080.begin(0x40); delay(10);

  //shut off wifi
  Serial.println(F("setup completed"));
  digitalWrite(WIFI_EN, LOW);
}

void do_once() { // do at least once, but not all the time
  //----------------set id----------------
  Serial.println(F("Doing do_once()"));
  // set id (& save it and EEprom)
  writeEEPROM(EEP0, ID_LOCATION, SERIAL_ID);
  delay(10);
  //----------------set clock-----------------
  rtc_write_date(0, start_minute, start_hour, start_day_of_week, start_day, start_month, 17); // note: rename function to in order of the time registers in the memory of the rtc
  // second, minute, hour, day of the week, day, month, year
  delay(10);

  // set clock & save to rtc
  if (DEBUG_MODE == 1) {
    Serial.println(F("setup:"));
    Serial.println(readEEPROM(EEP0, ID_LOCATION), DEC);
    delay(10);
    rtc_read_timestamp(0);
    delay(10);
  }
  else {
    
  }
  // set gain & save to EEprom
} 


void loop(){
  long toc = millis();
  rtc_read_timestamp(1);
  data_array[0] = integer_time[2]; //hour
  data_array[1] = integer_time[1]; //integer_time[1]; // minute
  data_array[2] = integer_time[0]; // seconds
  yr = integer_time[6];
  mon = integer_time[5]; 
  dy = integer_time[4];
  for (int channel = 0; channel < 4; channel++) {
    data_array[channel+3] = ads.readADC_SingleEnded(channel); // reading or the mean reading
    delay(5);
  }
  data_array[7] = hdc1080.readTemperature() * 100.0;
  data_array[8] = hdc1080.readHumidity() * 100.0;
  data_array[9] = sht31.readTemperature() * 100.0;
  data_array[10] = sht31.readHumidity() * 100.0;
  data_array[11] = analogRead(VOLT) / 1.023 * VREF * VDIV;  //in mV
  delay(5); 
  memset(cbuf, 0, sizeof cbuf);
  sprintf(cbuf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",data_array[0],data_array[1], data_array[2], data_array[3], data_array[4], data_array[5],data_array[6],data_array[7],data_array[8],data_array[9],data_array[10],data_array[11]); // write data to send to cbuf
  delay(10);
  char nbuf[12];
  if(mon < 10 && dy < 10){
    sprintf(nbuf, "F%d0%d0%d.txt", yr, mon, dy);
  }else if(mon < 10 && dy >= 10){
    sprintf(nbuf, "F%d0%d%d.txt", yr, mon, dy);
  }else if(mon >= 10 && dy < 10){
    sprintf(nbuf, "F%d0%d%d.txt", yr, mon, dy);
  }
  Serial.print(nbuf);
  Serial.print(F(": "));
  Serial.print(cbuf);
  File myFile = SD.open(nbuf, FILE_WRITE);
  myFile.print(cbuf);
  delay(10);
  
  if (mySerial.available()){
    char c1, c2, c3, c4;
    bool has_read = false;
    for(int i = 0; i < 30; i++){
      c1 = mySerial.read(); delay(10);
      if(c1 == 0x42){
        c2 = mySerial.read(); delay(10);
        c3 = mySerial.read(); delay(10);
        c4 = mySerial.read(); delay(10);
        if(c2 == 0x4D && c3 == 0x00 && c4 == 0x1C){
          has_read = true;
          break;
        }
      }
    }
    if(has_read){
      bool mark = false;
      for(int i = 0; i < 12; i++){
        c1 = mySerial.read();  delay(10);
        c2 = mySerial.read();  delay(10);
        int i1 = c1, i2 = c2;
        if(i1 < 0) i1 = -i1 + 256;
        if(i2 < 0) i2 = -i2 + 256;
        int res = i1 * 256 + i2;
        Serial.print(res);
        if(i < 11) Serial.print(',');
        myFile.print(res);
        if(i < 11) myFile.print(',');
      }   
      Serial.println();
      myFile.println();
      mySerial.flush();
    }
  }
  myFile.close();  
  while( millis() - toc < FREQUENCY){delay(10);}
}


//----------------FUNCTIONS----------------------
long writeEEPROMdouble(int deviceaddress, unsigned int eeaddress, int value){
  //    int value = input[i];
  //    int address = 64 + 2 * i;
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(two);  // write 2nd bit
  Wire.write(one);  // write first byte
  Wire.endTransmission();
  delay(50);
}

byte readEEPROMdouble(int deviceaddress, unsigned int eeaddress ){
  long two = readEEPROM(deviceaddress, eeaddress);
  long one = readEEPROM(deviceaddress, eeaddress + 1);
  int output =  ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);  //append bit-shifted data
  return output;
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress ){
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ){
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();

  delay(10);
}

void which_afe(int num){
  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(num, LOW);
}

void set_afe(int num, int analyte, int index){
  delay(500);
  if(DEBUG_MODE == 0){
    Serial.print(F("AFE"));
    Serial.print(index);
    Serial.println(F("..."));
  }
  which_afe(num);
  delay(100);
  if(DEBUG_MODE == 0){
    uint8_t res = configure_LMP91000(analyte);
    Serial.println(F("done"));
    Serial.print(F("Config Result: "));    Serial.println(res);
    Serial.print(F("STATUS: "));    Serial.println(lmp91000.read(LMP91000_STATUS_REG), HEX);
    Serial.print(F("TIACN: "));    Serial.println(lmp91000.read(LMP91000_TIACN_REG), HEX);
    Serial.print(F("REFCN: "));    Serial.println(lmp91000.read(LMP91000_REFCN_REG), HEX);
    Serial.print(F("MODECN: "));    Serial.println(lmp91000.read(LMP91000_MODECN_REG), HEX);
  }
  digitalWrite(num, HIGH);
}

//  float convert_to_mv(float val) {
//    if (val > 32767) val -= 65535;
//    return val * mv_adc_ratio;
//  }
//--------READ DATA------------------

//--------AFE and ADC---------------
void set_adc_gain(int g){
  gain_index = g;
  ads.setGain(gain[gain_index]);
}

int configure_LMP91000(int gas){
  return lmp91000.configure( LMP91000_settings[gas][0], LMP91000_settings[gas][1], MODECN_DEFAULT);
}

//------- CLOCK STUFF ---------------
uint8_t dec2bcd(uint8_t n) {
  return n + 6 * (n / 10);
}

uint8_t bcd2dec(uint8_t n) {
  return n - 6 * (n >> 4);
}

void rtc_write_date(int sec, int mint, int hr24, int dotw, int day, int mon, int yr){
  Serial.println("SETTING RTC");
  Wire.beginTransmission(RTC_ADDR);
  Wire.write((uint8_t)TIME_REG);
  Wire.write(dec2bcd(sec) | B10000000);
  Wire.write(dec2bcd(mint));
  Wire.write(dec2bcd(hr24)); //hr
  Wire.write(dec2bcd(dotw) | B00001000); //dotw
  Wire.write(dec2bcd(day)); //date
  Wire.write(dec2bcd(mon)); //month
  Wire.write(dec2bcd(yr)); //year
  Wire.write((byte) 0);
  Wire.endTransmission();
}

void rtc_read_timestamp(int mode){
  byte rtc_out[RTC_TS_BITS];
  Wire.beginTransmission(RTC_ADDR);
  Wire.write((byte)0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("no luck"));
    //return false;
  }
  else {    //request 7 bytes (secs, min, hr, dow, date, mth, yr)
    Wire.requestFrom(RTC_ADDR, RTC_TS_BITS);

    // cycle through bytes and remove non-time data (eg, 12 hour or 24 hour bit)
    for (int i = 0; i < RTC_TS_BITS; i++) {
      rtc_out[i] = Wire.read();
      if (mode == 0){} //printBits(rtc_out[i]);
      else
      {
        if (i == 0) rtc_out[i] &= B01111111;
        else if (i == 3) rtc_out[i] &= B00000111;
        else if (i == 5) rtc_out[i] &= B00011111;
      }
    }
  }

  for (int i = 0; i < RTC_TS_BITS ; i++) {
    int ii = rtc_out[i];
    integer_time[i] = bcd2dec(ii);
  }
}
