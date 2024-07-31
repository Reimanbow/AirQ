#include <Arduino.h>
#include <M5Unified.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

M5Canvas canvas(&M5.Display);

// SCD40
#include <SensirionI2CScd4x.h>
SensirionI2CScd4x scd40;

// SEN55
#include <SensirionI2CSen5x.h>
SensirionI2CSen5x sen55;

// WiFi
const char* ssid = "";
const char* pass = "";
const char* server_addr = "";
const int server_port = 1883;
const char* device_ID = "AirQ";
const char* pub_topic = "/env";

WiFiClient client;
PubSubClient mqttClient(client);

void connectMqtt(){
  while(!mqttClient.connected()){
    if(mqttClient.connect(device_ID)){
      canvas.fillScreen(BLACK);
      canvas.setCursor(0, 0);
      canvas.print("environment sensing");
      canvas.pushSprite(0, 0);
    }else{
      canvas.fillScreen(BLACK);
      canvas.setCursor(0, 0);
      canvas.print("not connect MQTT");
      canvas.pushSprite(0, 0);
      delay(500);
    }
  }
}

void mqttLoop(){
  if(!mqttClient.connected()){
    connectMqtt();
  }
  mqttClient.loop();
}

void errorHandling(uint16_t error){
  canvas.clearDisplay();
  canvas.setCursor(0, 0);
  char errorMessage[256];
  errorToString(error, errorMessage, 256);
  canvas.println(errorMessage);
  canvas.pushSprite(0, 0);
}

void setup(){
  auto cfg = M5.config();
  M5.begin(cfg);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextSize(1);

  // 内蔵センサの初期化にはM5.In_I2Cを使う
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);
  
  pinMode(13, OUTPUT);
  pinMode(15, OUTPUT);
  Wire.begin(M5.In_I2C.getSDA(), M5.In_I2C.getSCL());

  // SCD40
  uint16_t error;
  char errorMessage[256];

  scd40.begin(Wire);
  error = scd40.stopPeriodicMeasurement();
  
  if(error){
    errorHandling(error);
    return;
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  scd40.getSerialNumber(serial0, serial1, serial2);

  error = scd40.startPeriodicMeasurement();
  
  if(error){
    errorHandling(error);
    return;
  }

  // SEN55
  sen55.begin(Wire);
  error = sen55.deviceReset();
  if(error){
    errorHandling(error);
    return;
  }
  float tempOffset = 0.0F;
  error = sen55.setTemperatureOffsetSimple(tempOffset);
  if(error){
    errorHandling(error);
    return;
  }

  // WiFi
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED){
    canvas.fillScreen(BLACK);
    canvas.setCursor(0, 0);
    canvas.print("not connect WiFi");
    canvas.pushSprite(0, 0);
    delay(500);
  }
  mqttClient.setServer(server_addr, server_port);
  
  // start SEN55
  error = sen55.startMeasurement();
  if(error){
    errorHandling(error);
    return;
  }
}

// 1s周期でデータを取得する
void loop(){
  mqttLoop();
  unsigned long start_ms = millis();
  // canvas.clearDisplay();
  // canvas.setCursor(0, 0);

  uint16_t error;

  uint16_t co2 = 0;
  float temperature = 0.0f;
  float humidity = 0.0f;
  bool isDataReady = false;
  error = scd40.getDataReadyFlag(isDataReady);
  if(error){
    //errorHandling(error);
    return;
  }
  if(!isDataReady){
    //errorHandling(error);
    return;
  }
  error = scd40.readMeasurement(co2, temperature, humidity);
  if(error){
    //errorHandling(error);
    return;
  }
  // canvas.printf("SCD40\n");
  // canvas.printf("CO2: %d\n", co2);
  // canvas.printf("Temperature: %f\n", temperature);
  // canvas.printf("Humidity: %f\n\n", humidity);

  // SEN55
  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;

  error = sen55.readMeasuredValues(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
    massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
    noxIndex);
  if(error){
    //errorHandling(error);
    return;
  }

  float temp_avg = (temperature + ambientTemperature) / 2;
  float humi_avg = (humidity + ambientHumidity) / 2;

  // toJson
  char json[400];
  const size_t capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> doc;
  doc["CO2"] = co2;
  doc["temperature"] = temp_avg;
  doc["humidity"] = humi_avg;
  doc["pm1.0"] = massConcentrationPm1p0;
  doc["pm2.5"] = massConcentrationPm2p5;
  doc["pm4.0"] = massConcentrationPm4p0;
  doc["pm10.0"] = massConcentrationPm10p0;
  doc["voc"] = vocIndex;
  doc["nox"] = noxIndex;
  serializeJson(doc, json);
  mqttClient.publish(pub_topic, json);
  // canvas.printf("SEN55\n");
  // canvas.printf("PM1.0: %f\n", massConcentrationPm1p0);
  // canvas.printf("PM2.5: %f\n", massConcentrationPm2p5);
  // canvas.printf("PM4.0: %f\n", massConcentrationPm4p0);
  // canvas.printf("PM10.0: %f\n", massConcentrationPm10p0);
  // canvas.printf("Humidity: %f\n", ambientHumidity);
  // canvas.printf("Temperature: %f\n", ambientTemperature);
  // canvas.printf("VOC: %f\n", vocIndex);
  // canvas.printf("NOx: %f\n", noxIndex);

  // canvas.pushSprite(0, 0);
  while((millis() - start_ms) < 1000);
}
