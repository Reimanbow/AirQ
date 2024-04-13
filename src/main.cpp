#include <Arduino.h>
#include <M5Unified.h>
#include <Wire.h>

M5Canvas canvas(&M5.Display);

//SCD
#include <SensirionI2CScd4x.h>

SensirionI2CScd4x scd40;

void setup(){
  auto cfg = M5.config();
  M5.begin(cfg);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextSize(2);

  // 内蔵センサの初期化にはM5.In_I2Cを使う
  Wire.begin(M5.In_I2C.getSDA(), M5.In_I2C.getSCL());

  uint16_t error;
  char errorMessage[256];

  scd40.begin(Wire);
  error = scd40.stopPeriodicMeasurement();
  
  if(error){
    errorToString(error, errorMessage, 256);
    canvas.printf("%s", errorMessage);
    canvas.pushSprite(0, 0);
    delay(1000);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  scd40.getSerialNumber(serial0, serial1, serial2);

  error = scd40.startPeriodicMeasurement();
  
  if(error){
    errorToString(error, errorMessage, 256);
    canvas.printf("%s", errorMessage);
    canvas.pushSprite(0, 0);
    delay(1000);
  }
}

// 1s周期でデータを取得する
void loop(){
  unsigned long start_ms = millis();
  canvas.clearDisplay();
  canvas.setCursor(0, 0);

  uint16_t error;
  char errorMessage[256];

  uint16_t co2 = 0;
  float temperature = 0.0f;
  float humidity = 0.0f;
  bool isDataReady = false;
  error = scd40.getDataReadyFlag(isDataReady);
  if(error){
    return;
  }
  if(!isDataReady){
    return;
  }
  error = scd40.readMeasurement(co2, temperature, humidity);
  if(error){
    return;
  }
  canvas.printf("CO2: %d\n", co2);
  canvas.printf("Temperature: %f\n", temperature);
  canvas.printf("Humidity: %f\n", humidity);

  canvas.pushSprite(0, 0);
  while((millis() - start_ms) < 1000);
}
