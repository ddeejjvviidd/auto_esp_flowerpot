/**
 * @file main.ino
 * @brief Main file for ESP32-powered smart flowerpot.
 * @author ddeejjvviidd
 * @date 2026-04-08
 * This file contains the core setup and a loop of the functionality.
 */

#define DEBUG_MODE 0

#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define DEBUG_BEGIN(x) Serial.begin(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
	#define DEBUG_PRINTF(...)
  #define DEBUG_BEGIN(x)
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <vector>
#include <DHT.h>
#include <Preferences.h>

#include "kvetinac_utils.h"
#include <display_utils.h>

#include "Menu.h"
#include "MenuItemInt.h"
#include "MenuItemIntUnit.h"
#include "MenuItemBool.h"
#include "MenuItemEnum.h"
#include "MenuItemRTCSet.h"
#include "MenuItemWateringTime.h"

#define DEBOUNCE_DELAY 50

#define WATERING_TIME_PER_100ML 10 // seconds
#define WATER_FILL_WAIT_TIME (60000 * 10) // 10 minut

#define PUMP_SAFETY_SHUTOFF (51UL * 1000UL) // 51 seconds
#define PUMP_MOISTURE_SAFETY_SHUTOFF (20UL * 1000UL) // 20 seconds

#define WET_SOIL 1100 // namereno ponorenim do vody
#define DRY_SOIL 2500 // uplne suchy sensor

#define MOISTURE_MODE_COOLDOWN (60UL * 1000UL) // 60 seconds

#define MENU_AFK_TIMEOUT (20UL * 1000UL) // 20 seconds
#define SYSTEM_SLEEP_TIMEOUT (120UL * 1000UL) // 120 seconds

#define MANUAL_START_WATERING 25
#define MANUAL_WATERING_STEP 25
#define MANUAL_MAX_WATERING 200

//======================================
//            GPIO DEFINES
//======================================
#define b1 25 // <
#define b2 26 // >
#define b3 27 // ok / enter menu
#define b4 14 // back / manual watering / wake

#define LEFT_BUTTON b1_push
#define RIGHT_BUTTON b2_push
#define OK_BUTTON b3_push
#define MENU_BUTTON b3_push
#define BACK_BUTTON b4_push

#define OLED_SDA 21 // blue cable
#define OLED_SCL 22 // white cable
#define OLED_RES 19 // yellow cable
#define OLED_DC 18 // yellow cable

#define RTC_SDA 32 // white cable
#define RTC_SCL 33 // blue cable

#define SOIL_SENSOR_PIN 34

#define DHT_PIN 13 
#define DHT_TYPE DHT11

#define BATTERY_VCC 35

#define LED1 2
#define LED2 4

#define PUMP_RELAY 16
#define TANK_SENSOR 17

#define MENU_SCROLL_INTERVAL (20UL * 1000UL) // 10 seconds


// --------------- GLOBAL VARS ---------------

enum screenStates{
	TO_INFO_SCREEN,
	INFO_SCREEN,
	TO_MENU_SCREEN,
	MENU_SCREEN,
	MANUAL_WATERING
};

int screenState = INFO_SCREEN;

enum PeriodicalWatering {
	PERIODICAL_IDLE,
	PERIODICAL_ACTIVE,
	PERIODICAL_REFILL,
	PERIODICAL_COMPLETE
};

enum MoistureWatering {
	MOISTURE_IDLE,
	MOISTURE_WATERING,
	MOISTURE_COMPLETE
};

PeriodicalWatering periodicalWateringState = PERIODICAL_IDLE;
MoistureWatering moistureWateringState = MOISTURE_IDLE;

enum MainMenu {
	WATERING_INFO,
	SOIL_INFO,
	DHT_TEMP,
	DHT_HUMIDITY,
	BATTERY_INFO,
	TIME_INFO,
};

const int mainMenuItemCount = 6;

int mainMenuTotal = 6;
int mainMenuDefaultPage;
MainMenu mainMenuState = WATERING_INFO;

unsigned long wateringStartTime = 0;
unsigned long remainingWateringTime = 0;
unsigned long refillStartTime = 0;
unsigned long systemStartTime;

unsigned long moistureWateringStartTime = 0;
unsigned long moistureCooldownStartTime = 0;

unsigned long now;
unsigned long lastInteraction;
unsigned long lastSystemInteraction;

unsigned long b1_lastDebounceTime = 0;
unsigned long b2_lastDebounceTime = 0;
unsigned long b3_lastDebounceTime = 0;
unsigned long b4_lastDebounceTime = 0;

bool b1_lastState = false;
bool b2_lastState = false;
bool b3_lastState = false;
bool b4_lastState = false;

bool b1_pressed = false;
bool b2_pressed = false;
bool b3_pressed = false;
bool b4_pressed = false;

bool b1_push = false;
bool b2_push = false;
bool b3_push = false;
bool b4_push = false;

int wateringMode;

const char * wateringModeEnums[] = {
	"Manual", "Periodical", "Sensor"
};

bool deepSleep;

Menu* menu;

Preferences prefs;

// for periodical watering
uint8_t h_watering_time;
uint8_t m_watering_time;
uint32_t last_watering_timestamp;
int per_days;
int amount;

//for boundaries watering
int lowThreshold;
int highThreshold;

int adcLowThreshold = 0;
int adcHighThreshold = 0;

unsigned long b4_pressStart = 0;
unsigned long b4_pressDuration = 0;
bool b4_wasPressed = false;

const int SAMPLE_COUNT = 10;
int soil_moisture_buffer[SAMPLE_COUNT] = {};
int sampleIndex = -1;
int soil_moisture = 1500;

