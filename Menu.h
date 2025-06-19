#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MenuItem.h"
#include <display_utils.h>

class Menu {
private:
  Adafruit_SSD1306* display; // display to draw on
  MenuItem** items; // array of menu items
  int menuSize = 0; // total count
  int totalItems = 0; // current position for appending new

  int currentItem = 0; // for browsing menu

  bool isInMenu = false;


public:
  //constructor
  Menu(Adafruit_SSD1306* display, int menuSize) : display(display), menuSize(menuSize) {
    items = new MenuItem*[menuSize];
  }

  void addMenuItem(MenuItem* item) {
    if(totalItems == menuSize) {
      return;
    }

    if(totalItems < menuSize){
      items[totalItems] = item;
      totalItems++;
    }
  };

  void addVisibleConditionToLastItem(std::function<bool()> visibleCondition) {
    if (totalItems > 0) {
      items[totalItems - 1]->setVisibleCondition(visibleCondition);
    }
  }

  bool isEditingItem() {
    return items[currentItem]->isEditing();
  }

  void draw() {
    // display->clearDisplay();
    // display->setCursor(0, 16);
    // display->print(items[currentItem]->getLabel());
    // display->display();
    items[currentItem]->draw(display);
    if(items[currentItem]->isEditing() == false){
      print4ValuesInRow(display,(currentItem<1) ? "" : "<",(currentItem<totalItems-1) ? ">" : "","edit","back",1,57);
      display->display();
    }
  };

  void next() {
    if (!items[currentItem]->isEditing()) {
      int originalIndex = currentItem;
      do {
        currentItem++;
        if (currentItem >= totalItems) {
          currentItem = originalIndex; // žádná další viditelná položka
          break;
        }
      } while (!items[currentItem]->isVisible());
    } else {
      items[currentItem]->onRight();
    }
    draw();
  };

  void previous() {
    if (!items[currentItem]->isEditing()) {
      int originalIndex = currentItem;
      do {
        currentItem--;
        if (currentItem < 0) {
          currentItem = originalIndex; // žádná předchozí viditelná položka
          break;
        }
      } while (!items[currentItem]->isVisible());
    } else {
      items[currentItem]->onLeft();
    }
    draw();
  };

  void select() {
    items[currentItem]->onClick();
    draw();
  };

  void back(){
    if(items[currentItem]->isEditing() == true){
      items[currentItem]->onBack();
    }
    draw();
  };

  void goHome(){
    currentItem = 0;
  }

};

#endif