//----------------PREAMBLE-----------------
#define SERIAL_ID 17

#define DEBUG_MODE 0
#define DO_SEND_DATA true
#define DO_SETUP_TEST_POST true
#define DEBUG_CONTINUOUS false
#define DEBUG_FAN_OFF true
#define VERBOSE_SERIAL true

#define MEASUREMENT_SECONDS 60
#define SLEEP_MINUTES 10
#define MAX_POST_TRIES 10

#define HUMTEMP_READS 10
#define STAT_COUNT 10
#define GAIN_INDEX 2  //defines ADC range -- 1 for 3.3V, [2 for 2.048V], 3 for 1.024V
#define ADC_CHANNEL_COUNT 4
#define USE_COLUMN_SENSORS false

int start_second = 0;
int start_minute = 59;
int start_hour = 21;
int start_day_of_week = 5; //Sunday is 0, Saturday 7
int start_day = 13;
int start_month = 4;
int start_year = 18;

int last_hour = -1;
int unreported_reads = 0;
int cache = 0;

bool _reset = false; // whether to reset eeprom; always rerun
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

//--------------- definitions-----------------
#define SEND_HOUR 13
#define FAN_INTERVAL 30000//30000
#define RTC_ADDR 0x6F
#define RTC_TS_BITS 7
#define TIME_REG 0x00
#define EEP0 0x50    //Address of 24LC256 eeprom chip
#define EEP1 0x51    //Address of 24LC256 eeprom chip

#define ID_LOCATION 0 // where we save the id
#define CHECK_SETUP_INDEX 2 // where we save whether or not we've run the do_once setup functions, including setting id and setting clock
#define CHECK_SEND_LOCATION 4// where we save if the data successfully send 
#define LOOP_COUNTER_LOCATION 5 // where we log the number of data points not sent, or loop_counter
#define EEP_WRITE_LOCATION_INDEX 6// where we log where the counter is; keep rolling it over to conserve lifetime of eeprom memory
#define START_WRITE_LOCATION 64 // 2nd page 
#define MAX_MESSAGE_LENGTH 25

#define VDIV 5.02   //voltage divider: (1 + 4.02) / 1
#define VREF 1.1    //value of ATMega328 internal reference voltage
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

#define DATA_ARRAY_SIZE 18
#define EEPROM_BLOCKSIZE 36

int gain_index = GAIN_INDEX;
float mv_adc_ratio = 0.0;
adsGain_t gain[6] = {GAIN_TWOTHIRDS, GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT, GAIN_SIXTEEN};
float mv_ratios[6] = {0.1875, 0.125, 0.0625, 0.03125, 0.015625, 0.0078125};
adsGain_t adc_pga_gain;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SHT31 sht32;

ClosedCube_HDC1080 hdc1080;
LMP91000 lmp91000;
Adafruit_ADS1115 ads(0x49);

// initialize wifi serial connection
SoftwareSerial mySerial(5, 6); // RX, TX

//From D-ULPSM Rev 0.3 LMP91000 Settings //need to set TIACN register, REFCN register, MODECN register
// this is a 2-d array because the settings need to be split between different bytes
int LMP91000_settings[8][2] = {
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_1PCT},  //CO
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_4PCT}, //EtOH
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_0PCT},  //H2S
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_10PCT}, //SO2
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}, //NO2  //INT_Z 20
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_1PCT},  //O3   //INT_Z 20
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_20PCT | LMP91000_BIAS_SIGN_POS | LMP91000_BIAS_8PCT},  //IAQ
  {LMP91000_TIA_GAIN_EXT | LMP91000_RLOAD_50OHM, LMP91000_REF_SOURCE_EXT | LMP91000_INT_Z_50PCT | LMP91000_BIAS_SIGN_NEG | LMP91000_BIAS_10PCT}  //RESP
};
int MODECN_DEFAULT = LMP91000_FET_SHORT_DISABLED | LMP91000_OP_MODE_AMPEROMETRIC;

