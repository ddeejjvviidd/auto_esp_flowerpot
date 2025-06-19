#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

extern Preferences prefs;

class MenuItem {
private:

protected:
  Adafruit_SSD1306* display; // display to draw on
  std::function<bool()> visibleCondition = []() { return true; }; // visible by default
  bool editing = false;

  bool enablePreference = true;

public:
  virtual const char* getLabel() = 0;
  virtual void draw(Adafruit_SSD1306* display) = 0;
  virtual void onLeft() = 0;
  virtual void onRight() = 0;
  virtual void onClick() = 0;
  virtual void onBack() = 0;
  virtual ~MenuItem() {};

  void setVisibleCondition(std::function<bool()> condition) {
    visibleCondition = condition;
  }

  bool isEditing() {
    return editing;
  }

  bool isVisible() {
    return visibleCondition();
  }

};

#endif