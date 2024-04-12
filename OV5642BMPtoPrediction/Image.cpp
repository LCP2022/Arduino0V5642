#include "Image.h"

uint8_t vid =0 , pid = 0,temp,count = 1;
static int k = 0;
uint8_t *Pixel;
unsigned long startTime; // Variable to hold the start time
unsigned long elapsedTime;

ArduCAM myCAM( OV5642, OV5642_CS );

void OV5642Setup(){
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
void TakeImage(){
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
uint8_t read_fifo_burst_BMP(ArduCAM myCAM){
//-------------------BMP Image Save to SD-----------------------------
  uint8_t temp, temp_last;
  uint32_t length = 0;
  char str[8];
  static int k = 0, p = 0; // for name
  uint8_t VH, VL;
  byte buf[256];
  File outFile;
  startTime = millis(); // Record the start time

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
  for (temp = 0; temp < BMPIMAGEOFFSET; temp++){ 
    buf[p++]= bmp_header[temp];
    //Serial.print(bmp_header[temp],HEX);
  }
  outFile.write(buf, p);
  Serial.println("");
  p = 0;
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  int i = 0, j = 0;
  for (i = 0; i < HEIGHT; i++){ 
    for (j = 0; j < WIDTH; j++){
      VH = SPI.transfer(0x00);
      VL = SPI.transfer(0x00);
      if(p<256){
        buf[p++] = VL;
        buf[p++] = VH;
        //Serial.print(VL,HEX);
        //Serial.print(VH,HEX);
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
  p=0;
  elapsedTime = millis() - startTime;
  // Print the elapsed time to the serial monitor
  Serial.print("Image Save OK Elapsed Time (ms): ");
  Serial.println(elapsedTime);

//------------------------------------------------------------
  startTime = millis(); // Record the start time
  uint8_t *RSPixel = LoadFile(str);
  elapsedTime = millis() - startTime;
  Serial.print("Resize and Grayscale Done Elapsed Time (ms): ");
  Serial.println(elapsedTime);
  SetPixel(RSPixel);
  
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



  return 1;
}
uint8_t RGB565toGS(uint16_t hexvalue){
    // Extracting R, G, and B components from RGB565
    uint8_t r = (hexvalue & 0xF800) >> 8;  // 5 bits for red
    uint8_t g = (hexvalue & 0x07E0) >> 3;  // 6 bits for green
    uint8_t b = (hexvalue & 0x001F) << 3;  // 5 bits for blue
    
    // Converting to grayscale using the luminosity method (0.299*R + 0.587*G + 0.114*B)
    uint8_t gray = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
    return gray;
}
// Function to resize the image
uint8_t *resizeImageAverage(uint8_t *Pixel) {
  uint8_t ScaleFactor = 10;
  //Max pooling 
  uint8_t *RSPixel = (uint8_t *)malloc(RSWIDTH*RSHEIGHT* sizeof(uint8_t));
  for (int y = 0; y < RSHEIGHT; y++) {
        for (int x = 0; x < RSWIDTH; x++) {
            // Calculate the corresponding coordinates in the original image
            int startX = x * ScaleFactor;
            int startY = y * ScaleFactor;
            int endX = startX + ScaleFactor;
            int endY = startY + ScaleFactor;

            // Initialize the max value
            int sum = 0;

            // Find the maximum value in the block
            for (int i = startY; i < endY; i++) {
                for (int j = startX; j < endX; j++) {
                  if (i < HEIGHT && j < WIDTH){
                    sum += Pixel[i * WIDTH + j];
                  }
                }
            }

            // Store the maximum value in the resized image
            RSPixel[y * RSWIDTH + x] = (int) sum/(ScaleFactor*ScaleFactor);
        }
    }
    Serial.print("Resize to ");
    Serial.print(RSWIDTH);
    Serial.print(" x ");
    Serial.print(RSHEIGHT);
    Serial.println("Done!!!");
    return RSPixel;
}


void SetPixel(uint8_t *tempPixel){
  Pixel = tempPixel;
}
uint8_t *GetPixel(){
  return Pixel;
}
