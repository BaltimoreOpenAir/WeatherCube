//----------------PREAMBLE-----------------
#define SERIAL_ID 17
#define GAIN_INDEX 2  //defines ADC range -- 1 for 3.3V, [2 for 2.048V], 3 for 1.024V
#define MV_RATIO 0.0625
#define ADC_GAIN GAIN_TWO
#define DEBUG_MODE 0
#define DO_SETUP_TEST_POST false
#define STAT_COUNT 10
#define ADC_CHANNEL_COUNT 4
#define FREQUENCY 2000// sampling frequency in ms
#define PM true
#define SD_CS   10
#define PMS_SET A2
#define PMS_RESET A1
#define HW_BAUD 57600
//#define ESP_BAUD 9600
#define PMS_BAUD 9600
#define MAX_MESSAGE_LENGTH 120

#define start_second 0
#define start_minute 59
#define start_hour 21
#define start_day_of_week 5 //Sunday is 0, Saturday 7
#define start_day 13
#define start_month 4
#define start_year 18

//bool _reset = false; // whether to reset eeprom; always rerun
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

//#include "keys.h" // wifi passwords, amazon table details, serial id

//--------------- definitions-----------------
#define SEND_HOUR 13
#define FAN_INTERVAL 30000//30000
//#define READ_INTERVAL 180//180000//60000 // change for debug
//#define SLEEP_INTERVAL 600 //600000
#define RTC_ADDR 0x6F
#define RTC_TS_BITS 7
#define TIME_REG 0x00
//#define EEP0 0x50    //Address of 24LC256 eeprom chip
//#define EEP1 0x51    //Address of 24LC256 eeprom chip

#define ID_LOCATION 0 // where we save the id
#define CHECK_SETUP_INDEX 2 // where we save whether or not we've run the do_once setup functions, including setting id and setting clock
#define CHECK_SEND_LOCATION 4// where we save if the data successfully send 
#define LOOP_COUNTER_LOCATION 5 // where we log the number of data points not sent, or loop_counter
//#define EEP_WRITE_LOCATION_INDEX 6// where we log where the counter is; keep rolling it over to conserve lifetime of eeprom memory
//#define START_WRITE_LOCATION 64 // 2nd page
//#define MAX_MESSAGE_LENGTH 25

#define VDIV 5.02   //voltage divider: (1 + 4.02) / 1
#define VREF 1.1    //value of ATMega328 internal reference voltage
#define VREF_EN 4
#define WIFI_EN 8
#define FAN_EN 9
#define VOLT A0

// numeric code to index the array LMP91000
#define CO    0
//#define EtOH  1
//#define H2S   2
//#define SO2   3
//#define NO2   4
#define O3    1
#define IAQ   2
#define RESP  3

#define HDC_ADDRESS 0x40

#define PMS_DATA_INDEX 16
#define DATA_ARRAY_SIZE 32  //16 for AFE, 16 for PMS
//#define EEPROM_BLOCKSIZE 36
#define TOTAL_MEASUREMENT_TIME 1 // In minutes
#define SLEEP_MINUTES 0//112
#define MAX_POST_TRIES 10


//int gain_index = GAIN_INDEX;
//float mv_adc_ratio = 0.0;
//adsGain_t gain[6] = {GAIN_TWOTHIRDS, GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT, GAIN_SIXTEEN};
//float mv_ratios[6] = {0.1875, 0.125, 0.0625, 0.03125, 0.015625, 0.0078125};
adsGain_t adc_pga_gain;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
//Adafruit_SHT31 sht32 = Adafruit_SHT31();
ClosedCube_HDC1080 hdc1080;
LMP91000 lmp91000;
Adafruit_ADS1115 ads(0x49);

// initialize wifi serial connection
//SoftwareSerial esp(5, 6); // RX, TX
SoftwareSerial pms(3, 7);

//From D-ULPSM Rev 0.3 LMP91000 Settings //need to set TIACN register, REFCN register, MODECN register
// this is a 2-d array because the settings need to be split between different bytes
int LMP91000_settings[4][2] = {
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_1PCT},  //CO
  //{LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_4PCT}, //EtOH
  //{LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_0PCT},  //H2S
  //{LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_10PCT}, //SO2
  //{LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}, //NO2  //INT_Z 20
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_1PCT},  //O3   //INT_Z 20
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_8PCT},  //IAQ
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}  //RESP
};
int MODECN_DEFAULT = LMP91000_FET_SHORT_DISABLED | LMP91000_OP_MODE_AMPEROMETRIC;

// declaring instances of objects in the statistic class
Statistic stat[10];

char inbyte;
//byte cbuf[MAX_MESSAGE_LENGTH];
char* _test;

