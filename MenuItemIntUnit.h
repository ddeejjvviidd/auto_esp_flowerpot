#ifndef MENU_ITEM_INT_UNIT_H
#define MENU_ITEM_INT_UNIT_H

#include <Arduino.h>
#include <MenuItem.h>

class MenuItemIntUnit : public MenuItem {
private:
  const char* label;
  const char* settings_key;
  const char* unit = "";

  int* value;
  int oldValue;
  int step = 1;

  bool useLimits = false;
  int minVal = 0;
  int maxVal = 0;

public:
  MenuItemIntUnit(const char* label, const char* settings_key, int* value, int minVal, int maxVal, int step, const char* unit)
   : label(label), settings_key(settings_key), value(value), minVal(minVal), maxVal(maxVal), useLimits(true), step(step), unit(unit) {}
  
  const char* getLabel() override {
    return label;
  }

  void draw(Adafruit_SSD1306* display) override {
    display->clearDisplay();
    display->setCursor(0, 16);
    printAlignedText(display, label, 2, 10, 1, editing ? 0 : 1);
    
    char buffer[7];
    sprintf(buffer, "%d %s", *value, unit);  // Přidáme jednotky k hodnotě
    printAlignedText(display, buffer, 2, 33, 1, editing ? 1 : 0);

    if(editing) {
      print4ValuesInRow(display,(useLimits && *value == minVal) ? "" : "<", (useLimits && *value == maxVal) ? "" : ">","save","abort",1,57);
    }
    display->display();
  }

  void onLeft() override {
    if(editing) {
      int newVal = *value - step;
      if(!useLimits || newVal >= minVal){
        *value = newVal;
      }
    }
  }

  void onRight() override {
    if(editing) {
      int newVal = *value + step;
      if (!useLimits || newVal <= maxVal) {
        *value = newVal;
      }
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