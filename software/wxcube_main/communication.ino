bool is_reporting_time(bool report_by_time){
  if(report_by_time){
    rtc_read_timestamp(1);
    if(last_hour != integer_time[2]){
      int report_minute = SERIAL_ID % 100;
      if(abs(report_minute - integer_time[1]) < (MEASUREMENT_SECONDS / 60) + 1){
        last_hour = integer_time[2];
        return true;
      }
    }
  }else{
    if(cache >= DEBUG_READS_PER_POST){
      return true;
    }
  }
  return false;
}

void send_data() {
  digitalWrite(WIFI_EN, HIGH);
  delay(2000);
  //long one_reading_array[EEPROM_BLOCKSIZE];
  bool post_success;

  mySerial.write("!");
  mySerial.write(ESP_MESSAGE_TERMINATOR);
  mySerial.flush();
  delay(800);
  //Serial.println(loop_counter);
  for (int reading_counter = unreported_reads; reading_counter > 0; reading_counter--) {
    //reading_counter = loop_counter;
    post_success = false;
    Serial.print(F("Sending reading number "));
    Serial.println(reading_counter);
    delay(50);

    //// first pull from EEPROM
    Serial.println(F("Reading from EEPROM at ..."));
    Serial.println(eeprom_write_location - reading_counter * EEPROM_BLOCKSIZE);
    Serial.println(F("to..."));
    Serial.println(eeprom_write_location - reading_counter * EEPROM_BLOCKSIZE + DATA_ARRAY_SIZE * 2 + 1);

    long one_reading_array[EEPROM_BLOCKSIZE];
    for (int i = 0; i < DATA_ARRAY_SIZE * 2; i += 2) {
      byte two_e = readEEPROM(EEP0, eeprom_write_location - reading_counter * EEPROM_BLOCKSIZE + i);
      byte one_e = readEEPROM(EEP0, eeprom_write_location - reading_counter * EEPROM_BLOCKSIZE + i + 1);
      one_reading_array[i / 2] = ((two_e << 0) & 0xFF) + ((one_e << 8) & 0xFFFF) - 32768; //readEEPROMdouble(EEP0,  64 + i + loop_counter * 32);
      delay(50);
    }

    Serial.println(F("Array in EEPROM is: "));
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      Serial.print(one_reading_array[i]);
      if(i < DATA_ARRAY_SIZE - 1){Serial.print(",");}
      else{Serial.println();}
    }
    //memset(one_reading_array, 0, sizeof(one_reading_array));
    //// check serial messages
    if (mySerial.available()) {
      while (mySerial.available()) {
        Serial.write(mySerial.read());
        delay(10);
      }
    }
    char cbuf[25];
    Serial.println(F("DB post string: "));
    for (int i = 0; i < 14; i++) { //DATA_ARRAY_SIZE; i++) {
      String s = "";
      s += types[i];
      if ( one_reading_array[i] == -32768) {
        s += "NaN"; //"NaN";
      }
      else {
        s += String(one_reading_array[i]);
      }
      s += ESP_MESSAGE_TERMINATOR;
      s.toCharArray(cbuf, MAX_MESSAGE_LENGTH);
      //Serial.println(F("data to be posted is..."));
      Serial.print(s);
      Serial.print(", ");
      mySerial.write(cbuf);
      mySerial.flush();
      delay(800);
    }
    //Serial.println(F("posting next set of data..."));
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
    s += ESP_MESSAGE_TERMINATOR;
    Serial.println(s);
    s.toCharArray(cbuf, MAX_MESSAGE_LENGTH);
    //Serial.println(F("rest of data to be posted is..."));
    //Serial.println(s);

    mySerial.write(cbuf);
    mySerial.flush();
    delay(800);

    //bool post_success =  false;
    char inbyte;
    int number_tries = 0;
    while (post_success == false && number_tries < MAX_POST_TRIES) {
      mySerial.write("p");
      mySerial.write(ESP_MESSAGE_TERMINATOR);
      mySerial.flush();
      Serial.println(F("Attempted post..."));
      delay(5000);
      long toc = millis();
      //while (millis() - toc < 50000) {
      int counter = 0;
      // while (counter < 25){
      if (mySerial.available()) {
        while (mySerial.available()) { // & counter < 50) {
          inbyte = mySerial.read();
          //Serial.write(mySerial.read());
          Serial.write(inbyte);
          delay(30);
          if (inbyte == '#') {
            inbyte = mySerial.read();
            if (inbyte == 'S') {
              post_success = true;
              Serial.println(F("post success!"));
              unreported_reads = 0;
            }
          }
        }
      }
      number_tries = number_tries + 1;
      delay(50);
    }
  }
  if (post_success == true) {
    loop_counter = 0 ;
    Serial.println(F("Writing loop_counter to eeprom"));
    writeEEPROM(EEP0, LOOP_COUNTER_LOCATION, loop_counter);
  }
  else {
    Serial.println(F("Writing loop_counter to eeprom"));
    Serial.println(loop_counter);
    writeEEPROM(EEP0, LOOP_COUNTER_LOCATION, loop_counter);
    // save loop_counter to memory
  }
  mySerial.write("@");
  mySerial.write(ESP_MESSAGE_TERMINATOR);
  delay(1000);
  digitalWrite(WIFI_EN, LOW);
}
