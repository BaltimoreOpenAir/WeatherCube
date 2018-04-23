void which_afe(int num)
{
  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(num, LOW);
}

void set_afe(int num, int analyte, int index)
{
  if(VERBOSE_SERIAL){
    Serial.print(F("Setting AFE"));
    Serial.print(index);
  }
  which_afe(num);
  delay(100);
  uint8_t res = configure_LMP91000(analyte);
  delay(100);
  if(VERBOSE_SERIAL){
    Serial.println(F("...done"));
    Serial.print(F("Config Result: "));    Serial.println(res);
    Serial.print(F("STATUS: "));    Serial.println(lmp91000.read(LMP91000_STATUS_REG), HEX);
    Serial.print(F("TIACN: "));    Serial.println(lmp91000.read(LMP91000_TIACN_REG), HEX);
    Serial.print(F("REFCN: "));    Serial.println(lmp91000.read(LMP91000_REFCN_REG), HEX);
    Serial.print(F("MODECN: "));    Serial.println(lmp91000.read(LMP91000_MODECN_REG), HEX);
  }
  digitalWrite(num, HIGH);
}

void set_adc_gain(int g)
{
  gain_index = g;
  ads.setGain(gain[gain_index]);
  mv_adc_ratio = mv_ratios[gain_index];
}

int configure_LMP91000(int gas)
{
  return lmp91000.configure( LMP91000_settings[gas][0], LMP91000_settings[gas][1], MODECN_DEFAULT);
}
