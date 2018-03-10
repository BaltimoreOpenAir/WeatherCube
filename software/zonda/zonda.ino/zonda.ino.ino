//----------------PREAMBLE-----------------
//----------------big, important definitions-----------------
#define SERIAL_ID 15
#define DEBUG_MODE 0
#define FREQUENCY 2000// sampling frequency in ms
int start_hour = 17;
int start_minute =51;
int start_day = 5;
int start_day_of_week = 6; // sunday is 0; 
int start_month = 3;
bool reset = false; // whether to reset eeprom; always rerun
#define PM true
// # define INITIAL_DATE
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

//#include "keys.h" // wifi passwords, amazon table details, serial id 
// note:  this needs to be included with the esp code because of the esp8266 aws libraries dependencies -anna
// 

//--------------- setup for saving on the SD card 
//#NEWCODE
#include <SPI.h>
#include <SD.h>
#define SD_CS   10
char cbuf[50];
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
#define MAX_MESSAGE_LENGTH 96


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

#define DATA_ARRAY_SIZE 22 // 10 +12
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
// this is a 2-d array because the settings need to be split between different byte
int LMP91000_settings[8][2] = {
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_1PCT},  //CO
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_4PCT}, //EtOH
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_0PCT},  //H2S
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_10PCT}, //SO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}, //NO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_1PCT},  //O3
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_8PCT},  //IAQ
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}  //RESP
};

int MODECN_DEFAULT = LMP91000_FET_SHORT_DISABLED | LMP91000_OP_MODE_AMPEROMETRIC;
/*
// declaring instances of objects in the statistic class
Statistic stat0;
Statistic stat1;
Statistic stat2;
Statistic stat3;
Statistic stat4;
Statistic stat5;
Statistic stat6;
Statistic stat7;
Statistic stat8;
Statistic stat9;
*/
char inbyte;
//byte cbuf[MAX_MESSAGE_LENGTH];
char* test;

long post_index = 0;
bool running_fan = true;
bool debug_set_rtc = false; // run the clock debug mode, sets clock and prints it
bool fan_on;
long data[12];
//tells device whether or not temp/ humidity sensor is connected
//bool b_hdc1080 = false, b_sht31_1 = true, b_sht31_2 = true;
//bool b_hdc1080 = false, b_sht31_1 = false, b_sht31_2 = false;

long read_interval, fan_interval;
int gain_index;

bool immediate_run = false;
bool debug_run = false;
//int reading_location = 10;
long data_array[DATA_ARRAY_SIZE];
int loop_counter = 0;
//int loop_minimum = 96;
int eeprom_write_location = 64;
int device;
byte integer_time[RTC_TS_BITS]; // what we'll store time array in
char types[] = {"oanbschdtrefgivHmN"};
// o3 mean, o3 std, no2 mean, no2 std, so2 mean, so2 std, h2s mean, h2s std,
// temp1, rh1, temp2, rh2, temp3, battery voltage, hour, minute
void do_once() { // do at least once, but not all the time
  //----------------set id----------------
  Serial.println("Doing do_once()");
  // set id (& save it and EEprom)
  writeEEPROM(EEP0, ID_LOCATION, SERIAL_ID);
  delay(10);
  //----------------set clock-----------------
  rtc_write_date(0, start_minute, start_hour, start_day_of_week, start_day, start_month, 17); // note: rename function to in order of the time registers in the memory of the rtc
  // second, minute, hour, day of the week, day, month, year
  delay(10);

  // set clock & save to rtc
  if (DEBUG_MODE == 1) {
    Serial.println("setup:");
    Serial.println(readEEPROM(EEP0, ID_LOCATION), DEC);
    delay(10);
    rtc_read_timestamp(0);
    delay(10);
  }
  else {
  }
  // set gain & save to EEprom
} 

  void setup()
  { // put your setup code here, to run once:
    //  rtc_write_date(0, 20, 12, 3, 30, 8, 17);
    Serial.begin(57600);
    Serial.println("Starting setup...");

    //----------------pin stuff-----------------
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
    // enable SD card
    pinMode(SD_CS, OUTPUT);
    //  PM sensor 
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);

    digitalWrite(VREF_EN, HIGH);
    //digitalWrite(WIFI_EN, LOW);
    digitalWrite(FAN_EN, LOW);
    pinMode(WIFI_EN, OUTPUT);
    digitalWrite(WIFI_EN, HIGH);
    delay(100);

  //----------------wire and serial-----------------    
    mySerial.begin(9600); // also in use for the PMS sensor
    Wire.begin();

   //----------------SD card-------------------------
  if (!SD.begin(SD_CS)) {
    logfile_error = true;
    Serial.println(F("init failed!"));
  }
  else{
    Serial.println(F("init done."));
  }
  delay(100);