const int BATTERY_SAMPLE_COUNT = 20;
int battery_adc_buffer[BATTERY_SAMPLE_COUNT] = {};
int battery_sample_index = 0;

float battery_voltage = 4.10;
int battery_percent = 99;

unsigned long manualWateringStartTime = 0;
bool manualWateringInProgress = false;
unsigned long manualWateringDuration = 0; // celkový čas pro aktuální zavlažování
int manualWateringAmount = 0; // kolik vody se má dodat (ml)
bool b4_holdActive = false;
unsigned long b4_holdStart = 0;

bool autoScrollMenu;
unsigned long lastMenuScrollTime = 0;

bool rtcFAILED = false;

unsigned long lastPrintMillis = 0; // TODO: delete


//======================================
//        SPI DISPLAY SETUP
//======================================

//TwoWire WireOLED = TwoWire(0);

#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &WireOLED, -1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_SDA, OLED_SCL, OLED_DC, OLED_RES, -1);
// textSize, charW, charH
// 1				 6			8
// 2				 12			16
// 3 				 18			24
 
int defaultTextSize = 2;
uint16_t defaultDisplayColor = SSD1306_WHITE;



// --------------- I2C RTC ---------------
TwoWire WireRTC = TwoWire(1);
RTC_DS1307 rtc;


// --------------- DHT11 TEMP SENSOR ---------------
DHT dht(DHT_PIN, DHT_TYPE);


/**
 * @brief Arduino setup function. Init of all components, peripherals and variables.
 * Also contains loading animation for OLED. Safety checks are performed for 
 * RTC module and OLED. Based on RTC availability, some features may be disabled
 * and user is informed on the display. 
 * @warning If OLED initialization fails, the system will halt in an infinite loop.
 * 
 */
void setup() {

	// --------------- CORE SETUP ---------------
	DEBUG_BEGIN(9600);
	//delay(2000);
	DEBUG_PRINTLN("Boot successful.");
	setCpuFrequencyMhz(80);

	DEBUG_PRINT("CPU running at: ");
	DEBUG_PRINT(getCpuFrequencyMhz());
	DEBUG_PRINTLN(" Mhz.");

	// --------------- PIN MODES ---------------
	pinMode(b1, INPUT_PULLUP);
	pinMode(b2, INPUT_PULLUP);
	pinMode(b3, INPUT_PULLUP);
	pinMode(b4, INPUT_PULLUP);

	pinMode(DHT_PIN, INPUT_PULLUP);

	pinMode(PUMP_RELAY, OUTPUT);
	pinMode(TANK_SENSOR, INPUT_PULLUP);

	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);

	analogSetPinAttenuation(SOIL_SENSOR_PIN, ADC_11db);   // měří až do ~3,3 V
	analogSetPinAttenuation(BATTERY_VCC, ADC_11db);
	analogSetWidth(12); 


	// --------------- SPI OLED INIT ---------------
	//WireOLED.begin(OLED_SDA, OLED_SCL);
	//WireOLED.setClock(100000);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    DEBUG_PRINTLN(F("Failed to initialize OLED!"));
    for(;;);
  } else {
		display.ssd1306_command(SSD1306_DISPLAYOFF);
		delay(100);  // small delay to ensure it works
		display.ssd1306_command(SSD1306_DISPLAYON);
		display.clearDisplay();
		display.setTextSize(defaultTextSize);
		display.setTextColor(defaultDisplayColor);
    DEBUG_PRINTLN(F("OLED init successful."));
	}

	// --------------- I2C RTC INIT ---------------
  WireRTC.begin(RTC_SDA, RTC_SCL);
	if (!rtc.begin(&WireRTC)) {
		DEBUG_PRINTLN(F("Failed to initialize RTC!"));
		display.clearDisplay();
		display.setCursor(0,16);
		display.print("RTC fail");
		display.setTextSize(1);
		display.setCursor(0,50);
		display.print("No periodic");
		rtcFAILED = true;
		display.display();
		delay(2000);
  } else {
		DEBUG_PRINTLN(F("RTC init successful."));
	}

	//if (!rtc.isrunning() && !rtcFAILED) {
  //  DEBUG_PRINTLN(F("RTC was not running, check battery."));
  //  DEBUG_PRINTLN(F("Setting RTC time."));
	//	rtc.adjust(DateTime(2004, 5, 14, 0, 0, 0));
  //}

	for (int i = 0; i < 32; i++) {  // 16 kroků animace
			drawLoadingCircle(display, i);  // Vykreslí kolečko s odpovídajícím krokem
			delay(50);  // Čekání mezi jednotlivými kroky animace
	}

	// --------------- DHT INIT ---------------
	dht.begin();


	// --------------- SETTINGS MENU INIT ---------------
	menu = new Menu(&display, 10);

	prefs.begin("settings", true);
	wateringMode = prefs.getInt("w_mode", 0); // settings key: w_mode
	if(rtcFAILED && wateringMode == 1) {
		wateringMode = 0;
	}
	h_watering_time = prefs.getUChar("h_time", 7); // settings key: h_time
	m_watering_time = prefs.getUChar("m_time", 0);; // settings key: m_time
	last_watering_timestamp = prefs.getUInt("w_ts", 0); // setting key: w_ts
	per_days = prefs.getInt("per_days", 1); // settings key: per_days
	amount = prefs.getInt("amount", 100); // setting key: amount    (ml)
	lowThreshold = prefs.getInt("lowT", 20); // setting key: lowT
	highThreshold = prefs.getInt("highT", 80); // setting key: highT
	deepSleep = prefs.getBool("deepsleep", true); // setting key: deepsleep
	autoScrollMenu = prefs.getBool("autoscrl", true); // settings key: autoscrl
	mainMenuDefaultPage = prefs.getInt("dmenup", 0); // settings key: dmenup
	prefs.end();

	mainMenuState = (MainMenu)mainMenuDefaultPage;

	adcLowThreshold = moisturePercentToAdc(lowThreshold);
	adcHighThreshold = moisturePercentToAdc(highThreshold);

	menu->addMenuItem(new MenuItemEnum("Mode", "w_mode", wateringModeEnums, 3, &wateringMode));

	menu->addMenuItem(new MenuItemWateringTime("Day time", "h_time", &h_watering_time, "m_time", &m_watering_time, "w_ts", &last_watering_timestamp));
	menu->addVisibleConditionToLastItem([]() {
		return wateringMode == 1;
	});

	menu->addMenuItem(new MenuItemInt("Per days", "per_days", &per_days, 1, 7));
	menu->addVisibleConditionToLastItem([]() {
		return wateringMode == 1;
	});

	menu->addMenuItem(new MenuItemIntUnit("Amount", "amount", &amount, 25, 500, 25, "ml"));
	menu->addVisibleConditionToLastItem([]() {
		return wateringMode == 1;
	});

	menu->addMenuItem(new MenuItemIntUnit("Low", "lowT", &lowThreshold, 10, 50, 5, "%"));
	menu->addVisibleConditionToLastItem([]() {
		return wateringMode == 2;
	});
	menu->addMenuItem(new MenuItemIntUnit("High", "highT", &highThreshold, 55, 90, 5, "%"));
	menu->addVisibleConditionToLastItem([]() {
		return wateringMode == 2;
	});

	if(!rtcFAILED){
		menu->addMenuItem(new MenuItemRTCSet("Time now", &last_watering_timestamp));
	}

	menu->addMenuItem(new MenuItemBool("Sleep", "deepsleep", &deepSleep));

	menu->addMenuItem(new MenuItemBool("Menu scrl", "autoscrl", &autoScrollMenu));

	menu->addMenuItem(new MenuItemInt("Menu page", "dmenup", &mainMenuDefaultPage, 0, 5));
	
	// mozna menu swipe

	systemStartTime = millis();
	lastSystemInteraction = millis();
	lastMenuScrollTime = millis();
	moistureCooldownStartTime = millis() + MOISTURE_MODE_COOLDOWN;
}


