//// Trying to send message to ESP
//----------------PREAMBLE-----------------
//----------------big, important definitions-----------------
#define SERIAL_ID 1
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
//#include "keys.h" // wifi passwords, amazon table details, serial id

//--------------- definitions-----------------
#define FAN_INTERVAL 6000
#define READ_INTERVAL 100 // change for debug
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

#define DATA_ARRAY_SIZE 16
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
SoftwareSerial mySerial(5, 6); // RX, TX

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
int loop_minimum = 2;
int eeprom_write_location = 64;
int device;
byte integer_time[RTC_TS_BITS]; // what we'll store time array in
char types[] = {"oonnsshhtrtrtrdhm"};
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
    Serial.begin(19200);
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
  Serial.println("Starting ...");

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
  //digitalWrite(WIFI_EN, LOW);
  digitalWrite(FAN_EN, LOW);
  pinMode(WIFI_EN, OUTPUT);
  digitalWrite(WIFI_EN, HIGH);
  delay(1000);
  Serial.begin(57600);
  mySerial.begin(9600);
  Wire.begin();


  int check_variable = readEEPROM(EEP0, CHECK_SETUP_INDEX);
  if (check_variable != 57) {
    do_once();
    writeEEPROM(EEP0, CHECK_SETUP_INDEX, 57);
  }

  // make dummy data to send
  //long input[] = {145.2, 3.45, 5.2, 350.0, 405.0};
  //  for (int i = 0; i < 6; i++) {
  //    writeEEPROMdouble(EEP0, 64 + 2 * i, input[i]);
  //  }

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

  set_afe(10, O3, 1); // (pin number, sensor type, index#)
  set_afe(11, H2S, 2);
  set_afe(12, SO2, 3);
  set_afe(13, NO2, 4);
  // set clock
  delay(50);
  rtc_write_date(0, 0, 0, 0, 0, 0, 0);
  rtc_read_timestamp(0);
  delay(50);

  Serial.println("setup completed");
  digitalWrite(WIFI_EN, LOW);
}

