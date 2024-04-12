#ifndef IMAGE_H
#define IMAGE_H
/*----------------------------Header File------------------------*/
#include "Arduino.h"
#include "Pinconfig.h"
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"
#include "SDRead.h"



const int WIDTH =320;
const int HEIGHT =240;
const int BMPIMAGEOFFSET =66;
const int RSWIDTH =32;
const int RSHEIGHT =24;
/*--------------------------------------------------------------*/

/*-------------------------OV5642 Function ---------------------*/
void OV5642Setup();
void TakeImage();
uint8_t read_fifo_burst_BMP(ArduCAM myCAM);
uint8_t OV5642SPIStatus();
uint8_t OV5642I2CStatus();
void OV5642CAMSetting();
uint8_t SDStatus();
/*--------------------------------------------------------------*/

/*-----------------------Image Function  -----------------------*/
uint8_t RGB565toGS(uint16_t hexvalue);
uint8_t *resizeImageAverage(uint8_t *Pixel);
void SetPixel(uint8_t *tempPixel);
uint8_t *GetPixel();
/*--------------------------------------------------------------*/




//read reverse
//first 14 bytes = header
//second 40 bytes = inof header
//thrid 12bytes =  color table 
const uint8_t bmp_header[BMPIMAGEOFFSET] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 
  0x28, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 
  0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00
};


#endif 
