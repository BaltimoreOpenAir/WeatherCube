void do_once() { // do at least once, but not all the time
  //----------------set id----------------
  Serial.println(F("Doing do_once()"));
  // set id (& save it and EEprom)
  writeEEPROM(EEP0, ID_LOCATION, SERIAL_ID);
  delay(10);
  //----------------set clock-----------------
  rtc_write_date(start_second, start_minute, start_hour, start_day_of_week, start_day, start_month, start_year); // note: rename function to in order of the time registers in the memory of the rtc
  // second, minute, hour, day of the week, day, month, year
  delay(10);

  // set clock & save to rtc
  if (DEBUG_MODE == 1) {
    Serial.println(F("setup:"));
    Serial.println(readEEPROM(EEP0, ID_LOCATION), DEC);
    delay(10);
    rtc_read_timestamp(0);
    delay(10);
  }
  else {
  }
  // set gain & save to EEprom
}

void test_post()
{
  digitalWrite(WIFI_EN, HIGH);
  delay(2000);
  mySerial.write("!");
  mySerial.write(ESP_MESSAGE_TERMINATOR);
  mySerial.flush();
  delay(800);
  
  long one_reading_array[EEPROM_BLOCKSIZE];
  memset(one_reading_array, 0, sizeof(one_reading_array));
  //// check serial messages
  //if (mySerial.available()) {
    while (mySerial.available()) {
      Serial.write(mySerial.read());
      delay(10);
    }
  //}
  bool post_success =  false;
  int counter = 0;
  while (post_success == false && counter < MAX_POST_TRIES ) {
    //// send data to AWS
    char cbuf[25];
    for (int i = 0; i < 14; i++) { //DATA_ARRAY_SIZE; i++) {
      String s = "";
      s += types[i];
      if ( one_reading_array[i] == -32768) {
        s += "NaN"; //"NaN";
      }
      else {
        s += String(one_reading_array[i]);
      }
      s += ESP_MESSAGE_TERMINATOR;//"x";
      s.toCharArray(cbuf, MAX_MESSAGE_LENGTH);
      Serial.println(F("data to be posted is..."));
      Serial.println(s);
      mySerial.write(cbuf);
      mySerial.flush();
      delay(800);
    }
    Serial.println(F("posting next set of data..."));

    // hitting limit of 17 fields for dynamodb... adding rest to last field
    String s = "";
    s += types[14];
    for (int i = 14; i < DATA_ARRAY_SIZE; i++) { //DATA_ARRAY_SIZE; i++) {
      if ( one_reading_array[i] == -32768) {
        s += "NaN"; //"NaN";
      }
      else {
        s += String(one_reading_array[i]);
      }
      s += ",";
    }
    s += ESP_MESSAGE_TERMINATOR;//"x";
    s.toCharArray(cbuf, MAX_MESSAGE_LENGTH);
    Serial.println(F("rest of data to be posted is..."));
    Serial.println(s);
    mySerial.write(cbuf);
    mySerial.flush();
    delay(800);
    mySerial.write("p");
    mySerial.write(ESP_MESSAGE_TERMINATOR);
    mySerial.flush();
    Serial.println(F("Attempted post..."));
    delay(5000);
    char inbyte;
    long toc = millis();
    //while (millis() - toc < 50000) {
    //int counter = 0;
    // while (counter < 25){
    //if (mySerial.available()) {
      while (mySerial.available()) { // & counter < 50) {
        inbyte = mySerial.read(); delay(20);
        if(inbyte != '#'){
          Serial.write(inbyte); delay(20);
        }else{
          //Serial.write(inbyte);
          inbyte = mySerial.read();
          //Serial.write(inbyte);
          if (inbyte == 'S') {
            post_success = true;
            Serial.println(F("POST SUCCESS!"));
          }
          else if(inbyte == 'E'){
            inbyte = mySerial.read();
            if(inbyte == '1'){
              Serial.println(F("POST ERROR: Invalid request"));
            }else if(inbyte == '2'){
              Serial.println(F("POST ERROR: Required arguments were not set for PutItemInput"));
            }else if(inbyte == '3'){
              Serial.println(F("POST ERROR: Problem parsing http response of PutItem"));
            }else{
              Serial.println(F("POST ERROR: Connection problem"));
            }
          }
        }
      }
    //}
    counter = counter + 1;
    Serial.println(F("counter increasing to..."));
    Serial.println(counter);
  }


  delay(5000);
  mySerial.write("@");
  mySerial.write(ESP_MESSAGE_TERMINATOR);
  long toc = millis();
  while (millis() - toc < 15000) {
    int counter = 0;
    if (mySerial.available()) {
      while (mySerial.available()) { // & counter < 50) {
        Serial.write(mySerial.read());
        delay(500);
        ++counter;
      }
    }
  }
  digitalWrite(WIFI_EN, LOW);
}
