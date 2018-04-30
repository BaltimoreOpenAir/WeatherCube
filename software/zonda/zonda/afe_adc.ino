/*void which_afe(int num)
{
  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(num, LOW);
}*/

void which_afe(int num)
{
  byte b = B00001110;
  if(num == 1){b = B00001101;}
  else if(num == 2){b = B00001011;}
  else if(num == 3){b = B00000111;}

  //Serial.print("MCP23008: ");
  //Serial.println(b);
  Wire.beginTransmission(0x20); //begins talking
  Wire.write(0x00); //selects the IODIRA register
  Wire.write(0x00); //this sets all port A pins to outputs
  Wire.endTransmission(); //stops talking to device
  delay(50);
  Wire.beginTransmission(0x20); //starts talking
  Wire.write(0x09); //selects the GPIO pins
  Wire.write(b); // turns on pins 0 and 1 of GPIOA
  Wire.endTransmission(); //ends communication with the device  
  delay(50);
}

void set_afe(int num, int analyte, int index)
{
  delay(500);
  Serial.print(F("Setting AFE"));
  Serial.print(index);
  which_afe(num);
  delay(100);
  uint8_t res = configure_LMP91000(analyte);
  Serial.println(F("...done"));
  Serial.print(F("Config Result: "));    Serial.println(res);
  Serial.print(F("STATUS: "));    Serial.println(lmp91000.read(LMP91000_STATUS_REG), HEX);
  Serial.print(F("TIACN: "));    Serial.println(lmp91000.read(LMP91000_TIACN_REG), HEX);
  Serial.print(F("REFCN: "));    Serial.println(lmp91000.read(LMP91000_REFCN_REG), HEX);
  Serial.print(F("MODECN: "));    Serial.println(lmp91000.read(LMP91000_MODECN_REG), HEX);
  digitalWrite(num, HIGH);
}

/*void set_adc_gain(int g)
{
  gain_index = g;
  ads.setGain(gain[gain_index]);
  mv_adc_ratio = mv_ratios[gain_index];
}*/

int configure_LMP91000(int gas)
{
  return lmp91000.configure( LMP91000_settings[gas][0], LMP91000_settings[gas][1], MODECN_DEFAULT);
}

void read_afe_data()
{
  afe_note_code = 0;
  memset (data_array, -1, sizeof(data_array));
  rtc_read_timestamp(1);
  data_array[0] = ads.readADC_SingleEnded(0); delay(10);
  data_array[1] = ads.readADC_SingleEnded(1); delay(10);
  data_array[2] = ads.readADC_SingleEnded(2); delay(10);
  data_array[3] = ads.readADC_SingleEnded(3); delay(10);
  data_array[4] = hdc1080.readTemperature() * 100.0;
  data_array[5] = hdc1080.readHumidity() * 100.0;
  data_array[6] = sht31.readTemperature() * 100.0;
  data_array[7] = sht31.readHumidity() * 100.0;
  //data_array[8] = sht32.readTemperature() * scaler;
  //data_array[9] = sht32.readHumidity() * scaler;
  data_array[10] = 100.0 * analogRead(VOLT) / 1023.0 * VREF * VDIV;
  data_array[11] = integer_time[0]; //second
  data_array[12] = 100.0 * integer_time[2] + integer_time[1]; //hour minute
  data_array[13] = 100.0 * integer_time[5] + integer_time[4]; //month day
  data_array[14] = integer_time[6]; //year
  data_array[15] = afe_note_code;
}
