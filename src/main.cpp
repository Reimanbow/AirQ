#include <Arduino.h>
#include <M5Unified.h>

M5Canvas canvas(&M5.Display);

void setup(){
  auto cfg = M5.config();
  M5.begin(cfg);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
}

void loop(){
  canvas.fillScreen(WHITE);
  canvas.setCursor(0, 0);
  canvas.printf("Hello World");
  canvas.pushSprite(0, 0);
}
