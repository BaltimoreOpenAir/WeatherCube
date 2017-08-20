#define O3    0
#define NO2   2
#define SO2   4
#define H2S   6
#define TEMP1 8
#define HUM1  9
#define TEMP2 10
#define HUM2  11
#define TEMP3 12
#define BATT  13
#define HOUR  14
#define MIN   15


//#define SERIAL_ID 0
#define NUMBER_SAVES_LOCATION 3 // location of byte where the number of saves is 
#define POSTRATE 10000
#define MAX_MESSAGE_LENGTH 25
#define DEBUG_MODE 1

#include <ESP8266WiFi.h>
#include "Esp8266AWSImplementations.h"
#include "AmazonDynamoDBClient.h"
#include "AWSFoundationalTypes.h"
#include "keys.h"

// Common AWS constants
const char* AWS_REGION = "us-west-2";  // REPLACE with your region
const char* AWS_ENDPOINT = "amazonaws.com";

// Init and connect Esp8266 WiFi to local wlan
//const char* pSSID = "Samsung Galaxy S 5 3120"; // REPLACE with your network SSID (name)
//const char* pPassword = "102867nola"; // REPLACE with your network password (use for WPA, or use as key for WEP)
//const char* pSSID = "FiOS-V2Y8G"; // REPLACE with your network SSID (name)
//const char* pPassword = "few736cage4414drip"; // REPLACE with your network password (use for WPA, or use as key for WEP)

// Constants describing DynamoDB table and values being used
const char* TABLE_NAME = "ESP8266AWSDemo";
const char* HASH_KEY_NAME = "id";
const char* HASH_KEY_VALUE = "ESP01"; // Our sensor ID, to be REPLACED in case of multiple sensors.
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

AttributeValue O3AV_avg, O3AV_std, NO2AV_avg, NO2AV_std, SO2AV_avg, SO2AV_std, H2SAV_avg, H2SAV_std, tempAV1, humAV1, tempAV2, humAV2, tempAV3, battAV, hour, minute; // o3AV, no2AV, so2AV, h2sAV;
//AttributeValue attributes[] = {tempAV, humAV, battAV, o3AV, no2AV, so2AV, h2sAV};
AttributeValue attributes[] = {O3AV_avg, O3AV_std, NO2AV_avg, NO2AV_std, SO2AV_avg, SO2AV_std, H2SAV_avg, H2SAV_std, tempAV1, humAV1, tempAV2, humAV2, tempAV3, battAV, hour, minute};
//char attributes_names[][5] = {"temp", "hum", "batt", "o3", "no2", "so2", "h2s"};
char attributes_names[][14] = {"O3_avg", "O3_std", "NO2_avg", "NO2_std", "SO2_avg", "SO2_std", "H2S_avg", "H2S_std", "temp1", "hum1", "temp2", "hum2", "temp3", "battAV", "hour", "minute"};

void setup() {
  Serial.begin(9600);
  Serial.println("testing");
  tempAV1.setS("-9999");
  WiFi.hostname("ESP8266");
  WiFi.begin(pSSID, pPassword);
  delay(100);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    Serial.print("Not connected. ");
    delay(900);
  }
  Serial.println("Connected!") ;

  //wait a short period for the wifi to connect
  //we'll take arguments from the atmega in the meantime if the connection process is slow
  check_or_wait_for_wifi(1000);
  Serial.println("Setup completed");
}

void initialize_wifi() { //Initialize ddbClient.
  ddbClient.setAWSRegion(AWS_REGION);
  ddbClient.setAWSEndpoint(AWS_ENDPOINT);
  ddbClient.setAWSSecretKey(awsSecKey);
  ddbClient.setAWSKeyID(awsKeyID);
  ddbClient.setHttpClient(&httpClient);
  ddbClient.setDateTimeProvider(&dateTimeProvider);
  //Serial.println("wifi initialized...");
}