/**
 * @brief Arduino main loop funtion.
 * Handles button input, sensor reading, watering logic, display updates, 
 * screen state management, sleep management.
 */
void loop() {

	handleButtons();

	readSoilMoistureADC(); 

	readBatteryADC();

	if(digitalRead(TANK_SENSOR)){
		digitalWrite(LED2, HIGH);
		//DEBUG_PRINTLN("Empty tank!");
		digitalWrite(PUMP_RELAY, LOW);
	} else {
		digitalWrite(LED2, LOW);
		//digitalWrite(LED1, LOW);
	}

	if(battery_voltage < 3.40){
		digitalWrite(LED1, HIGH);
	} else {
		digitalWrite(LED1, LOW);
	}

	if(rtcFAILED && wateringMode == 1) {
		wateringMode = 0;
	}

	//float batteryVoltage = analogRead(BATTERY_VCC);
	//DEBUG_PRINTLN(batteryVoltage);

	//float teplota = dht.readTemperature();
  //float vlhkost = dht.readHumidity();
//
	//if (isnan(teplota) || isnan(vlhkost)) {
  //  DEBUG_PRINTLN("Chyba při čtení z DHT senzoru!");
  //  return;
  //}

	//DEBUG_PRINT("Teplota: ");
  //DEBUG_PRINT(teplota);
  //DEBUG_PRINTLN(" °C");
//
  //DEBUG_PRINT("Vlhkost: ");
  //DEBUG_PRINT(vlhkost);
  //DEBUG_PRINTLN(" %");

	//if(digitalRead(TANK_SENSOR)){
	//	DEBUG_PRINTLN("Prazdna nadrz!");
	//}

	switch(screenState) {
		case INFO_SCREEN:
			
			if (millis() - systemStartTime > 5000) {
				handleWatering();
			}

			if(periodicalWateringState != PERIODICAL_IDLE || moistureWateringState != MOISTURE_IDLE){
				//zajisti, ze pokud je v cinnosti zalevani, nic jineho se nestane
				return;
			}

			handleSleep();

			//display.clearDisplay();
			//display.setCursor(0,16);
			//display.print("Main menu");
			//display.display();

			handleMainMenu(); // all the fancy drawing and shit

			if(MENU_BUTTON) {
				screenState = MENU_SCREEN;
				menu->draw();
			}

			if(b1_push) { // doleva
				mainMenuState = (MainMenu)((mainMenuState - 1 + mainMenuItemCount) % mainMenuItemCount);
				lastMenuScrollTime = now;
			} else if (b2_push || (autoScrollMenu && (millis() - lastMenuScrollTime >= MENU_SCROLL_INTERVAL))) { // doprava
				mainMenuState = (MainMenu)((mainMenuState + 1) % mainMenuItemCount);
				lastMenuScrollTime = now;
			}

			//cca 10000ms pro 100ml

			//if(millis() - lastPrintMillis >= 10000) {
			//	lastPrintMillis = millis();
			//	DEBUG_PRINTF("Last watering timestamp: %lu\n", last_watering_timestamp);
			//	DEBUG_PRINTF("Next watering should be: %lu\n", last_watering_timestamp + (per_days * 86400UL));
			//}

			if(b4_pressed){
				if(!b4_holdActive && !digitalRead(TANK_SENSOR)) {
					b4_holdStart = millis();
					b4_holdActive = true;
				} else if (millis() - b4_holdStart >= 1000) {
					screenState = MANUAL_WATERING;
					manualWateringAmount = MANUAL_START_WATERING; // začínáme s 25 ml
					manualWateringInProgress = false; // neprobíhá žádné zalévání
					b4_holdActive = false; // reset
				}
			} else {
				b4_holdActive = false; // reset
			}
			
			break;

		case MENU_SCREEN:
			lastSystemInteraction = millis();

			if(LEFT_BUTTON) menu->previous();
			if(RIGHT_BUTTON) menu->next();

			if(OK_BUTTON) menu->select();

			if(BACK_BUTTON && !menu->isEditingItem()) {
				screenState = INFO_SCREEN;
				menu->goHome();
				adcLowThreshold = moisturePercentToAdc(lowThreshold);
				adcHighThreshold = moisturePercentToAdc(highThreshold);
        b4_pressed = false;
        b4_holdActive = false;
        b4_lastDebounceTime = 0;
        b4_lastState = false;
			} else if (BACK_BUTTON) {
				menu->back();
			}

			//timeout AFK
			if((millis() > lastInteraction + MENU_AFK_TIMEOUT) && !menu->isEditingItem()){
				screenState = INFO_SCREEN;
				menu->goHome();
				adcLowThreshold = moisturePercentToAdc(lowThreshold);
				adcHighThreshold = moisturePercentToAdc(highThreshold);
			}
			
			break;

		case MANUAL_WATERING:
			lastSystemInteraction = millis();

			if(digitalRead(TANK_SENSOR)) {
				//safety shutoff
				digitalWrite(PUMP_RELAY, LOW);
				digitalWrite(LED2, HIGH);
				manualWateringInProgress = false;
				screenState = INFO_SCREEN;
				break;
			}
			
			if (!manualWateringInProgress) {
				// start
				digitalWrite(PUMP_RELAY, HIGH);
				manualWateringStartTime = millis();
				manualWateringDuration = WATERING_TIME_PER_100ML * manualWateringAmount / 100 * 1000UL; // přepočet na čas v ms
				manualWateringInProgress = true;
			}

			display.clearDisplay();
			printAlignedText(&display, "Manual", 2, 10, 1, 0);
			char printAmount[10];
			sprintf(printAmount, "%dml", manualWateringAmount);
    	printAlignedText(&display, printAmount, 2, 33, 1, 0);
			display.display();

			if (manualWateringInProgress) {

				if (millis() - manualWateringStartTime >= manualWateringDuration) {

					// Pokud uživatel drží tlačítko a není dosaženo limitu
					if (b4_pressed && manualWateringAmount + MANUAL_WATERING_STEP <= MANUAL_MAX_WATERING) {
						manualWateringAmount += MANUAL_WATERING_STEP;
						manualWateringDuration = WATERING_TIME_PER_100ML * manualWateringAmount / 100 * 1000UL; // znovu vypocitat
					} else {
						digitalWrite(PUMP_RELAY, LOW);
						manualWateringInProgress = false;
						screenState = INFO_SCREEN;
					}
				}

			}

			break;

		default:
			screenState = INFO_SCREEN;
			break;
	}
}


