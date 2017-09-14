#define POSTRATE 10000
#include <ESP8266WiFi.h>
const char WiFiSSID[] = "passionatepurpleporpoise"; //"NETGEAR58";
const char WiFiPSK[] = "misfits1807$"; //"brightowl968";
const char PhantHost[] = "data.sparkfun.com";
const String publicKey = "AJ6QY4gOVmuNADG9ZMYK";
const String privateKey = "rzaMKbWv26crBoP9m6Na";
String httpHeader = "POST /input/" + publicKey + ".txt HTTP/1.1\n" +
                    "Host: " + PhantHost + "\n" +
                    "Phant-Private-Key: " + privateKey + "\n" +
                    "Connection: close\n" + 
                    "Content-Type: application/x-www-form-urlencoded\n";

unsigned long timer = 0;
int counter = 0;
int countdown = 100;

void setup() 
{
  Serial.begin(9600);
  Serial.println("\nstarted");
  connectWiFi();
  Serial.println("\nexiting setup");
}

void loop() 
{
  timer = millis();
  Serial.print("posting to Phant at ");
  Serial.println(timer);
  postToPhant();
  //wait until start of next post cycle, specified by POSTRATE
  while (millis() - timer < POSTRATE)
  {
    delay(1);
  }
}

void connectWiFi()
{
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if ESP8266 connected to WiFi.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Delays allow the ESP8266 to set up WiFi connection.
    delay(100);
    --countdown;
    if(countdown <= 0)
    {
      Serial.println("WARNING: may have failed to set up wifi connection...");
      break;
    }
  }
}

void postToPhant()
{
  // Create a client, and initiate a connection
  WiFiClient client;
  Serial.println("Connecting to Phant...");
  if (client.connect(PhantHost, 80) <= 0)
  {
    Serial.println(F("failed to connect."));
    return;
  }
  Serial.println(F("connected."));
  
  // Set up our Phant post parameters:
  String params = "no2=" + String(timer) + "&";
  params += "ozone=" + String(counter++) + "&";
  params += "temp=" + String(2.2);
    
  client.print(httpHeader);  client.print("Content-Length: "); client.println(params.length());
  client.println();  client.print(params);
  for(int i = 0; i < 30; i++)
  {
    if(client.available()) break;
    delay(100);
  }
  // return characters currently in the receive buffer.
  while (client.available())
    Serial.write(client.read());
  
  // close connection if active
  if (client.connected())
    client.stop();
}
