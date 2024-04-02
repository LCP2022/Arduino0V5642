#include <TensorFlowLite.h>
//#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"

#include "tensorflow/lite/schema/schema_generated.h"


  #include "object_model.h"
#include "imagezero.h"

const tflite::Model* tflModel = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr; 

// An area of memory to use for input, output, and intermediate arrays.
constexpr int tensorArenaSize = 40 * 1024;
uint8_t  tensorArena[tensorArenaSize];
float out[3]; 


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);
  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(object_model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    return;
  }
  //tflite::AllOpsResolver micro_op_resolver;
  static tflite::MicroMutableOpResolver<5> micro_op_resolver;
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();


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
//  Serial.print("Output Array Size :");
//  Serial.print(tflOutputTensor->dims->size);
//  Serial.print(" Array shape :");
//  Serial.print(tflOutputTensor->dims->data[0]);
//  Serial.print(",");
//  Serial.print(tflOutputTensor->dims->data[1]);
//  Serial.print(" Type :");
//  Serial.println(tflOutputTensor->type);

  for(int i =0;i<24;i++){
    for(int j =0;j<32;j++){
        tflInputTensor->data.f[i*24+j] = num[i*24+j] / 255.0;
//        Serial.print(tfinput->data.f[i*24+j]);
//        Serial.print(" ");
    }
//   Serial.println("");
  }
  Serial.println("Pixel value slot in done");

  TfLiteStatus invokeStatus = tflInterpreter->Invoke();
  tflOutputTensor = tflInterpreter->output(0);
  out[0] = tflOutputTensor->data.f[0];
  out[1] = tflOutputTensor->data.f[1];
  out[2] = tflOutputTensor->data.f[2];

  Serial.println(out[0]);
  Serial.println(out[1]);
  Serial.println(out[2]);

  float maxVal = out[0];
  int maxIndex = 0;
  for(int k =0; k < 3;k++){
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


void loop() {
  // put your main code here, to run repeatedly:
}