// to reset...
    if (reset== true){
      writeEEPROM(EEP0, CHECK_SETUP_INDEX, 0);
    }
    //writeEEPROM(EEP0, CHECK_SETUP_INDEX, 0);
    Serial.println("finished pin setup");
    Serial.println(readEEPROM(EEP0, CHECK_SETUP_INDEX));
    int check_variable = readEEPROM(EEP0, CHECK_SETUP_INDEX);

    if (check_variable != 57) {
      Serial.println("Doing initial setup...");
      do_once();
      writeEEPROM(EEP0, CHECK_SETUP_INDEX, 57);
      writeEEPROM(EEP0, EEP_WRITE_LOCATION_INDEX, 64); 
      writeEEPROM(EEP0, LOOP_COUNTER_LOCATION, 0); 
    }
    else {
      Serial.println("Initial setup already completed");
      loop_counter = readEEPROM(EEP1, LOOP_COUNTER_LOCATION);
      eeprom_write_location = readEEPROM(EEP1, EEP_WRITE_LOCATION_INDEX);
    }
    Serial.println("check variable completed");
  

    //----------------set adc/gain stuff-----------------

    gain_index = 3;
    // make this an array
    float mv_adc_ratio = 0.03125;
    set_adc_gain(gain_index);

    //--------------AFE and ADC------------------------
    //                                                                ADS1015  ADS1115
    //                                                                -------  -------
    // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
    ads.begin();

    set_afe(10, O3, 1); // (pin number, sensor type, index#)
    set_afe(11, H2S, 2);
    set_afe(12, SO2, 3);
    set_afe(13, NO2, 4);
    // set clock
    delay(50);
//    if (SET_CLOCK==1){ 
//      rtc_write_date(0, 37, 16, 6, 5, 3, 18);
//      Serial.println("Clock is set"); }
    //rtc_write_date(0, 24, 0, 2, 29, 8, 17); // note: rename function to in order of the time registers in the memory of the rtc
    // second, minute, hour, day of the week, day, month, year
    rtc_read_timestamp(1);
    Serial.println("Time is...") ;
    for (int i = 0; i < RTC_TS_BITS; i++) {
      Serial.print(integer_time[i]);
    }

    // turn on thermometers
    sht31.begin(0x44);
    sht32.begin(0x45);
    hdc1080.begin(0x40);

    //  delay(50);
    delay(50);

    //----------------set clock-----------------
    //rtc_write_date(0, 10, 9, 7, 28, 8, 17); // note: rename function to in order of the time registers in the memory of the rtc
    // second, minute, hour, day of the week, day, month, year
    Serial.println("setup completed");
    digitalWrite(WIFI_EN, LOW);
  }



  void loop() // run over and over
  { 
    /*
char readchar;


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
  
    */
    //----------------fan on-----------------
    //Serial.println("fan on");
    //digitalWrite(FAN_EN, HIGH);  delay(FAN_INTERVAL);
    // turn fan off and wait for anything trapped in inductive coils to discharge
    //digitalWrite(FAN_EN, LOW);  delay(2000);
    ////-----------take readings------------------
    long toc = millis();
    rtc_read_timestamp(1);
    data_array[0]= integer_time[2]; //hour
    data_array[1] = integer_time[1]; //integer_time[1]; // minute
    data_array[2] = integer_time[0]; // seconds

        for (int channel = 0; channel < 4; channel++) {
        // read the channel and convert to millivolts
          long toc2 = millis();
          //float a = convert_to_mv(ads.readADC_SingleEnded(channel));
          data_array[channel+3] = ads.readADC_SingleEnded(channel); // reading or the mean reading
          // add that to the statistics objec
          delay(5);
          }
    data_array[7] = hdc1080.readTemperature();
    data_array[8] = hdc1080.readHumidity();
    data_array[9] = sht31.readTemperature();
    data_array[10] = sht31.readHumidity();


    //data_array[8] = sht32.readTemperature();
    //data_array[9] = sht32.readHumidity();

    //data_array[10] = analogRead(VOLT) / 1023.0 * VREF * VDIV;
    //data_array[11] = PM readings...
    
 
    delay(5); 

// write data to sd card 
    // rtc_read_timestamp(1); // updates integer_time
    // second, minute, hour, day of the week, day, month, year
      yr = integer_time[6];
      mon = integer_time[5]; 
      dy = integer_time[4];
    String s = "";
    for (int i = 0; i <11 ; i++) { //DATA_ARRAY_SIZE; i++) {
     s += data_array[i];
     s+= ","; 
    }
    //s+="\n"; 
    s.toCharArray(cbuf, 96); // MAX_MESSAGE_LENGTH);
    //sprintf(cbuf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",data_array[0],data_array[1], data_array[2], data_array[3], data_array[4], data_array[5],data_array[6],data_array[7],data_array[8],data_array[9],data_array[10]); // write data to send to cbuf
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
    Serial.print(": ");
    //Serial.println("you could never pretend that i dont love you i could never pretend that im your man thats exactly the way that i want it thats exactly the way that i am"); 
    //Serial.println(""); 
    File myFile = SD.open(nbuf, FILE_WRITE);
    Serial.println(cbuf);
    myFile.print(cbuf);
    delay(10); 
    /*
    s = "";
//    for (int i = 13; i <DATA_ARRAY_SIZE ; i++) { //DATA_ARRAY_SIZE; i++) { ///should be 12 but 1st pm value seems to be jibberish
    for(int i = 0; i < 12; i++){
     s += data[i];
     //s += data_array[i];
     s+= ","; 
    }
    s+="\n"; 
    s.toCharArray(cbuf, 96); // MAX_MESSAGE_LENGTH);
    Serial.print(cbuf);
    myFile.print(cbuf);
    delay(10); 
*/

// read and save PM 
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
        i1 = int(c1);
        i2 = int(c2);
        
        //these two lines are speculative but should help address the overflow problem
        if(i1 < -5) i1 += 256;
        if(i2 < -5) i2 += 256;
        data[i] = i1 * 256 + i2;
        //Serial.println("pm data is..."); 
        //Serial.println(i1 * 256 + i2); 
      }

      Serial.print('\t');
      for(int i = 0; i < 12; i++){
        Serial.print(data[i]);
        Serial.print(','); 
        //Serial.print('\t');
        myFile.print(data[i]);
        myFile.print(","); 
      }
    }
}
    myFile.print("\n"); 
    myFile.close();
  Serial.println(""); 