// declaring instances of objects in the statistic class
Statistic stat[10];

long post_index = 0;
bool running_fan = true;
bool debug_set_rtc = false; // run the clock debug mode, sets clock and prints it
bool fan_on;
long read_interval, fan_interval;
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

void setup()
{
  Serial.begin(57600);
  Serial.println(F("Starting setup..."));

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
  digitalWrite(FAN_EN, LOW);
  digitalWrite(WIFI_EN, HIGH);
  delay(100);
  mySerial.begin(9600);
  Wire.begin();

  //rtc_write_date(start_second, start_minute, start_hour, start_day_of_week, start_day, start_month, start_year); //uncomment to manually update RTC timestamp
  
  // to reset...
  if (_reset == true) {
    writeEEPROM(EEP0, CHECK_SETUP_INDEX, 0);
  }
  //
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
    loop_counter = readEEPROM(EEP0, LOOP_COUNTER_LOCATION);
    eeprom_write_location = readEEPROM(EEP0, EEP_WRITE_LOCATION_INDEX);
  }
  Serial.println(F("check variable completed"));

  //----------------set adc/gain stuff------------------
  set_adc_gain(gain_index);
  ads.begin();

  //----------------set afe stuff-----------------------
  set_afe(10, CO, 1); // (pin number, sensor type, index#)
  set_afe(11, O3, 2);
  set_afe(12, IAQ, 3);
  set_afe(13, RESP, 4);
//O3,H2S,SO2,NO2
/*set_afe(0, CO, 1); // (pin number, sensor type, index#)
  set_afe(1, O3, 2);
  set_afe(2, IAQ, 3);
  set_afe(3, RESP, 4);*/

  //----------------set clock---------------------------
  delay(50);

  print_readable_timestamp();

  // turn on thermometers
  hdc1080.begin(0x40);
  sht31.begin(0x44);
  if(USE_COLUMN_SENSORS) {
    sht32 = Adafruit_SHT31();
    sht32.begin(0x45);
  }
  
  if (DO_SETUP_TEST_POST) {test_post();}
  delay(50);
  Serial.println(F("setup completed"));
  digitalWrite(WIFI_EN, LOW);
  delay(3000);
}

void loop() // run over and over
{
  acquire_data();
  process_data();
  delay(500);
  Serial.println(F("loop_counter: "));
  Serial.println(++loop_counter);
}

void acquire_data() {
  digitalWrite(VREF_EN, HIGH);
  //----------------fan on-----------------
  Serial.println(F("fan on"));
  if(!DEBUG_FAN_OFF){digitalWrite(FAN_EN, HIGH);} // delay(FAN_INTERVAL);
  // turn fan off and wait for inductive coils to discharge
  //digitalWrite(FAN_EN, LOW);  delay(2000);
  ////-----------take readings------------------

  Serial.println(F("Taking data..."));
  read_data(); // note: updates data_array
  // sensor 1, sensor 2, sensor 3, sensor4, temp, rh, temp, rh temp, rh,
  digitalWrite(FAN_EN, LOW);
}

