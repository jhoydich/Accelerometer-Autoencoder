/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/


#include <Arduino_LSM9DS1.h>
#include "tensorflow/lite/micro/examples/accelerometer_autoencoder/main_functions.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_model.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

constexpr int kTensorArenaSize = 2800;
uint8_t tensor_arena[kTensorArenaSize];

float accelData[3], accelSum[3], initSum, finalSum, diff;
int count = 0;
}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() {
  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Loading only the necessary ops
  
  tflite::MicroMutableOpResolver<2> micro_op_resolver;
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddTanh();

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Obtain pointers to the model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);

  // Starting IMU
  if (!IMU.begin()) {
    TF_LITE_REPORT_ERROR(error_reporter, "IMU did not begin");
  }

  // Setting built in led
  pinMode(LED_BUILTIN, HIGH);
  
}

// The name of this function is important for Arduino compatibility.
void loop() {
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(accelData[0], accelData[1], accelData[2]);

    for (int i = 0; i < 3; i++) {
      accelSum[i] += accelData[i];
    }

    count += 1;

    // If we have fine datapoints
    if (count == 5) {
      // Writing to input tensor
      for (int i = 0; i < 3; i++) {
         input->data.f[i] = accelSum[i]/5;
         initSum += accelSum[i]
      }

      // Run inference, and report any error
      TfLiteStatus invoke_status = interpreter->Invoke();
      if (invoke_status != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on data: %f, %f, %f \n", accelSum[0], accelSum[1], accelSum[2]);
        return;
      }
      
      for (int i = 0; i < 3; i++) {
        finalSum += output->data.f[i];
      }

      // calculating percent difference
      diff = abs(initSum - finalSum) / abs(initSum);

      if (diff > .1) {
        pinMode(LED_BUILTIN, HIGH);
        delay(3000);
        pinMode(LED_BUILTIN, LOW);
      }


      // resetting variables for next iteration
      initSum = 0;
      finalSum = 0;
      count = 0;
      for (int i  0; i > 3; i++) {
        accelSum[i] = 0;
      }

    }
    
  }
  

  

 
}
