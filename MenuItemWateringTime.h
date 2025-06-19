#ifndef MENU_ITEM_WATERING_TIME_H
#define MENU_ITEM_WATERING_TIME_H

#include <Arduino.h>
#include <MenuItem.h>
#include <RTClib.h>

extern RTC_DS1307 rtc;


class MenuItemWateringTime : public MenuItem {
private:
  const char* label;
  const char* h_settings_key;
  const char* m_settings_key;
  const char* timestamp_key;
  
  uint8_t * hours;
  uint8_t * minutes;
  uint32_t * last_watering_timestamp;
  uint8_t old_hours, old_minutes;

  uint8_t cursor =  0; // 0 = hours, 1 = minutes;

  int step = 15;

public:
  MenuItemWateringTime(const char* label, const char* h_settings_key, uint8_t* hours, const char* m_settings_key, uint8_t* minutes, const char* timestamp_key, uint32_t* last_watering_timestamp)
   : label(label), h_settings_key(h_settings_key), hours(hours), m_settings_key(m_settings_key), minutes(minutes), timestamp_key(timestamp_key), last_watering_timestamp(last_watering_timestamp) {}

  const char* getLabel() override {
    return label;
  }

  void draw(Adafruit_SSD1306* display) override {
    display->clearDisplay();

    display->setCursor(0, 16);
    printAlignedText(display, label, 2, 10, 1, editing ? 0 : 1);

    char buffer[6];
    if(!editing){
      sprintf(buffer, "%02u:%02u", *hours, *minutes);
    } else {
      if(cursor == 0) {
        sprintf(buffer, " %02u h", *hours);
      }  else {
        sprintf(buffer, " %02u m", *minutes);
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
      *hours = (*hours + 23) % 24;
    } else {
      *minutes = (*minutes + 60 - step) % 60;
    }
  }

  void onRight() override {
    if (!editing) return;
    if (cursor == 0) {
      *hours = (*hours + 1) % 24;
    } else {
      *minutes = (*minutes + step) % 60;
    }
  }

  void onClick() override {
    if (!editing) {
      editing = true;
      old_hours = *hours;
      old_minutes = *minutes;
    } else {
      if(cursor == 0) {
        cursor = 1;
      } else {
        
        if(enablePreference){
          prefs.begin("settings", false);
          if (*hours != old_hours) {
            prefs.putUChar(h_settings_key, *hours);
          }
          if(*minutes != old_minutes) {
            prefs.putUChar(m_settings_key, *minutes);
          }

          //reset last watering time so the system counts a new one
          *last_watering_timestamp = 0;
          //prefs.putUInt(timestamp_key, *last_watering_time_timestamp);

          prefs.end();
        }

        cursor = 0;
        editing = false;
      }
    }
  }

  void onBack() override {
    if (editing) {
      cursor = 0;
      *hours = old_hours;
      *minutes = old_minutes;
      editing = false;
    }
  }

};

#endif