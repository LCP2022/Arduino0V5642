// Tested on Arduino nano 33 Ble 
// SD Card
// OV5642
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"
#define BMPIMAGEOFFSET 66
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
const uint8_t OV5642_CS = 7;
const uint8_t SD_CS = 6;

ArduCAM myCAM( OV5642, OV5642_CS );

uint8_t read_fifo_burst_BMP(ArduCAM myCAM);
uint8_t OV5642SPIStatus();
uint8_t OV5642I2CStatus();
void OV5642CAMSetting();
uint8_t SDStatus();

uint8_t vid, pid,temp,count=1;

void setup() {
  // put your setup code here, to run once:
   //  set Serial port
  Serial.begin(115200);
  while(!Serial);
  //  Set pin configure
  pinMode(OV5642_CS,OUTPUT);
  digitalWrite(OV5642_CS,HIGH);
  pinMode(SD_CS,OUTPUT);

  Wire.begin(); // I2C wire
  SPI.begin();  // SPI Wire 

  //  Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  while(!SDStatus());
  while(!OV5642SPIStatus());
  while(!OV5642I2CStatus());
  OV5642CAMSetting();
}

void loop() {
  // put your main code here, to run repeatedly:
   uint8_t UserChoice = 0;
  Serial.print("type 1 to capture images ");
  Serial.println(count);
  while (!(UserChoice == 1)){
      UserChoice = Serial.parseInt();
  }
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  Serial.println("Start Capture");
  //Start capture
  myCAM.start_capture();
  while(!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  Serial.println(F("Capture Done."));  
  delay(50);
  read_fifo_burst_BMP(myCAM);
  myCAM.clear_fifo_flag();
  
  count++;
  UserChoice = 0;

}

uint8_t OV5642SPIStatus(){
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println(F("ACK CMD SPI interface Error! END"));
    delay(1000);
    return 0;
  }else{
    Serial.println(F("ACK CMD SPI interface OK. END"));
    return 1;
  }
}
uint8_t SDStatus(){
  if(SD.begin(SD_CS)){
    Serial.println(F("SD Card detected."));
    return 1;
  }
  else{
    Serial.println(F("SD Card Error"));
    delay(1000);
    return 0;
  }
}
uint8_t OV5642I2CStatus(){
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if((vid != 0x56) || (pid != 0x42)){
    Serial.println(F("ACK CMD Can't find OV5642 module! END"));
    delay(1000);
    return 0;
  }
 else{
    Serial.println(F("ACK CMD OV5642 detected. END"));
    return 1;
  } 
}

void OV5642CAMSetting(){
  myCAM.set_format(BMP);
  myCAM.InitCAM();
  myCAM.clear_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM.wrSensorReg16_8(0x3818, 0x81);
  myCAM.wrSensorReg16_8(0x3621, 0xA7);
  myCAM.OV5642_set_Light_Mode(Simple_AWB);
  delay(1000);
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
}

uint8_t read_fifo_burst_BMP(ArduCAM myCAM)
{
  uint8_t temp, temp_last;
  uint32_t length = 0;
  char str[8];
  static int k = 0, p = 0; // for name
  uint8_t VH, VL;
  byte buf[256];
  File outFile;

  length = myCAM.read_fifo_length();
  //Read 320x240x2 byte from FIFO
  Serial.println(length, DEC);
  if (length >= MAX_FIFO_SIZE ) 
  {
    Serial.println(F("ACK CMD Over size. END"));
    myCAM.clear_fifo_flag();
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("ACK CMD Size is 0. END"));
    myCAM.clear_fifo_flag();
    return 0;
  }
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".bmp"); //Generate file name
  outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
  if (!outFile){
  Serial.println(F("File open error"));
  return 0;
  }
  for (temp = 0; temp < BMPIMAGEOFFSET; temp++)
  { 
    buf[p++]= bmp_header[temp];
    Serial.print(bmp_header[temp],HEX);
  }
  outFile.write(buf, p);
  Serial.println("");
  p = 0;
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  int i = 0, j = 0;
  for (i = 0; i < 240; i++)
  { 
    for (j = 0; j < 320; j++)
    {
        VH = SPI.transfer(0x00);
        VL = SPI.transfer(0x00);
        if(p<256){
          buf[p++] = VL;
          buf[p++] = VH;
          Serial.print(VL,HEX);
          Serial.print(VH,HEX);
        }
        else{
          //Write 256 bytes image data to file
          myCAM.CS_HIGH();
          outFile.write(buf, 256);
          p = 0;
          buf[p++] = VL;
          buf[p++] = VH;
          myCAM.CS_LOW();
          myCAM.set_fifo_burst();
        }
      
    }
  }
  //Close the file
  myCAM.CS_HIGH();
  outFile.write(buf, p);
  outFile.close();
  Serial.println("");
  //Clear the capture done flag
  myCAM.clear_fifo_flag();
  Serial.println("Image Save OK");
  p=0;

  return 1;
}