void read_data()
{
  int number_reads = 0;
  for (int i = 0; i < STAT_COUNT; i++) {
    stat[i].clear();
  }
  rtc_read_timestamp(1);
  long toc = millis();
  float a = 0;
  float scaler = 1;
  Serial.println(F("air quality reads..."));
  while (millis() - toc < MEASUREMENT_SECONDS * 1000) {
    long toc2 = millis();
    if(VERBOSE_SERIAL) {Serial.print(millis());Serial.print(": ");}
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
      a = ads.readADC_SingleEnded(i); delay(10);
      stat[i].add(scaler * a);
      if(VERBOSE_SERIAL) {Serial.print(scaler * a); Serial.print('\t');}
    }
    if(VERBOSE_SERIAL) Serial.println();

    while (millis() - toc2 < 1000) {
      delay(50);  // make sure we're waiting 1 sec between reads
    }
    ++number_reads;
  }

  scaler = 100;
  //read temp/hum sensors
  Serial.println(F("hum/temp reads..."));
  for (int temp_counter = 0; temp_counter < HUMTEMP_READS; temp_counter++) {
    int i = 0;
    a = hdc1080.readTemperature();
    if(VERBOSE_SERIAL) Serial.print(scaler * a); Serial.print('\t');
    stat[ADC_CHANNEL_COUNT + i++].add(scaler * a);

    a = hdc1080.readHumidity();
    if(VERBOSE_SERIAL) Serial.print(scaler * a); Serial.print('\t');
    stat[ADC_CHANNEL_COUNT + i++].add(scaler * a);

    a = sht31.readTemperature();
    if(VERBOSE_SERIAL) Serial.print(scaler * a); Serial.print('\t');
    stat[ADC_CHANNEL_COUNT + i++].add(scaler * a);

    a = sht31.readHumidity();
    if(VERBOSE_SERIAL) Serial.print(scaler * a); Serial.print('\t');
    stat[ADC_CHANNEL_COUNT + i++].add(scaler * a);

    if(USE_COLUMN_SENSORS){
      a = sht32.readTemperature();
      if(VERBOSE_SERIAL) Serial.print(scaler * a); Serial.print('\t');
      stat[ADC_CHANNEL_COUNT + i++].add(scaler * a);
      
      a = sht32.readHumidity();
      if(VERBOSE_SERIAL) Serial.print(scaler * a);
      stat[ADC_CHANNEL_COUNT + i++].add(scaler * a);
    }
    else{
      stat[ADC_CHANNEL_COUNT + i++].add(0);
      stat[ADC_CHANNEL_COUNT + i++].add(0);
    }
    if(VERBOSE_SERIAL) Serial.println();
  }

  for (int i = 0; i < STAT_COUNT; i++) {
    data_array[i] = stat[i].average();
  }

  int i = 0;
  for (int j = STAT_COUNT; j < STAT_COUNT + ADC_CHANNEL_COUNT; j++) {
    data_array[j] = stat[i++].unbiased_stdev();
  }

  i = 0;
  rtc_read_timestamp(1); // updates integer_time
  data_array[STAT_COUNT + ADC_CHANNEL_COUNT + i++] = scaler * analogRead(VOLT) / 1023.0 * VREF * VDIV;
  data_array[STAT_COUNT + ADC_CHANNEL_COUNT + i++] = 100 * (integer_time[2]) + integer_time[1]; //hour minute
  data_array[STAT_COUNT + ADC_CHANNEL_COUNT + i++] = integer_time[5] * 100 + integer_time[4]; //month day
  data_array[STAT_COUNT + ADC_CHANNEL_COUNT + i++] = number_reads;
}