bool check_or_wait_for_wifi(int max_wait_ms) {
  unsigned long timer = millis();
  if (wifi_connected) return true;
  while (millis() - timer < max_wait_ms) {
    if (WiFi.status() == WL_CONNECTED) {
      wifi_connected = true;
      initialize_wifi();
      return true;
    }
  }
  return false;
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
        //char mbuf[MAX_MESSAGE_LENGTH];
        if (cbuf[0] == 't') {
          setParameter(mbuf, TEMP1, true);
        }
        else if (cbuf[0] == 'n'){
          setParameter(mbuf, NO2, true); 
        }
        else if (cbuf[0] == 's'){
          setParameter(mbuf, SO2, true); 
        }
        else if (cbuf[0] == 'h'){
          setParameter(mbuf, H2S, true); 
        }
        // FILL OUT 
        else if (cbuf[0] == 'p') {
          if (check_or_wait_for_wifi(10000)) {
            if (!did_dummy_post) {
              postToAWS(did_dummy_post, false);
              did_dummy_post = true;
            }
            postToAWS(did_dummy_post, true);
          }
          else {
            Serial.println("cannot connect to AWS at this time...");
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
void get_message(char m[], char c[]) {
  for (int i = 2; i < MAX_MESSAGE_LENGTH; i++) {
    m[i - 2] = c[i];
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
  id.setS(HASH_KEY_VALUE);
  AttributeValue timest;
  timest.setN(dateTimeProvider.getDateTime());
  if (not_dummy && verboz) {
    Serial.print("Attempting AWS post at ");
    Serial.println(dateTimeProvider.getDateTime());
  }

  /* Create the Key-value pairs and make an array of them. */
  MinimalKeyValuePair <MinimalString, AttributeValue> att1(HASH_KEY_NAME, id);
  MinimalKeyValuePair <MinimalString, AttributeValue> att2(RANGE_KEY_NAME, timest);
  MinimalKeyValuePair <MinimalString, AttributeValue> att3("o3_avg", O3AV_avg);
  MinimalKeyValuePair <MinimalString, AttributeValue> att4("o3_std", O3AV_std);
  MinimalKeyValuePair <MinimalString, AttributeValue> att5("no2_avg", NO2AV_avg);
  MinimalKeyValuePair <MinimalString, AttributeValue> att6("no2_std", NO2AV_std);
  MinimalKeyValuePair <MinimalString, AttributeValue> att7("so2_avg", SO2AV_avg);
  MinimalKeyValuePair <MinimalString, AttributeValue> att8("so2_std", SO2AV_std);
  MinimalKeyValuePair <MinimalString, AttributeValue> att9("h2s_avg", H2SAV_avg);
  MinimalKeyValuePair <MinimalString, AttributeValue> att10("h2s_std", H2SAV_std);

  MinimalKeyValuePair <MinimalString, AttributeValue> att11(attributes_names[TEMP1], attributes[TEMP1]);
  MinimalKeyValuePair <MinimalString, AttributeValue> att12(attributes_names[HUM1], attributes[HUM1]);
  MinimalKeyValuePair <MinimalString, AttributeValue> att13(attributes_names[TEMP2], attributes[TEMP2]);
  MinimalKeyValuePair <MinimalString, AttributeValue> att14(attributes_names[HUM2], attributes[HUM2]);
  MinimalKeyValuePair <MinimalString, AttributeValue> att15(attributes_names[TEMP3], attributes[TEMP3]);

  MinimalKeyValuePair <MinimalString, AttributeValue> att16(attributes_names[BATT], attributes[BATT]);
  MinimalKeyValuePair <MinimalString, AttributeValue> att17(attributes_names[HOUR], attributes[HOUR]);
  MinimalKeyValuePair <MinimalString, AttributeValue> att18(attributes_names[MIN], attributes[MIN]);

  //change these to match TEMP above
//  MinimalKeyValuePair <MinimalString, AttributeValue> att4("batt", battAV);
//  MinimalKeyValuePair <MinimalString, AttributeValue> att5("o3_avg", o3AV_avg);
//  MinimalKeyValuePair <MinimalString, AttributeValue> att6("no2", no2AV);
//  MinimalKeyValuePair <MinimalString, AttributeValue> att7("so2", so2AV);
//  MinimalKeyValuePair <MinimalString, AttributeValue> att8("h2s", h2sAV);
  MinimalKeyValuePair <MinimalString, AttributeValue> itemArray[] = {att1, att2, att3, att4, att5, att6, att7, att8, att9, att10, att11, att12, att13, att14, att15, att16}; //, att4, .....

  /* Set values for putItemInput. */
  putItemInput.setItem(MinimalMap < AttributeValue > (itemArray, 16));
  putItemInput.setTableName(TABLE_NAME);

  /* Perform putItem and check for errors. */
  PutItemOutput putItemOutput = ddbClient.putItem(putItemInput,
                                actionError);
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
