#ifndef SDREAD_H
#define SDREAD_H

#include "Arduino.h"
#include "Pinconfig.h"
#include "Image.h"

const int headerSize = 66;
uint8_t * LoadFile(char* filename);  


#endif