/**
 * @brief Handles drawing of the main info screen and all of its pages.
 * Based on the current mainMenuState, different information is drawn on the display.
 */
void handleMainMenu() {
  
	display.clearDisplay();
	display.setCursor(122,1);
	display.setTextSize(1);
	display.print(mainMenuState);
	display.setTextSize(2);

	switch(mainMenuState){
		case WATERING_INFO:
			if (wateringMode == 0) {
				printAlignedText(&display, "Manual", 2, 10, 1, 0);
    		printAlignedText(&display, "hold btn", 2, 33, 1, 0);
			} else if (wateringMode == 1 && !rtcFAILED) {
				printAlignedText(&display, "Periodical", 2, 10, 1, 0);

				time_t now_ts = rtc.now().unixtime();
				uint32_t next_day_ts = last_watering_timestamp + per_days * 86400UL;
  			next_day_ts = (next_day_ts / 86400UL) * 86400UL + h_watering_time * 3600UL + m_watering_time * 60UL;

				char autoModeStr[10];

				if (now_ts >= next_day_ts) {
					snprintf(autoModeStr, sizeof(autoModeStr), "soon");
				} else {
					uint32_t remaining = next_day_ts - now_ts;
					uint16_t days = remaining / 86400UL;
					uint8_t hours = (remaining % 86400UL) / 3600UL;
					uint8_t minutes = (remaining % 3600UL) / 60UL;

					if (days > 0) {
						snprintf(autoModeStr, sizeof(autoModeStr), "in %dd%dh", days, hours);
					} else if (hours > 0) {
						snprintf(autoModeStr, sizeof(autoModeStr), "in %dh%dm", hours, minutes);
					} else {
						snprintf(autoModeStr, sizeof(autoModeStr), "in %d min", minutes);
					}
				}
    		printAlignedText(&display, autoModeStr, 2, 33, 1, 0);

			} else if (wateringMode == 2) {
				printAlignedText(&display, "Sensor", 2, 10, 1, 0);
				char sensorModeStr[10];
				snprintf(sensorModeStr, sizeof(sensorModeStr), "%d%%<%d%%", adcToMoisturePercent(soil_moisture), lowThreshold);
    		printAlignedText(&display, sensorModeStr, 2, 33, 1, 0);
			}
			break;

		case SOIL_INFO: {
			printAlignedText(&display, "Soil", 2, 10, 1, 0);
			
			int moist = adcToMoisturePercent(soil_moisture);
			char soilStr[10];
			if (moist > 66){
				snprintf(soilStr, sizeof(soilStr), "%d%% wet", moist);
			} else if (moist > 33) {
				snprintf(soilStr, sizeof(soilStr), "%d%% ok", moist);
			} else {
				snprintf(soilStr, sizeof(soilStr), "%d%% low", moist);
			}
				
    	printAlignedText(&display, soilStr, 2, 33, 1, 0);
			break;
		}

		case DHT_TEMP: {
			float dhtTemp = dht.readTemperature();

			printAlignedText(&display, "Temp", 2, 10, 1, 0);

			char dhtSecondLine[10];
			snprintf(dhtSecondLine, sizeof(dhtSecondLine), "%.1fC", dhtTemp);
    	printAlignedText(&display, dhtSecondLine, 2, 33, 1, 0);
			break;
		}

		case DHT_HUMIDITY: {
  		float dhtHumidity = dht.readHumidity();

			printAlignedText(&display, "Air hum.", 2, 10, 1, 0);

			char dhtSecondLine[10];
			snprintf(dhtSecondLine, sizeof(dhtSecondLine), "%.0f%%", dhtHumidity);
    	printAlignedText(&display, dhtSecondLine, 2, 33, 1, 0);
			break;
		}

		case BATTERY_INFO: {

			printAlignedText(&display, "Battery", 2, 10, 1, 0);

			char dhtSecondLine[10];
			//snprintf(dhtSecondLine, sizeof(dhtSecondLine), "%.2fV", battery_voltage); // battery_percent
			snprintf(dhtSecondLine, sizeof(dhtSecondLine), "%d%%", battery_percent); // battery_percent
    	printAlignedText(&display, dhtSecondLine, 2, 33, 1, 0);

			break;
		}

		case TIME_INFO: {
			
			DateTime currentTime;
			if(!rtcFAILED){
				currentTime = rtc.now();
			} else {
    		currentTime = DateTime(2004, 5, 14, 0, 0, 0);
			}

			printAlignedText(&display, "Time", 2, 10, 1, 0);

			char dhtSecondLine[10];
			if(!rtcFAILED){
				snprintf(dhtSecondLine, sizeof(dhtSecondLine), "%02d:%02d:%02d", currentTime.hour(), currentTime.minute(), currentTime.second());
			} else {
				snprintf(dhtSecondLine, sizeof(dhtSecondLine), "RTC fail");
			}
    	printAlignedText(&display, dhtSecondLine, 2, 33, 1, 0);

			break;
		}

		default:
			mainMenuState = WATERING_INFO;
			break;
	}

	display.setCursor(1, 54);
	display.drawLine(0, 54, 128 - 1, 54, SSD1306_WHITE);
	print4ValuesInRow(&display, "<", ">", "menu", "pump", 1, 57);
  display.display();
}


