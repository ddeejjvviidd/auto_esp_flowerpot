/**
 * @file MenuItem.h
 * @brief File responsible for the menu item system of the ESP32-powered smart flowerpot.
 * This file defines the MenuItem class.
 * @author ddeejjvviidd
 * @date 2026-04-08
 */

#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

extern Preferences prefs;

/**
 * @brief The MenuItem class is an abstract base class for all menu items in the menu system.
 * Each menu item must implement the pure virtual functions defined in this class.
 * @class MenuItem 
 */
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