void loop() // run over and over
{ long toc = millis();

  //----------------fan on-----------------
  digitalWrite(FAN_EN, HIGH);  delay(FAN_INTERVAL);
  // turn fan off and wait for anything trapped in inductive coils to discharge
  digitalWrite(FAN_EN, LOW);  delay(2000);
  ////-----------take readings------------------

  read_data(); // note: updates data_array
  // sensor 1, sensor 2, sensor 3, sensor4, temp, rh, temp, rh temp, rh,

  // note: have 4 more bytes, maybe can add timestamp?
  Serial.println("Data taken is...");
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    Serial.println(data_array[i]);
  }
  //long data = data_array;
  //long data[6] = {145.2, 3.45, 5.2, 350.0, 405.0}; // t, h, battery, o, n, s, h
  //char types[] = {"oonnsshhtrtrtr"};
  ////-----------save readings------------------
  Serial.println("Writing data");
  // note: should have that eeprom_write_location = loop_counter*32
  // pick which eeprom memory to go to
  // first case: eeprom write location less than size of 1st eepro
  if (eeprom_write_location < (1024.0 * 128.0 - 64.0)) {
    device = EEP0;
  }
  // second case: eeprom write location needs to move to eeprom2
  else if (eeprom_write_location < (1024.0 * 128.0)) {
    device = EEP1;
    eeprom_write_location = 0;
  }
  // third case: eeprom write location needs to move back to eeprom1
  else {
    device = EEP0;
    eeprom_write_location = 64;
  }

  for (int i = 0; i < DATA_ARRAY_SIZE; i += 1) {
    int save_location = eeprom_write_location + i * 2 + loop_counter * EEPROM_BLOCKSIZE ;
    //Serial.println(save_location);
    writeEEPROMdouble(device, save_location , (data_array[i] + 32768)); // note: save as signed integer
  }

  if (loop_counter > 2) { //loop_minimum) {
    ////-----------pull readings------------------
    for (int reading_counter = loop_counter; reading_counter > 1; reading_counter--) {
      //// first pull from EEPROM
      Serial.println("Reading from EEPROM...");
      long one_reading_array[EEPROM_BLOCKSIZE];
      for (int i = 0; i < DATA_ARRAY_SIZE * 2; i += 2) {
        byte two_e = readEEPROM(EEP0, eeprom_write_location - reading_counter * EEPROM_BLOCKSIZE + i);
        byte one_e = readEEPROM(EEP0, eeprom_write_location - reading_counter * EEPROM_BLOCKSIZE + i + 1);
        one_reading_array[i / 2] = ((two_e << 0) & 0xFF) + ((one_e << 8) & 0xFFFF) - 32768; //readEEPROMdouble(EEP0,  64 + i + loop_counter * 32);
      }

      Serial.println("Data from EEPROM is...");
      for (int i = 0; i < 17; i++) {
        Serial.println(one_reading_array[i]);
      }
      // check time and delay so that all boxes aren't uploading at once
      if (DEBUG_MODE != 1) {
        rtc_read_timestamp(0);
        int minute = integer_time[1];
        while (minute - (int) SERIAL_ID > 15) {
          Serial.println("Delaying...");
          delay(60000);
          rtc_read_timestamp(0);
          minute = integer_time[1];
        }
      }
      // turn on wifi
      digitalWrite(WIFI_EN, HIGH);
      delay(20000);
      //// check serial messages
      if (mySerial.available()) {
        while (mySerial.available()) {
          Serial.write(mySerial.read());
        }
      }
      //// send data to AWS
      char cbuf[25];
      for (int i = 0; i < DATA_ARRAY_SIZE; i++) { //DATA_ARRAY_SIZE; i++) {
        String s = "";
        s += types[i];
        if ( one_reading_array[i] == -32768) {
          s += "NaN"; //"NaN";
        }
        else {
          s += String(one_reading_array[i]);
        }
        //s += "x";
        s.toCharArray(cbuf, MAX_MESSAGE_LENGTH);
        Serial.println("data to be posted is...") ;
        Serial.println(s);
        mySerial.write(cbuf);
        mySerial.flush();
        delay(800);
      }
      String s = "x";
      s.toCharArray(cbuf, MAX_MESSAGE_LENGTH);
      Serial.println("data to be posted is...") ;
      Serial.println(s);
      mySerial.write(cbuf);
      mySerial.flush();

    }
    if (mySerial.available()) {
      while (mySerial.available()) {
        Serial.write(mySerial.read());
      }
    }
    // add: wifi low
    digitalWrite(WIFI_EN, LOW);
    // check for success message
    // if (mySerial.read() == "1"){
    loop_counter = 0;

  }
  else {
    Serial.println("Increasing loop_counter") ;
    loop_counter ++;
  }

  while (millis() - toc < READ_INTERVAL ) {
    delay(1000);
    // deep sleep
  }

  eeprom_write_location = eeprom_write_location + EEPROM_BLOCKSIZE;
  Serial.println("Looping");
  //  Serial.println("loop_counter is : ");
  //  Serial.println(loop_counter);
  //
  //  Serial.println("eeprom write location is : " );
  //  Serial.println(eeprom_write_location);
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
//--------READ DATA------------------
//-----------------------------------
void read_data() {
  stat0.clear();
  stat1.clear();
  stat2.clear();
  stat3.clear();
  long tic = millis();
  // add when we're done debugging
  //while (tic - millis < 1000*3*60){
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
  float a = convert_to_mv(ads.readADC_SingleEnded(0));
  delay(5);
  stat0.add(a);

  a = convert_to_mv(ads.readADC_SingleEnded(1));
  delay(5);
  stat1.add(a);

  a = convert_to_mv(ads.readADC_SingleEnded(2));
  delay(5);
  stat2.add(a);

  a = convert_to_mv(ads.readADC_SingleEnded(3));
  delay(5);
  stat3.add(a);

  // read temperature
  data_array[8] = sht31.readTemperature();
  data_array[9] = sht31.readHumidity();
  data_array[10] = sht32.readTemperature();
  data_array[11] = sht32.readHumidity();
  data_array[12] = hdc1080.readTemperature();

  // read voltage
  float v = analogRead(VOLT);  delay(10);
  // take a second reading b/c first one is often off
  // convert by the ADC range to get the fraction of the full range
  // multiply by the reference voltage to get actual voltage
  // multiply by the VDIV to get the unscaled voltage
  // note: dot on the board is where you can read off this voltage with multimeter, often off to
  data_array[13] = analogRead(VOLT) / 1023.0 * VREF * VDIV;

  rtc_read_timestamp(1); // updates integer_time
  data_array[14] = integer_time[2]; //hour
  data_array[15] = integer_time[1]; //minute

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

int rtc_read_timestamp(int mode)
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
