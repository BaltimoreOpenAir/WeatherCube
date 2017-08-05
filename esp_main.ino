
/* This sample uses PutItem on a DynamoDB table to save the state of a virtual
   temperature sensor.

   For this demo to work you must have keys.h/.ccp files that contain your AWS
   access
  #include "Esp8266AWSImplementations.h"
  #include "AmazonDynamoDBClient.h"
  #include "AWSFoundationalTypes.h"
  keys and define "awsSecKey" and "awsKeyID", a DynamoDB table with the
   name defined by the constant TABLE_NAME with hash and range keys as defined
   by constants HASH_KEY_NAME/RANGE_KEY_NAME. */
#define SERIAL_ID 0
#define NUMBER_SAVES_LOCATION 3 // location of byte where the number of saves is 
#define POSTRATE 10000

#define EEP0 0x50    //Address of 24LC256 eeprom chip
#define EEP1 0x51    //Address of 24LC256 eeprom chip

#define EEPROM_BLOCKSIZE 32


#define DEBUG_MODE 1
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "Esp8266AWSImplementations.h"
#include "AmazonDynamoDBClient.h"
#include "AWSFoundationalTypes.h"
#include "keys.h"

SoftwareSerial esp(0,1); // RX, TX

// Common AWS constants
const char* AWS_REGION = "us-west-2";  // REPLACE with your region
const char* AWS_ENDPOINT = "amazonaws.com";

// Init and connect Esp8266 WiFi to local wlan
//const char* pSSID = "Samsung Galaxy S 5 3120"; // REPLACE with your network SSID (name)
//const char* pPassword = "102867nola"; // REPLACE with your network password (use for WPA, or use as key for WEP)
const char* pSSID = "passionatepurplepurpoise"; // REPLACE with your network SSID (name)
const char* pPassword = "Misfits1807$"; // REPLACE with your network password (use for WPA, or use as key for WEP)

// Constants describing DynamoDB table and values being used
const char* TABLE_NAME = "ESP8266AWSDemo";
const char* HASH_KEY_NAME = "id";
const char* HASH_KEY_VALUE = "ESP01"; // Our sensor ID, to be REPLACED in case of multiple sensors.
const char* RANGE_KEY_NAME = "timest";


/* Temperature reading. */
int reading;

Esp8266HttpClient httpClient;
Esp8266DateTimeProvider dateTimeProvider;

AmazonDynamoDBClient ddbClient;

/* Reused objects. */
GetItemInput getItemInput;
PutItemInput putItemInput;
AttributeValue hashKey;
AttributeValue rangeKey;
ActionError actionError;


void setup() {
  Wire.begin();

  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("\n started");
  // esp.begin(9600);
  Serial.println("writing some dummy eeprom data");
  for (int i = 0; i < 15; i++) {
    writeEEPROM(EEP0, i, 32);
    delay(10);
  }

  Serial.println("try reading the data");
  for (int i = 0; i < 15; i++) {
    Serial.print(readEEPROM(EEP0, i));
    Serial.print(", ");
  }

  if (DEBUG_MODE == 1) {
    /* Begin serial communication. */
    Serial.begin(9600);
    Serial.println();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(pSSID);
  }
  else {
  }

  WiFi.hostname("ESP8266");
  // If pPassword is set the most secure supported mode will be automatically selected
  WiFi.begin(pSSID, pPassword);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    Serial.print("It's great!");
    delay(500);
  }

  Serial.println();

  /* Initialize ddbClient. */
  ddbClient.setAWSRegion(AWS_REGION);
  ddbClient.setAWSEndpoint(AWS_ENDPOINT);
  ddbClient.setAWSSecretKey(awsSecKey);
  ddbClient.setAWSKeyID(awsKeyID);
  ddbClient.setHttpClient(&httpClient);
  ddbClient.setDateTimeProvider(&dateTimeProvider);
}

void putTemp(int temp) {

  /* Create an Item. */
  AttributeValue id;
  id.setS(HASH_KEY_VALUE);
  AttributeValue timest;
  timest.setN(dateTimeProvider.getDateTime());

  /* Create an AttributeValue for 'temp', convert the number to a
     string (AttributeValue object represents numbers as strings), and
     use setN to apply that value to the AttributeValue. */
  char numberBuffer[4];
  AttributeValue tempAttributeValue;
  sprintf(numberBuffer, "%d", temp);
  tempAttributeValue.setN(numberBuffer);

  /* Create the Key-value pairs and make an array of them. */
  MinimalKeyValuePair < MinimalString, AttributeValue
  > att1(HASH_KEY_NAME, id);
  MinimalKeyValuePair < MinimalString, AttributeValue
  > att2(RANGE_KEY_NAME, timest);
  MinimalKeyValuePair < MinimalString, AttributeValue
  > att3("temp", tempAttributeValue);
  MinimalKeyValuePair<MinimalString, AttributeValue> itemArray[] = { att1,
                                                                     att2, att3
                                                                   };

  /* Set values for putItemInput. */
  putItemInput.setItem(MinimalMap < AttributeValue > (itemArray, 3));
  putItemInput.setTableName(TABLE_NAME);

  /* Perform putItem and check for errors. */
  PutItemOutput putItemOutput = ddbClient.putItem(putItemInput,
                                actionError);
  switch (actionError) {
    case NONE_ACTIONERROR:
      Serial.println("PutItem succeeded!");
      break;
    case INVALID_REQUEST_ACTIONERROR:
      Serial.print("ERROR: Invalid request");
      Serial.println(putItemOutput.getErrorMessage().getCStr());
      break;
    case MISSING_REQUIRED_ARGS_ACTIONERROR:
      Serial.println(
        "ERROR: Required arguments were not set for PutItemInput");
      break;
    case RESPONSE_PARSING_ACTIONERROR:
      Serial.println("ERROR: Problem parsing http response of PutItem");
      break;
    case CONNECTION_ACTIONERROR:
      Serial.println("ERROR: Connection problem");
      break;
  }
  /* wait to not double-record */
  delay(750);

}

void loop() {
  //Wire.begin();
  reading = esp.read();

  //reading = random(20, 30);
  Serial.print("Temperature = ");
  Serial.println(reading);
  putTemp(reading);

  delay(2000);
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
