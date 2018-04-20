void read_pms_data()
{
  pms_note_code = 0;
  int idx = PMS_DATA_INDEX;
  if (pms.available()) {
    char c0, c1, c2[2], cdata[30];
    bool has_read = false;
    c1 = pms.read();
    for (int i = 0; i < 62; i++) { //more than one read frame
      c0 = c1;
      c1 = pms.read(); delay(10);
      if (c0 == 0x42 && c1 == 0x4D) {
        if(i > 0) pms_note_code = i;
        pms.readBytes(cdata, 30); delay(10);
        if (cdata[0] == 0x00 && cdata[1] == 0x1C) { //28-byte reading frame
          has_read = true;
          break;
        }
      }
    }
    if (has_read) {
      bool mark = false;
      //for(int j = 0; j <
      for (int i = 0; i < 10; i += 2) {
        int i1 = cdata[i], i2 = cdata[i + 1];
        if (i1 < 0) i1 = -i1 + 256;
        if (i2 < 0) i2 = -i2 + 256;
        int res = i1 * 256 + i2;
        data_array[idx++] = res;
      }
      data_array[DATA_ARRAY_SIZE - 1] = pms_note_code;
      pms.readBytes(c2, 2); delay(10);
      pms.readBytes(c2, 2); delay(10);
      pms.flush();
    }
  }
}


/*void reset_PMS(){
  Serial.println("Restarting PM!");
  digitalWrite(PMS_RESET, LOW);
  delay(100);
  digitalWrite(PMS_RESET, HIGH);
  delay(4000);
}*/

