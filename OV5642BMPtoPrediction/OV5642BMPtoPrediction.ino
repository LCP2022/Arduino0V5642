#include <TensorFlowLite.h>
//#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "object_model.h"

#include "Image.h"
#include "Pinconfig.h"
#include "Label.h"

const tflite::Model* tflModel = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr; 

// An area of memory to use for input, output, and intermediate arrays.
constexpr int tensorArenaSize = 60 * 1024;
uint8_t  tensorArena[tensorArenaSize];
float out[NUM_OBJECT]; 


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);

  OV5642Setup();
  tflModel = tflite::GetModel(object_model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    return;
  }
  //tflite::AllOpsResolver micro_op_resolver;
  static tflite::MicroMutableOpResolver<9> micro_op_resolver;
  micro_op_resolver.AddDequantize();
  micro_op_resolver.AddQuantize();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();
  micro_op_resolver.AddMean();

  static tflite::MicroInterpreter static_interpreter(tflModel, micro_op_resolver, tensorArena, tensorArenaSize);
  tflInterpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = tflInterpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }
  MicroPrintf("Allocate Tensors Successful");

  tflInputTensor = tflInterpreter->input(0);
  Serial.print("Input Array Size :");
  Serial.print(tflInputTensor->dims->size);
  Serial.print(" Array shape :");
  Serial.print(tflInputTensor->dims->data[0]);
  Serial.print(",");
  Serial.print(tflInputTensor->dims->data[1]);
  Serial.print(",");
  Serial.print(tflInputTensor->dims->data[2]);
  Serial.print(",");
  Serial.print(tflInputTensor->dims->data[3]);
  Serial.print(" Type :");
  Serial.println(tflInputTensor->type);
  // Types supported by tensor
  // kTfLiteNoType = 0,
  // kTfLiteFloat32 = 1,
  // kTfLiteInt32 = 2,
  // kTfLiteUInt8 = 3,
  // kTfLiteInt64 = 4,
  // kTfLiteString = 5,
  // kTfLiteBool = 6,
  // kTfLiteInt16 = 7,
  // kTfLiteComplex64 = 8,
  // kTfLiteInt8 = 9,
  tflOutputTensor = tflInterpreter->output(0);
  Serial.print("Output Array Size :");
  Serial.print(tflOutputTensor->dims->size);
  Serial.print(" Array shape :");
  for(int i =0;i< tflOutputTensor->dims->size;i++){
    Serial.print(tflOutputTensor->dims->data[i]);
    Serial.print(","); 
  }
  Serial.print(" Type :");
  Serial.println(tflOutputTensor->type);
}

void loop() {
  // put your main code here, to run repeatedly:
  TakeImage();
  uint8_t *Array = GetPixel();
  for(int i =0;i<RSHEIGHT;i++){
    for(int j =0;j<RSWIDTH;j++){
        float temp = Array[i*RSWIDTH+j] / 255.0;
        tflInputTensor->data.f[i*RSWIDTH+j] = temp;
        //Serial.print(Array[i*RSWIDTH+j]);
        //Serial.print(",");
    }
  // Serial.println("");
  }
  Serial.println("Pixel value slot in done");
  
  unsigned long startTime = millis(); // Record the start time
  TfLiteStatus invokeStatus = tflInterpreter->Invoke();
  unsigned long elapsedTime = millis() - startTime;
  Serial.print("Inference Done Elapsed Time (ms): ");
  Serial.println(elapsedTime);
  tflOutputTensor = tflInterpreter->output(0);
  for(int i =0;i< NUM_OBJECT;i++)
  {
     Serial.print(OBJECT[i]);
     Serial.print(" : ");
     out[i] = tflOutputTensor->data.f[i];
     Serial.println(out[i]);
  }

  float maxVal = out[0];
  int maxIndex = 0;
  for(int k =0; k < NUM_OBJECT;k++){
    if (out[k] > maxVal) {
         maxVal = out[k];
         maxIndex = k;
      } 
  }
  Serial.print("Number ");
  Serial.print(maxIndex);
  Serial.println(" detected");
  Serial.print("Confidence: ");
  Serial.println(maxVal);
}
