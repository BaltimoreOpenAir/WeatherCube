# WeatherCube
This repo provides code and schematics for the WeatherCube, an environmental monitoring platform used for the Baltimore OpenAir project. More info is on our project page at baltimoreopenair.github.io

#

# Hardware

## Circuit Boards 

### Component list:

1- The main board: It supports the microcontrollers. Its the brain of the device

2- Charging controller: provides power to the main board, charge up the battery with the solar panel

3- Lithium polymere battery: holds the power for the device

4- Analog front end board: holds the air quality sensors

5- Coin CR battery: powers the clock in case of loss of power

6- temperature sensor board: measures temperature and humidity in the tunnel

7- solar pannel: powers the unit

8- 4x air quality sensor: for N02,03,SO2 and H2S

9- 3X Molex picoblade cable assembly (2X 4 pins and 1X 5 pins): connects boards together

10- FTDI ttl/usb controller: allows to load code

11- ttl/usb ESP adapter: allows to connect the FTDI controller to load code in the ESP wifi chip

12- ttl/usb ATMEGA adapter: allows to connect the FTDI controller to load code in the ATMEGA microcontroller


## Electronic Assembly:

### Necessary equipment

1- Soldering iron

2- snips

3- box cutter

4- lead free rosin core solder of small gauge

5- a breadboard to serve as jig

6- A good multimeter for quality check

7- tweezers

9- a brush (such as a toothbrush)

10- methanol or rubbing alcohol

### Solder male header pins onto the main sensor board 

Use male 2.54mm (1/10th in) pitch headers. The headers come in 40 pins assembly, and will need to be sniped to the correct size. for this board you need 3x 7pins, 2x 2pins and 1x 1pin headers. You can use the ttl/usb adapter and/or a breadboard as jig to hold the pins in place at a right angle from the board. the pins should protrude in the front of the board and be soldered from the back. The positions of the headers are marked by yellow cirlces in the following image.

<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/main_board_pins.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="soldering of the 1/10th pitch headers on the main board (front of the board)"/>

### Solder female header pins onto the Analog Front End (AFE) board
Use female 2mm pitch 2pin headers. You will need 12x 2oin headers. The pins should protrude from the front of the board and be soldered from the back. You can make a jig with 2mm male headers on a unassembled AFE PCB in order to correctly align the female headers. The pins should be scrubed with high proof methanol or other alcohol after soldering to avoid sensor bias. Resistance should be in excess of 50M Ohm between the reference and control electrode when the sensor is not mounted. The positions of the headers are marked by yellow cirlces in the following image. 

<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/AFE-soldering.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="soldering of the 2mm pitch headers on the AFE board (front of the board)"/>

### Solder the main sensor board to the solar charging board
The main board is to be soldered with the wires coming from the charge controller. If the battery is already soldered to the charge controller, you will be soldering live wires if the black and white wire connect. This does not constitute a danger of electrocution due to the low DC voltage but may lead to burining the charge controller if the board is shorted, and is a risk of fire. Therefore, extreme caution is necessary. The recommended order for soldering is red wire (5v) then the white wire ( enable wire). At this point, it is important to check that the red and white wires do not connect in any way, and trimming the wire ends may be a good idea. You can then solder the black wire (ground). Be careful not to short the circuit accidentally with the tip of your soldering iron. This closes the circuit. The positions of the wires are marked in the following image.

<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/main_board_wires.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="soldering of the wires on the main board"/>

### Solder the usb/ttl adapters

The usb/ttl adapter need both male and female headers of 2.54mm pitch. The male headers are suposed to be protruding on the front of both boards and the female headers on the back. You may use a soldered main board as a jig to gold the female headers in place. To snip the female headers to the correct size, first count the numbers of pins you need and pull the next pin. You can then cut the header to sizer with a box cutter and shave the plastic for esthetics. The following image displat the positions of the male headers (yellow circles) and female headers (purple circles). For this, you need 2X 6pin male headers and 4X 7pin female headers. for the female headerds of the usb/ttl ESP adapter, you will need to pull some of the pins to fit in the soldering holes.

<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/USBATMEGA-soldering.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="soldering headers on the usb/ttl atmega adapter"/>

<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/USBESP-soldering.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="soldering headers on the usb/ttl esp adapter"/>

### Plug things together
Follow the following figures to connect the differents component together. Do not forget to place the enable jumper on the main board once the code is loaded in

<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/main_board_connections.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="Connections on the main board"/>


<img src="https://github.com/BaltimoreOpenAir/WeatherCube/tree/master/figures/AFE-connections.svg" width="280" style="float:right; margin: 1em 0 4em 2em;"
title="Connections on the Analog front end"/>

## Sensors 
Read the calibration codes off the sensor QR codes using any QR code reader. Save these codes for later calibration. Then, plug in the sensors clockwise from the top left in the following order: O3, H2S, SO2, and NO2.

## Code loading

