# WeatherCube
This repo provides code and schematics for the WeatherCube, an environmental monitoring platform. 

#

# Hardware

# Software 

## Arduino Installation
Step zero is to install Arduino. Note that in Linux, you may need need to launch Arduino using sudo priveleges. 

### 1. Add the ESP8266 board to Arduino. 

Follow: https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/installing-the-esp8266-arduino-addon 

### 2. Verify that you have the following libraries. In Arduino, click Sketch--> Include Library --> Manage Libraries and search for the following libraries. 
- Statistic
- SoftwareSerial
- Wire
- Adafruit_ADS1015
- EEPROM
- ESP8266WiFi

### 3. Download the following libraries for the Arduino Uno and save to your default Arduino library path (~/Arduino/libraries/): 
- [LMP91000](https://github.com/LinnesLab/LMP91000)
- [ClosedCube_HDC1080](https://github.com/closedcube/ClosedCube_HDC1080_Arduino/tree/master/src)
- [Adafruit_SHT31](https://github.com/adafruit/Adafruit_SHT31)
- [LowPower](https://github.com/rocketscream/Low-Power)
Note that on github, to download individual files you right-click on "raw" in the middle right and select "save file as". The files must be located in folders of the same name. 

### 4. Download the following files/libraries for the ESP8266 : 
Download and move the following files into the `AWSArduinoSDK` directory you just created: 
- [Esp8266AWSImplementations.h](https://github.com/daniele-salvagni/aws-sdk-esp8266/tree/master/src/esp8266)
- AmazonDynamoDBClient.h(https://github.com/daniele-salvagni/aws-sdk-esp8266/tree/master/src/common)
- AWSFoundationalTypes.h(https://github.com/daniele-salvagni/aws-sdk-esp8266/tree/master/src/common)



## AWS setup 
These instructions are courtesy Daniele Salvagni at https://github.com/daniele-salvagni/aws-sdk-esp8266/ 

### 1. Create an AWS account

Note this requires a credit card.

### 2. Set up the DynamoDB Table

You will need to set up a DynamoDB table with the same name, hash key, and range key as in the sample you are using. These values are defined as constants in the sample, i.e. `HASH_KEY_NAME` and `TABLE_NAME`.

You can follow the steps below to get the tables set up with the right values:

* Log into the [AWS Console](http://console.aws.amazon.com/) and navigate to **DynamoDB**.
* Click on the "Create Table" button.
* Enter `ESP8266AWSDemo` as the *Table Name*, `id` as the primary *Partition Key*, and `timest` as the primary *Sort Key*. Be sure to mark *id* as *String* and *timest* as *Number*.
* Just one *Read Capacity Unit* and one *Write Capacity Unit* will be more than enough for this demo. Press create with these values.
* After the table has finished creating you are good to go, your ESP will take care about populating it.


### 3. Creating `keys.h` and `keys.cpp`

You will need to create and add `keys.h` and `keys.cpp` into the `AWSArduinoSDK` directory you made. Alternatively, if you want to use different access keys for every sketch you make, just put them inside your current sketch folder. These files define the `awsKeyID` and `awsSecKey` values used by the sketch, the files may be structured as following:

```
// keys.h
#ifndef KEYS_H_
#define KEYS_H_

extern const char* awsKeyID;  // Declare these variables to
extern const char* awsSecKey; // be accessible by the sketch

#endif
```

```
// keys.cpp
#include "keys.h"

const char* awsKeyID = "YOUR AWS KEY ID HERE";
const char* awsSecKey = "YOUR AWS SECRET KEY HERE";
```

*Do not share your AWS keys with anyone, it is reccomended to create a dedicated user with restricted permissions, do not use your root account. If you accidentally push your keys online disable them immediatly from the [AWS IAM console](https://console.aws.amazon.com/iam/home)*

## Programming 
### 1.  Bootload the boards using Atmel studio (if we've given you boards, we've already done this)
### 2. Load a blink sketch onto the ATMega(or any sketch onto the ATMega that doesn't tie up the serial lines). Program Arduino using the Uno settings 
### 3. Edit the keys.h file from AWS setup step 3
### 4. Load esp_main sketch onto the ESP8266 
### 4. Load wxcube_main sketch onto the ATMega using the Uno settings


More info is on our project page at baltimoreopenair.github.io