/**
 * @brief Handles both of the automatic watering modes.
 * In periodical mode, this function checks last time of watering
 * and compares it to the current time. If the set period has passed,
 * watering starts.
 * In sensor mode, soil moisture is continuously monitored and compared
 * to the set threshold. If the soil is too dry, watering starts.
 * @warning This function does not handle the manual watering mode, 
 * which is triggered whenever by holding the b4 button on the info screen.
 */
void handleWatering(){

	if (wateringMode == 1) {
		//periodical watering
		DateTime now = rtc.now();
		uint32_t now_ts = now.unixtime();


		if(last_watering_timestamp <= 1083708000 || last_watering_timestamp > (now_ts + 86400UL)) {
			// probehl reset, nebo je neco spatne, nutno spocitat znovu
			DateTime newTarget(now.year(), now.month(), now.day(), h_watering_time, m_watering_time, 0);

			if(newTarget.unixtime() > now_ts) {
				// Dnes tento cas jeste nebyl, takze se nastavi timestamp tak, aby se zalevalo dnes
				last_watering_timestamp = newTarget.unixtime() - (per_days * 86400);
				DEBUG_PRINTF("RESET: Nastavuji last_watering_timestamp aby se zalevalo dnes v %02d:%02d:%02d.\nTimestamp: %d.\n", newTarget.hour(), newTarget.minute(), newTarget.second(), last_watering_timestamp);
			} else {
				// dnes uz cas byl, po resetu se ale nastavi nejblizsi dalsi hodina a minuta zalevani, bezprostredne dalsi den
				last_watering_timestamp = newTarget.unixtime() - (per_days * 86400) + 86400;

				DEBUG_PRINTF("RESET: Jiz je po case dnesniho zalevani, nastavuji zavlazovani bezprostredne na zitra %02d:%02d:%02d.\nTimestamp:", newTarget.hour(), newTarget.minute(), newTarget.second(), last_watering_timestamp);
			}

			prefs.begin("settings", false);
			prefs.putUInt("w_ts", last_watering_timestamp);
			prefs.end();
		}

		switch (periodicalWateringState) {

			case PERIODICAL_IDLE: {
				DateTime last_watering(last_watering_timestamp);

				DateTime next_watering = last_watering + TimeSpan(per_days * 86400);

				if (now_ts >= next_watering.unixtime() && wateringMode == 1) {
					DEBUG_PRINTF("Jde se zalevat. Last_watering: %d, Next_watering: %d, Now: %d.\n", last_watering.unixtime(), next_watering.unixtime(), now_ts);

					display.clearDisplay();
					printAlignedText(&display, "Watering", 2, 24, 1, 0);
					display.display();

					wateringStartTime = millis();
					remainingWateringTime = (amount / 100) * WATERING_TIME_PER_100ML * 1000UL;
					digitalWrite(PUMP_RELAY, HIGH);
					periodicalWateringState = PERIODICAL_ACTIVE;
				}
				break;
			}

			case PERIODICAL_ACTIVE:
				lastSystemInteraction = millis();

				if(b4_pressed) {
					//user wants to cancel it
					periodicalWateringState = PERIODICAL_COMPLETE;
					break;
				}

				if (millis() - wateringStartTime >= remainingWateringTime) {
					digitalWrite(PUMP_RELAY, LOW);
					periodicalWateringState = PERIODICAL_COMPLETE;
				} else {
					if (digitalRead(TANK_SENSOR)){
						digitalWrite(PUMP_RELAY, LOW);
						digitalWrite(LED2, HIGH);

						unsigned long elapsed = millis() - wateringStartTime;
						remainingWateringTime = (elapsed < remainingWateringTime)
							? remainingWateringTime - elapsed : 0;

						refillStartTime = millis();

						display.clearDisplay();
						printAlignedText(&display, "Refill!", 2, 24, 1, 0);
						display.display();

						periodicalWateringState = PERIODICAL_REFILL;
					}
				}
				break;

			case PERIODICAL_REFILL:
				lastSystemInteraction = millis();

				if(b4_pressed) {
					//user wants to cancel it
					periodicalWateringState = PERIODICAL_COMPLETE;
					break;
				}
				if(!digitalRead(TANK_SENSOR)) {
					//nadrz doplnena
					digitalWrite(LED2, LOW);
					digitalWrite(PUMP_RELAY, HIGH);
					wateringStartTime = millis();
					periodicalWateringState = PERIODICAL_ACTIVE;
				} else if (millis() - refillStartTime > WATER_FILL_WAIT_TIME) {
					periodicalWateringState = PERIODICAL_COMPLETE;
				}
				break;

			case PERIODICAL_COMPLETE:
				lastSystemInteraction = millis();

				digitalWrite(PUMP_RELAY, LOW);

				DateTime newTimestamp(now.year(), now.month(), now.day(), h_watering_time, m_watering_time, 0);
				last_watering_timestamp = newTimestamp.unixtime(); 

				DEBUG_PRINTF("Zavlazovani dokonceno. Novy timestamp nastaven na %d\n", last_watering_timestamp);

				DEBUG_PRINTF("Dalsi zalevani tak probehne za %d dny.\nJe to ok?\n", per_days);

				//prefs.begin("settings", false);
				//prefs.putUInt("w_ts", last_watering_timestamp);
				//prefs.end();

				periodicalWateringState = PERIODICAL_IDLE;
				break;	
		}
	} else if (wateringMode == 2) {
		//sensor based

		if(sampleIndex < 0) {
			DEBUG_PRINTLN("ADC ma nevalidni hodnotu.");
			return; // nutno pockat, dokud ADC poprve nepoda validni hodnotu
		}

		switch(moistureWateringState){
			
			case MOISTURE_IDLE:
				
				if (millis() - moistureCooldownStartTime < MOISTURE_MODE_COOLDOWN) {
					// jeste porad nevyprsel cooldown
					DEBUG_PRINTLN("Cooldown.");
					break;
				}

				if(soil_moisture > adcLowThreshold) {
					DEBUG_PRINTLN("Detekovano zalevani.");

					if (digitalRead(TANK_SENSOR)) {
						// nadrz je prazdna
						return;
					} else {
						digitalWrite(PUMP_RELAY, HIGH);
						moistureWateringStartTime = millis();
						moistureWateringState = MOISTURE_WATERING;

						display.clearDisplay();
						printAlignedText(&display, "Watering", 2, 24, 1, 0);
						display.display();
					}
				}
				break;

			case MOISTURE_WATERING:
			
				lastSystemInteraction = millis();

				if(b4_pressed) {
					//user wants to cancel it
					moistureWateringState = MOISTURE_COMPLETE;
					break;
				}

				if (digitalRead(TANK_SENSOR)) {
					digitalWrite(PUMP_RELAY, LOW);
					digitalWrite(LED2, HIGH);
					moistureWateringState = MOISTURE_COMPLETE;
					break;
				}

				if (millis() - moistureWateringStartTime >= PUMP_MOISTURE_SAFETY_SHUTOFF){
					// bezpecnostni vypnuti, bezi prilis dlouho
					digitalWrite(PUMP_RELAY, LOW);
					moistureWateringState = MOISTURE_COMPLETE;
					break;
				}

				if(soil_moisture <= adcHighThreshold) {
					digitalWrite(PUMP_RELAY, LOW);
					moistureWateringState = MOISTURE_COMPLETE;
					break;
				}

				break;

			case MOISTURE_COMPLETE:
				DEBUG_PRINTLN("Zalevani dokonceno.");
				lastSystemInteraction = millis();
				digitalWrite(PUMP_RELAY, LOW);
				moistureCooldownStartTime = millis();
				moistureWateringState = MOISTURE_IDLE;
				break;
		}
	}  
}


