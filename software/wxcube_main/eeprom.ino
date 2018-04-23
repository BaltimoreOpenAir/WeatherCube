long writeEEPROMdouble(int deviceaddress, unsigned int eeaddress, int value)
{
  //    int value = input[i];
  //    int address = 64 + 2 * i;
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  // write 2nd bit
  Wire.write(two);
  // write first byte
  Wire.write(one);
  Wire.endTransmission();
  delay(50);
}

void getEEPROMChunk(long *data_holder, int chunk_size, int read_index, int device_address){
  for (int i = 0; i < chunk_size * 2; i += 2) {
    long read_location = read_index + i; //* EEPROM_BLOCKSIZE + i;
    byte two_e = readEEPROM(device_address, read_location);
    byte one_e = readEEPROM(device_address, read_location + 1);
    data_holder[i / 2] = ((two_e << 0) & 0xFF) + ((one_e << 8) & 0xFFFF) - 32768; //readEEPROMdouble(EEP0,  64 + i + loop_counter * 32);
    delay(50);
  }

}

byte readEEPROMdouble(int deviceaddress, unsigned int eeaddress )
{
  long two = readEEPROM(deviceaddress, eeaddress);
  long one = readEEPROM(deviceaddress, eeaddress + 1);
  //    // append bit-shifted data
  int output =  ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
  return output;
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress )
{
  byte rdata = 0xFF;

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, 1);

  if (Wire.available()) rdata = Wire.read();

  return rdata;
}

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data )
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();

  delay(10);
}
