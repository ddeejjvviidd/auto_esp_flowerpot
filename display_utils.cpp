#include "display_utils.h"

void printAlignedText(Adafruit_SSD1306* display, const char* text, uint8_t size, int y, uint8_t align, uint8_t boxType) {
  display->setTextSize(size);
  int textWidth = strlen(text) * 6 * size;
  _printAlignedInternal(display, text, textWidth, size, y, align, boxType);
}

void printAlignedText(Adafruit_SSD1306* display, int value, uint8_t size, int y, uint8_t align, uint8_t boxType) {
  display->setTextSize(size);
  int len = (value == 0) ? 1 : (value < 0 ? 1 : 0);
  for (int v = value; v != 0; v /= 10) len++;
  int textWidth = len * 6 * size;
  _printAlignedInternal(display, value, textWidth, size, y, align, boxType);
}

void printAlignedText(Adafruit_SSD1306* display, float value, uint8_t size, int y, uint8_t align, uint8_t boxType) {
  display->setTextSize(size);
  int intPart = (int)value;
  int len = (intPart == 0) ? 1 : (intPart < 0 ? 1 : 0);
  for (int v = intPart; v != 0; v /= 10) len++;
  len += 3; // např. ".14"
  int textWidth = len * 6 * size;
  _printAlignedInternal(display, value, textWidth, size, y, align, boxType);
}

void print2ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, uint8_t size, int y) {
  display->setTextSize(size);
  int charWidth = 6 * size;
  int cellWidth = SCREEN_WIDTH / 2;

  display->setTextColor(SSD1306_WHITE);
  int x1 = (cellWidth - strlen(val1) * charWidth) / 2;
  int x2 = cellWidth + (cellWidth - strlen(val2) * charWidth) / 2;

  display->setCursor(x1, y);
  display->print(val1);
  display->setCursor(x2, y);
  display->print(val2);
  display->setTextSize(DEFAULT_TEXT_SIZE);
}

void print3ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, const char* val3, uint8_t size, int y) {
  display->setTextSize(size);
  int charWidth = 6 * size;
  int cellWidth = SCREEN_WIDTH / 3;

  display->setTextColor(SSD1306_WHITE);
  int x1 = (cellWidth - strlen(val1) * charWidth) / 2;
  int x2 = cellWidth + (cellWidth - strlen(val2) * charWidth) / 2;
  int x3 = 2 * cellWidth + (cellWidth - strlen(val3) * charWidth) / 2;

  display->setCursor(x1, y);
  display->print(val1);
  display->setCursor(x2, y);
  display->print(val2);
  display->setCursor(x3, y);
  display->print(val3);
  display->setTextSize(DEFAULT_TEXT_SIZE);
}

void print4ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, const char* val3, const char* val4, uint8_t size, int y) {
  display->setTextSize(size);
  int charWidth = 6 * size;
  int cellWidth = SCREEN_WIDTH / 4;

  display->setTextColor(SSD1306_WHITE);
  int x1 = (cellWidth - strlen(val1) * charWidth) / 2;
  int x2 = cellWidth + (cellWidth - strlen(val2) * charWidth) / 2;
  int x3 = 2 * cellWidth + (cellWidth - strlen(val3) * charWidth) / 2;
  int x4 = 3 * cellWidth + (cellWidth - strlen(val4) * charWidth) / 2;

  display->setCursor(x1, y);
  display->print(val1);
  display->setCursor(x2, y);
  display->print(val2);
  display->setCursor(x3, y);
  display->print(val3);
  display->setCursor(x4, y);
  display->print(val4);
  display->setTextSize(DEFAULT_TEXT_SIZE);
}

void print5ValuesInRow(Adafruit_SSD1306* display, const char* val1, const char* val2, const char* val3, const char* val4, const char* val5, uint8_t size, int y) {
  display->setTextSize(size);
  int charWidth = 6 * size;
  int cellWidth = SCREEN_WIDTH / 5;

  display->setTextColor(SSD1306_WHITE);

  int x1 = (cellWidth - strlen(val1) * charWidth) / 2;
  int x2 = cellWidth + (cellWidth - strlen(val2) * charWidth) / 2;
  int x3 = 2 * cellWidth + (cellWidth - strlen(val3) * charWidth) / 2;
  int x4 = 3 * cellWidth + (cellWidth - strlen(val4) * charWidth) / 2;
  int x5 = 4 * cellWidth + (cellWidth - strlen(val5) * charWidth) / 2;

  display->setCursor(x1, y);
  display->print(val1);
  display->setCursor(x2, y);
  display->print(val2);
  display->setCursor(x3, y);
  display->print(val3);
  display->setCursor(x4, y);
  display->print(val4);
  display->setCursor(x5, y);
  display->print(val5);

  display->setTextSize(DEFAULT_TEXT_SIZE); // obnova výchozí velikosti, pokud nějakou používáš
}
