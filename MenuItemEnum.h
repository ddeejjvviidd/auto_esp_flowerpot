#ifndef MENU_ITEM_ENUM_H
#define MENU_ITEM_ENUM_H

#include <Arduino.h>
#include <MenuItem.h>

class MenuItemEnum : public MenuItem {
private:
  const char* label;
  const char* settings_key;

  const char** enums;
  int optionCount;
  int* value;
  int oldValue;
  int step = 1;

public:
  MenuItemEnum(const char* label, const char* settings_key, const char** enums, int optionCount, int* value) : label(label), settings_key(settings_key), enums(enums), optionCount(optionCount), value(value) {}

  const char* getLabel() override {
    return label;
  }

  void draw(Adafruit_SSD1306* display) override {
    display->clearDisplay();
    display->setCursor(0, 16);
    printAlignedText(display, label, 2, 10, 1, editing ? 0 : 1);
    
    printAlignedText(display, enums[*value], 2, 33, 1, editing ? 1 : 0);

    if(editing) {
      print4ValuesInRow(display,(*value == 0) ? "" : "<",(*value == optionCount-1) ? "" : ">","save","abort",1,57);
    }
    display->display();
  }

  void onLeft() override {
    if(editing && *value > 0) {
      *value = *value - step;
    }
  }

  void onRight() override {
    if(editing && *value < (optionCount-1)) {
      *value = *value + step;
    }
  }

  void onClick() override {
    if(!editing){
      // starting to edit
      editing = true;
      oldValue = *value;
    } else {

      if(*value != oldValue){

        if(enablePreference){
          prefs.begin("settings", false);
          prefs.putInt(settings_key, *value);
          prefs.end();
        }
      }

      editing = false;
    }
  }

  void onBack() override {
    if(editing){
      *value = oldValue; // return unsaved
      editing = false; // end of editing
    }
  }
  
};

#endif