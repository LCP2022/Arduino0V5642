#include "SDRead.h"
uint8_t *LoadFile(char* filename){
  uint32_t index =0;
  uint8_t *Pixel = (uint8_t *)malloc(WIDTH*HEIGHT* sizeof(uint8_t));
  File bmpFile;
  bmpFile = SD.open(filename,FILE_READ);
  if (!bmpFile) {
    Serial.println("Failed to open BMP file!");
    return 0;
  }
  Serial.println("Load File Successful");
  // Skip BMP header
  if (!bmpFile.seek(headerSize)) {
    Serial.println("Failed to skip BMP header!");
    bmpFile.close();
    return 0;
  }
  Serial.println("Located Hex value");
  while (bmpFile.available()) {
    //bliner interpolation LCM 16->15 in count 
    uint8_t byte1 = bmpFile.read();
    uint8_t byte2 =  bmpFile.read();
    uint16_t colorhex = ( byte2 << 8) | byte1;
    uint8_t GS = RGB565toGS(colorhex);
    Pixel[index++] = GS;
    //Serial.print(GS,DEC);
    //Serial.print(",");
  }
  Serial.println("");
  bmpFile.close();
  Serial.println("Done");
  
  uint8_t *RSPixel = resizeImageAverage(Pixel);
//  uint32_t count = 0;
//  for(int i =0;i<RSHEIGHT;i++){ 
//    for(int j =0;j<RSWIDTH;j++){
//      count++;
//      Serial.print(RSPixel[i*RSWIDTH+j],DEC); //not sure why is it width
//      Serial.print(",");
//    }
//    Serial.println("");
//  }
//  Serial.println(count);
  free(Pixel);
  return RSPixel;
}
