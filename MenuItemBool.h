#ifndef MENU_ITEM_BOOL_H
#define MENU_ITEM_BOOL_H

#include <Arduino.h>
#include <MenuItem.h>

class MenuItemBool : public MenuItem {
private:
  const char* label;
  const char* settings_key;

  bool* value;
  bool oldValue;

public:
  MenuItemBool(const char* label, const char* settings_key, bool* value) : label(label), settings_key(settings_key), value(value) {}

  const char* getLabel() override {
    return label;
  }

  void draw(Adafruit_SSD1306* display) override {
    display->clearDisplay();
    display->setCursor(0, 16);
    printAlignedText(display, label, 2, 10, 1, editing ? 0 : 1);

    printAlignedText(display, *value ? "ON" : "OFF", 2, 33, 1, editing ? 1 : 0);
    
    if(editing) {
      print4ValuesInRow(display,"<",">","save","abort",1,57);
    }
    display->display();
  }

  void onLeft() override {
    if(editing) {
      *value = !(*value);
    }
  }

  void onRight() override {
    if(editing) {
      *value = !(*value);
    }
  }

  void onClick() override {
    if(!editing){
      // starting to edit
      editing = true;
      oldValue = *value;
    } else {
      // saving a new value

      if(*value != oldValue){
        //new value detected
        if(enablePreference){
          prefs.begin("settings", false);
          prefs.putBool(settings_key, *value);
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