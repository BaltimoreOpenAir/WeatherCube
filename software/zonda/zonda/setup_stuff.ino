void do_once() { // do at least once, but not all the time
  //----------------set id----------------
  Serial.println(F("Doing do_once()"));
  // set id (& save it and EEprom)
  //writeEEPROM(EEP0, ID_LOCATION, SERIAL_ID);
  delay(10);
  //----------------set clock-----------------
  rtc_write_date(start_second, start_minute, start_hour, start_day_of_week, start_day, start_month, start_year); // note: rename function to in order of the time registers in the memory of the rtc
  // second, minute, hour, day of the week, day, month, year
  delay(10);

  // set clock & save to rtc
  if (DEBUG_MODE == 1) {
    Serial.println(F("setup:"));
    //Serial.println(readEEPROM(EEP0, ID_LOCATION), DEC);
    delay(10);
    rtc_read_timestamp(0);
    delay(10);
  }
  else {
    
  }
  // set gain & save to EEprom
}
