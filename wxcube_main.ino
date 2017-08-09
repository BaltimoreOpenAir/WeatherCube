

//----------------PREAMBLE-----------------
//----------------big, important definitions-----------------
#define SERIAL_ID 0
#define DEBUG_MODE 1
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
#define TOTAL_MEASUREMENTS 15


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
SoftwareSerial esp(5, 6); // RX, TX

//From D-ULPSM Rev 0.3 LMP91000 Settings //need to set TIACN register, REFCN register, MODECN register
// this is a 2-d array because the settings need to be split between different byte
int LMP91000_settings[8][2] = {
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_1PCT},  //CO
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_4PCT}, //EtOH
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_0PCT},  //H2S
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_10PCT}, //SO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}, //NO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_1PCT},  //O3
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_8PCT},  //IAQ
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}  //RESP
};

int MODECN_DEFAULT = LMP91000_FET_SHORT_DISABLED | LMP91000_OP_MODE_AMPEROMETRIC;

// declaring instances of objects in the statistic class
Statistic stat0;
Statistic stat1;
Statistic stat2;
Statistic stat3;

char inbyte;
//byte cbuf[MAX_MESSAGE_LENGTH];
char* test;

long post_index = 0;
bool running_fan = true;
bool debug_set_rtc = false; // run the clock debug mode, sets clock and prints it
bool fan_on;
//tells device whether or not temp/ humidity sensor is connected
bool b_hdc1080 = false, b_sht31_1 = true, b_sht31_2 = true;

long read_interval, fan_interval;
int gain_index;
float mv_adc_ratio = 0.0;

bool immediate_run = false;
bool debug_run = false;

int reading_location = 10;
float data_array[DATA_ARRAY_SIZE];
int loop_counter = 0;


//----------------CODE----------------

