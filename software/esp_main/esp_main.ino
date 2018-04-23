#define O3    0
#define O3_STD 1
#define NO2   2
#define NO2_STD   3
#define SO2   4
#define SO2_STD   5
#define H2S   6
#define H2S_STD   7
#define TEMP1 8
#define HUM1  9
#define TEMP2 10
#define HUM2  11
#define TEMP3 12
#define HUM3  13
#define BATT  14
#define HOUR  15
#define MONTH   16
#define READS   17
#define FIELDS_COUNT 18

//#define SERIAL_ID 0
//#define NUMBER_SAVES_LOCATION 3 // location of byte where the number of saves is 
#define POSTRATE 10000
#define MAX_MESSAGE_LENGTH 35
#define DEBUG_MODE 1

#include "ESP8266WiFi.h"
#include "Esp8266AWSImplementations.h"
#include "AmazonDynamoDBClient.h"
#include "AWSFoundationalTypes.h"
#include "keys.h"

// debug mode 
bool debug_mode = false; 

// Common AWS constants
const char* AWS_REGION = "us-west-2";  // REPLACE with your region
const char* AWS_ENDPOINT = "amazonaws.com";

// Init and connect Esp8266 WiFi to local wlan
//const char* pSSID = "Samsung Galaxy S 5 3120"; // REPLACE with your network SSID (name)
//const char* pPassword = "102867nola"; // REPLACE with your network password (use for WPA, or use as key for WEP)
const char* pSSID = "[NETWORK]"; // REPLACE with your network SSID (name)
const char* pPassword = "[PASSWORD]"; // REPLACE with your network password (use for WPA, or use as key for WEP)
const char* SERIAL_ID = "17";

// Constants describing DynamoDB table and values being used
const char* TABLE_NAME =  "BaltimoreOpenAir2017"; // "ESP8266AWSDemo";//
const char* HASH_KEY_NAME = "id";
//const char* HASH_KEY_VALUE = SERIAL_ID; // Our sensor ID, to be REPLACED in case of multiple sensors.
const char* RANGE_KEY_NAME = "timest";

//for reading in serial streams
char cbuf[MAX_MESSAGE_LENGTH];
char inbyte;
int counter;

//wifi connection bools
bool wifi_connected = false;
bool did_dummy_post = false;

Esp8266HttpClient httpClient;
Esp8266DateTimeProvider dateTimeProvider;
AmazonDynamoDBClient ddbClient;

// Reused AWS objects.
GetItemInput getItemInput;
PutItemInput putItemInput;
AttributeValue hashKey;
AttributeValue rangeKey; 
ActionError actionError;
AttributeValue tempAV1;
//AttributeValue o3AV_avg, o3AV_std, no2AV_avg, no2AV_std, so2AV_avg, so2AV_std, h2sAV_avg, h2sAV_std, tempAV1, humAV1, tempAV2, humAV2, tempAV3, humAV3, battAV, hour, month, reads; // o3AV, no2AV, so2AV, h2sAV;
AttributeValue attributes[FIELDS_COUNT];// = {o3AV_avg, o3AV_std, no2AV_avg, no2AV_std, so2AV_avg, so2AV_std, h2sAV_avg, h2sAV_std, tempAV1, humAV1, tempAV2, humAV2, tempAV3, humAV3, battAV, hour, month, reads};
char attributes_names[][FIELDS_COUNT] = {"O3_avg", "O3_std", "NO2_avg", "NO2_std", "SO2_avg", "SO2_std", "H2S_avg", "H2S_std", "temp1", "hum1", "temp2", "hum2", "temp3", "hum3", "battAV", "hour", "month", "reads"};

AttributeValue id;
AttributeValue timest;
MinimalKeyValuePair <MinimalString, AttributeValue> itemArray[17];// = {att1, att2, att3, att4, att5, att6, att7, att8, att9, att10, att11, att12, att13, att14, att15, att16, att17};

void setup() {
  Serial.begin(9600);
  Serial.println("testing");
  
  for(int i = 0; i < FIELDS_COUNT; i++){
    attributes[i].setS("-9999");
  }
  WiFi.hostname("ESP8266");
  WiFi.begin(pSSID, pPassword);
  delay(100);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //Serial.print("Not connected. ");
    delay(900);
  }
  Serial.println("Connected!") ;

  //wait a short period for the wifi to connect
  //we'll take arguments from the atmega in the meantime if the connection process is slow
  check_or_wait_for_wifi(1000);
  Serial.println("Wifi setup completed");
}

