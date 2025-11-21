#include <M5Unified.h>

#include <stdlib.h>
#include <stdint.h>
#include "EEPROM.h"
#define MIN_TILT 0.15
#define SPEED 3
#define MEM_SIZE 512

void move_position(float, float);
void save_position();
void load_position();

uint16_t black_color = M5.Lcd.color565(0, 0, 0);
uint16_t red_color = M5.Lcd.color565(255, 0, 0);
uint8_t rect_x, rect_y = 0, rect_w = 50, rect_h = 20;
float acc_x, acc_y, acc_z;

void setup() {
  M5.begin();
  M5.Imu.init();
  Serial.begin(115200);
  Serial.flush();
  EEPROM.begin(MEM_SIZE); // schrijven van 0 t.e.m. 511
  M5.Lcd.fillScreen(black_color);
}

void loop() {
  M5.update();
  delay(20);
  M5.Lcd.clear();
  M5.Imu.getAccelData(&acc_x, &acc_y, &acc_z);
  Serial.printf("acc X: %f, acc Y: %f\n", acc_x, acc_y);
  Serial.flush();
  move_position(acc_x, acc_y);
  if (M5.BtnA.isPressed())
    save_position();
  if (M5.BtnB.isPressed())
    load_position();
  M5.Lcd.fillRect(rect_x, rect_y, rect_w, rect_h, red_color);
}

void save_position() {
  int addr = 0;
  EEPROM.writeByte(addr, rect_x);
  addr++;
  EEPROM.writeByte(addr, rect_y);
  EEPROM.commit();
}

void load_position() {
  int addr = 0;
  rect_x = EEPROM.readByte(addr);
  addr++;
  rect_y = EEPROM.readByte(addr);
}

void move_position(float acc_x, float acc_y) {
  if (acc_x > MIN_TILT) {
    M5.Lcd.fillRect(rect_x, rect_w, rect_w, rect_h, black_color);
    rect_x -= SPEED;
  } else if (acc_x < -MIN_TILT) {
    M5.Lcd.fillRect(rect_x, rect_y, rect_w, rect_h, black_color);
    rect_x += SPEED;
  }
  if (acc_y > MIN_TILT) {
    M5.Lcd.fillRect(rect_x, rect_y, rect_w, rect_h, black_color);
    rect_y += SPEED;
  } else if (acc_y < -MIN_TILT) {
    M5.Lcd.fillRect(rect_x, rect_y, rect_w, rect_h, black_color);
    rect_y -= SPEED;
  }
  if (rect_y >= M5.Lcd.height())
    rect_y = 0;
  if (rect_x >= M5.Lcd.width())
    rect_x = 0;
}