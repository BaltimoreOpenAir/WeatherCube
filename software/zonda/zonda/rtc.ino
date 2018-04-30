void rtc_write_date(int sec, int mint, int hr24, int dotw, int dy, int mon, int yr)
{
  Wire.beginTransmission(RTC_ADDR);
  Wire.write((uint8_t)TIME_REG);
  Wire.write(dec2bcd(sec) | B10000000); //stops the oscillator (Bit 7, ST == 0)
  Wire.write(dec2bcd(mint));
  Wire.write(dec2bcd(hr24)); //hr
  Wire.write(dec2bcd(dotw) | B00001000); //dotw
  Wire.write(dec2bcd(dy)); //date
  Wire.write(dec2bcd(mon)); //month
  Wire.write(dec2bcd(yr)); //year

  Wire.write((byte) 0);
  //Wire.write((uint8_t)0x00);                     
  /*
     Wire.write(dec2bcd(05));
    Wire.write(dec2bcd(04));                  //sets 24 hour format (Bit 6 == 0)
    Wire.endTransmission();
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((uint8_t)TIME_REG);
    Wire.write(dec2bcd(30) | _BV(ST));    //set the seconds and start the oscillator (Bit 7, ST == 1)
  */
  Wire.endTransmission();
}

void rtc_read_timestamp(int mode)
{
  byte rtc_out[RTC_TS_BITS];
  //int integer_time[RTC_TS_BITS];
  Wire.beginTransmission(RTC_ADDR);
  Wire.write((byte)0x00);
  if (Wire.endTransmission() != 0) {
    //Serial.println(F("no luck"));
    //return false;
  }
  else {
    //request 7 bytes (secs, min, hr, dow, date, mth, yr)
    Wire.requestFrom(RTC_ADDR, RTC_TS_BITS);

    // cycle through bytes and remove non-time data (eg, 12 hour or 24 hour bit)
    for (int i = 0; i < RTC_TS_BITS; i++) {
      rtc_out[i] = Wire.read();
      //if (mode == 0) printBits(rtc_out[i]);
      //else
      //{
        //byte b = rtc_out[i];
        if (i == 0) rtc_out[i] &= B01111111;
        else if (i == 3) rtc_out[i] &= B00000111;
        else if (i == 5) rtc_out[i] &= B00011111;
        //int j = bcd2dec(b);
        //Serial.print(j);
        //if(i < how_many - 1) Serial.print(",");
      //}
    }
  }

  for (int i = 0; i < RTC_TS_BITS ; i++) {
    int ii = rtc_out[i];
    integer_time[i] = bcd2dec(ii);
  }
}
