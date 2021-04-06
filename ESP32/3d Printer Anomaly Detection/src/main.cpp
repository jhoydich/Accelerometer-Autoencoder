// Created by: Jeremiah Hoydich
// Based off of various tensorflow lite tutorials / readings

#include "accel_model.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "Wifi.h"


// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

// Accelerometer object
Adafruit_ADXL343 accel = Adafruit_ADXL343(3);


constexpr int kTensorArenaSize = 2800;
uint8_t tensor_arena[kTensorArenaSize];

// various globals for accelerometer data
float accelData[3], accelSum[3], initSum, finalSum, diff;

int count = 0;

// time
float t1, t2;

int logLevel = 0;

// wifi values
const char* ssid = "3d Printer Autoencoder";
const char* pwd = "pa55w0rD!";

}  // namespace

AsyncWebServer server(80);
IPAddress local_IP(192, 168, 4, 1);

// Replaces placeholder with LED state value
String processor(const String& var){
  return "String";
}



void setup() {
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    return;
  }

  WiFi.softAP(ssid, pwd);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });


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

  // Had issue loading only necessary ops, so I'll load them all
  static tflite::AllOpsResolver resolver;

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

  // Initializing the accelerometer
  if(!accel.begin())
  {
    while(1);
  }

  // starting accelerometer
  accel.setRange(ADXL343_RANGE_2_G);
  accel.setDataRate(ADXL343_DATARATE_200_HZ);

  server.begin();
  t1 = millis();
}

// The name of this function is important for Arduino compatibility.
void loop() {
  t2 = millis();

  // want 5 millisecond interval for 200 Hz
  if (t2 - t1 >= 5) {
    if (logLevel > 2) {
      TF_LITE_REPORT_ERROR(error_reporter, "Time: %f", t2 - t1);
    }
    
    t1 = t2;
  
    // getting accelerometer data
    sensors_event_t event;
    accel.getEvent(&event);
    accelData[0] = event.acceleration.x / 9.81;
    accelData[1] = event.acceleration.y / 9.81;   
    accelData[2] = event.acceleration.z / 9.81;
    
    if (logLevel > 2) {
      TF_LITE_REPORT_ERROR(error_reporter, "Data: %f, %f, %f \n", accelData[0], accelData[1], accelData[2]);
    }  
    
    // running sum
    for (int i = 0; i < 3; i++) {
      accelSum[i] += accelData[i];
    }

    count += 1;

    // Once we have 5 data points
    if (count == 5) {
      // Writing to input tensor
      for (int i = 0; i < 3; i++) {
          input->data.f[i] = accelSum[i]/5;
          initSum += accelSum[i] / 5;
          if (logLevel > 1) {
            TF_LITE_REPORT_ERROR(error_reporter, "Initial Sum: %f \n", initSum);
          }    
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
        TF_LITE_REPORT_ERROR(error_reporter, "Diff: %f ", diff);
        TF_LITE_REPORT_ERROR(error_reporter, "Input data: %f, %f, %f \n", accelSum[0], accelSum[1], accelSum[2]);
        TF_LITE_REPORT_ERROR(error_reporter, "Output data: %f, %f, %f \n", output->data.f[0], output->data.f[1], output->data.f[2]);       
      }


      // resetting variables for next iteration
      initSum = 0;
      finalSum = 0;
      count = 0;
      for (int i = 0; i < 3; i++) {
        accelSum[i] = 0;
      }

    }

  }

}