// delay for desired sample frequency
    while( millis()-toc < FREQUENCY){
      delay(50);
      //Serial.println("waiting...");
    }
    
   Serial.println("Looping");
  }

// end of main body code
 

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

  void which_afe(int num)
  {
    digitalWrite(10, HIGH);
    digitalWrite(11, HIGH);
    digitalWrite(12, HIGH);
    digitalWrite(13, HIGH);
    digitalWrite(num, LOW);
  }

  void set_afe(int num, int analyte, int index)
  {
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
  //-----------------------------------

  //--------AFE and ADC---------------
  //-----------------------------------

  void set_adc_gain(int g)
  {
    gain_index = g;
    ads.setGain(gain[gain_index]);
  }

  int configure_LMP91000(int gas)
  {
    return lmp91000.configure( LMP91000_settings[gas][0], LMP91000_settings[gas][1], MODECN_DEFAULT);
  }

  //------- CLOCK STUFF ---------------
  //-----------------------------------


  uint8_t dec2bcd(uint8_t n) {
    return n + 6 * (n / 10);
  }

  uint8_t bcd2dec(uint8_t n) {
    return n - 6 * (n >> 4);
  }

  void printBits(byte myByte) {
    for (byte mask = 0x80; mask; mask >>= 1) {
      if (mask  & myByte)
        Serial.print('1');
      else
        Serial.print('0');
    }
    Serial.println();
  }

  void rtc_write_date(int sec, int mint, int hr24, int dotw, int day, int mon, int yr)
  {
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
    //Wire.write((uint8_t)0x00);                     //stops the oscillator (Bit 7, ST == 0)
    /*
       Wire.write(dec2bcd(05));
      Wire.write(dec2bcd(04));                  //sets 24 hour format (Bit 6 == 0)
      Wire.endTransmission();
      Wire.beginTransmission(RTC_ADDR);
      Wire.write((uint8_t)TIME_REG);
      Wire.write(dec2bcd(30) | _BV(ST));    //set the seconds and start the oscillator (Bit 7, ST == 1)
    */
    Wire.endTransmission();
  }

  void rtc_read_timestamp(int mode)
  {
    byte rtc_out[RTC_TS_BITS];
    //int integer_time[RTC_TS_BITS];
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((byte)0x00);
    if (Wire.endTransmission() != 0) {
      Serial.println("no luck");
      //return false;
    }
    else {
      //request 7 bytes (secs, min, hr, dow, date, mth, yr)
      Wire.requestFrom(RTC_ADDR, RTC_TS_BITS);

      // cycle through bytes and remove non-time data (eg, 12 hour or 24 hour bit)
      for (int i = 0; i < RTC_TS_BITS; i++) {
        rtc_out[i] = Wire.read();
        if (mode == 0) printBits(rtc_out[i]);
        else
        {
          //byte b = rtc_out[i];
          if (i == 0) rtc_out[i] &= B01111111;
          else if (i == 3) rtc_out[i] &= B00000111;
          else if (i == 5) rtc_out[i] &= B00011111;
          //int j = bcd2dec(b);
          //Serial.print(j);
          //if(i < how_many - 1) Serial.print(",");
        }
      }
    }

    for (int i = 0; i < RTC_TS_BITS ; i++) {
      int ii = rtc_out[i];
      integer_time[i] = bcd2dec(ii);
    }
  }

