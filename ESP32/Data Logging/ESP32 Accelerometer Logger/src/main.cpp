#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"



// queue for readings
static QueueHandle_t readingQueue;
static uint8_t readingQueueLen = 1000;

struct reading {
  float x;
  float y;
  float z;
  float t;
};

Adafruit_ADXL343 accel = Adafruit_ADXL343(3);

// prototypes
void readAccel(void *parameter);
void handleData(void *parameter);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    Serial.println("Ooops, no ADXL343 detected ... Check your wiring!");
    while(1);
  }

  // starting accelerometer
  accel.setRange(ADXL343_RANGE_2_G);
  accel.setDataRate(ADXL343_DATARATE_200_HZ);


  readingQueue = xQueueCreate(readingQueueLen, sizeof(reading));

  xTaskCreatePinnedToCore(&readAccel, "read accel", 5000, NULL, 10, NULL, 1);
  xTaskCreatePinnedToCore(&handleData, "handle data", 10000, NULL, 7, NULL, 0);
}


void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}


void readAccel(void *parameter) {
  reading r;
  
  while (1) {
    sensors_event_t event;
    accel.getEvent(&event);
    r.t = millis();
    r.x = event.acceleration.x;
    r.y = event.acceleration.y;   
    r.z = event.acceleration.z;
    
    // sending data into queue
    if (xQueueSend(readingQueue, (void *)&r, 10) != pdTRUE) {
        Serial.println("ERROR: Could not put item in reading queue.");
    }

    vTaskDelay(3 / portTICK_PERIOD_MS);
  }
}

void handleData(void *parameter) {
  reading r;
  char str[35], xstr[8], ystr[8], zstr[8], tstr[7];
  

  while (1) {
    if (xQueueReceive(readingQueue, (void *)&r, 0) == pdTRUE) {
      dtostrf(r.t, 8, 0, tstr);
      dtostrf(r.x, 7, 3, xstr);
      dtostrf(r.y, 7, 3, ystr);
      dtostrf(r.z, 7, 3, zstr);
      
      strcat(str, tstr);
      strcat(str, ",");
      strcat(str, xstr);
      strcat(str, ",");
      strcat(str, ystr);
      strcat(str, ",");
      strcat(str, zstr);
      strcat(str, "\n");
      
      Serial.println(str);

      memset(str, 0, 28);
    }

    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}