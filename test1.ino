// ESP
#define MAX_MESSAGE_LENGTH 6
#include <ESP8266WiFi.h>
#include "Esp8266AWSImplementations.h"
#include "AmazonDynamoDBClient.h"
#include "AWSFoundationalTypes.h"
#include "keys.h"

char inbyte;
char message[MAX_MESSAGE_LENGTH];
bool wifi_connected = false;
bool did_dummy_post = false;

Esp8266HttpClient httpClient;
Esp8266DateTimeProvider dateTimeProvider;
AmazonDynamoDBClient ddbClient;

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

// Reused AWS objects.
GetItemInput getItemInput;
PutItemInput putItemInput;
AttributeValue hashKey;
AttributeValue rangeKey;
ActionError actionError;
void setup() {
  Serial.begin(9600);
  // define below code as connectwifi() function
  WiFi.hostname("ESP8266");
  // If pPassword is set the most secure supported mode will be automatically selected
  WiFi.begin(pSSID, pPassword);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    Serial.print("Not connected. ");
    delay(900);
  }

    /* Initialize ddbClient. */
  ddbClient.setAWSRegion(AWS_REGION);
  ddbClient.setAWSEndpoint(AWS_ENDPOINT);
  ddbClient.setAWSSecretKey(awsSecKey);
  ddbClient.setAWSKeyID(awsKeyID);
  ddbClient.setHttpClient(&httpClient);
  ddbClient.setDateTimeProvider(&dateTimeProvider);
  
  Serial.println("Posting test message 0");
  
  post_0();                         
  memset(message, 0, sizeof(message));

}

void loop() {
 Serial.println("Trying to post string"); 
 post_str("try two"); 

if (Serial.available() > 0) {
 
 Serial.println("Reading in message from ATMEGA");
 int counter = 0; 
 while (inbyte != '\r' && inbyte != 'x' && counter < MAX_MESSAGE_LENGTH) {
      inbyte = Serial.read();
      delay(10); 
      message[counter] = inbyte; 
 }
 post_str(message); 
}

Serial.write("Test"); 
Serial.flush();
}

void post_str(char* input_message){ 
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
  MinimalString msg = input_message; 
  tempAttributeValue.setS(msg);

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
}


void post_0(){ 
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
  sprintf(numberBuffer, "%d", int(0));
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
}