//----------------do only once-----------------
void do_once() { // do at least once, but not all the time
  //----------------set id----------------

  // set id (& save it and EEprom)
  writeEEPROM(EEP0, ID_LOCATION, SERIAL_ID);
  delay(10);
  //----------------set clock-----------------
  rtc_write_date(15, 15, 20, 4, 2, 8, 17); // note: rename function to in order of the time registers in the memory of the rtc
  // second, minute, hour, day of the week, day, month, year
  delay(10);

  // set clock & save to rtc


  if (DEBUG_MODE == 1) {
    //Serial.begin(19200);
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



//----------------setup-----------------
void setup() {
  //----------------initialize communications-----------------
  Wire.begin();

  Serial.begin(19200);
  Serial.println("test");

  // put your setup code here, to run once:
  int check_variable = readEEPROM(EEP0, CHECK_SETUP_INDEX);
  if (check_variable != 57) {
    do_once();
    writeEEPROM(EEP0, CHECK_SETUP_INDEX, 57);
  }
  else {
    if (DEBUG_MODE == 1) {
      //Serial.begin(19200);
      Serial.println("setup previously completed with settings:");
      Serial.println(readEEPROM(EEP0, ID_LOCATION), DEC);
      delay(10);
      rtc_read_timestamp(0);
      delay(10);
    }
  }
  if (DEBUG_MODE == 1) {
    Serial.println(readEEPROM(EEP0, CHECK_SETUP_INDEX), DEC);
    delay(10);
  }
  else {
  }

  //----------------turn things on/pin stuff -----------------
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

  digitalWrite(VREF_EN, LOW);
  digitalWrite(WIFI_EN, LOW);
  digitalWrite(FAN_EN, LOW);


  //----------------set adc/gain stuff-----------------

  gain_index = 3;
  // make this an array
  mv_adc_ratio = 0.03125;
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

  set_afe(10, H2S, 1); // (pin number, sensor type, index#)
  set_afe(11, H2S, 2);
  set_afe(12, H2S, 3);
  set_afe(13, H2S, 4);

}

//----------------do continually-----------------
void loop() {
  // turn on
  long toc = millis(); // take current time

  //----------------fan on-----------------
  digitalWrite(FAN_EN, HIGH);  delay(FAN_INTERVAL);
  // turn fan off and wait for anything trapped in inductive coils to discharge
  digitalWrite(FAN_EN, LOW);  delay(2000);
  //----------------take readings-----------------

  read_data(); //note: reads to data_array

  if (DEBUG_MODE == 1) {
    Serial.println("data array is : ");
    for (int i = 0; i < 14; i++) {
      Serial.println(data_array[i]) ;
    }
  }

  //----------------write readings-----------------
  // note: need to save out gain and whatever else chris wants; 4 additional bytes
  if (loop_counter < 1600) {
    write_data(EEP0, loop_counter * EEPROM_BLOCKSIZE); // write data to a given device at the page given by loop_counter
  }
  else {
    write_data(EEP1, (loop_counter - 1600)*EEPROM_BLOCKSIZE); // write data to a given device at the page given by loop_counter
  }
  // check that write was successful
  loop_counter++ ;
  delay(50);

  //  Serial.println("counter number is now : ");
  //  Serial.println(loop_counter) ;

  //----------------read the data from EEPROM and send to ESP-----------------
  test = {"t55xh5.22x"}; 
  digitalWrite(WIFI_EN, HIGH);
  Serial.println("Wifi on!"); 
  delay(500); 
  esp.begin(9600); 
  delay(1000); 
  
  if (esp.available()){
    Serial.println("ESP available!"); 
    while (esp.available()){
      for (int i = 0; i < 4; i++){
      esp.write(test);
      delay(50); 
      }
      delay(500); 
      esp.flush(); 
    }
    Serial.println("Sent ESP the following message:"); 
    Serial.println(test); 

    delay(5000); 
  }
    digitalWrite(WIFI_EN, LOW);
//  int when_send = 24 * 6;
//  if (DEBUG_MODE == 1) {
//    when_send = 3;
//  }
//  // if we have more then
//  if (loop_counter > when_send) {
//    boolean sending_success = false;
//    int max_tries = 0;
//    while (sending_success = false && max_tries < 20) {
//      // turn on
//      digitalWrite(WIFI_EN, HIGH); // turn on the ESP
//      Serial.println(F("ESP ON"));
//      esp.begin(9600); 
//      // read data from EEPROM memory and put in array day_array
//      int day_array[loop_counter * EEPROM_BLOCKSIZE];
//      read_double();
//      // send data
//      data_send(day_array);
//      //
//      max_tries ++;
//      char a = esp.read();
//      if (a = 1) {
//        sending_success = true;
//      }
//      else {
//        delay(500);
//      }
//    }
//
//    digitalWrite(WIFI_EN, LOW);
//    Serial.println(F("ESP OFF"));
//    // max_tries = 20
//    boolean success = false;
//    if (success == true) {
//    }
//  }
//  else {
//  }
  // wifi_co
  //----------------deep sleep mode-----------------

  while (millis() - toc < READ_INTERVAL ) {
    ;
    delay(500);
    // deep sleep 
  }
}

//----------------FUNCTIONS-----------------
// read double data
void read_double() {
  int day_array[loop_counter * EEPROM_BLOCKSIZE];
  for (int i = 0; i < loop_counter * EEPROM_BLOCKSIZE; i = i + 2) {
    long two = readEEPROM(EEP0, 64 + i);
    long one = readEEPROM(EEP0, 64 + i + 1);
    // append bit-shifted data
    day_array[i] =  ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
  }
  //return day_array;
}
// send
void data_send(int alldays[]) {
  long days_not_sent = readEEPROM(EEP0, DAYS_NOT_SENT_LOCATION);
  delay(6000);
  // send day_array over serial to ESP
  // decleare the letters to be sent first as a character array
  if (esp.available()){
  char sensor_codes[7] = "thbonsy";
  for (int i = 0; i < days_not_sent; i++) {
    int one_day[7];
    int iii = 0; 
    for (int ii = i*7; ii < (i+1)*7; ii++) {
      one_day[iii] = alldays[ii];
      iii++; }

    for (int ii = 0; ii < 7; ii++ ) {
      // compose message to be sent
      String s;
      s += sensor_codes[ii];
      s += one_day[ii];
      s += "x";
      char nbuf[4]; 

      // send message 
      s.toCharArray(nbuf, 4); 
      Serial.println(nbuf); 
      delay(500); 
    }
  }
  }
    // if we can't connect, save messages
  else {
//    // save 0 to check send location to indicate that we didn't send the data
    writeEEPROM(EEP0, CHECK_SEND_LOCATION, byte(0) );
//    // add a number to the number of days not sent
    writeEEPROM(EEP0, DAYS_NOT_SENT_LOCATION, byte(days_not_sent + 1));
  }
}
//write
void write_data(int deviceaddress, unsigned int address )
{
  // Write out data several times consecutively starting at address 64
  //int counter=0;
  address = address + 64;
  //  int write_size = 0;
  //  do {
  //    write_size++;
  //  } while (data_array[write_size]);


  if (DEBUG_MODE == 1) {
    Serial.println("data array is : ");
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      Serial.println(data_array[i]) ;
    }
    //    Serial.println("data array in bytes is :");
    //    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    //      Serial.println( byte(data_array[i])) ;
    //    }
  }
  else {
  }
  //
  //  Serial.println("Write size is: ");
  //  Serial.println(DATA_ARRAY_SIZE);
  Wire.beginTransmission(EEP0);
  Wire.write((int)(address >> 8));   // MSB
  Wire.write((int)(address & 0xFF)); //
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    //byte value = byte(data_array[i]) ;
    int value = data_array[i];

    // check that value is positive and can fit into 2 bytes

    // convert data into two bytes
    byte two = (value & 0xFF);
    byte one = ((value >> 8) & 0xFF);
    Wire.write(two);
    //current_address = current_address + 1;
    // write second byte
    Wire.write(one);
  }
  Wire.endTransmission();

  if (DEBUG_MODE == 1) {

    Serial.println("Attempted to write. Written values are : ") ;
    for (int i = address; i < address + DATA_ARRAY_SIZE * 2; i = i + 2) {
      long two = readEEPROM(EEP0, i);
      long one = readEEPROM(EEP0, i + 1);
      //Serial.println(i);
      Serial.println( ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF));
    }
  }
  else {
  }

}
//read
void read_data()
{
  stat0.clear();
  stat1.clear();
  stat2.clear();
  stat3.clear();
  for (int N = 0; N < TOTAL_MEASUREMENTS; N++) {
    for (int channel = 0; channel < 4; channel++) {
      // read the channel and convert to millivolts
      float a = convert_to_mv(ads.readADC_SingleEnded(channel));
      // add that to the statistics objec
      delay(5);
      if (channel % 4 == 0) {
        stat0.add(a);
      }
      else if (channel % 4 == 1) {
        stat1.add(a);
      }
      else if (channel % 4 == 2) {
        stat2.add(a);
      }
      else if (channel % 4 == 3) {
        stat3.add(a);
      }
      else {
      }
      // wait between reads, in milliseconds
      delay(5);
    }
  }

  //float data_array[14]; // save out all the data: 8 voltages for sensors  and 2 for temp/rh
  //int x = 1000;

  for (int channel = 0; channel < 4; channel++) {
    int x = 100;
    if (channel % 4 == 0) {
      float avg = stat0.average();
      float std = stat0.unbiased_stdev();
      //      Serial.println("voltage average is:");
      //      Serial.println(avg*100) ;

      data_array[0] = avg * x;
      data_array[1] = std * x;

      //      Serial.println("first data_array element is:");
      //      Serial.println(data_array[0]) ;

    }
    else if (channel % 4 == 1) {
      float avg = stat1.average();
      float std = stat1.unbiased_stdev();
      data_array[2] = avg * x;
      data_array[3] = std * x;
    }
    else if (channel % 4 == 2) {
      float avg = stat2.average();
      float std = stat2.unbiased_stdev();
      //      Serial.println(avg/1) ;
      //      Serial.println(std/1) ;
      data_array[4] = avg * x;
      data_array[5] = std * x;
    }
    else if (channel % 4 == 3) {
      float avg = stat3.average();
      float std = stat3.unbiased_stdev();
      //      Serial.println(x*avg/1) ;
      //      Serial.println(x*std/1) ;
      //      Serial.println(std/1) ;
      data_array[6] = avg * x;
      data_array[7] = std * x;
    }
    else {
    }
  }
  // read temperature
  data_array[8] = sht31.readTemperature();
  data_array[9] = sht31.readHumidity();
  data_array[10] = sht32.readTemperature();
  data_array[11] = sht32.readHumidity();
  data_array[12] = hdc1080.readTemperature();
  data_array[13] = hdc1080.readHumidity();

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

void rtc_read_timestamp(int mode)
{
  Wire.beginTransmission(RTC_ADDR);
  Wire.write((byte)0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("no luck");
    //return false;
  }
  else {
    //request 7 bytes (secs, min, hr, dow, date, mth, yr)
    Wire.requestFrom(RTC_ADDR, RTC_TS_BITS);
    byte rtc_out[RTC_TS_BITS];
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
  //  Serial.print(F("AFE"));
  //  Serial.print(index);
  //  Serial.println(F("..."));
  which_afe(num);
  delay(100);
  //  uint8_t res = configure_LMP91000(analyte);
  //  Serial.println(F("done"));
  //  Serial.print(F("Config Result: "));    Serial.println(res);
  //  Serial.print(F("STATUS: "));    Serial.println(lmp91000.read(LMP91000_STATUS_REG), HEX);
  //  Serial.print(F("TIACN: "));    Serial.println(lmp91000.read(LMP91000_TIACN_REG), HEX);
  //  Serial.print(F("REFCN: "));    Serial.println(lmp91000.read(LMP91000_REFCN_REG), HEX);
  //  Serial.print(F("MODECN: "));    Serial.println(lmp91000.read(LMP91000_MODECN_REG), HEX);
  digitalWrite(num, HIGH);
}

float convert_to_mv(float val) {
  if (val > 32767) val -= 65535;
  return val * mv_adc_ratio;
}

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