/**
 * @brief This function manages the deep sleep of device. 
 * If deep sleep is enabled and RTC is functional, the function
 * sets the device to sleep based on current watering mode.
 * In periodical mode, the sleep duration is calculated based on
 * the next scheduled watering time.
 * In sensor mode, the device always sleeps for 1 hour before
 * waking up and checking the soil moisture again.
 * In manual mode, the device goes to sleep indefinitely until
 * the user wakes it up by pressing the b4 button.
 * @warning During deep sleep, the device can only be woken up
 * by the b4 button.
 */
void handleSleep(){
	// pokud je deepSleep == true
	if (!deepSleep || rtcFAILED) return;

	// lastSystemInteraction je delsi nez SYSTEM_SLEEP_TIMEOUT
	// pokud je moistureWateringState == MOISTURE_IDLE
	// pokud je periodicalWateringState == PERIODICAL_IDLE
	if((millis() - lastSystemInteraction > SYSTEM_SLEEP_TIMEOUT) &&
		(periodicalWateringState == PERIODICAL_IDLE) &&
		(moistureWateringState == MOISTURE_IDLE)) {
			
		//esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0); //deepsleep na b1
		//esp_sleep_enable_timer_wakeup(10*1000000);

		uint64_t sleepDurationUs = 0;

		time_t now_ts = rtc.now().unixtime();

		if(wateringMode == 1) {
			// PERIODICAL WATERING
			time_t nextWateringTs = last_watering_timestamp + (per_days * 86400);
			if (nextWateringTs > now_ts){
				sleepDurationUs = (uint64_t)(nextWateringTs - now_ts) * 1000000ULL;
			} else {
				sleepDurationUs = (uint64_t)(now_ts + (per_days * 86400));
			}
		} else if (wateringMode == 2) {
			// SENSOR BASED
			sleepDurationUs = 3600000000ULL; // 1 hodina
		} else if (wateringMode == 0) {
			// MANUAL rezim => spi porad dokud neni vzbuzeno
			DEBUG_PRINTF("Jdu spat dokud me nekdo neprobudi\n");
			display.clearDisplay();
			printAlignedText(&display, "Sleep", 2, 10, 1, 0);
			printAlignedText(&display, "push B4 to wake up", 1, 57, 1, 0);
			display.display();

			esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0); // probudit na low == 0
			delay(4000);
			display.clearDisplay();
			display.display();
			display.ssd1306_command(SSD1306_DISPLAYOFF);
			esp_deep_sleep_start();
		}

		if(sleepDurationUs > 0) {
			uint64_t sleepDurationSec = sleepDurationUs / 1000000ULL;
			uint32_t hours = sleepDurationSec / 3600;
			uint32_t minutes = (sleepDurationSec % 3600) / 60;
			DEBUG_PRINTF("Jdu spat na %d hodin\n", hours);

			char sleepText[20];
			if(hours >= 1) {
				sprintf(sleepText, "for %dh", hours);
			} else {
				sprintf(sleepText, "for %dm", minutes);
			}
			display.clearDisplay();
			printAlignedText(&display, "Sleep", 2, 10, 1, 0);
			printAlignedText(&display, sleepText, 2, 33, 1, 0);
			printAlignedText(&display, "push B4 to wake up", 1, 57, 1, 0);
			display.display();

			esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0); // Probouzej na LOW
			esp_sleep_enable_timer_wakeup(sleepDurationUs); // Nastav dobu spánku
			delay(4000);
			display.clearDisplay();
			display.display();
			display.ssd1306_command(SSD1306_DISPLAYOFF);
			esp_deep_sleep_start();
		}

	}
}