The software section describes the code that need to be loaded for the project. Here we describe the hardware setup necessary to communicate with the assembled device.

1- take the jumper off the enable pin on the main board.

2- connect the usb/ttl adapters on the wifi chip and on the atmega adapter.

3- make sure the FTDI controller jumper is on 3.3V

4- connect the FTDI controller with the usb cable to the computer. make sure that the ftdi controller has the proper driver installed. This can be done on windows by running this [executable](https://cdn.sparkfun.com/assets/learn_tutorials/7/4/CDM21228_Setup.exe).

5- connect the ftdi controller on the esp adapter.

6-  Using Arduino IDE, select the right com port and select the generic esp8266 in the tool section and load the esp8266 code. Make sure to modify the code with the target wifi credentials.

7- disconnect the FTDI controller and connect it to the atmega adapter. Using Arduino IDE, select the right com port and the arduino UNO board, load the code. You may have trouble to establish connections with the atmega when loading a code more than once. Restarting the computer seem to help in that case.

8- disconnect the adapters and put back the enable jumper. your weathercube is ready to be deployed!

# Software 

## Arduino Installation
Step zero is to install [Arduino](https://www.arduino.cc/). Note that in Linux, you may need need to launch Arduino using sudo priveleges. 

### 1. Add the ESP8266 board to Arduino. 

Follow: https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/installing-the-esp8266-arduino-addon 

### 2. Verify that you have the following libraries. If not, in Arduino, click Sketch--> Include Library --> Manage Libraries and search for the following libraries. When you find the library, click on it and an install button should appear. Click on this button to install. 
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

Create an account at https://aws.amazon.com/. Note this requires a credit card but it's possible to be on AWS' free tier. You may also qualify for educational credits. 

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
### 1.  Bootload the boards using Atmel studio (if we've given you boards, we've already done this for you)
### 2. Load the Blink example sketch onto the ATMega selecting Arduino Uno as the board 
For this step, you could use any sketch onto the ATMega that doesn't tie up the serial lines, such as reset_esp.ino.
### 3. Edit the keys.h file from AWS setup step 3
### 4. Load esp_main sketch onto the ESP8266 
### 4. Load wxcube_main sketch onto the ATMega using the Uno settings

# Data 
### Data access through AWS Console
To view your data, log in to the [AWS Console](http://console.aws.amazon.com/) and navigate to **DynamoDB**. Click on "Tables" in the left hand menu, select your table and click on "Items". To download your table, first create a filter by selecting "filter". You can select for ids or for time. Note that ids are strings and time is a number such as '20170908000015'.  Click start search. When the search is complete, click "Actions" and select  "Download as CSV". 

### Command line/Python data access
Install and configure Boto3 in Python following the [documentation](https://boto3.readthedocs.io/en/latest/guide/quickstart.html#installation). 

The following sample code can be used to retrieve one sensor's data and turn it into a Pandas dataframe with a 'datetime' index. 

```
import pandas as pd
import boto3
from boto3.dynamodb.conditions import Key, Attr
dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('BaltimoreOpenAir2017')
sel_id = 1 # change this to the sensor id you are querying 
# the following will pull any data after September 8, 2017; change to your desired date
response = table.query(KeyConditionExpression=Key('id').eq(str(sel_id)) & Key('timest').gt(20170908000015)) 
df = pd.DataFrame(response['Items'])
# clean up the dataframe
df['timest'] = [pd.to_datetime(str(date)[:-4], format = '%Y%m%d%H%M') for date in df['timest']]
# read in batteryAV to get the time
df[['something', 'HourMinute', 'MonthDay', 'AV', 'drop'] ] = df['battAV'].str.split(',').apply(pd.Series)
# key in the calibration codes, m_o3, m_no2, m_so2, m_h2s
df[['O3_avg', 'O3_std']]   = df[['O3_avg', 'O3_std']].astype(float)/ m_o3)
df[['NO2_avg', 'NO2_std']] = df[['NO2_avg', 'NO2_std']].astype(float)/m_no2)
df[['SO2_avg', 'SO2_std']] = df[['SO2_avg', 'SO2_std']].astype(float)/m_so2)
df[['H2S_avg', 'H2S_std']] = df[['H2S_avg', 'H2S_std']].astype(float)/m_h2s)

# set up time index 
date_index = []
for date in df[['MonthDay', 'HourMinute']].values : 
    try: 
        if date[0] == '32767' : 
            date_index.append(np.nan)
        elif date[0] == '0' : 
            date_index.append(np.nan)
        else:
            date_index.append(pd.to_datetime('2017' +date[0]+date[1], format = '%Y%m%d%H%M'))
    except ValueError:  
        date_index.append(np.nan)
df['date_index'] = date_index
```

For an implementation of the code & a proposed calibration technique, see (https://github.com/BaltimoreOpenAir/BOAFieldEvaluation/blob/master/MDE_Beltsville_Analysis.ipynb). 


