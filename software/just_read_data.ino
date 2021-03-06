// just read the data from sensors

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

void setup() {
  Wire.begin();
  Serial.begin(19200);
  // put your setup code here, to run once:
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

  Serial.println("Setup complete");
}

void loop() {
  Serial.println("Reading data");
  read_data() ;
  Serial.println("Data is...");
  for (int i = 0; i < DATA_ARRAY_SIZE; i ++) {
    Serial.println(data_array[i]);
  }

  Serial.println("Writing data");
  for (int i = 0; i < DATA_ARRAY_SIZE * 2; i += 2) {
    writeEEPROMdouble(EEP0, 64 + i + loop_counter * 32, (data_array[i / 2] +32768)); // note: save as signed integer
  }

  if (loop_counter > 0) {
    ////-----------send readings------------------
    for (int reading_counter = 0; reading_counter < loop_counter + 1; reading_counter++) {
      //// first pull from EEPROM
      Serial.println("Reading from EEPROM...");
      long one_reading_array[EEPROM_BLOCKSIZE];
      for (int i = 0; i < DATA_ARRAY_SIZE * 2; i += 2) {
        byte two_e = readEEPROM(EEP0, 64 + i + loop_counter * 32);
        byte one_e = readEEPROM(EEP0, 64 + i + loop_counter * 32 + 1);
        one_reading_array[i / 2] = ((two_e << 0) & 0xFF) + ((one_e << 8) & 0xFFFF) - 32768; //readEEPROMdouble(EEP0,  64 + i + loop_counter * 32);
      }

      Serial.println("Data from EEPROM is...");
      for (int i = 0; i < 13; i++) {
        Serial.println(one_reading_array[i]);
      }
    }

  }
  loop_counter ++;
}

//-----------------FUNCTIONS--------------
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

void read_data() { // edits data_array
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

  for (int channel = 0; channel < 4; channel++) {
    int x = 100;
    if (channel % 4 == 0) {
      float avg = stat0.average();
      float std = stat0.unbiased_stdev();
      data_array[0] = avg * x;
      data_array[1] = std * x;
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
      data_array[4] = avg * x;
      data_array[5] = std * x;
    }
    else if (channel % 4 == 3) {
      float avg = stat3.average();
      float std = stat3.unbiased_stdev();
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