/**
 * @brief Reads the soil moisture senzor value and
 * stores it in a buffer. Once the buffer is full, the average
 * soil moisture value is calculated and stored in the soil_moisture variable.
 */
void readSoilMoistureADC() {
	//sensor fully submerged in water = 1050
	//sensor out of water and dry = 1750
	int adcValue = analogRead(SOIL_SENSOR_PIN);
	//DEBUG_PRINTLN(adcValue);
	//if(adcValue < 50 || adcValue > 3950){
	//	//issue;
	//}

	soil_moisture_buffer[sampleIndex] = adcValue;
	sampleIndex++;

	if(sampleIndex >= SAMPLE_COUNT) {
		sampleIndex = 0;

		int sum = 0;
		for(int i = 0; i < SAMPLE_COUNT; i++){
			sum += soil_moisture_buffer[i];
		}
		soil_moisture = sum / SAMPLE_COUNT;

		//DEBUG_PRINTF("Prumer: %d, ADC skutecne: %d\n", soil_moisture, adcValue);
		//DEBUG_PRINT("Soil: ");
		//DEBUG_PRINTLN(soil_moisture);
		//DEBUG_PRINTF("Low: %d, high: %d, lowADC: %d, highADC: %d\n", lowThreshold, highThreshold, adcLowThreshold, adcHighThreshold);
	}
}


/**
 * @brief Converts a given soil moisture percentage to the corresponding ADC value.
 * The conversion is based on the defined DRY_SOIL and WET_SOIL ADC values.
 * Higher percentage means wetter soil, which corresponds to a lower ADC value.
 * @param percent The soil moisture percentage to convert (int 0-100).
 * @return The corresponding ADC value.
 */
int moisturePercentToAdc(int percent) {
	// jistota je jistota
	percent = constrain(percent, 0, 100);

	// převod: vyšší procento znamená menší ADC (vlhčí půda)
	return DRY_SOIL - ((DRY_SOIL - WET_SOIL) * percent) / 100;
}


/**
 * @brief Converts an ADC value from the soil moisture sensor to a percentage
 * representing the soil moisture level.
 * The conversion is based on the defined DRY_SOIL and WET_SOIL ADC values.
 * Higher ADC values correspond to drier soil, which results in a lower percentage.
 * @param adcValue The ADC value to convert (int).
 * @return The corresponding soil moisture percentage.
 */
int adcToMoisturePercent(int adcValue) {
  // Omezit na rozsah
  adcValue = constrain(adcValue, WET_SOIL, DRY_SOIL);

  // Inverzní výpočet: vyšší ADC → sušší půda → menší procento vlhkosti
  return ((DRY_SOIL - adcValue) * 100) / (DRY_SOIL - WET_SOIL);
}