void process_data()
{
  if(VERBOSE_SERIAL){
    Serial.println(F("Data taken is..."));
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      Serial.print(data_array[i]);
      if(i < DATA_ARRAY_SIZE - 1) {Serial.print(',');}
      else {Serial.println();}
    }
  }
  //---------------save readings------------------
  Serial.println(F("Writing data"));
  // note: should have that eeprom_write_location = loop_counter*32
  // pick which eeprom memory to go to
  // first case: eeprom write location less than size of 1st eepro
  if (eeprom_write_location < (1024.0 * 128.0 - 64.0)) {
    device = EEP0;
    Serial.println("Device is EEP0");
  }
  // second case: eeprom write location needs to move to eeprom2
  else if (eeprom_write_location < (1024.0 * 128.0)) {
    device = EEP1;
    eeprom_write_location = 0;
    Serial.println("Device is EEP1");
  }
  // third case: eeprom write location needs to move back to eeprom1
  else {
    device = EEP0;
    eeprom_write_location = 64;
    Serial.println("Device is EEP0");
  }

  Serial.print("write location is: ");
  Serial.println(eeprom_write_location);
  
  Serial.print(F("from: "));
  Serial.println(eeprom_write_location + 0 * 2); //+ loop_counter * EEPROM_BLOCKSIZE );
  Serial.print(F("to: "));
  Serial.println(eeprom_write_location + DATA_ARRAY_SIZE * 2);  //+ loop_counter * EEPROM_BLOCKSIZE );

  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    int save_location = eeprom_write_location + i * 2;// + loop_counter * EEPROM_BLOCKSIZE ;
    //Serial.print(save_location);
    //if(i < DATA_ARRAY_SIZE - 1){Serial.print(",");}
    //else{Serial.println();}
    writeEEPROMdouble(device, save_location, (data_array[i] + 32768)); // note: save as signed integer
  }

  // deep sleep
  // note: millis() won't count up while in 'deep sleep' mode or idle mode
  rtc_read_timestamp(1);
  int now_min = integer_time[1];
  int last_min = now_min;
  int min_elapsed = 0;
  //NOTE: COULD UPDATE THIS TO INCLUDE SECONDS ELAPSED, FOR MORE ACCURACY
  //print_readable_timestamp();

  long one_reading_array[EEPROM_BLOCKSIZE];
  Serial.println(F("\nData from new reading method is: "));
  //int read_array[EEPROM_BLOCKSIZE];
  getEEPROMChunk(one_reading_array, EEPROM_BLOCKSIZE, eeprom_write_location, device);
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    Serial.print(one_reading_array[i]);
    if(i < DATA_ARRAY_SIZE - 1){Serial.print(",");}
    else{Serial.println();}
  }
  /*for (int i = 0; i < DATA_ARRAY_SIZE * 2; i += 2) {
    long read_location = eeprom_write_location + i; //* EEPROM_BLOCKSIZE + i;
    byte two_e = readEEPROM(device, read_location);
    byte one_e = readEEPROM(device, read_location + 1);
    one_reading_array[i / 2] = ((two_e << 0) & 0xFF) + ((one_e << 8) & 0xFFFF) - 32768; //readEEPROMdouble(EEP0,  64 + i + loop_counter * 32);
    delay(50);
  }

  Serial.println(F("\nData from EEPROM is: "));
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    Serial.print(one_reading_array[i]);
    if(i < DATA_ARRAY_SIZE - 1){Serial.print(",");}
    else{Serial.println();}
  }*/
  
  eeprom_write_location += EEPROM_BLOCKSIZE;
  ++unreported_reads;
  //Serial.println(F("Increasing loop_counter to: "));
  //Serial.println(++loop_counter);
  
  //send_data();
  ++cache;
  if(!DEBUG_CONTINUOUS){
    while (min_elapsed < SLEEP_MINUTES)
    {
      if(VERBOSE_SERIAL){Serial.println(F("deep sleep time..."));}
      delay(100);
      for (int i = 0; i < 7; i++) {
        LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
      }
      LowPower.idle(SLEEP_2S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF); //fine for now...
      last_min = now_min;
      rtc_read_timestamp(1);
      now_min = integer_time[1];
      min_elapsed += now_min - last_min;
      if(now_min < last_min) { min_elapsed += 60; }//so 59 to 0 gives +1 instead of -59
      
      if(VERBOSE_SERIAL){ print_readable_timestamp(); }

      //if(DO_SEND_DATA && cache > 2){
      if (is_reporting_time()) {
        digitalWrite(FAN_EN, LOW);
        delay(500);
        //if(DO_SEND_DATA) {
          Serial.println(F("Time to send data!\nSending..."));
          send_data();
          cache = 0;
        //} 
        Serial.println(F("Going back to sleep..."));
        delay(500);
        for (int i = 0; i < 30; i++) {  // go back to sleep for 4 minutes so we don't double send
          LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                        SPI_OFF, USART0_OFF, TWI_OFF);
        }
        writeEEPROM(EEP0, EEP_WRITE_LOCATION_INDEX, eeprom_write_location);
      //}
      }
    }
  }
  if(VERBOSE_SERIAL){Serial.println(F("Time to take more measurements"));}
}