void loop() {
  if (Serial.available() > 0) { // something came across serial
    delay(10);
    inbyte = -1;
    int counter = 0;
    memset(cbuf, 0, sizeof(cbuf));
    while (inbyte != '\r' && inbyte != 'x' && counter < MAX_MESSAGE_LENGTH) {
      inbyte = Serial.read();
      delay(10);
      if (inbyte == '\r' || inbyte == 'x') {
        char mbuf[MAX_MESSAGE_LENGTH];
        if (cbuf[0] == 't') {
          setParameter(mbuf, TEMP1, debug_mode);
        }
        else if (cbuf[0] == 'o') {
          setParameter(mbuf, O3, debug_mode);
        }
        else if (cbuf[0] == 'a') {
          setParameter(mbuf, O3_STD, debug_mode);
        }
        else if (cbuf[0] == 'n') {
          setParameter(mbuf, NO2, debug_mode);
        }
        else if (cbuf[0] == 'b') {
          setParameter(mbuf, NO2_STD, debug_mode);
        }
        else if (cbuf[0] == 's') {
          setParameter(mbuf, SO2, debug_mode);
        }
        else if (cbuf[0] == 'c') {
          setParameter(mbuf, SO2_STD, debug_mode);
        }
        else if (cbuf[0] == 'h') {
          setParameter(mbuf, H2S, debug_mode);
        }
        else if (cbuf[0] == 'd') {
          setParameter(mbuf, H2S_STD, debug_mode);
        }
        else if (cbuf[0] == 'r') {
          setParameter(mbuf, HUM1, debug_mode);
        }
        else if (cbuf[0] == 'e') {
          setParameter(mbuf, TEMP2, debug_mode);
        }
        else if (cbuf[0] == 'f') {
          setParameter(mbuf, HUM2, debug_mode);
        }
        else if (cbuf[0] == 'g') {
          setParameter(mbuf, TEMP3, debug_mode);
        }
        else if (cbuf[0] == 'i') {
          setParameter(mbuf, HUM3, debug_mode);
        }
        else if (cbuf[0] == 'v') {
          setParameter(mbuf, BATT, debug_mode);
        }
        else if (cbuf[0] == 'H') {
          setParameter(mbuf, HOUR, debug_mode);
        }
        else if (cbuf[0] == 'm') {
          setParameter(mbuf, MONTH, debug_mode);
        }
        else if (cbuf[0] == 'N') {
          setParameter(mbuf, READS, debug_mode);
        }
        // FILL OUT
        else if (cbuf[0] == 'p') {
          if(debug_mode){
            Serial.println("Will try to post...");
            delay(100);
          }
          if (check_or_wait_for_wifi(10000)) {
            if (!did_dummy_post) {
              postToAWS(did_dummy_post, false);
              did_dummy_post = true;
            }
            postToAWS(did_dummy_post, true);
          }
          else {
            if(debug_mode){
              Serial.println("cannot connect to AWS at this time...");
            }
          }
        }
      }
      else if (inbyte == -1 || inbyte < 32 || inbyte > 126) continue;
      else {
        cbuf[counter++] = inbyte;
      }
    }
    Serial.flush();
  }
  delay(200);
}

//-------FUNCTIONS----------------
bool check_or_wait_for_wifi(long max_wait_ms) {
  unsigned long timer = millis();
  if (wifi_connected) return true;
  while (millis() - timer < max_wait_ms) {
    if (WiFi.status() == WL_CONNECTED) {
      wifi_connected = true;
      initialize_wifi();
      return true;
    }
    delay(100);
    Serial.println(millis());
  }
  return false;
}

void initialize_wifi() { //Initialize ddbClient.
  ddbClient.setAWSRegion(AWS_REGION); delay(100);
  ddbClient.setAWSEndpoint(AWS_ENDPOINT); delay(100);
  ddbClient.setAWSSecretKey(awsSecKey); delay(100);
  ddbClient.setAWSKeyID(awsKeyID); delay(100);
  ddbClient.setHttpClient(&httpClient); delay(100);
  ddbClient.setDateTimeProvider(&dateTimeProvider); delay(100);
  //Serial.println("wifi initialized...");
}

void get_message(char m[], char c[]) {
  for (int i = 1; i < MAX_MESSAGE_LENGTH; i++) {
    m[i - 1] = c[i];
  }
}

void setParameter(char m[], int attribute_index, bool verboz) { //parse param value recognized in serial stream
  get_message(m, cbuf);
  MinimalString ms = m;
  attributes[attribute_index].setS(ms);
  if (verboz) {
    Serial.print("set ");
    Serial.print(attributes_names[attribute_index]);
    Serial.print(": ");
    Serial.println(m);
  }
}

void postToAWS(bool not_dummy, bool verboz) {

  /* Create an Item. */
  AttributeValue id;
  id.setS(SERIAL_ID);

//id.setS(HASH_KEY_VALUE);
  AttributeValue timest;
  timest.setN(dateTimeProvider.getDateTime());
  if (not_dummy && verboz) {
    Serial.print("Attempting AWS post at ");
    Serial.println(dateTimeProvider.getDateTime());
  }

  itemArray[0] = MinimalKeyValuePair <MinimalString, AttributeValue>(HASH_KEY_NAME, id);
  itemArray[1] = MinimalKeyValuePair <MinimalString, AttributeValue>(RANGE_KEY_NAME, timest);
  for(int i = 0; i < 15; i++){
    itemArray[i + 2] = MinimalKeyValuePair <MinimalString, AttributeValue>(attributes_names[i], attributes[i]);
  }
  
  /* Set values for putItemInput. */
  putItemInput.setItem(MinimalMap < AttributeValue > (itemArray, 17)); // maximum attribute number, doesn't seem to be zero indexed??
  putItemInput.setTableName(TABLE_NAME);

  /* Perform putItem and check for errors. */
  PutItemOutput putItemOutput = ddbClient.putItem(putItemInput,actionError);
  switch (actionError) {
    //response codes simple, but unlikely to randomly occur in i/o stream, for atmega parsing
    case NONE_ACTIONERROR:
      if (not_dummy) Serial.println("#S");
      break;
    case INVALID_REQUEST_ACTIONERROR:
      Serial.println("#E1");//("ERROR: Invalid request");
      //Serial.println(putItemOutput.getErrorMessage().getCStr());
      break;
    case MISSING_REQUIRED_ARGS_ACTIONERROR:
      Serial.println("#E2");//("ERROR: Required arguments were not set for PutItemInput");
      break;
    case RESPONSE_PARSING_ACTIONERROR:
      Serial.println("#E3");//("ERROR: Problem parsing http response of PutItem");
      break;
    case CONNECTION_ACTIONERROR:
      Serial.println("#E4");//("ERROR: Connection problem");
  }
}



