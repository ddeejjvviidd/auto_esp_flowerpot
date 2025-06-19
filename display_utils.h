#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DEFAULT_TEXT_SIZE 2

template<typename T>
void _printAlignedInternal(Adafruit_SSD1306* display, T value, int textWidth, uint8_t size, int y, uint8_t align, uint8_t boxType) {
  int charWidth = 6 * size;
  int charHeight = 8 * size;
  int x;

  switch (align) {
    case 0: x = 0; break;
    case 1: x = (SCREEN_WIDTH - textWidth) / 2; break;
    case 2: x = SCREEN_WIDTH - textWidth; break;
    default: x = 0; break;
  }

  if (boxType == 1) {
    display->drawRect(0, y - 3, SCREEN_WIDTH, charHeight + 4, SSD1306_WHITE);
  } else if (boxType == 2) {
    display->drawRect(x - 2, y - 3, textWidth + 4, charHeight + 5, SSD1306_WHITE);
  }

  display->setCursor(x, y);
  display->setTextColor(SSD1306_WHITE);
  display->println(value);
  display->setTextSize(DEFAULT_TEXT_SIZE);
}

// Deklarace funkcí
void printAlignedText(Adafruit_SSD1306* display, const char* text, uint8_t size, int y, uint8_t align, uint8_t boxType);
void printAlignedText(Adafruit_SSD1306* display, int value, uint8_t size, int y, uint8_t align, uint8_t boxType);
void printAlignedText(Adafruit_SSD1306* display, float value, uint8_t size, int y, uint8_t align, uint8_t boxType);

void print2ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, uint8_t size, int y);
void print3ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, const char* val3, uint8_t size, int y);
void print4ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, const char* val3, const char* val4, uint8_t size, int y);
void print5ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, const char* val3, const char* val4, const char* val5, uint8_t size, int y);
#endif
