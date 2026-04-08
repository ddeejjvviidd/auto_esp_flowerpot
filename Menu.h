/**
 * @file Menu.h
 * @brief File responsible for the menu system of the ESP32-powered smart flowerpot.
 * This file defines the Menu class.
 * @author ddeejjvviidd
 * @date 2026-04-08
 */

#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MenuItem.h"
#include <display_utils.h>

/**
 * @brief The Menu class manages an array of MenuItem objects, and allows
 * the user to interact with them using the functions.
 * @class Menu
 */
class Menu {
private:
  Adafruit_SSD1306* display; // display to draw on
  MenuItem** items; // array of menu items
  int menuSize = 0; // total count
  int totalItems = 0; // current position for appending new

  int currentItem = 0; // for browsing menu

  bool isInMenu = false;


public:
  /**
   * @brief Constructor for the Menu class. Initializes the menu with a given display
   * reference and menu page size. Allocates memory for the menu items array.
   * @param display Pointer to the initialized display objects to draw on.
   * @param menuSize The maximum number of menu items that can be added to the menu.
   */
  Menu(Adafruit_SSD1306* display, int menuSize) : display(display), menuSize(menuSize) {
    items = new MenuItem*[menuSize];
  }

  /**
   * @brief Adds a new menu item to the menu.
   * @param item Pointer to the MenuItem object to add.
   */
  void addMenuItem(MenuItem* item) {
    if(totalItems == menuSize) {
      return;
    }

    if(totalItems < menuSize){
      items[totalItems] = item;
      totalItems++;
    }
  };

  /**
   * @brief Adds a visibility condition to the last added menu item.
   * @param visibleCondition The condition that determines if the item is visible.
   */
  void addVisibleConditionToLastItem(std::function<bool()> visibleCondition) {
    if (totalItems > 0) {
      items[totalItems - 1]->setVisibleCondition(visibleCondition);
    }
  }

  /**
   * @brief Checks if the current menu item is in editing mode.
   * @return true if the current item is being edited, false otherwise.
   */
  bool isEditingItem() {
    return items[currentItem]->isEditing();
  }

  /**
   * @brief Draws the current menu item on the display, if the item is not in editing mode.
   */
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

  /**
   * @brief Moves to the next menu item. Skips items that are not visible. Skips
   * to the first item when end of the menu is reached.
   * If the current item is in editing mode, calls the onRight() function of the item instead.
   */
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

  /**
   * @brief Moves to the previous menu item. Skips items that are not visible. Skips
   * to the last item when beginning of the menu is reached.
   * If the current item is in editing mode, calls the onLeft() function of the item instead.
   */
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

  /**
   * @brief Forwards the click event to the current menu item. Calls the onClick()
   * function of the current item.
   */
  void select() {
    items[currentItem]->onClick();
    draw();
  };

  /**
   * @brief Forwards the back event to the current menu item. If the item is in editing mode.
   */
  void back(){
    if(items[currentItem]->isEditing() == true){
      items[currentItem]->onBack();
    }
    draw();
  };

  /**
   * @brief Navigates to the home menu item. Index 0.
   */
  void goHome(){
    currentItem = 0;
  }

};

#endif