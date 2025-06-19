#ifndef MENU_ITEM_RTC_SET_H
#define MENU_ITEM_RTC_SET_H

#include <Arduino.h>
#include <MenuItem.h>
#include <RTClib.h>

extern RTC_DS1307 rtc;


class MenuItemRTCSet : public MenuItem {
private:
  const char* label;
  uint8_t hours, minutes;
  uint8_t old_hours, old_minutes;
  uint32_t * last_watering_timestamp;

  uint8_t cursor =  0; // 0 = hours, 1 = minutes;

public:
  MenuItemRTCSet(const char* label, uint32_t* last_watering_timestamp) : label(label) {}

  const char* getLabel() override {
    return label;
  }

  void draw(Adafruit_SSD1306* display) override {
    display->clearDisplay();

    display->setCursor(0, 16);
    printAlignedText(display, label, 2, 10, 1, editing ? 0 : 1);

    char buffer[6];
    if(!editing){
      DateTime now = rtc.now();
      hours = now.hour();
      minutes = now.minute();
      sprintf(buffer, "%02u:%02u", hours, minutes);
    } else {
      if(cursor == 0) {
        sprintf(buffer, " %02u h", hours);
      }  else {
        sprintf(buffer, " %02u m", minutes);
      }
    }

    printAlignedText(display, buffer, 2, 33, 1, editing ? 1 : 0);

    if (editing) {
      print4ValuesInRow(display, "<", ">", "save", "abort", 1, 57);
    }

    display->display();
  }

  void onLeft() override {
    if (!editing) return;
    if (cursor == 0) {
      hours = (hours + 23) % 24;
    } else {
      minutes = (minutes + 59) % 60;
    }
  }

  void onRight() override {
    if (!editing) return;
    if (cursor == 0) {
      hours = (hours + 1) % 24;
    } else {
      minutes = (minutes + 1) % 60;
    }
  }

  void onClick() override {
    if (!editing) {
      editing = true;
      DateTime now = rtc.now();
      hours = old_hours = now.hour();
      minutes = old_minutes = now.minute();
    } else {
      if(cursor == 0) {
        cursor = 1;
      } else {
        // save to RTC modul
        if (hours != old_hours || minutes != old_minutes) {
          rtc.adjust(DateTime(2004, 5, 14, hours, minutes, 0));
          //*last_watering_timestamp = 0;
          cursor = 0;
        }
        editing = false;
      }
    }
  }

  void onBack() override {
    if (editing) {
      cursor = 0;
      hours = old_hours;
      minutes = old_minutes;
      editing = false;
    }
  }

};

#endif