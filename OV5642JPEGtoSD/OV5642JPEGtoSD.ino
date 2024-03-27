// Tested on Arduino nano 33 Ble 
// SD Card
// OV5642
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"

const uint8_t OV5642_CS = 7;
const uint8_t SD_CS = 6;

ArduCAM myCAM( OV5642, OV5642_CS );

uint8_t read_fifo_burst(ArduCAM myCAM);
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
  read_fifo_burst(myCAM);
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
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
  myCAM.OV5642_set_JPEG_size(OV5642_320x240);
  delay(1000);
  myCAM.OV5642_set_Light_Mode(Simple_AWB);
  delay(1000);
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
}

// for Jpeg
uint8_t read_fifo_burst(ArduCAM myCAM)
{   
  char str[8];
  byte buf[256];
  static int i = 0,k = 0;
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  bool is_header = false;
  File outFile;

  length = myCAM.read_fifo_length();
  Serial.print("size_of_JPEG_");
  Serial.print(count);
  Serial.print(" = ");
  Serial.println(length, DEC);

  if (length >= MAX_FIFO_SIZE) //512 kb
  {
    Serial.println(F("ACK CMD Over size. END"));
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("ACK CMD Size is 0. END"));
    return 0;
  }
  //Construct a file name
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".jpg");
  //Open the new file
  outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
  if(!outFile){
    Serial.println(F("File open faild"));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  Serial.print("JPEG_Data_");
  Serial.print(count);
  Serial.print(" = [ ");
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    if (is_header == true){
    //Write image data to buffer if not full
    if (i < 256)
      buf[i++] = temp;
    else{
    //Write 256 bytes image data to file
      myCAM.CS_HIGH();
      outFile.write(buf, 256);
      i = 0;
      buf[i++] = temp;
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
    }
      Serial.print("'");
      Serial.print(temp,HEX);
      Serial.print("',");
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF)){ 
    // Start of the JPEG file 
      is_header = true;
      buf[i++] = temp_last;
      buf[i++] = temp;  
      Serial.print("'");
      Serial.print(temp_last,HEX);
      Serial.print("','");
      Serial.print(temp,HEX);
      Serial.print("',");
    }
    if ( (temp == 0xD9) && (temp_last == 0xFF) ){
    //  end of the JPEG file 
    buf[i++] = temp;
    myCAM.CS_HIGH();
    outFile.write(buf, i);
    outFile.close();
    break;
    }
    delayMicroseconds(15);
  }
  myCAM.CS_HIGH();
  Serial.print(" ]");
  Serial.println("");
  Serial.println(F("Image save OK."));
  is_header = false;
  i = 0 ;
  return 1;
}