long post_index = 0;
bool running_fan = true;
bool debug_set_rtc = false; // run the clock debug mode, sets clock and prints it
bool fan_on;
long read_interval, fan_interval;
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

int afe_note_code = -9999, pms_note_code = -9999;
bool logfile_error = false;
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

void setup()
{
  Serial.begin(HW_BAUD);
  Serial.println(F("Starting setup..."));

  //--------------------pin stuff----------------------
  analogReference(INTERNAL);
  pinMode(VOLT, INPUT);
  pinMode(VREF_EN, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(WIFI_EN, OUTPUT);
  pinMode(FAN_EN, OUTPUT);
  pinMode(PMS_SET, OUTPUT);
  pinMode(PMS_RESET, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(VREF_EN, LOW);
  digitalWrite(FAN_EN, LOW);
  digitalWrite(PMS_SET, HIGH);
  digitalWrite(PMS_RESET, HIGH);
  digitalWrite(WIFI_EN, LOW);
  Serial.println(F("finished pin setup"));
  delay(100);

  //esp.begin(ESP_BAUD);
  pms.begin(PMS_BAUD);
  Wire.begin();

   //----------------SD card-------------------------
  if (!SD.begin(SD_CS)) {
    logfile_error = true;
    Serial.println(F("init failed!"));
  }
  else{
    Serial.println(F("init done."));
    /*File logfile = SD.open(F("logfile.txt"));
    if (logfile) {
      while (logfile.available()) {
        Serial.write(logfile.read());
      }
      logfile.close();
    } else {
      Serial.println("error opening logfile.txt");
    }*/
  }
  delay(100);

  //----------------set adc/afe stuff------------------
  //set_adc_gain(gain_index);
  ads.setGain(ADC_GAIN);
  ads.begin();
  set_afe(0, CO, 1); // (pin number, sensor type, index#)
  set_afe(1, O3, 2);
  set_afe(2, IAQ, 3);
  set_afe(3, RESP, 4);
  delay(50);

  //----------------read (/set) timestamp--------------
  
  //rtc_write_date(start_second, start_minute, start_hour, start_day_of_week, start_day, start_month, start_year); //uncomment to manually update RTC timestamp
  rtc_read_timestamp(1);
  Serial.print(F("Timestamp: "));
  Serial.print(integer_time[2]); Serial.print(':');
  Serial.print(integer_time[1]); Serial.print(':');
  Serial.print(integer_time[0]); Serial.print(", ");
  Serial.print(integer_time[5]); Serial.print('/');
  Serial.print(integer_time[4]); Serial.print('/');
  Serial.print(integer_time[6]); Serial.println();

  //----------------turn on thermometers--------------
  sht31.begin(0x44);
  //sht32.begin(0x45);
  hdc1080.begin(0x40);

  //----------------wrap-up---------------------------
  Serial.println(F("setup completed"));
  delay(3000);
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

void loop()
{
  digitalWrite(VREF_EN, HIGH);
  long toc = millis();
  acquire_data();  //Serial.println(millis() - toc);
  process_data();  //Serial.println(millis() - toc);
  //digitalWrite(VREF_EN, LOW);
  while ( millis() - toc < FREQUENCY - 100) {delay(10);}
  while( millis() - toc < FREQUENCY){;}
}

void acquire_data()
{
  digitalWrite(FAN_EN, HIGH); 
  // delay(FAN_INTERVAL);
  //digitalWrite(FAN_EN, LOW);  delay(2000);
  read_afe_data();
  read_pms_data();
  digitalWrite(FAN_EN, LOW);
}

void process_data()
{
  char nbuf[12];
  rtc_read_timestamp(1);
  int dy = integer_time[4];
  int mon = integer_time[5];
  int yr = integer_time[6];  
  if(mon < 10 && dy < 10){
    sprintf(nbuf, "F%d0%d0%d.txt", yr, mon, dy);
  }else if(mon < 10 && dy >= 10){
    sprintf(nbuf, "F%d0%d%d.txt", yr, mon, dy);
  }else if(mon >= 10 && dy < 10){
    sprintf(nbuf, "F%d0%d%d.txt", yr, mon, dy);
  }
  File myFile = SD.open(nbuf, FILE_WRITE);
  //myFile.print(nbuf);
  Serial.print(nbuf);
  Serial.print(": ");
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    Serial.print(data_array[i]);
    myFile.print(data_array[i]);
    if(i < DATA_ARRAY_SIZE - 1){
      Serial.print(',');
      myFile.print(',');
    }
    else{
      Serial.println();
      myFile.println();
    }
  }
  myFile.close();
}