/**
 * @brief Reads the battery voltage from the sensor and 
 * stores it in a buffer. Once the buffer is full, the average
 * battery voltage is calculated and stored in the battery_voltage variable.
 * @warning The voltage is calculated based on a linear approximation.
 * @todo This function is a mess and does not show proper values. Needs
 * to be reworked and calibrated properly.
 */
void readBatteryADC(){
  int adcValue = analogRead(BATTERY_VCC);
  battery_adc_buffer[battery_sample_index] = adcValue;
  battery_sample_index++;

  if (battery_sample_index >= BATTERY_SAMPLE_COUNT) {
    battery_sample_index = 0;

    long sum = 0;
    for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
      sum += battery_adc_buffer[i];
    }
    int avg_adc = sum / BATTERY_SAMPLE_COUNT;

    // 1. Převod ADC hodnoty na napětí na pinu (předpoklad 12bit rozlišení a 3.3V logika)
    float v_adc = (avg_adc / 4095.0) * 3.3;

    // 2. Přepočet přes odporový dělič modulu (typicky 1:5)
    // Pokud je hodnota trochu nepřesná, uprav číslo 5.0 nahoru nebo dolů (např. 5.12),
    // abys dorovnal toleranci rezistorů na modulu a nepřesnost vnitřní reference ESP32.
    battery_voltage = v_adc * 6.06; 

    // Percent count (Li-Ion 3.0V to 4.2V)
    battery_percent = (battery_voltage - 3.0) / (4.2 - 3.0) * 100.0;
    battery_percent = constrain(battery_percent, 0, 99);

    // debug výstup
    DEBUG_PRINT("Battery voltage: ");
    DEBUG_PRINT(battery_voltage);
    DEBUG_PRINT(" V, ");
    DEBUG_PRINT("Battery level: ");
    DEBUG_PRINT(battery_percent);
    DEBUG_PRINTLN(" %");
  }
}


/**
 * @brief Handles states of all the buttons, including debounce and long press.
 */
void handleButtons() {
	now = millis();

	b1_pressed = !digitalRead(b1);
	b2_pressed = !digitalRead(b2);
	b3_pressed = !digitalRead(b3);
	b4_pressed = !digitalRead(b4);

	if (b1_pressed != b1_lastState && (now - b1_lastDebounceTime) > DEBOUNCE_DELAY) {
    b1_lastDebounceTime = now;
    b1_push = (b1_lastState == LOW && b1_pressed);
    b1_lastState = b1_pressed;
		lastInteraction = millis();
		lastSystemInteraction = millis();
		lastMenuScrollTime = millis();
    if (b1_push) DEBUG_PRINTLN("b1 pressed");
  } else {
    b1_push = false;
  }

  if (b2_pressed != b2_lastState && (now - b2_lastDebounceTime) > DEBOUNCE_DELAY) {
    b2_lastDebounceTime = now;
    b2_push = (b2_lastState == LOW && b2_pressed);
    b2_lastState = b2_pressed;
		lastInteraction = millis();
		lastSystemInteraction = millis();
		lastMenuScrollTime = millis();
    if (b2_push) DEBUG_PRINTLN("b2 pressed");
  } else {
    b2_push = false;
  }

  if (b3_pressed != b3_lastState && (now - b3_lastDebounceTime) > DEBOUNCE_DELAY) {
    b3_lastDebounceTime = now;
    b3_push = (b3_lastState == LOW && b3_pressed);
    b3_lastState = b3_pressed;
		lastInteraction = millis();
		lastSystemInteraction = millis();
		lastMenuScrollTime = millis();
    if (b3_push) DEBUG_PRINTLN("b3 pressed");
  } else {
    b3_push = false;
  }

  if (b4_pressed != b4_lastState && (now - b4_lastDebounceTime) > DEBOUNCE_DELAY) {
    b4_lastDebounceTime = now;
    b4_push = (b4_lastState == LOW && b4_pressed);
    b4_lastState = b4_pressed;
		lastInteraction = millis();
		lastSystemInteraction = millis();
		lastMenuScrollTime = millis();
    if (b4_push) DEBUG_PRINTLN("b4 pressed");
  } else {
    b4_push = false;
  }
}


/** 
 * @brief Draws a loading circle on the display. 
 * @param display Reference to the display object to draw on.
 * @param step The step in the animation sequence.
 */
void drawLoadingCircle(Adafruit_SSD1306& display, int step) {
    display.clearDisplay();
    int radius = 10;  // Poloměr kolečka
    int centerX = 64; // Střed displeje (X)
    int centerY = 32; // Střed displeje (Y)
    int thickness = 3; // Tloušťka segmentu (zvětši tuto hodnotu pro tlustší čáru)

    // Zrychlená verze vykreslování segmentů s tlustšími čarami
    for (int i = 0; i < 4; i++) {
        float angle = (M_PI / 2) * (i + step) / 4.0;  // Rozdělíme 90° na čtyři segmenty
        int x1 = centerX + radius * cos(angle);  // Počáteční X souřadnice
        int y1 = centerY - radius * sin(angle);  // Počáteční Y souřadnice
        int x2 = centerX + (radius + thickness) * cos(angle);  // Konečná X souřadnice (zvětšíme poloměr pro tlustší čáru)
        int y2 = centerY - (radius + thickness) * sin(angle);  // Konečná Y souřadnice

        display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);  // Vykreslí tlustší čáru mezi dvěma body
    }

    display.display();  // Vykreslení na displeji
}