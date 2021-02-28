/* 
Created by: Jeremiah Hoydich
Created on: 1/22/21
Purpose: Test neural network model for tensorflow lite micro
This code is based off of the examples provided by the tensorflow team
*/

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_model.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/testing/micro_test.h"
#include "tensorflow/lite/schema/schema_generated.h"

TF_LITE_MICRO_TESTS_BEGIN

TF_LITE_MICRO_TEST(LoadModelAndPerformInference) {

  // Define the input and the expected output, they are the same since
  // the network is an autoencoder
  float accelData[3] = {0.0, 0.0, 1.0};
  int dataLen = 3;

  // Set up logging
  tflite::MicroErrorReporter micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  const tflite::Model* model = ::tflite::GetModel(accel_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(&micro_error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.\n",
                         model->version(), TFLITE_SCHEMA_VERSION);
  }

  // Loading only the necessary ops
  
  tflite::MicroMutableOpResolver<2> micro_op_resolver;
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddTanh();
  
  
  constexpr int kTensorArenaSize = 2800;
  uint8_t tensor_arena[kTensorArenaSize];

  // Build an interpreter to run the model with
  tflite::MicroInterpreter interpreter(model, micro_op_resolver, tensor_arena,
                                       kTensorArenaSize, &micro_error_reporter);
  // Allocate memory from the tensor_arena for the model's tensors
  TF_LITE_MICRO_EXPECT_EQ(interpreter.AllocateTensors(), kTfLiteOk);

  // Obtain a pointer to the model's input tensor
  TfLiteTensor* input = interpreter.input(0);

  // Make sure the input has the properties we expect
  TF_LITE_MICRO_EXPECT_NE(nullptr, input);

  // The property "dims" tells us the tensor's shape. It has one element for
  // each dimension. Our input is a 2D tensor containing 3 elements, so "dims"
  // should have size 2.

  TF_LITE_MICRO_EXPECT_EQ(2, input->dims->size);

  // The value of each element gives the length of the corresponding tensor.
  TF_LITE_MICRO_EXPECT_EQ(1, input->dims->data[0]);
  TF_LITE_MICRO_EXPECT_EQ(3, input->dims->data[1]);


  // The input is a 32 bit float
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteFloat32, input->type);



  // Place sample input into input tensor
  for (int i = 0; i < 3; i++) {
    input->data.f[i] = accelData[i];
  }
  

  // Run the model and check that it succeeds
  TfLiteStatus invoke_status = interpreter.Invoke();
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

  // Obtain a pointer to the output tensor and make sure it has the
  // properties we expect. It should be the same as the input tensor.
  TfLiteTensor* output = interpreter.output(0);
  TF_LITE_MICRO_EXPECT_EQ(2, output->dims->size);
  TF_LITE_MICRO_EXPECT_EQ(1, output->dims->data[0]);
  TF_LITE_MICRO_EXPECT_EQ(3, output->dims->data[1]);
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteFloat32, output->type);


  // Predicted values
  float accPredictions[3];
  for (int i = 0; i < dataLen; i++) {
    accPredictions[i] = output->data.f[i];
  }
  


  // Check if the output is within a small range of the expected output
  float epsilon = 0.1f;
  for (int i = 0; i < dataLen; i++) {
    TF_LITE_MICRO_EXPECT_NEAR(accPredictions[i], accelData[i], epsilon);
  }
  
}

TF_LITE_MICRO_TESTS_END
