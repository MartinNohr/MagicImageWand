/*
 Name:		MagicImageWand.ino
 Created:	12/18/2020 6:12:01 PM
 Author:	Martin
*/
/*
	BLE code based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
*/
#include "MagicImageWand.h"

RTC_DATA_ATTR int nBootCount = 0;

// some forward references that Arduino IDE needs
int IRAM_ATTR readByte(bool clear);
void IRAM_ATTR ReadAndDisplayFile(bool doingFirstHalf);
uint16_t IRAM_ATTR readInt();
uint32_t IRAM_ATTR readLong();
void IRAM_ATTR FileSeekBuf(uint32_t place);

//static const char* TAG = "lightwand";
//esp_timer_cb_t oneshot_timer_callback(void* arg)
void IRAM_ATTR oneshot_LED_timer_callback(void* arg)
{
	bStripWaiting = false;
	//int64_t time_since_boot = esp_timer_get_time();
	//Serial.println("in isr");
	//ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
}

class MyServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		BLEDeviceConnected = true;
		//tft.clear();
		//WriteMessage("BLE connected");
		//tft.clear();
		//DisplayCurrentFile();
		//Serial.println("BLE connected");
	};

	void onDisconnect(BLEServer* pServer) {
		BLEDeviceConnected = false;
		//tft.clear();
		//WriteMessage("BLE disconnected");
		//tft.clear();
		//DisplayCurrentFile();
		//Serial.println("BLE disconnected");
	}
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic* pCharacteristic) {
		BLEUUID uuid = pCharacteristic->getUUID();
		//Serial.println("UUID:" + String(uuid.toString().c_str()));
		std::string value = pCharacteristic->getValue();
		//String stmp = value.c_str();

		if (value.length() > 0) {
			//Serial.println("*********");
			//Serial.print("New value: ");
			//for (int i = 0; i < value.length(); i++) {
			//	Serial.print(value[i]);
			//}
			//Serial.println();
			//Serial.println("*********");
			// see if this a UUID we can change
			if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_RUN))) {
				sBLECommand = value;
			}
			else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_WANDSETTINGS))) {
				//Serial.println(value.c_str());
				ARDUINOJSON_NAMESPACE::StaticJsonDocument<200> doc;
				const char* json = value.c_str();

				// Deserialize the JSON document
				DeserializationError error = deserializeJson(doc, json);

				// Test if parsing succeeds.
				if (error) {
					//Serial.print("deserializeJson() failed: ");
					//Serial.println(error.c_str());
					return;
				}
				JsonObject object = doc.as<JsonObject>();
				// see if brightness is there
				JsonVariant jv = doc.getMember("bright");
				if (!jv.isNull()) {
					nStripBrightness = jv.as<int>();
				}
				// see if repeat
				jv = doc.getMember("repeatcount");
				if (!jv.isNull()) {
					repeatCount = jv.as<int>();
				}
				// see if repeat delay
				jv = doc.getMember("repeatdelay");
				if (!jv.isNull()) {
					repeatDelay = jv.as<int>();
				}
				// change the current file
				jv = doc.getMember("current");
				if (!jv.isNull()) {
					String fn = jv.as<char*>();
					// lets search for the file
					int ix = LookUpFile(fn);
					if (ix != -1) {
						CurrentFileIndex = ix;
						DisplayCurrentFile();
					}
				}
				// change framehold
				jv = doc.getMember("framehold");
				if (!jv.isNull()) {
					nFrameHold = jv.as<int>();
				}
				// change builtin setting
				jv = doc.getMember("builtin");
				if (!jv.isNull()) {
					String bist = jv.as<char*>();
					bist.toUpperCase();
					bool bi = bist[0] == 'T';
					if (bi != bShowBuiltInTests) {
						ToggleFilesBuiltin(NULL);
						//Serial.println("builtin:" + String(bShowBuiltInTests ? "true" : "false"));
						CurrentFileIndex = 0;
						tft.fillScreen(TFT_BLACK);
						DisplayCurrentFile();
					}
				}
			}
			//else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_LEDBRIGHT))) {
			//	nStripBrightness = stmp.toInt();
			//	nStripBrightness = constrain(nStripBrightness, 1, 100);
			//}
			//else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_BUILTIN))) {
			//	stmp.toUpperCase();
			//	bool newval = stmp.equals("TRUE") ? true : false;
			//	//Serial.println("newval:" + String(newval ? "true" : "false"));
			//	if (newval != bShowBuiltInTests) {
			//		//Serial.println("builtin:" + String(bShowBuiltInTests ? "true" : "false"));
			//		ToggleFilesBuiltin(NULL);
			//		//Serial.println("builtin:" + String(bShowBuiltInTests ? "true" : "false"));
			//		tft.clear();
			//		DisplayCurrentFile();
			//	}
			//}
			//else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_2LEDSTRIPS))) {
			//	stmp.toUpperCase();
			//	bSecondStrip = stmp.compareTo("TRUE") == 0 ? true : false;
			//}
			else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_FILEINFO))) {
				// change the current file here, maybe we search for it?
			}
			//else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_FRAMETIME))) {
			//	frameHold = stmp.toInt();
			//}
			//else if (uuid.equals(BLEUUID(CHARACTERISTIC_UUID_STARTDELAY))) {
			//	startDelay = stmp.toInt();
			//}
			UpdateBLE(false);
		}
	}
};

void EnableBLE()
{
	BLEDevice::init("MN LED Image Painter");
	BLEServer* pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	BLEService* pServiceDevInfo = pServer->createService(BLEUUID((uint16_t)0x180a));
	BLECharacteristic* pCharacteristicDevInfo = pServiceDevInfo->createCharacteristic(
		BLEUUID((uint16_t)0x2a29),
		BLECharacteristic::PROPERTY_READ
	);
	pCharacteristicDevInfo->setValue("NOHR PHOTO");

	pCharacteristicDevInfo = pServiceDevInfo->createCharacteristic(
		BLEUUID((uint16_t)0x2a24),	// model
		BLECharacteristic::PROPERTY_READ
	);
	pCharacteristicDevInfo->setValue("LED Image Painter Small Display");

	pCharacteristicDevInfo = pServiceDevInfo->createCharacteristic(
		BLEUUID((uint16_t)0x2a28),	// software version
		BLECharacteristic::PROPERTY_READ
	);
	pCharacteristicDevInfo->setValue("1.1");

	pCharacteristicDevInfo = pServiceDevInfo->createCharacteristic(
		BLEUUID((uint16_t)0x2a27),	// hardware version
		BLECharacteristic::PROPERTY_READ
	);
	pCharacteristicDevInfo->setValue("1.0");

	BLEService* pService = pServer->createService(SERVICE_UUID);
	// filepath
	pCharacteristicFileInfo = pService->createCharacteristic(
		CHARACTERISTIC_UUID_FILEINFO,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE
	);
	// Create a BLE Descriptor
	//pCharacteristicFilename->addDescriptor(new BLE2902());

	// Wand settins
	pCharacteristicWandSettings = pService->createCharacteristic(
		CHARACTERISTIC_UUID_WANDSETTINGS,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE
	);

	// run command
	pCharacteristicRun = pService->createCharacteristic(
		CHARACTERISTIC_UUID_RUN,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE
	);

	// all the filenames
	pCharacteristicFileList = pService->createCharacteristic(
		CHARACTERISTIC_UUID_FILELIST,
		BLECharacteristic::PROPERTY_READ
	);

	// add anybody that can be changed or can call us with something to do
	MyCharacteristicCallbacks* pCallBacks = new MyCharacteristicCallbacks();
	pCharacteristicRun->setCallbacks(pCallBacks);
	pCharacteristicWandSettings->setCallbacks(pCallBacks);
	pCharacteristicFileInfo->setCallbacks(pCallBacks);
	UpdateBLE(false);
	pService->start();
	pServiceDevInfo->start();
	BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x180a));
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();
}

#define TFT_ENABLE 4
// use these to control the LCD brightness
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

void setup()
{
	Serial.begin(115200);
	delay(10);
	tft.init();
	// configure LCD PWM functionalitites
	pinMode(TFT_ENABLE, OUTPUT);
	digitalWrite(TFT_ENABLE, 1);
	ledcSetup(ledChannel, freq, resolution);
	// attach the channel to the GPIO to be controlled
	ledcAttachPin(TFT_ENABLE, ledChannel);
	SetDisplayBrightness(nDisplayBrightness);
	tft.fillScreen(TFT_BLACK);
	tft.setRotation(3);
	tft.setFreeFont();
	//Serial.println("boot: " + String(nBootCount));
	CRotaryDialButton::getInstance()->begin(DIAL_A, DIAL_B, DIAL_BTN);
	setupSDcard();
	//listDir(SD, "/", 2, "");
	//gpio_set_direction((gpio_num_t)LED, GPIO_MODE_OUTPUT);
	//digitalWrite(LED, HIGH);
	gpio_set_direction((gpio_num_t)FRAMEBUTTON, GPIO_MODE_INPUT);
	gpio_set_pull_mode((gpio_num_t)FRAMEBUTTON, GPIO_PULLUP_ONLY);
	oneshot_LED_timer_args = {
				oneshot_LED_timer_callback,
				/* argument specified here will be passed to timer callback function */
				(void*)0,
				ESP_TIMER_TASK,
				"one-shotLED"
	};
	esp_timer_create(&oneshot_LED_timer_args, &oneshot_LED_timer);

	int width = tft.width();
	int height = tft.height();
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE);
	rainbow_fill();
	if (nBootCount == 0)
	{
		tft.setTextColor(TFT_BLACK);
		//tft.setFreeFont(&FreeMono18pt7b);
		tft.setTextFont(2);
		tft.drawRect(0, 0, width - 1, height - 1, TFT_WHITE);
		tft.setTextSize(2);
		//tft.setTextColor(TFT_GREEN);
		tft.drawString("Magic Image Wand", 10, 5);
		//tft.setTextSize(2);
		//tft.setTextColor(TFT_BLUE);
		tft.drawString("Version 0.02", 30, 50);
		tft.setTextSize(1);
		tft.drawString(__DATE__, 70, 90);
		//tft.setTextColor(TFT_BLACK);
	}
	tft.setTextFont(2);
	tft.setTextSize(1);
	//Serial.println("font height: " + String(tft.fontHeight()));
	//tft.setFont(ArialMT_Plain_10);
	charHeight = tft.fontHeight() + 1;
	tft.setTextColor(TFT_WHITE);

	EEPROM.begin(1024);
	// this will fix the signature if necessary
	if (SaveSettings(false, true)) {
		// get the autoload flag
		SaveSettings(false, false, true);
	}
	// load the saved settings if flag is true and the button isn't pushed
	if ((nBootCount == 0) && bAutoLoadSettings && gpio_get_level((gpio_num_t)DIAL_BTN)) {
		// read all the settings
		SaveSettings(false);
	}

	menuPtr = new MenuInfo;
	MenuStack.push(menuPtr);
	MenuStack.top()->menu = MainMenu;
	MenuStack.top()->index = 0;
	MenuStack.top()->offset = 0;
	//const int ledPin = 16;  // 16 corresponds to GPIO16

	//// configure LED PWM functionalitites
	//ledcSetup(ledChannel, freq, resolution);
	//// attach the channel to the GPIO to be controlled
	//ledcAttachPin(WandPin, ledChannel);
	//ledcWrite(ledChannel, 200);
	FastLED.addLeds<NEOPIXEL, DATA_PIN1>(leds, 0, NUM_LEDS);
	//FastLED.addLeds<NEOPIXEL, DATA_PIN2>(leds, 0, NUM_LEDS);	// to test parallel second strip
	//if (bSecondStrip)
	// create the second led controller
	FastLED.addLeds<NEOPIXEL, DATA_PIN2>(leds, NUM_LEDS, NUM_LEDS);
	//FastLED.setTemperature(whiteBalance);
	FastLED.setTemperature(CRGB(whiteBalance.r, whiteBalance.g, whiteBalance.b));
	FastLED.setBrightness(nStripBrightness);
	if (nBootCount == 0) {
		//bool oldSecond = bSecondStrip;
		//bSecondStrip = true;
		RainbowPulse();
		//bSecondStrip = oldSecond;
		//// Turn the LED on, then pause
		//SetPixel(0, CRGB::Red);
		//SetPixel(1, CRGB::Red);
		//SetPixel(4, CRGB::Green);
		//SetPixel(5, CRGB::Green);
		//SetPixel(8, CRGB::Blue);
		//SetPixel(9, CRGB::Blue);
		//SetPixel(12, CRGB::White);
		//SetPixel(13, CRGB::White);
		//SetPixel(NUM_LEDS - 0, CRGB::Red);
		//SetPixel(NUM_LEDS - 1, CRGB::Red);
		//SetPixel(NUM_LEDS - 4, CRGB::Green);
		//SetPixel(NUM_LEDS - 5, CRGB::Green);
		//SetPixel(NUM_LEDS - 8, CRGB::Blue);
		//SetPixel(NUM_LEDS - 9, CRGB::Blue);
		//SetPixel(NUM_LEDS - 12, CRGB::White);
		//SetPixel(NUM_LEDS - 13, CRGB::White);
		//SetPixel(0 + NUM_LEDS, CRGB::Red);
		//SetPixel(1 + NUM_LEDS, CRGB::Red);
		//SetPixel(4 + NUM_LEDS, CRGB::Green);
		//SetPixel(5 + NUM_LEDS, CRGB::Green);
		//SetPixel(8 + NUM_LEDS, CRGB::Blue);
		//SetPixel(9 + NUM_LEDS, CRGB::Blue);
		//SetPixel(12 + NUM_LEDS, CRGB::White);
		//SetPixel(13 + NUM_LEDS, CRGB::White);
		//for (int ix = 0; ix < 255; ix += 5) {
		//	FastLED.setBrightness(ix);
		//	FastLED.show();
		//	delayMicroseconds(50);
		//}
		//for (int ix = 255; ix >= 0; ix -= 5) {
		//	FastLED.setBrightness(ix);
		//	FastLED.show();
		//	delayMicroseconds(50);
		//}
		//delayMicroseconds(50);
		//FastLED.clear(true);
		//delayMicroseconds(50);
		//FastLED.setBrightness(nStripBrightness);
		//delay(50);
		//// Now turn the LED off
		//FastLED.clear(true);
		//delayMicroseconds(50);
		//// run a white dot up the display and back
		//for (int ix = 0; ix < STRIPLENGTH; ++ix) {
		//	SetPixel(ix, CRGB::White);
		//	if (ix)
		//		SetPixel(ix - 1, CRGB::Black);
		//	FastLED.show();
		//	delayMicroseconds(50);
		//}
		//for (int ix = STRIPLENGTH - 1; ix >= 0; --ix) {
		//	SetPixel(ix, CRGB::White);
		//	if (ix)
		//		SetPixel(ix + 1, CRGB::Black);
		//	FastLED.show();
		//	delayMicroseconds(50);
		//}
	}
	FastLED.clear(true);
	delay(100);
	tft.fillScreen(TFT_BLACK);

	if (bEnableBLE) {
		EnableBLE();
	}
	// wait for button release
	while (!digitalRead(DIAL_BTN))
		;
	delay(30);	// debounce
	while (!digitalRead(DIAL_BTN))
		;
	// clear the button buffer
	CRotaryDialButton::getInstance()->clear();
	if (!bSdCardValid) {
		DisplayCurrentFile();
		delay(2000);
		ToggleFilesBuiltin(NULL);
	}
	DisplayCurrentFile();
	/*
		analogSetCycles(8);                   // Set number of cycles per sample, default is 8 and provides an optimal result, range is 1 - 255
		analogSetSamples(1);                  // Set number of samples in the range, default is 1, it has an effect on sensitivity has been multiplied
		analogSetClockDiv(1);                 // Set the divider for the ADC clock, default is 1, range is 1 - 255
		analogSetAttenuation(ADC_11db);       // Sets the input attenuation for ALL ADC inputs, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
		//analogSetPinAttenuation(36, ADC_11db); // Sets the input attenuation, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
		analogSetPinAttenuation(37, ADC_11db);
		// ADC_0db provides no attenuation so IN/OUT = 1 / 1 an input of 3 volts remains at 3 volts before ADC measurement
		// ADC_2_5db provides an attenuation so that IN/OUT = 1 / 1.34 an input of 3 volts is reduced to 2.238 volts before ADC measurement
		// ADC_6db provides an attenuation so that IN/OUT = 1 / 2 an input of 3 volts is reduced to 1.500 volts before ADC measurement
		// ADC_11db provides an attenuation so that IN/OUT = 1 / 3.6 an input of 3 volts is reduced to 0.833 volts before ADC measurement
	//   adcAttachPin(VP);                     // Attach a pin to ADC (also clears any other analog mode that could be on), returns TRUE/FALSE result
	//   adcStart(VP);                         // Starts an ADC conversion on attached pin's bus
	//   adcBusy(VP);                          // Check if conversion on the pin's ADC bus is currently running, returns TRUE/FALSE result
	//   adcEnd(VP);

		//adcAttachPin(36);
		adcAttachPin(37);
	*/
}

void loop()
{
	static bool didsomething = false;
	bool lastStrip = bSecondStrip;
	bool bLastEnableBLE = bEnableBLE;
	didsomething = bSettingsMode ? HandleMenus() : HandleRunMode();
	// wait for no keys
	if (didsomething) {
		//Serial.println("calling wait for none");
		UpdateBLE(false);
		didsomething = false;
		// see if BLE enabled
		if (bEnableBLE != bLastEnableBLE) {
			if (bEnableBLE) {
				EnableBLE();
			}
			else {
				// shutdown BLE
				BLEDeviceConnected = false;
				// TODO: is there anything else we need to do here?
			}
		}
		delay(1);
	}
	// disconnecting
	if (!BLEDeviceConnected && oldBLEDeviceConnected) {
		delay(500); // give the bluetooth stack the chance to get things ready
		BLEDevice::startAdvertising();
		//Serial.println("start advertising");
		oldBLEDeviceConnected = BLEDeviceConnected;
	}
	// connecting
	if (BLEDeviceConnected && !oldBLEDeviceConnected) {
		// do stuff here on connecting
		oldBLEDeviceConnected = BLEDeviceConnected;
		UpdateBLE(false);
	}
}

void UpdateBLE(bool bProgressOnly)
{
	// update the settings, except for progress percent which is done in ShowProgress.
	if (BLEDeviceConnected) {
		String js;
		// running information and commands
		DynamicJsonDocument runinfo(256);
		runinfo["running"] = bIsRunning;
		runinfo["progress"] = nProgress;
		runinfo["repeatsleft"] = nRepeatsLeft;
		js = "";
		serializeJson(runinfo, js);
		pCharacteristicRun->setValue(js.c_str());
		if (!bProgressOnly) {
			// file information
			DynamicJsonDocument filesdoc(1024);
			filesdoc["builtin"] = bShowBuiltInTests;
			filesdoc["path"] = currentFolder.c_str();
			filesdoc["current"] = FileNames[CurrentFileIndex].c_str();
			js = "";
			serializeJson(filesdoc, js);
			pCharacteristicFileInfo->setValue(js.c_str());
			// settings
			DynamicJsonDocument wsdoc(1024);
			wsdoc["secondstrip"] = bSecondStrip;
			wsdoc["bright"] = nStripBrightness;
			wsdoc["framehold"] = nFrameHold;
			wsdoc["framebutton"] = nFramePulseCount;
			wsdoc["startdelay"] = startDelay;
			wsdoc["repeatdelay"] = repeatDelay;
			wsdoc["repeatcount"] = repeatCount;
			wsdoc["gamma"] = bGammaCorrection;
			wsdoc["reverse"] = bReverseImage;
			wsdoc["upsidedown"] = bUpsideDown;
			wsdoc["mirror"] = bMirrorPlayImage;
			wsdoc["chain"] = bChainFiles;
			wsdoc["chainrepeats"] = nChainRepeats;
			wsdoc["scaley"] = bScaleHeight;
			js = "";
			serializeJson(wsdoc, js);
			pCharacteristicWandSettings->setValue(js.c_str());
			// file list
			DynamicJsonDocument filelist(512);
			filelist["builtin"] = bShowBuiltInTests;
			filelist["path"] = currentFolder.c_str();
			filelist["count"] = FileNames.size();
			filelist["ix"] = CurrentFileIndex;
			JsonArray data = filelist.createNestedArray("files");
			for (auto st : FileNames) {
				data.add(st.c_str());
			}
			js = "";
			serializeJson(filelist, js);
			pCharacteristicFileList->setValue(js.c_str());
		}
	}
}

bool RunMenus(int button)
{
	// save this so we can see if we need to save a new changed value
	bool lastAutoLoadFlag = bAutoLoadSettings;
	// see if we got a menu match
	bool gotmatch = false;
	int menuix = 0;
	MenuInfo* oldMenu;
	bool bExit = false;
	for (int ix = 0; !gotmatch && MenuStack.top()->menu[ix].op != eTerminate; ++ix) {
		// see if this is one is valid
		if (!MenuStack.top()->menu[ix].valid) {
			continue;
		}
		//Serial.println("menu button: " + String(button));
		if (button == BTN_SELECT && menuix == MenuStack.top()->index) {
			//Serial.println("got match " + String(menuix) + " " + String(MenuStack.top()->index));
			gotmatch = true;
			//Serial.println("clicked on menu");
			// got one, service it
			switch (MenuStack.top()->menu[ix].op) {
			case eText:
			case eTextInt:
			case eTextCurrentFile:
			case eBool:
				bMenuChanged = true;
				if (MenuStack.top()->menu[ix].function) {
					(*MenuStack.top()->menu[ix].function)(&MenuStack.top()->menu[ix]);
				}
				break;
			case eList:
				bMenuChanged = true;
				if (MenuStack.top()->menu[ix].function) {
					(*MenuStack.top()->menu[ix].function)(&MenuStack.top()->menu[ix]);
				}
				bExit = true;
				// if there is a value, set the min value in it
				if (MenuStack.top()->menu[ix].value) {
					*(int*)MenuStack.top()->menu[ix].value = MenuStack.top()->menu[ix].min;
				}
				break;
			case eMenu:
				if (MenuStack.top()->menu) {
					oldMenu = MenuStack.top();
					MenuStack.push(new MenuInfo);
					MenuStack.top()->menu = oldMenu->menu[ix].menu;
					bMenuChanged = true;
					MenuStack.top()->index = 0;
					MenuStack.top()->offset = 0;
					//Serial.println("change menu");
					// check if the new menu is an eList and if it has a value, if it does, set the index to it
					if (MenuStack.top()->menu->op == eList && MenuStack.top()->menu->value) {
						int ix = *(int*)MenuStack.top()->menu->value;
						MenuStack.top()->index = ix;
						// adjust offset if necessary
						if (ix > 4) {
							MenuStack.top()->offset = ix - 4;
						}
					}
				}
				break;
			case eBuiltinOptions: // find it in builtins
				if (BuiltInFiles[CurrentFileIndex].menu != NULL) {
					MenuStack.top()->index = MenuStack.top()->index;
					MenuStack.push(new MenuInfo);
					MenuStack.top()->menu = BuiltInFiles[CurrentFileIndex].menu;
					MenuStack.top()->index = 0;
					MenuStack.top()->offset = 0;
				}
				else {
					WriteMessage("No settings available for:\n" + String(BuiltInFiles[CurrentFileIndex].text));
				}
				bMenuChanged = true;
				break;
			case eExit: // go back a level
				bExit = true;
				break;
			case eReboot:
				WriteMessage("Rebooting in 2 seconds\nHold button for factory reset", false, 2000);
				ESP.restart();
				break;
			}
		}
		++menuix;
	}
	// if no match, and we are in a submenu, go back one level, or if bExit is set
	if (bExit || (!bMenuChanged && MenuStack.size() > 1)) {
		bMenuChanged = true;
		menuPtr = MenuStack.top();
		MenuStack.pop();
		delete menuPtr;
	}
	// see if the autoload flag changed
	if (bAutoLoadSettings != lastAutoLoadFlag) {
		// the flag is now true, so we should save the current settings
		SaveSettings(true, false, true);
	}
}

#define MENU_LINES 8
// display the menu
// if MenuStack.top()->index is > MENU_LINES, then shift the lines up by enough to display them
// remember that we only have room for MENU_LINES lines
void ShowMenu(struct MenuItem* menu)
{
	MenuStack.top()->menucount = 0;
	int y = 0;
	int x = 0;
	char line[100];
	bool skip = false;
	// loop through the menu
	for (; menu->op != eTerminate; ++menu) {
		menu->valid = false;
		switch (menu->op) {
		case eIfEqual:
			// skip the next one if match, only booleans are handled so far
			skip = *(bool*)menu->value != (menu->min ? true : false);
			//Serial.println("ifequal test: skip: " + String(skip));
			break;
		case eElse:
			skip = !skip;
			break;
		case eEndif:
			skip = false;
			break;
		}
		if (skip) {
			menu->valid = false;
			continue;
		}
		char line[100], xtraline[100];
		// only displayable menu items should be in this switch
		line[0] = '\0';
		int val;
		bool exists;
		switch (menu->op) {
		case eTextInt:
		case eText:
		case eTextCurrentFile:
			menu->valid = true;
			if (menu->value) {
				val = *(int*)menu->value;
				if (menu->op == eText)
					sprintf(line, menu->text, val);
				else if (menu->op == eTextInt) {
					if (menu->decimals == 0) {
						sprintf(line, menu->text, val);
					}
					else {
						sprintf(line, menu->text, val / 10, val % 10);
					}
				}
				//Serial.println("menu text1: " + String(line));
			}
			else {
				if (menu->op == eTextCurrentFile) {
					sprintf(line, menu->text, MakeMIWFilename(FileNames[CurrentFileIndex], false).c_str());
					//Serial.println("menu text2: " + String(line));
				}
				else {
					strcpy(line, menu->text);
					//Serial.println("menu text3: " + String(line));
				}
			}
			// next line
			++y;
			break;
		case eList:
			menu->valid = true;
			// the list of macro files
			// min holds the macro number
			val = menu->min;
			// see if the macro is there and append the text
			exists = SD.exists("/" + String(val) + ".miw");
			sprintf(line, menu->text, val, exists ? menu->on : menu->off);
			// next line
			++y;
			break;
		case eBool:
			menu->valid = true;
			if (menu->value) {
				// clean extra bits, just in case
				bool* pb = (bool*)menu->value;
				//*pb &= 1;
				sprintf(line, menu->text, *pb ? menu->on : menu->off);
				//Serial.println("bool line: " + String(line));
			}
			else {
				strcpy(line, menu->text);
			}
			// increment displayable lines
			++y;
			break;
		case eBuiltinOptions:
			// for builtins only show if available
			if (BuiltInFiles[CurrentFileIndex].menu != NULL) {
				menu->valid = true;
				sprintf(line, menu->text, BuiltInFiles[CurrentFileIndex].text);
				++y;
			}
			break;
		case eMenu:
		case eExit:
		case eReboot:
			menu->valid = true;
			if (menu->value) {
				sprintf(xtraline, menu->text, *(int*)menu->value);
			}
			else {
				strcpy(xtraline, menu->text);
			}
			if (menu->op == eExit)
				sprintf(line, "%s%s", "-", xtraline);
			else
				sprintf(line, "%s%s", (menu->op == eReboot) ? "" : "+", xtraline);
			++y;
			//Serial.println("menu text4: " + String(line));
			break;
		}
		if (strlen(line) && y >= MenuStack.top()->offset) {
			DisplayMenuLine(y - 1, y - 1 - MenuStack.top()->offset, line);
		}
	}
	//Serial.println("menu: " + String(offsetLines) + ":" + String(y) + " active: " + String(MenuStack.top()->index));
	MenuStack.top()->menucount = y;
	// blank the rest of the lines
	for (int ix = y; ix < MENU_LINES; ++ix) {
		DisplayLine(ix, "");
	}
	// show line if menu has been scrolled
	if (MenuStack.top()->offset > 0)
		tft.drawLine(0, 0, 5, 0, TFT_WHITE);
	// show bottom line if last line is showing
	if (MenuStack.top()->offset + (MENU_LINES - 1) < MenuStack.top()->menucount - 1)
		tft.drawLine(0, tft.height() - 1, 5, tft.height() - 1, TFT_WHITE);
}

// switch between SD and built-ins
void ToggleFilesBuiltin(MenuItem* menu)
{
	// clear filenames list
	FileNames.clear();
	bool lastval = bShowBuiltInTests;
	int oldIndex = CurrentFileIndex;
	String oldFolder = currentFolder;
	if (menu != NULL) {
		ToggleBool(menu);
	}
	else {
		bShowBuiltInTests = !bShowBuiltInTests;
	}
	if (lastval != bShowBuiltInTests) {
		if (bShowBuiltInTests) {
			CurrentFileIndex = 0;
			for (int ix = 0; ix < sizeof(BuiltInFiles) / sizeof(*BuiltInFiles); ++ix) {
				// add each one
				FileNames.push_back(String(BuiltInFiles[ix].text));
			}
			currentFolder = "";
		}
		else {
			// read the SD
			currentFolder = lastFolder;
			GetFileNamesFromSD(currentFolder);
		}
	}
	// restore indexes
	CurrentFileIndex = lastFileIndex;
	lastFileIndex = oldIndex;
	currentFolder = lastFolder;
	lastFolder = oldFolder;
}

// toggle a boolean value
void ToggleBool(MenuItem* menu)
{
	bool* pb = (bool*)menu->value;
	*pb = !*pb;
	if (menu->change != NULL) {
		(*menu->change)(menu, -1);
	}
	//Serial.println("autoload: " + String(bAutoLoadSettings));
	//Serial.println("fixed time: " + String(bFixedTime));
}

// get integer values
void GetIntegerValue(MenuItem* menu)
{
	// -1 means to reset to original
	int stepSize = 1;
	int originalValue = *(int*)menu->value;
	//Serial.println("int: " + String(menu->text) + String(*(int*)menu->value));
	char line[50];
	CRotaryDialButton::Button button = BTN_NONE;
	bool done = false;
	tft.fillScreen(TFT_BLACK);
	DisplayLine(1, "Range: " + String(menu->min) + " to " + String(menu->max));
	DisplayLine(3, "Long Press to Accept");
	int oldVal = *(int*)menu->value;
	if (menu->change != NULL) {
		(*menu->change)(menu, 1);
	}
	do {
		//Serial.println("button: " + String(button));
		switch (button) {
		case BTN_LEFT:
			if (stepSize != -1)
				*(int*)menu->value -= stepSize;
			break;
		case BTN_RIGHT:
			if (stepSize != -1)
				*(int*)menu->value += stepSize;
			break;
		case BTN_SELECT:
			if (stepSize == -1) {
				stepSize = 1;
			}
			else {
				stepSize *= 10;
			}
			if (stepSize > (menu->max / 10)) {
				stepSize = -1;
			}
			break;
		case BTN_LONG:
			if (stepSize == -1) {
				*(int*)menu->value = originalValue;
				stepSize = 1;
			}
			else {
				done = true;
			}
			break;
		}
		// make sure within limits
		*(int*)menu->value = constrain(*(int*)menu->value, menu->min, menu->max);
		// show slider bar
		//OLEDDISPLAY_COLOR oldColor = tft.getColor();
		tft.fillRect(0, 30, tft.width() - 1, 6, TFT_BLACK);
		//tft.setColor(oldColor);
		DrawProgressBar(0, 2 * charHeight + 5, tft.width() - 1, 6, map(*(int*)menu->value, menu->min, menu->max, 0, 100));
		if (menu->decimals == 0) {
			sprintf(line, menu->text, *(int*)menu->value);
		}
		else {
			sprintf(line, menu->text, *(int*)menu->value / 10, *(int*)menu->value % 10);
		}
		DisplayLine(0, line);
		DisplayLine(4, stepSize == -1 ? "Reset: long press (Click +)" : "step: " + String(stepSize) + " (Click +)");
		if (menu->change != NULL && oldVal != *(int*)menu->value) {
			(*menu->change)(menu, 0);
			oldVal = *(int*)menu->value;
		}
		while (!done && (button = ReadButton()) == BTN_NONE) {
			delay(1);
		}
	} while (!done);
	if (menu->change != NULL) {
		(*menu->change)(menu, -1);
	}
}

void UpdateStripBrightness(MenuItem* menu, int flag)
{
	switch (flag) {
	case 1:		// first time
		for (int ix = 0; ix < 64; ++ix) {
			SetPixel(ix, CRGB::White);
		}
		FastLED.show();
		break;
	case 0:		// every change
		FastLED.setBrightness(*(int*)menu->value);
		FastLED.show();
		break;
	case -1:	// last time
		FastLED.clear(true);
		break;
	}
}

void UpdateStripWhiteBalanceR(MenuItem* menu, int flag)
{
	switch (flag) {
	case 1:		// first time
		for (int ix = 0; ix < 64; ++ix) {
			SetPixel(ix, CRGB::White);
		}
		FastLED.show();
		break;
	case 0:		// every change
		FastLED.setTemperature(CRGB(*(int*)menu->value, whiteBalance.g, whiteBalance.b));
		FastLED.show();
		break;
	case -1:	// last time
		FastLED.clear(true);
		break;
	}
}

void UpdateStripWhiteBalanceG(MenuItem* menu, int flag)
{
	switch (flag) {
	case 1:		// first time
		for (int ix = 0; ix < 64; ++ix) {
			SetPixel(ix, CRGB::White);
		}
		FastLED.show();
		break;
	case 0:		// every change
		FastLED.setTemperature(CRGB(whiteBalance.r, *(int*)menu->value, whiteBalance.b));
		FastLED.show();
		break;
	case -1:	// last time
		FastLED.clear(true);
		break;
	}
}

void UpdateStripWhiteBalanceB(MenuItem* menu, int flag)
{
	switch (flag) {
	case 1:		// first time
		for (int ix = 0; ix < 64; ++ix) {
			SetPixel(ix, CRGB::White);
		}
		FastLED.show();
		break;
	case 0:		// every change
		FastLED.setTemperature(CRGB(whiteBalance.r, whiteBalance.g, *(int*)menu->value));
		FastLED.show();
		break;
	case -1:	// last time
		FastLED.clear(true);
		break;
	}
}

void UpdateDisplayBrightness(MenuItem* menu, int flag)
{
	// control LCD brightness
	SetDisplayBrightness(*(int*)menu->value);
}

void SetDisplayBrightness(int val)
{
	ledcWrite(ledChannel, map(val, 0, 100, 0, 255));
}

// handle the menus
bool HandleMenus()
{
	if (bMenuChanged) {
		ShowMenu(MenuStack.top()->menu);
		bMenuChanged = false;
	}
	bool didsomething = true;
	CRotaryDialButton::Button button = ReadButton();
	int lastOffset = MenuStack.top()->offset;
	int lastMenu = MenuStack.top()->index;
	int lastMenuCount = MenuStack.top()->menucount;
	bool lastRecording = bRecordingMacro;
	switch (button) {
	case BTN_SELECT:
		RunMenus(button);
		bMenuChanged = true;
		break;
	case BTN_RIGHT:
		if (bAllowMenuWrap || MenuStack.top()->index < MenuStack.top()->menucount - 1) {
			++MenuStack.top()->index;
		}
		if (MenuStack.top()->index >= MenuStack.top()->menucount) {
			MenuStack.top()->index = 0;
			bMenuChanged = true;
			MenuStack.top()->offset = 0;
		}
		// see if we need to scroll the menu
		if (MenuStack.top()->index - MenuStack.top()->offset > (MENU_LINES - 1)) {
			if (MenuStack.top()->offset < MenuStack.top()->menucount - MENU_LINES) {
				++MenuStack.top()->offset;
			}
		}
		break;
	case BTN_LEFT:
		if (bAllowMenuWrap || MenuStack.top()->index > 0) {
			--MenuStack.top()->index;
		}
		if (MenuStack.top()->index < 0) {
			MenuStack.top()->index = MenuStack.top()->menucount - 1;
			bMenuChanged = true;
			MenuStack.top()->offset = MenuStack.top()->menucount - MENU_LINES;
		}
		// see if we need to adjust the offset
		if (MenuStack.top()->offset && MenuStack.top()->index < MenuStack.top()->offset) {
			--MenuStack.top()->offset;
		}
		break;
	case BTN_LONG:
		tft.fillScreen(TFT_BLACK);
		bSettingsMode = false;
		DisplayCurrentFile();
		bMenuChanged = true;
		break;
	default:
		didsomething = false;
		break;
	}
	// check some conditions that should redraw the menu
	if (lastMenu != MenuStack.top()->index || lastOffset != MenuStack.top()->offset) {
		bMenuChanged = true;
		//Serial.println("menu changed");
	}
	// see if the recording status changed
	if (lastRecording != bRecordingMacro) {
		MenuStack.top()->index = 0;
		MenuStack.top()->offset = 0;
		bMenuChanged = true;
	}
	return didsomething;
}

// handle keys in run mode
bool HandleRunMode()
{
	bool didsomething = true;
	switch (ReadButton()) {
	case BTN_SELECT:
		bCancelRun = bCancelMacro = false;
		ProcessFileOrTest();
		break;
	case BTN_RIGHT:
		if (bAllowMenuWrap || (CurrentFileIndex < FileNames.size() - 1))
			++CurrentFileIndex;
		if (CurrentFileIndex >= FileNames.size())
			CurrentFileIndex = 0;
		DisplayCurrentFile();
		break;
	case BTN_LEFT:
		if (bAllowMenuWrap || (CurrentFileIndex > 0))
			--CurrentFileIndex;
		if (CurrentFileIndex < 0)
			CurrentFileIndex = FileNames.size() - 1;
		DisplayCurrentFile();
		break;
		//case btnShowFiles:
		//	bShowBuiltInTests = !bShowBuiltInTests;
		//	GetFileNamesFromSD(currentFolder);
		//	DisplayCurrentFile();
		//	break;
	case BTN_LONG:
		tft.fillScreen(TFT_BLACK);
		bSettingsMode = true;
		break;
	default:
		didsomething = false;
		break;
	}
	return didsomething;
}

// check buttons and return if one pressed
enum CRotaryDialButton::Button ReadButton()
{
	enum CRotaryDialButton::Button retValue = BTN_NONE;
	// see if we got any BLE commands
	if (!sBLECommand.empty()) {
		//Serial.println(sBLECommand.c_str());
		String stmp = sBLECommand.c_str();
		stmp.toUpperCase();
		sBLECommand = "";
		if (stmp == "RUN") {
			return BTN_SELECT;
		}
		if (stmp == "RIGHT") {
			return BTN_RIGHT;
		}
		if (stmp == "LEFT") {
			return BTN_LEFT;
		}
	}
	// read the next button, or NONE it none there
	retValue = CRotaryDialButton::getInstance()->dequeue();
	//if (retValue != BTN_NONE)
	//	Serial.println("button:" + String(retValue));
	return retValue;
}

// save or restore the display
void SaveRestoreDisplay(bool save)
{
	if (save) {
		tft.readRect(0, 0, 240, 135, screenBuffer);
		bPauseDisplay = true;
		tft.fillScreen(TFT_BLACK);
	}
	else {
		tft.pushRect(0, 0, 240, 135, screenBuffer);
		bPauseDisplay = false;
	}
}

// just check for longpress and cancel if it was there
bool CheckCancel()
{
	// if it has been set, just return true
	if (bCancelRun || bCancelMacro)
		return true;
	int button = ReadButton();
	if (button) {
		if (button == BTN_LONG) {
			bCancelMacro = bCancelRun = true;
			return true;
		}
	}
	return false;
}

void setupSDcard()
{
	bSdCardValid = false;
#if USE_STANDARD_SD
	gpio_set_direction((gpio_num_t)SDcsPin, GPIO_MODE_OUTPUT);
	delay(50);
	//SPIClass(1);
	spiSDCard.begin(SDSckPin, SDMisoPin, SDMosiPin, SDcsPin);	// SCK,MISO,MOSI,CS
	delay(20);

	if (!SD.begin(SDcsPin, spiSDCard)) {
		//Serial.println("Card Mount Failed");
		return;
	}
	uint8_t cardType = SD.cardType();

	if (cardType == CARD_NONE) {
		//Serial.println("No SD card attached");
		return;
	}
#else
#define SD_CONFIG SdSpiConfig(SDcsPin, /*DEDICATED_SPI*/SHARED_SPI, SD_SCK_MHZ(10))
	SPI.begin(SDSckPin, SDMisoPin, SDMosiPin, SDcsPin);	// SCK,MISO,MOSI,CS
	if (!SD.begin(SD_CONFIG)) {
		Serial.println("SD initialization failed.");
		uint8_t err = SD.card()->errorCode();
		Serial.println("err: " + String(err));
		return;
	}
	//Serial.println("Mounted SD card");
	//SD.printFatType(&Serial);

	//uint64_t cardSize = (uint64_t)SD.clusterCount() * SD.bytesPerCluster() / (1024 * 1024 * 1024);
	//Serial.printf("SD Card Size: %llu GB\n", cardSize);
#endif
	bSdCardValid = GetFileNamesFromSD(currentFolder);
}

// return the pixel
CRGB IRAM_ATTR getRGBwithGamma() {
	if (bGammaCorrection) {
		b = gammaB[readByte(false)];
		g = gammaG[readByte(false)];
		r = gammaR[readByte(false)];
	}
	else {
		b = readByte(false);
		g = readByte(false);
		r = readByte(false);
	}
	return CRGB(r, g, b);
}

void fixRGBwithGamma(byte* rp, byte* gp, byte* bp) {
	if (bGammaCorrection) {
		*gp = gammaG[*gp];
		*bp = gammaB[*bp];
		*rp = gammaR[*rp];
	}
}

// up to 32 bouncing balls
void TestBouncingBalls() {
	CRGB colors[] = {
		CRGB::White,
		CRGB::Red,
		CRGB::Green,
		CRGB::Blue,
		CRGB::Yellow,
		CRGB::Cyan,
		CRGB::Magenta,
		CRGB::Grey,
		CRGB::RosyBrown,
		CRGB::RoyalBlue,
		CRGB::SaddleBrown,
		CRGB::Salmon,
		CRGB::SandyBrown,
		CRGB::SeaGreen,
		CRGB::Seashell,
		CRGB::Sienna,
		CRGB::Silver,
		CRGB::SkyBlue,
		CRGB::SlateBlue,
		CRGB::SlateGray,
		CRGB::SlateGrey,
		CRGB::Snow,
		CRGB::SpringGreen,
		CRGB::SteelBlue,
		CRGB::Tan,
		CRGB::Teal,
		CRGB::Thistle,
		CRGB::Tomato,
		CRGB::Turquoise,
		CRGB::Violet,
		CRGB::Wheat,
		CRGB::WhiteSmoke,
	};

	BouncingColoredBalls(nBouncingBallsCount, colors);
	FastLED.clear(true);
}

void BouncingColoredBalls(int balls, CRGB colors[]) {
	time_t startsec = time(NULL);
	float Gravity = -9.81;
	int StartHeight = 1;

	float* Height = (float*)calloc(balls, sizeof(float));
	float* ImpactVelocity = (float*)calloc(balls, sizeof(float));
	float* TimeSinceLastBounce = (float*)calloc(balls, sizeof(float));
	int* Position = (int*)calloc(balls, sizeof(int));
	long* ClockTimeSinceLastBounce = (long*)calloc(balls, sizeof(long));
	float* Dampening = (float*)calloc(balls, sizeof(float));
	float ImpactVelocityStart = sqrt(-2 * Gravity * StartHeight);

	for (int i = 0; i < balls; i++) {
		ClockTimeSinceLastBounce[i] = millis();
		Height[i] = StartHeight;
		Position[i] = 0;
		ImpactVelocity[i] = ImpactVelocityStart;
		TimeSinceLastBounce[i] = 0;
		Dampening[i] = 0.90 - float(i) / pow(balls, 2);
	}

	// run for allowed seconds
	long start = millis();
	long percent;
	//nTimerSeconds = nBouncingBallsRuntime;
	//int lastSeconds = 0;
	//EventTimers.every(1000L, SecondsTimer);
	int colorChangeCounter = 0;
	while (millis() < start + ((long)nBouncingBallsRuntime * 1000)) {
		ShowProgressBar((time(NULL) - startsec) * 100 / nBouncingBallsRuntime);
		//if (nTimerSeconds != lastSeconds) {
		//    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
		//    tft.setTextSize(2);
		//    tft.setCursor(0, 38);
		//    char num[50];
		//    nTimerSeconds = nTimerSeconds;
		//    sprintf(num, "%3d seconds", nTimerSeconds);
		//    tft.print(num);
		//}
		//percent = map(millis() - start, 0, nBouncingBallsRuntime * 1000, 0, 100);
		//ShowProgressBar(percent);
		if (CheckCancel())
			return;
		for (int i = 0; i < balls; i++) {
			if (CheckCancel())
				return;
			TimeSinceLastBounce[i] = millis() - ClockTimeSinceLastBounce[i];
			Height[i] = 0.5 * Gravity * pow(TimeSinceLastBounce[i] / nBouncingBallsDecay, 2.0) + ImpactVelocity[i] * TimeSinceLastBounce[i] / nBouncingBallsDecay;

			if (Height[i] < 0) {
				Height[i] = 0;
				ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
				ClockTimeSinceLastBounce[i] = millis();

				if (ImpactVelocity[i] < 0.01) {
					ImpactVelocity[i] = ImpactVelocityStart;
				}
			}
			Position[i] = round(Height[i] * (STRIPLENGTH - 1) / StartHeight);
		}

		for (int i = 0; i < balls; i++) {
			int ix;
			if (CheckCancel())
				return;
			ix = (i + nBouncingBallsFirstColor) % 32;
			SetPixel(Position[i], colors[ix]);
		}
		if (nBouncingBallsChangeColors && colorChangeCounter++ > (nBouncingBallsChangeColors * 100)) {
			++nBouncingBallsFirstColor;
			colorChangeCounter = 0;
		}
		FastLED.show();
		delayMicroseconds(50);
		FastLED.clear();
		// increment the starting color if they want it
	}
	free(Height);
	free(ImpactVelocity);
	free(TimeSinceLastBounce);
	free(Position);
	free(ClockTimeSinceLastBounce);
	free(Dampening);
}

#define BARBERSIZE 10
#define BARBERCOUNT 40
void BarberPole()
{
CRGB:CRGB red, white, blue;
	byte r, g, b;
	r = 255, g = 0, b = 0;
	fixRGBwithGamma(&r, &g, &b);
	red = CRGB(r, g, b);
	r = 255, g = 255, b = 255;
	fixRGBwithGamma(&r, &g, &b);
	white = CRGB(r, g, b);
	r = 0, g = 0, b = 255;
	fixRGBwithGamma(&r, &g, &b);
	blue = CRGB(r, g, b);
	//ShowProgressBar(0);
	for (int loop = 0; loop < (8 * BARBERCOUNT); ++loop) {
		//Serial.println("barber:" + String(loop));
		if ((loop % 10) == 0)
			ShowProgressBar(loop * 100 / (8 * BARBERCOUNT));
		if (CheckCancel())
			return;
		for (int ledIx = 0; ledIx < STRIPLENGTH; ++ledIx) {
			if (CheckCancel())
				return;
			// figure out what color
			switch (((ledIx + loop) % BARBERCOUNT) / BARBERSIZE) {
			case 0: // red
				SetPixel(ledIx, red);
				break;
			case 1: // white
			case 3:
				SetPixel(ledIx, white);
				break;
			case 2: // blue
				SetPixel(ledIx, blue);
				break;
			}
		}
		FastLED.show();
		delay(nFrameHold);
	}
	ShowProgressBar(100);
}

// checkerboard
void CheckerBoard()
{
	time_t start = time(NULL);
	int width = nCheckboardBlackWidth + nCheckboardWhiteWidth;
	bStripWaiting = true;
	int times = 0;
	CRGB color1 = CRGB::Black, color2 = CRGB::White;
	esp_timer_start_once(oneshot_LED_timer, nCheckerBoardRuntime * 1000000);
	int addPixels = 0;
	while (bStripWaiting) {
		for (int y = 0; y < STRIPLENGTH; ++y) {
			SetPixel(y, ((y + addPixels) % width) < nCheckboardBlackWidth ? color1 : color2);
		}
		FastLED.show();
		ShowProgressBar((time(NULL) - start) * 100 / nCheckerBoardRuntime);
		int count = nCheckerboardHoldframes;
		while (count-- > 0) {
			delay(nFrameHold);
			if (CheckCancel()) {
				esp_timer_stop(oneshot_LED_timer);
				bStripWaiting = false;
				break;
			}
		}
		if (bCheckerBoardAlternate && (times++ % 2)) {
			// swap colors
			CRGB temp = color1;
			color1 = color2;
			color2 = temp;
		}
		addPixels += nCheckerboardAddPixels;
	}
}

void RandomBars()
{
	ShowRandomBars(bRandomBarsBlacks, nRandomBarsRuntime);
}

// show random bars of lights with optional blacks between
void ShowRandomBars(bool blacks, int runtime)
{
	time_t start = time(NULL);
	byte r, g, b;
	srand(millis());
	char line[40];
	bStripWaiting = true;
	esp_timer_start_once(oneshot_LED_timer, runtime * 1000000);
	for (int pass = 0; bStripWaiting; ++pass) {
		ShowProgressBar((time(NULL) - start) * 100 / runtime);
		if (blacks && (pass % 2)) {
			// odd numbers, clear
			FastLED.clear(true);
		}
		else {
			// even numbers, show bar
			r = random(0, 255);
			g = random(0, 255);
			b = random(0, 255);
			fixRGBwithGamma(&r, &g, &b);
			// fill the strip color
			FastLED.showColor(CRGB(r, g, b));
		}
		int count = nRandomBarsHoldframes;
		while (count-- > 0) {
			delay(nFrameHold);
			if (CheckCancel()) {
				esp_timer_stop(oneshot_LED_timer);
				bStripWaiting = false;
				break;
			}
		}
	}
}

// running dot
void RunningDot()
{
	for (int colorvalue = 0; colorvalue <= 3; ++colorvalue) {
		if (CheckCancel())
			return;
		// RGBW
		byte r, g, b;
		switch (colorvalue) {
		case 0: // red
			r = 255;
			g = 0;
			b = 0;
			break;
		case 1: // green
			r = 0;
			g = 255;
			b = 0;
			break;
		case 2: // blue
			r = 0;
			g = 0;
			b = 255;
			break;
		case 3: // white
			r = 255;
			g = 255;
			b = 255;
			break;
		}
		fixRGBwithGamma(&r, &g, &b);
		char line[10];
		for (int ix = 0; ix < STRIPLENGTH; ++ix) {
			if (CheckCancel())
				return;
			//lcd.setCursor(11, 0);
			//sprintf(line, "%3d", ix);
			//lcd.print(line);
			if (ix > 0) {
				SetPixel(ix - 1, CRGB::Black);
			}
			SetPixel(ix, CRGB(r, g, b));
			FastLED.show();
			delay(nFrameHold);
		}
		// remember the last one, turn it off
		SetPixel(STRIPLENGTH - 1, CRGB::Black);
		FastLED.show();
	}
}

void OppositeRunningDots()
{
	for (int mode = 0; mode <= 3; ++mode) {
		if (CheckCancel())
			return;
		// RGBW
		byte r, g, b;
		switch (mode) {
		case 0: // red
			r = 255;
			g = 0;
			b = 0;
			break;
		case 1: // green
			r = 0;
			g = 255;
			b = 0;
			break;
		case 2: // blue
			r = 0;
			g = 0;
			b = 255;
			break;
		case 3: // white
			r = 255;
			g = 255;
			b = 255;
			break;
		}
		fixRGBwithGamma(&r, &g, &b);
		for (int ix = 0; ix < STRIPLENGTH; ++ix) {
			if (CheckCancel())
				return;
			if (ix > 0) {
				SetPixel(ix - 1, CRGB::Black);
				SetPixel(STRIPLENGTH - ix + 1, CRGB::Black);
			}
			SetPixel(STRIPLENGTH - ix, CRGB(r, g, b));
			SetPixel(ix, CRGB(r, g, b));
			FastLED.show();
			delay(nFrameHold);
		}
	}
}

void Sleep(MenuItem* menu)
{
	++nBootCount;
	//rtc_gpio_pullup_en(BTNPUSH);
	esp_sleep_enable_ext0_wakeup((gpio_num_t)DIAL_BTN, LOW);
	esp_deep_sleep_start();
}

void LightBar(MenuItem* menu)
{
	tft.fillScreen(TFT_BLACK);
	DisplayLine(0, "Light Bar");
	DisplayLine(3, "Rotate Dial to Change");
	DisplayLine(4, "Click to Set Operation");
	DisplayAllColor();
	FastLED.clear(true);
	// these were set by CheckCancel() in DisplayAllColor() and need to be cleared
	bCancelMacro = bCancelRun = false;
}

// show all in a color
void DisplayAllColor()
{
	DisplayLine(1, "");
	if (bDisplayAllRGB)
		FastLED.showColor(CRGB(nDisplayAllRed, nDisplayAllGreen, nDisplayAllBlue));
	else
		FastLED.showColor(CHSV(nDisplayAllHue, nDisplayAllSaturation, nDisplayAllBrightness));
	// show until cancelled, but check for rotations of the knob
	CRotaryDialButton::Button btn;
	int what = 0;	// 0 for hue, 1 for saturation, 2 for brightness, 3 for increment
	int increment = 10;
	bool bChange = true;
	while (true) {
		if (bChange) {
			String line;
			switch (what) {
			case 0:
				if (bDisplayAllRGB)
					line = "Red: " + String(nDisplayAllRed);
				else
					line = "HUE: " + String(nDisplayAllHue);
				break;
			case 1:
				if (bDisplayAllRGB)
					line = "Green: " + String(nDisplayAllGreen);
				else
					line = "Saturation: " + String(nDisplayAllSaturation);
				break;
			case 2:
				if (bDisplayAllRGB)
					line = "Blue: " + String(nDisplayAllBlue);
				else
					line = "Brightness: " + String(nDisplayAllBrightness);
				break;
			case 3:
				line = "";
				break;
			}
			line += " (step: " + String(increment) + ")";
			DisplayLine(2, line);
		}
		btn = ReadButton();
		bChange = true;
		switch (btn) {
		case BTN_NONE:
			bChange = false;
			break;
		case BTN_RIGHT:
			switch (what) {
			case 0:
				if (bDisplayAllRGB)
					nDisplayAllRed += increment;
				else
					nDisplayAllHue += increment;
				break;
			case 1:
				if (bDisplayAllRGB)
					nDisplayAllGreen += increment;
				else
					nDisplayAllSaturation += increment;
				break;
			case 2:
				if (bDisplayAllRGB)
					nDisplayAllBlue += increment;
				else
					nDisplayAllBrightness += increment;
				break;
			case 3:
				increment *= 10;
				break;
			}
			break;
		case BTN_LEFT:
			switch (what) {
			case 0:
				if (bDisplayAllRGB)
					nDisplayAllRed -= increment;
				else
					nDisplayAllHue -= increment;
				break;
			case 1:
				if (bDisplayAllRGB)
					nDisplayAllGreen -= increment;
				else
					nDisplayAllSaturation -= increment;
				break;
			case 2:
				if (bDisplayAllRGB)
					nDisplayAllBlue -= increment;
				else
					nDisplayAllBrightness -= increment;
				break;
			case 3:
				increment /= 10;
				break;
			}
			break;
		case BTN_SELECT:
			what = ++what % 4;
			break;
		case BTN_LONG:
			// put it back, we don't want it
			CRotaryDialButton::getInstance()->pushButton(btn);
			break;
		}
		if (CheckCancel())
			return;
		if (bChange) {
			increment = constrain(increment, 1, 100);
			if (bDisplayAllRGB) {
				nDisplayAllRed = constrain(nDisplayAllRed, 0, 255);
				nDisplayAllGreen = constrain(nDisplayAllGreen, 0, 255);
				nDisplayAllBlue = constrain(nDisplayAllBlue, 0, 255);
				FastLED.showColor(CRGB(nDisplayAllRed, nDisplayAllGreen, nDisplayAllBlue));
			}
			else {
				nDisplayAllHue = constrain(nDisplayAllHue, 0, 255);
				nDisplayAllSaturation = constrain(nDisplayAllSaturation, 0, 255);
				nDisplayAllBrightness = constrain(nDisplayAllBrightness, 0, 255);
				FastLED.showColor(CHSV(nDisplayAllHue, nDisplayAllSaturation, nDisplayAllBrightness));
			}
		}
		delay(10);
	}
}

void TestTwinkle() {
	TwinkleRandom(nTwinkleRuntime, nFrameHold, bTwinkleOnlyOne);
}
void TwinkleRandom(int Runtime, int SpeedDelay, boolean OnlyOne) {
	time_t start = time(NULL);
	bStripWaiting = true;
	esp_timer_start_once(oneshot_LED_timer, Runtime * 1000000);
	while (bStripWaiting) {
		ShowProgressBar((time(NULL) - start) * 100 / nTwinkleRuntime);
		SetPixel(random(STRIPLENGTH), CRGB(random(0, 255), random(0, 255), random(0, 255)));
		FastLED.show();
		delay(SpeedDelay);
		if (OnlyOne) {
			FastLED.clear(true);
		}
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
		}
	}
}

void TestCylon()
{
	CylonBounce(nCylonEyeRed, nCylonEyeGreen, nCylonEyeBlue, nCylonEyeSize, nFrameHold, 50);
}
void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
{
	for (int i = 0; i < STRIPLENGTH - EyeSize - 2; i++) {
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
			return;
		}
		FastLED.clear();
		SetPixel(i, CRGB(red / 10, green / 10, blue / 10));
		for (int j = 1; j <= EyeSize; j++) {
			SetPixel(i + j, CRGB(red, green, blue));
		}
		SetPixel(i + EyeSize + 1, CRGB(red / 10, green / 10, blue / 10));
		FastLED.show();
		delay(SpeedDelay);
	}
	delay(ReturnDelay);

	for (int i = STRIPLENGTH - EyeSize - 2; i > 0; i--) {
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
			return;
		}
		FastLED.clear();
		SetPixel(i, CRGB(red / 10, green / 10, blue / 10));
		for (int j = 1; j <= EyeSize; j++) {
			SetPixel(i + j, CRGB(red, green, blue));
		}
		SetPixel(i + EyeSize + 1, CRGB(red / 10, green / 10, blue / 10));
		FastLED.show();
		delay(SpeedDelay);
	}
	delay(ReturnDelay);
}

void TestMeteor() {
	meteorRain(nMeteorRed, nMeteorGreen, nMeteorBlue, nMeteorSize, 64, true, 30);
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay)
{
	FastLED.clear(true);

	for (int i = 0; i < STRIPLENGTH + STRIPLENGTH; i++) {
		if (CheckCancel())
			return;
		// fade brightness all LEDs one step
		for (int j = 0; j < STRIPLENGTH; j++) {
			if (CheckCancel())
				return;
			if ((!meteorRandomDecay) || (random(10) > 5)) {
				fadeToBlack(j, meteorTrailDecay);
			}
		}

		// draw meteor
		for (int j = 0; j < meteorSize; j++) {
			if (CheckCancel())
				return;
			if ((i - j < STRIPLENGTH) && (i - j >= 0)) {
				SetPixel(i - j, CRGB(red, green, blue));
			}
		}
		FastLED.show();
		delay(SpeedDelay);
	}
}

void TestConfetti()
{
	time_t start = time(NULL);
	gHue = 0;
	bStripWaiting = true;
	esp_timer_start_once(oneshot_LED_timer, nConfettiRuntime * 1000000);
	while (bStripWaiting) {
		EVERY_N_MILLISECONDS(nFrameHold) {
			if (bConfettiCycleHue)
				++gHue;
			confetti();
			FastLED.show();
			ShowProgressBar((time(NULL) - start) * 100 / nConfettiRuntime);
		}
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
		}
	}
	// wait for timeout so strip will be blank
	delay(100);
}

void confetti()
{
	// random colored speckles that blink in and fade smoothly
	fadeToBlackBy(leds, STRIPLENGTH, 10);
	int pos = random16(STRIPLENGTH);
	leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void TestJuggle()
{
	time_t start = time(NULL);
	bStripWaiting = true;
	esp_timer_start_once(oneshot_LED_timer, nJuggleRuntime * 1000000);
	while (bStripWaiting) {
		EVERY_N_MILLISECONDS(nFrameHold) {
			juggle();
			FastLED.show();
			ShowProgressBar((time(NULL) - start) * 100 / nJuggleRuntime);
		}
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
		}
	}
}

void juggle()
{
	// eight colored dots, weaving in and out of sync with each other
	fadeToBlackBy(leds, STRIPLENGTH, 20);
	byte dothue = 0;
	uint16_t index;
	for (int i = 0; i < 8; i++) {
		index = beatsin16(i + 7, 0, STRIPLENGTH);
		// use AdjustStripIndex to get the right one
		SetPixel(index, leds[AdjustStripIndex(index)] | CHSV(dothue, 255, 255));
		//leds[beatsin16(i + 7, 0, STRIPLENGTH)] |= CHSV(dothue, 255, 255);
		dothue += 32;
	}
}

void TestSine()
{
	time_t start = time(NULL);
	gHue = nSineStartingHue;
	bStripWaiting = true;
	esp_timer_start_once(oneshot_LED_timer, nSineRuntime * 1000000);
	while (bStripWaiting) {
		EVERY_N_MILLISECONDS(nFrameHold) {
			sinelon();
			FastLED.show();
			ShowProgressBar((time(NULL) - start) * 100 / nSineRuntime);
		}
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
		}
	}
}
void sinelon()
{
	// a colored dot sweeping back and forth, with fading trails
	fadeToBlackBy(leds, STRIPLENGTH, 20);
	int pos = beatsin16(nSineSpeed, 0, STRIPLENGTH);
	leds[AdjustStripIndex(pos)] += CHSV(gHue, 255, 192);
	if (bSineCycleHue)
		++gHue;
}

void TestBpm()
{
	time_t start = time(NULL);
	gHue = 0;
	bStripWaiting = true;
	esp_timer_start_once(oneshot_LED_timer, nBpmRuntime * 1000000);
	while (bStripWaiting) {
		EVERY_N_MILLISECONDS(nFrameHold) {
			bpm();
			FastLED.show();
			ShowProgressBar((time(NULL) - start) * 100 / nBpmRuntime);
		}
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
		}
	}
}

void bpm()
{
	// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(nBpmBeatsPerMinute, 64, 255);
	for (int i = 0; i < STRIPLENGTH; i++) { //9948
		SetPixel(i, ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10)));
	}
	if (bBpmCycleHue)
		++gHue;
}

void FillRainbow(struct CRGB* pFirstLED, int numToFill,
	uint8_t initialhue,
	int deltahue)
{
	CHSV hsv;
	hsv.hue = initialhue;
	hsv.val = 255;
	hsv.sat = 240;
	for (int i = 0; i < numToFill; i++) {
		pFirstLED[AdjustStripIndex(i)] = hsv;
		hsv.hue += deltahue;
	}
}

void TestRainbow()
{
	time_t start = time(NULL);
	gHue = nRainbowInitialHue;
	bStripWaiting = true;
	ShowProgressBar(0);
	FillRainbow(leds, STRIPLENGTH, gHue, nRainbowHueDelta);
	FadeInOut(nRainbowFadeTime * 100, true);
	esp_timer_start_once(oneshot_LED_timer, nRainbowRuntime * 1000000);
	while (bStripWaiting) {
		EVERY_N_MILLISECONDS(nFrameHold) {
			if (bRainbowCycleHue)
				++gHue;
			FillRainbow(leds, STRIPLENGTH, gHue, nRainbowHueDelta);
			if (bRainbowAddGlitter)
				addGlitter(80);
			FastLED.show();
			ShowProgressBar((time(NULL) - start) * 100 / nRainbowRuntime);
		}
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
			FastLED.setBrightness(nStripBrightness);
			return;
		}
	}
	FadeInOut(nRainbowFadeTime * 100, false);
	FastLED.setBrightness(nStripBrightness);
}

// create a user defined stripe set
// it consists of a list of stripes, each of which have a width and color
// there can be up to 10 of these
#define NUM_STRIPES 20
struct {
	int start;
	int length;
	CHSV color;
} Stripes[NUM_STRIPES];

void TestStripes()
{
	time_t start = time(NULL);
	FastLED.setBrightness(nStripBrightness);
	// let's fill in some data
	for (int ix = 0; ix < NUM_STRIPES; ++ix) {
		Stripes[ix].start = ix * 20;
		Stripes[ix].length = 12;
		Stripes[ix].color = CHSV(0, 0, 255);
	}
	int pix = 0;	// pixel address
	FastLED.clear(true);
	for (int ix = 0; ix < NUM_STRIPES; ++ix) {
		pix = Stripes[ix].start;
		// fill in each block of pixels
		for (int len = 0; len < Stripes[ix].length; ++len) {
			SetPixel(pix++, CRGB(Stripes[ix].color));
		}
	}
	bStripWaiting = true;
	ShowProgressBar(0);
	FastLED.show();
	esp_timer_start_once(oneshot_LED_timer, nStripesRuntime * 1000000);
	while (bStripWaiting) {
		ShowProgressBar((time(NULL) - start) * 100 / nStripesRuntime);
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
			return;
		}
		delay(1000);
	}
}

// alternating white and black lines
void TestLines()
{
	time_t start = time(NULL);
	FastLED.setBrightness(nStripBrightness);
	FastLED.clear(true);
	bool bWhite = true;
	for (int pix = 0; pix < STRIPLENGTH; ++pix) {
		// fill in each block of pixels
		for (int len = 0; len < (bWhite ? nLinesWhite : nLinesBlack); ++len) {
			SetPixel(pix++, bWhite ? CRGB::White : CRGB::Black);
		}
		bWhite = !bWhite;
	}
	bStripWaiting = true;
	ShowProgressBar(0);
	FastLED.show();
	esp_timer_start_once(oneshot_LED_timer, nLinesRuntime * 1000000);
	while (bStripWaiting) {
		ShowProgressBar((time(NULL) - start) * 100 / nStripesRuntime);
		if (CheckCancel()) {
			esp_timer_stop(oneshot_LED_timer);
			bStripWaiting = false;
			return;
		}
		delay(1000);
		// might make this work to toggle blacks and whites eventually
		//for (int ix = 0; ix < STRIPLENGTH; ++ix) {
		//	leds[ix] = (leds[ix] == CRGB::White) ? CRGB::Black : CRGB::White;
		//}
		FastLED.show();
	}
	FastLED.clear(true);
}

// time is in mSec
void FadeInOut(int time, bool in)
{
	if (in) {
		for (int i = 0; i <= nStripBrightness; ++i) {
			FastLED.setBrightness(i);
			FastLED.show();
			delay(time / nStripBrightness);
		}
	}
	else {
		for (int i = nStripBrightness; i >= 0; --i) {
			FastLED.setBrightness(i);
			FastLED.show();
			delay(time / nStripBrightness);
		}
	}
}

void addGlitter(fract8 chanceOfGlitter)
{
	if (random8() < chanceOfGlitter) {
		leds[random16(STRIPLENGTH)] += CRGB::White;
	}
}

void fadeToBlack(int ledNo, byte fadeValue) {
	// FastLED
	leds[ledNo].fadeToBlackBy(fadeValue);
}

// run file or built-in
void ProcessFileOrTest()
{
	String line;
	// let's see if this is a folder command
	String tmp = FileNames[CurrentFileIndex];
	if (tmp[0] == NEXT_FOLDER_CHAR) {
		FileIndexStack.push(CurrentFileIndex);
		tmp = tmp.substring(1);
		// change folder, reload files
		currentFolder += tmp + "/";
		GetFileNamesFromSD(currentFolder);
		DisplayCurrentFile();
		return;
	}
	else if (tmp[0] == PREVIOUS_FOLDER_CHAR) {
		tmp = tmp.substring(1);
		tmp = tmp.substring(0, tmp.length() - 1);
		if (tmp.length() == 0)
			tmp = "/";
		// change folder, reload files
		currentFolder = tmp;
		GetFileNamesFromSD(currentFolder);
		CurrentFileIndex = FileIndexStack.top();
		FileIndexStack.pop();
		DisplayCurrentFile();
		return;
	}
	if (bRecordingMacro) {
		strcpy(FileToShow, FileNames[CurrentFileIndex].c_str());
		WriteOrDeleteConfigFile(String(nCurrentMacro), false, false);
	}
	bIsRunning = true;
	nProgress = 0;
	// clear the rest of the lines
	for (int ix = 1; ix < MENU_LINES - 1; ++ix)
		DisplayLine(ix, "");
	//DisplayCurrentFile();
	if (startDelay) {
		// set a timer
		nTimerSeconds = startDelay;
		while (nTimerSeconds && !CheckCancel()) {
			line = "Start Delay: " + String(nTimerSeconds / 10) + "." + String(nTimerSeconds % 10);
			DisplayLine(1, line);
			delay(100);
			--nTimerSeconds;
		}
		DisplayLine(2, "");
	}
	int chainCount = bChainFiles ? FileCountOnly() - CurrentFileIndex : 1;
	int chainRepeatCount = bChainFiles ? nChainRepeats : 1;
	int lastFileIndex = CurrentFileIndex;
	// don't allow chaining for built-ins, although maybe we should
	if (bShowBuiltInTests) {
		chainCount = 1;
		chainRepeatCount = 1;
	}
	// set the basic LED info
	FastLED.setTemperature(CRGB(whiteBalance.r, whiteBalance.g, whiteBalance.b));
	FastLED.setBrightness(nStripBrightness);
	line = "";
	while (chainRepeatCount-- > 0) {
		while (chainCount-- > 0) {
			DisplayCurrentFile();
			if (bChainFiles && !bShowBuiltInTests) {
				line = "Files: " + String(chainCount + 1);
				DisplayLine(3, line);
				line = "";
			}
			// process the repeats and waits for each file in the list
			for (nRepeatsLeft = repeatCount; nRepeatsLeft > 0; nRepeatsLeft--) {
				// fill the progress bar
				if (!bShowBuiltInTests)
					ShowProgressBar(0);
				if (repeatCount > 1) {
					line = "Repeats: " + String(nRepeatsLeft) + " ";
				}
				if (!bShowBuiltInTests && nChainRepeats > 1) {
					line += "Chains: " + String(chainRepeatCount + 1);
				}
				DisplayLine(2, line);
				if (bShowBuiltInTests) {
					DisplayLine(3, "Running (long cancel)");
					// run the test
					(*BuiltInFiles[CurrentFileIndex].function)();
				}
				else {
					if (nRepeatCountMacro > 1 && bRunningMacro) {
						DisplayLine(3, String("Macro Repeats: ") + String(nMacroRepeatsLeft));
					}
					// output the file
					SendFile(FileNames[CurrentFileIndex]);
				}
				if (bCancelRun) {
					break;
				}
				if (!bShowBuiltInTests)
					ShowProgressBar(0);
				if (nRepeatsLeft > 1) {
					if (repeatDelay) {
						FastLED.clear(true);
						// start timer
						nTimerSeconds = repeatDelay;
						while (nTimerSeconds > 0 && !CheckCancel()) {
							line = "Repeat Delay: " + String(nTimerSeconds / 10) + "." + String(nTimerSeconds % 10);
							DisplayLine(1, line);
							line = "";
							delay(100);
							--nTimerSeconds;
						}
						//DisplayLine(2, "");
					}
				}
			}
			if (bCancelRun) {
				chainCount = 0;
				break;
			}
			if (bShowBuiltInTests)
				break;
			// see if we are chaining, if so, get the next file, if a folder we're done
			if (bChainFiles) {
				// grab the next file
				if (CurrentFileIndex < FileNames.size() - 1)
					++CurrentFileIndex;
				if (IsFolder(CurrentFileIndex))
					break;
				// handle any chain delay
				for (int dly = nChainDelay; dly > 0 && !CheckCancel(); --dly) {
					line = "Chain Delay: " + String(dly / 10) + "." + String(dly % 10);
					DisplayLine(1, line);
					delay(100);
				}
			}
			line = "";
			// clear
			FastLED.clear(true);
		}
		if (bCancelRun) {
			chainCount = 0;
			chainRepeatCount = 0;
			bCancelRun = false;
			break;
		}
		// start again
		CurrentFileIndex = lastFileIndex;
		chainCount = bChainFiles ? FileCountOnly() - CurrentFileIndex : 1;
		if (repeatDelay && (nRepeatsLeft > 1) || chainRepeatCount >= 1) {
			FastLED.clear(true);
			// start timer
			nTimerSeconds = repeatDelay;
			while (nTimerSeconds > 0 && !CheckCancel()) {
				line = "Repeat Delay: " + String(nTimerSeconds / 10) + "." + String(nTimerSeconds % 10);
				DisplayLine(1, line);
				line = "";
				delay(100);
				--nTimerSeconds;
			}
		}
	}
	if (bChainFiles)
		CurrentFileIndex = lastFileIndex;
	FastLED.clear(true);
	tft.fillScreen(TFT_BLACK);
	bIsRunning = false;
	if (!bRunningMacro)
		DisplayCurrentFile();
	nProgress = 0;
	// clear buttons
	CRotaryDialButton::getInstance()->clear();
}

void SendFile(String Filename) {
	// see if there is an associated config file
	String cfFile = MakeMIWFilename(Filename, true);
	SettingsSaveRestore(true, 0);
	ProcessConfigFile(cfFile);
	String fn = currentFolder + Filename;
	dataFile = SD.open(fn);
	// if the file is available send it to the LED's
	if (dataFile.available()) {
		for (int cnt = 0; cnt < (bMirrorPlayImage ? 2 : 1); ++cnt) {
			ReadAndDisplayFile(cnt == 0);
			bReverseImage = !bReverseImage; // note this will be restored by SettingsSaveRestore
			dataFile.seek(0);
			FastLED.clear(true);
			int wait = nMirrorDelay;
			while (wait-- > 0) {
				delay(100);
			}
			if (CheckCancel())
				break;
		}
		dataFile.close();
	}
	else {
		WriteMessage("open fail: " + fn, true, 5000);
		return;
	}
	ShowProgressBar(100);
	SettingsSaveRestore(false, 0);
}

// some useful BMP constants
#define MYBMP_BF_TYPE           0x4D42	// "BM"
#define MYBMP_BI_RGB            0L
//#define MYBMP_BI_RLE8           1L
//#define MYBMP_BI_RLE4           2L
//#define MYBMP_BI_BITFIELDS      3L

void IRAM_ATTR ReadAndDisplayFile(bool doingFirstHalf) {
	static int totalSeconds;
	if (doingFirstHalf)
		totalSeconds = -1;

	// clear the file cache buffer
	readByte(true);
	uint16_t bmpType = readInt();
	uint32_t bmpSize = readLong();
	uint16_t bmpReserved1 = readInt();
	uint16_t bmpReserved2 = readInt();
	uint32_t bmpOffBits = readLong();
	//Serial.println("\nBMPtype: " + String(bmpType) + " offset: " + String(bmpOffBits));

	/* Check file header */
	if (bmpType != MYBMP_BF_TYPE) {
		WriteMessage(String("Invalid BMP:\n") + currentFolder + FileNames[CurrentFileIndex], true);
		return;
	}

	/* Read info header */
	uint32_t imgSize = readLong();
	uint32_t imgWidth = readLong();
	uint32_t imgHeight = readLong();
	uint16_t imgPlanes = readInt();
	uint16_t imgBitCount = readInt();
	uint32_t imgCompression = readLong();
	uint32_t imgSizeImage = readLong();
	uint32_t imgXPelsPerMeter = readLong();
	uint32_t imgYPelsPerMeter = readLong();
	uint32_t imgClrUsed = readLong();
	uint32_t imgClrImportant = readLong();

	//Serial.println("imgSize: " + String(imgSize));
	//Serial.println("imgWidth: " + String(imgWidth));
	//Serial.println("imgHeight: " + String(imgHeight));
	//Serial.println("imgPlanes: " + String(imgPlanes));
	//Serial.println("imgBitCount: " + String(imgBitCount));
	//Serial.println("imgCompression: " + String(imgCompression));
	//Serial.println("imgSizeImage: " + String(imgSizeImage));
	/* Check info header */
	if (imgWidth <= 0 || imgHeight <= 0 || imgPlanes != 1 ||
		imgBitCount != 24 || imgCompression != MYBMP_BI_RGB || imgSizeImage == 0)
	{
		WriteMessage(String("Unsupported, must be 24bpp:\n") + currentFolder + FileNames[CurrentFileIndex], true);
		return;
	}

	int displayWidth = imgWidth;
	if (imgWidth > STRIPLENGTH) {
		displayWidth = STRIPLENGTH;           //only display the number of led's we have
	}

	/* compute the line length */
	uint32_t lineLength = imgWidth * 3;
	// fix for padding to 4 byte words
	if ((lineLength % 4) != 0)
		lineLength = (lineLength / 4 + 1) * 4;

	// Note:  
	// The x,r,b,g sequence below might need to be changed if your strip is displaying
	// incorrect colors.  Some strips use an x,r,b,g sequence and some use x,r,g,b
	// Change the order if needed to make the colors correct.

	long secondsLeft = 0, lastSeconds = 0;
	char num[50];
	int percent;
	unsigned minLoopTime = 0; // the minimum time it takes to process a line
	bool bLoopTimed = false;
	// note that y is 0 based and x is 0 based in the following code, the original code had y 1 based
	// also remember that height and width are effectively reversed since we rotated the BMP image for ease of reading and displaying here
	for (int y = bReverseImage ? imgHeight - 1 : 0; bReverseImage ? y >= 0 : y < imgHeight; bReverseImage ? --y : ++y) {
		// approximate time left
		if (bReverseImage)
			secondsLeft = ((long)y * (nFrameHold + minLoopTime) / 1000L) + 1;
		else
			secondsLeft = ((long)(imgHeight - y) * (nFrameHold + minLoopTime) / 1000L) + 1;
		// mark the time for timing the loop
		if (!bLoopTimed) {
			minLoopTime = millis();
		}
		if (bMirrorPlayImage) {
			if (totalSeconds == -1)
				totalSeconds = secondsLeft;
			if (doingFirstHalf) {
				secondsLeft += totalSeconds;
			}
		}
		if (secondsLeft != lastSeconds) {
			lastSeconds = secondsLeft;
			sprintf(num, "File Seconds: %d", secondsLeft);
			DisplayLine(1, num);
		}
		percent = map(bReverseImage ? imgHeight - y : y, 0, imgHeight, 0, 100);
		if (bMirrorPlayImage) {
			percent /= 2;
			if (!doingFirstHalf) {
				percent += 50;
			}
		}
		if (((percent % 5) == 0) || percent > 90) {
			ShowProgressBar(percent);
		}
		int bufpos = 0;
		//uint32_t offset = (bmpOffBits + (y * lineLength));
		//dataFile.seekSet(offset);
		CRGB pixel;
		// get to start of pixel data, moved this out of the loop below to speed things up
		//Serial.println("y=" + String(y));
		FileSeekBuf((uint32_t)bmpOffBits + (y * lineLength));
		for (int x = 0; x < displayWidth; x++) {
			//FileSeekBuf((uint32_t)bmpOffBits + ((y * lineLength) + (x * 3)));
			//dataFile.seekSet((uint32_t)bmpOffBits + ((y * lineLength) + (x * 3)));
			// this reads three bytes
			pixel = getRGBwithGamma();
			// see if we want this one
			if (bScaleHeight && (x * displayWidth) % imgWidth) {
				continue;
			}
			SetPixel(x, pixel);
		}
		// see how long it took to get here
		if (!bLoopTimed) {
			minLoopTime = millis() - minLoopTime;
			bLoopTimed = true;
			// if fixed time then we need to calculate the framehold value
			if (bFixedTime) {
				// divide the time by the number of frames
				nFrameHold = 1000 * nFixedImageTime / imgHeight;
				nFrameHold -= minLoopTime;
				nFrameHold = max(nFrameHold, 0);
			}
		}
		//Serial.println("loop: " + String(minLoopTime));
		// wait for timer to expire before we show the next frame
		while (bStripWaiting) {
			delayMicroseconds(100);
			// we should maybe check the cancel key here to handle slow frame rates?
		}
		// now show the lights
		FastLED.show();
		// set a timer while we go ahead and load the next frame
		bStripWaiting = true;
		esp_timer_start_once(oneshot_LED_timer, nFrameHold * 1000);
		// check keys
		if (CheckCancel())
			break;
		if (bManualFrameAdvance) {
			// check if frame advance button requested
			if (nFramePulseCount) {
				for (int ix = nFramePulseCount; ix; --ix) {
					// wait for press
					while (digitalRead(FRAMEBUTTON)) {
						if (CheckCancel())
							break;
						delay(10);
					}
					// wait for release
					while (!digitalRead(FRAMEBUTTON)) {
						if (CheckCancel())
							break;
						delay(10);
					}
				}
			}
			else {
				// by button click or rotate
				int btn;
				for (;;) {
					btn = ReadButton();
					if (btn == BTN_NONE)
						continue;
					else if (btn == BTN_LONG)
						CRotaryDialButton::getInstance()->pushButton(BTN_LONG);
					else if (btn == BTN_LEFT) {
						// backup a line, use 2 because the for loop does one when we're done here
						if (bReverseImage) {
							y += 2;
							if (y > imgHeight)
								y = imgHeight;
						}
						else {
							y -= 2;
							if (y < -1)
								y = -1;
						}
						break;
					}
					else
						break;
					if (CheckCancel())
						break;
					delay(10);
				}
			}
		}
		if (bCancelRun)
			break;
	}
	// all done
	readByte(true);
}

// put the current file on the display
void ShowBmp(MenuItem* menu)
{
	bool bOldGamma = bGammaCorrection;
	bGammaCorrection = false;
	tft.fillScreen(TFT_BLACK);
	memset(screenBuffer, 0, sizeof(screenBuffer));
	String fn = currentFolder + FileNames[CurrentFileIndex];
	dataFile = SD.open(fn);
	// if the file is available send it to the LED's
	if (!dataFile.available()) {
		WriteMessage("failed to open: " + currentFolder + FileNames[CurrentFileIndex], true);
		return;
	}
	// clear the file cache buffer
	readByte(true);
	uint16_t bmpType = readInt();
	uint32_t bmpSize = readLong();
	uint16_t bmpReserved1 = readInt();
	uint16_t bmpReserved2 = readInt();
	uint32_t bmpOffBits = readLong();

	/* Check file header */
	if (bmpType != MYBMP_BF_TYPE) {
		WriteMessage(String("Invalid BMP:\n") + currentFolder + FileNames[CurrentFileIndex], true);
		return;
	}

	/* Read info header */
	uint32_t imgSize = readLong();
	uint32_t imgWidth = readLong();
	uint32_t imgHeight = readLong();
	uint16_t imgPlanes = readInt();
	uint16_t imgBitCount = readInt();
	uint32_t imgCompression = readLong();
	uint32_t imgSizeImage = readLong();
	uint32_t imgXPelsPerMeter = readLong();
	uint32_t imgYPelsPerMeter = readLong();
	uint32_t imgClrUsed = readLong();
	uint32_t imgClrImportant = readLong();

	/* Check info header */
	if (imgWidth <= 0 || imgHeight <= 0 || imgPlanes != 1 ||
		imgBitCount != 24 || imgCompression != MYBMP_BI_RGB || imgSizeImage == 0)
	{
		WriteMessage(String("Unsupported, must be 24bpp:\n") + currentFolder + FileNames[CurrentFileIndex], true);
		return;
	}

	int displayWidth = imgWidth;
	if (imgWidth > STRIPLENGTH) {
		displayWidth = STRIPLENGTH;           //only display the number of led's we have
	}

	/* compute the line length */
	uint32_t lineLength = imgWidth * 3;
	// fix for padding to 4 byte words
	if ((lineLength % 4) != 0)
		lineLength = (lineLength / 4 + 1) * 4;
	// offset for showing the image
	int imgOffset = imgHeight;
	bool done = false;
	bool redraw = true;
	bool allowScroll = imgHeight > 240;
	while (!done && redraw) {
		// loop through the image, y is the image width, and x is the image height
		for (int y = 0; y < imgHeight; ++y) {
			int bufpos = 0;
			CRGB pixel;
			// get to start of pixel data for this column
			FileSeekBuf((uint32_t)bmpOffBits + (y * lineLength));
			for (int x = 0; x < displayWidth; ++x) {
				// this reads three bytes
				pixel = getRGBwithGamma();
				//Serial.println(String(pixel.r) + " " + String(pixel.g) + " " + String(pixel.b));
				// add to the display memory
				int row = x - 5;
				int col = y - imgOffset + 240;
				if (row >= 0 && row < 135 && col >= 0 && col < 240) {
					uint16_t color = tft.color565(pixel.r, pixel.g, pixel.b);
					uint16_t sbcolor;
					// the memory image colors are byte swapped
					swab(&color, &sbcolor, 2);
					screenBuffer[(134 - row) * 240 + (239 - col)] = sbcolor;
				}
			}
		}
		int oldImgOffset = imgOffset;
		// got it all, go show it
		tft.pushRect(0, 0, 240, 135, screenBuffer);
		CRotaryDialButton::getInstance()->clear();
		switch (CRotaryDialButton::getInstance()->waitButton(true, 20000)) {
		case CRotaryDialButton::BTN_LEFT:
			if (allowScroll) {
				imgOffset -= 240;
				imgOffset = max(240, imgOffset);
			}
			break;
		case CRotaryDialButton::BTN_RIGHT:
			if (allowScroll) {
				imgOffset += 240;
				imgOffset = min((int32_t)imgHeight, imgOffset);
			}
			break;
		case CRotaryDialButton::BTN_LONGPRESS:
			done = true;
			CRotaryDialButton::getInstance()->pushButton(CRotaryDialButton::BTN_LONGPRESS);
			break;
		case CRotaryDialButton::BTN_CLICK:
			done = true;
			break;
		}
		if (oldImgOffset != imgOffset) {
			//tft.fillScreen(TFT_BLACK);
			redraw = true;
		}
	}
	// all done
	readByte(true);
	bGammaCorrection = bOldGamma;
}

void DisplayLine(int line, String text, int32_t color)
{
	if (bPauseDisplay)
		return;
	int y = line * charHeight + (bSettingsMode && !bRunningMacro ? 0 : 6);
	tft.fillRect(0, y, tft.width(), charHeight, TFT_BLACK);
	tft.setTextColor(color);
	tft.drawString(text, 0, y);
}

// the star is used to indicate active menu line
void DisplayMenuLine(int line, int displine, String text)
{
	bool hilite = MenuStack.top()->index == line;
	String mline = (hilite ? "* " : "  ") + text;
	//if (MenuStack.top()->index == line)
		//tft.setFont(SansSerif_bold_10);
	DisplayLine(displine, mline, hilite ? TFT_WHITE : TFT_BLUE);
	//tft.setFont(ArialMT_Plain_10);
}

uint32_t IRAM_ATTR readLong() {
	uint32_t retValue;
	byte incomingbyte;

	incomingbyte = readByte(false);
	retValue = (uint32_t)((byte)incomingbyte);

	incomingbyte = readByte(false);
	retValue += (uint32_t)((byte)incomingbyte) << 8;

	incomingbyte = readByte(false);
	retValue += (uint32_t)((byte)incomingbyte) << 16;

	incomingbyte = readByte(false);
	retValue += (uint32_t)((byte)incomingbyte) << 24;

	return retValue;
}

uint16_t IRAM_ATTR readInt() {
	byte incomingbyte;
	uint16_t retValue;

	incomingbyte = readByte(false);
	retValue += (uint16_t)((byte)incomingbyte);

	incomingbyte = readByte(false);
	retValue += (uint16_t)((byte)incomingbyte) << 8;

	return retValue;
}
byte filebuf[512];
int fileindex = 0;
int filebufsize = 0;
uint32_t filePosition = 0;

int IRAM_ATTR readByte(bool clear) {
	//int retbyte = -1;
	if (clear) {
		filebufsize = 0;
		fileindex = 0;
		return 0;
	}
	// TODO: this needs to align with 512 byte boundaries, maybe
	if (filebufsize == 0 || fileindex >= sizeof(filebuf)) {
		filePosition = dataFile.position();
		//// if not on 512 boundary yet, just return a byte
		//if ((filePosition % 512) && filebufsize == 0) {
		//    //Serial.println("not on 512");
		//    return dataFile.read();
		//}
		// read a block
//        Serial.println("block read");
		do {
			filebufsize = dataFile.read(filebuf, sizeof(filebuf));
		} while (filebufsize < 0);
		fileindex = 0;
	}
	return filebuf[fileindex++];
	//while (retbyte < 0) 
	//    retbyte = dataFile.read();
	//return retbyte;
}


// make sure we are the right place
void IRAM_ATTR FileSeekBuf(uint32_t place)
{
	if (place < filePosition || place >= filePosition + filebufsize) {
		// we need to read some more
		filebufsize = 0;
		dataFile.seek(place);
	}
}

// count the actual files
int FileCountOnly()
{
	int count = 0;
	// ignore folders, at the end
	for (int files = 0; files < FileNames.size(); ++files) {
		if (!IsFolder(count))
			++count;
	}
	return count;
}

// return true if current file is folder
bool IsFolder(int index)
{
	return FileNames[index][0] == NEXT_FOLDER_CHAR
		|| FileNames[index][0] == PREVIOUS_FOLDER_CHAR;
}

// show the current file
void DisplayCurrentFile(bool path)
{
	//String name = FileNames[CurrentFileIndex];
	//String upper = name;
	//upper.toUpperCase();
 //   if (upper.endsWith(".BMP"))
 //       name = name.substring(0, name.length() - 4);
	if (bShowBuiltInTests) {
		DisplayLine(0, FileNames[CurrentFileIndex]);
	}
	else {
		if (bSdCardValid) {
			DisplayLine(0, ((path && bShowFolder) ? currentFolder : "") + FileNames[CurrentFileIndex] + (bMirrorPlayImage ? "><" : ""));
		}
		else {
			DisplayLine(0, "No SD Card or Files");
		}
	}
	if (!bIsRunning && bShowNextFiles) {
		for (int ix = 1; ix < MENU_LINES - 1; ++ix) {
			if (ix + CurrentFileIndex >= FileNames.size()) {
				DisplayLine(ix, "", TFT_BLUE);
			}
			else {
				DisplayLine(ix, "   " + FileNames[CurrentFileIndex + ix], TFT_BLUE);
			}
		}
	}
	tft.setTextColor(TFT_WHITE);
	// for debugging keypresses
	//DisplayLine(3, String(nButtonDowns) + " " + nButtonUps);
}

void ShowProgressBar(int percent)
{
	nProgress = percent;
	UpdateBLE(true);
	if (!bShowProgress || bPauseDisplay)
		return;
	static int lastpercent;
	if (lastpercent && (lastpercent == percent))
		return;
	if (percent == 0) {
		tft.fillRect(0, 0, tft.width() - 1, 6, TFT_BLACK);
	}
	DrawProgressBar(0, 0, tft.width() - 1, 6, percent);
	lastpercent = percent;
}

// display message on first line
void WriteMessage(String txt, bool error, int wait)
{
	tft.fillScreen(TFT_BLACK);
	if (error)
		txt = "**" + txt + "**";
	tft.setCursor(0, 0);
	tft.setTextWrap(true);
	tft.print(txt);
	delay(wait);
}

// create the associated MIW name
String MakeMIWFilename(String filename, bool addext)
{
	String cfFile = filename;
	cfFile = cfFile.substring(0, cfFile.lastIndexOf('.'));
	if (addext)
		cfFile += String(".MIW");
	return cfFile;
}

// look for the file in the list
// return -1 if not found
int LookUpFile(String name)
{
	int ix = 0;
	for (auto nm : FileNames) {
		if (name.equalsIgnoreCase(nm)) {
			return ix;
		}
		++ix;
	}
	return -1;
}

// process the lines in the config file
bool ProcessConfigFile(String filename)
{
	bool retval = true;
	String filepath = ((bRunningMacro || bRecordingMacro) ? String("/") : currentFolder) + filename;
#if USE_STANDARD_SD
	SDFile rdfile;
#else
	FsFile rdfile;
#endif
	rdfile = SD.open(filepath);
	if (rdfile.available()) {
		String line, command, args;
		while (line = rdfile.readStringUntil('\n'), line.length()) {
			if (CheckCancel())
				break;
			// read the lines and do what they say
			int ix = line.indexOf('=', 0);
			if (ix > 0) {
				command = line.substring(0, ix);
				command.trim();
				command.toUpperCase();
				args = line.substring(ix + 1);
				args.trim();
				// loop through the var list looking for a match
				for (int which = 0; which < sizeof(SettingsVarList) / sizeof(*SettingsVarList); ++which) {
					if (command.compareTo(SettingsVarList[which].name) == 0) {
						switch (SettingsVarList[which].type) {
						case vtInt:
						{
							int val = args.toInt();
							int min = SettingsVarList[which].min;
							int max = SettingsVarList[which].max;
							if (min != max) {
								val = constrain(val, min, max);
							}
							*(int*)(SettingsVarList[which].address) = val;
						}
						break;
						case vtBool:
							args.toUpperCase();
							*(bool*)(SettingsVarList[which].address) = args[0] == 'T';
							break;
						case vtBuiltIn:
						{
							bool bLastBuiltIn = bShowBuiltInTests;
							args.toUpperCase();
							bool value = args[0] == 'T';
							if (value != bLastBuiltIn) {
								ToggleFilesBuiltin(NULL);
							}
						}
						break;
						case vtShowFile:
						{
							// get the folder and set it first
							String folder;
							String name;
							int ix = args.lastIndexOf('/');
							folder = args.substring(0, ix + 1);
							name = args.substring(ix + 1);
							int oldFileIndex = CurrentFileIndex;
							// save the old folder if necessary
							String oldFolder;
							if (!bShowBuiltInTests && !currentFolder.equalsIgnoreCase(folder)) {
								oldFolder = currentFolder;
								currentFolder = folder;
								GetFileNamesFromSD(folder);
							}
							// search for the file in the list
							int which = LookUpFile(name);
							if (which >= 0) {
								CurrentFileIndex = which;
								// call the process routine
								strcpy(FileToShow, name.c_str());
								tft.fillScreen(TFT_BLACK);
								ProcessFileOrTest();
							}
							if (oldFolder.length()) {
								currentFolder = oldFolder;
								GetFileNamesFromSD(currentFolder);
							}
							CurrentFileIndex = oldFileIndex;
						}
						break;
						case vtRGB:
						{
							// handle the RBG colors
							CRGB* cp = (CRGB*)(SettingsVarList[which].address);
							cp->r = args.toInt();
							args = args.substring(args.indexOf(',') + 1);
							cp->g = args.toInt();
							args = args.substring(args.indexOf(',') + 1);
							cp->b = args.toInt();
						}
						break;
						default:
							break;
						}
						// we found it, so carry on
						break;
					}
				}
			}
		}
		rdfile.close();
	}
	else
		retval = false;
	return retval;
}

// read the files from the card or list the built-ins
// look for start.MIW, and process it, but don't add it to the list
bool GetFileNamesFromSD(String dir) {
	// start over
	// first empty the current file names
	FileNames.clear();
	if (nBootCount == 0)
		CurrentFileIndex = 0;
	if (bShowBuiltInTests) {
		for (int ix = 0; ix < (sizeof(BuiltInFiles) / sizeof(*BuiltInFiles)); ++ix) {
			FileNames.push_back(String(BuiltInFiles[ix].text));
		}
	}
	else {
		String startfile;
		if (dir.length() > 1)
			dir = dir.substring(0, dir.length() - 1);
#if USE_STANDARD_SD
		File root = SD.open(dir);
		File file;
#else
		FsFile root = SD.open(dir, O_RDONLY);
		FsFile file;
#endif
		String CurrentFilename = "";
		if (!root) {
			//Serial.println("Failed to open directory: " + dir);
			//Serial.println("error: " + String(root.getError()));
			//SD.errorPrint("fail");
			return false;
		}
		if (!root.isDirectory()) {
			//Serial.println("Not a directory: " + dir);
			return false;
		}

		file = root.openNextFile();
		if (dir != "/") {
			// add an arrow to go back
			String sdir = currentFolder.substring(0, currentFolder.length() - 1);
			sdir = sdir.substring(0, sdir.lastIndexOf("/"));
			if (sdir.length() == 0)
				sdir = "/";
			FileNames.push_back(String(PREVIOUS_FOLDER_CHAR));
		}
		while (file) {
#if USE_STANDARD_SD
			CurrentFilename = file.name();
#else
			char fname[40];
			file.getName(fname, sizeof(fname));
			CurrentFilename = fname;
#endif
			// strip path
			CurrentFilename = CurrentFilename.substring(CurrentFilename.lastIndexOf('/') + 1);
			//Serial.println("name: " + CurrentFilename);
			if (CurrentFilename != "System Volume Information") {
				if (file.isDirectory()) {
					FileNames.push_back(String(NEXT_FOLDER_CHAR) + CurrentFilename);
				}
				else {
					String uppername = CurrentFilename;
					uppername.toUpperCase();
					if (uppername.endsWith(".BMP")) { //find files with our extension only
						//Serial.println("name: " + CurrentFilename);
						FileNames.push_back(CurrentFilename);
					}
					else if (uppername == "START.MIW") {
						startfile = CurrentFilename;
					}
				}
			}
			file.close();
			file = root.openNextFile();
		}
		root.close();
		std::sort(FileNames.begin(), FileNames.end(), CompareNames);
		// see if we need to process the auto start file
		if (startfile.length())
			ProcessConfigFile(startfile);
	}
	return true;
}

// compare strings for sort ignoring case
bool CompareNames(const String& a, const String& b)
{
	String a1 = a, b1 = b;
	a1.toUpperCase();
	b1.toUpperCase();
	return a1.compareTo(b1) < 0;
}

// save and restore important settings, two sets are available
// 0 is used by file display, and 1 is used when running macros
bool SettingsSaveRestore(bool save, int set)
{
	static void* memptr[2] = { NULL, NULL };
	if (save) {
		// get some memory and save the values
		if (memptr[set])
			free(memptr[set]);
		memptr[set] = malloc(sizeof saveValueList);
		if (!memptr[set])
			return false;
	}
	void* blockptr = memptr[set];
	if (memptr[set] == NULL) {
		return false;
	}
	for (int ix = 0; ix < (sizeof saveValueList / sizeof * saveValueList); ++ix) {
		if (save) {
			memcpy(blockptr, saveValueList[ix].val, saveValueList[ix].size);
		}
		else {
			memcpy(saveValueList[ix].val, blockptr, saveValueList[ix].size);
		}
		blockptr = (void*)((byte*)blockptr + saveValueList[ix].size);
	}
	if (!save) {
		// if it was saved, restore it and free the memory
		if (memptr[set]) {
			free(memptr[set]);
			memptr[set] = NULL;
		}
	}
	return true;
}

void EraseStartFile(MenuItem* menu)
{
	WriteOrDeleteConfigFile("", true, true);
}

void SaveStartFile(MenuItem* menu)
{
	WriteOrDeleteConfigFile("", false, true);
}

void EraseAssociatedFile(MenuItem* menu)
{
	WriteOrDeleteConfigFile(FileNames[CurrentFileIndex].c_str(), true, false);
}

void SaveAssociatedFile(MenuItem* menu)
{
	WriteOrDeleteConfigFile(FileNames[CurrentFileIndex].c_str(), false, false);
}

void LoadAssociatedFile(MenuItem* menu)
{
	String name = FileNames[CurrentFileIndex];
	name = MakeMIWFilename(name, true);
	if (ProcessConfigFile(name)) {
		WriteMessage(String("Processed:\n") + name);
	}
	else {
		WriteMessage(String("Failed reading:\n") + name, true);
	}
}

void LoadStartFile(MenuItem* menu)
{
	String name = "START.MIW";
	if (ProcessConfigFile(name)) {
		WriteMessage(String("Processed:\n") + name);
	}
	else {
		WriteMessage("Failed reading:\n" + name, true);
	}
}

// create the config file, or remove it
// startfile true makes it use the start.MIW file, else it handles the associated name file
bool WriteOrDeleteConfigFile(String filename, bool remove, bool startfile)
{
	bool retval = true;
	String filepath;
	if (startfile) {
		filepath = currentFolder + String("START.MIW");
	}
	else {
		filepath = ((bRecordingMacro || bRunningMacro) ? String("/") : currentFolder) + MakeMIWFilename(filename, true);
	}
	if (remove) {
		if (!SD.exists(filepath.c_str()))
			WriteMessage(String("Not Found:\n") + filepath);
		else if (SD.remove(filepath.c_str())) {
			WriteMessage(String("Erased:\n") + filepath);
		}
		else {
			WriteMessage(String("Failed to erase:\n") + filepath, true);
		}
	}
	else {
		String line;
#if USE_STANDARD_SD
		File file = SD.open(filepath.c_str(), bRecordingMacro ? FILE_APPEND : FILE_WRITE);
#else
		FsFile file = SD.open(filepath.c_str(), bRecordingMacro ? (O_APPEND | O_WRITE | O_CREAT) : (O_WRITE | O_TRUNC | O_CREAT));
#endif
		if (file) {
			// loop through the var list
			for (int ix = 0; ix < sizeof(SettingsVarList) / sizeof(*SettingsVarList); ++ix) {
				switch (SettingsVarList[ix].type) {
				case vtBuiltIn:
					line = String(SettingsVarList[ix].name) + "=" + String(*(bool*)(SettingsVarList[ix].address) ? "TRUE" : "FALSE");
					break;
				case vtShowFile:
					if (*(char*)(SettingsVarList[ix].address)) {
						line = String(SettingsVarList[ix].name) + "=" + (bShowBuiltInTests ? "" : currentFolder) + String((char*)(SettingsVarList[ix].address));
					}
					break;
				case vtInt:
					line = String(SettingsVarList[ix].name) + "=" + String(*(int*)(SettingsVarList[ix].address));
					break;
				case vtBool:
					line = String(SettingsVarList[ix].name) + "=" + String(*(bool*)(SettingsVarList[ix].address) ? "TRUE" : "FALSE");
					break;
				case vtRGB:
				{
					// handle the RBG colors
					CRGB* cp = (CRGB*)(SettingsVarList[ix].address);
					line = String(SettingsVarList[ix].name) + "=" + String(cp->r) + "," + String(cp->g) + "," + String(cp->b);
				}
				break;
				default:
					line = "";
					break;
				}
				if (line.length())
					file.println(line);
			}
			file.close();
			WriteMessage(String("Saved:\n") + filepath);
		}
		else {
			retval = false;
			WriteMessage(String("Failed to write:\n") + filepath, true);
		}
	}
	return retval;
}

// save some settings in the eeprom
// return true if valid, false if failed
bool SaveSettings(bool save, bool bOnlySignature, bool bAutoloadOnlyFlag)
{
	bool retvalue = true;
	int blockpointer = 0;
	for (int ix = 0; ix < (sizeof(saveValueList) / sizeof(*saveValueList)); blockpointer += saveValueList[ix++].size) {
		if (save) {
			EEPROM.writeBytes(blockpointer, saveValueList[ix].val, saveValueList[ix].size);
			if (ix == 0 && bOnlySignature) {
				break;
			}
			if (ix == 1 && bAutoloadOnlyFlag) {
				break;
			}
		}
		else {  // load
			if (ix == 0) {
				// check signature
				char svalue[sizeof(signature)];
				memset(svalue, 0, sizeof(svalue));
				size_t bytesread = EEPROM.readBytes(0, svalue, sizeof(signature));
				if (strcmp(svalue, signature)) {
					WriteMessage("bad eeprom signature\nrepairing...", true);
					return SaveSettings(true);
				}
				if (bOnlySignature) {
					return true;
				}
			}
			else {
				EEPROM.readBytes(blockpointer, saveValueList[ix].val, saveValueList[ix].size);
			}
			if (ix == 1 && bAutoloadOnlyFlag) {
				return true;
			}
		}
	}
	if (save) {
		retvalue = EEPROM.commit();
	}
	else {
		int savedFileIndex = CurrentFileIndex;
		// we don't know the folder path, so just reset the folder level
		currentFolder = "/";
		setupSDcard();
		CurrentFileIndex = savedFileIndex;
		// make sure file index isn't too big
		if (CurrentFileIndex >= FileNames.size()) {
			CurrentFileIndex = 0;
		}
		// set the brightness values since they might have changed
		SetDisplayBrightness(nDisplayBrightness);
		// don't need to do this here since it is always set right before running
		//FastLED.setBrightness(nStripBrightness);
	}
	WriteMessage(String(save ? (bAutoloadOnlyFlag ? "Autoload Saved" : "Settings Saved") : "Settings Loaded"));
	return retvalue;
}

// save the eeprom settings
void SaveEepromSettings(MenuItem* menu)
{
	SaveSettings(true);
}

// load eeprom settings
void LoadEepromSettings(MenuItem* menu)
{
	SaveSettings(false);
}

// save the macro with the current settings
void SaveMacro(MenuItem* menu)
{
	bRecordingMacro = true;
	WriteOrDeleteConfigFile(String(nCurrentMacro), false, false);
	bRecordingMacro = false;
}

// saves and restores settings
void RunMacro(MenuItem* menu)
{
	bCancelMacro = false;
	for (nMacroRepeatsLeft = nRepeatCountMacro; nMacroRepeatsLeft; --nMacroRepeatsLeft) {
		MacroLoadRun(menu, true);
		if (bCancelMacro) {
			break;
		}
		tft.fillScreen(TFT_BLACK);
		for (int wait = nRepeatWaitMacro; nMacroRepeatsLeft > 1 && wait; --wait) {
			if (CheckCancel()) {
				nMacroRepeatsLeft = 0;
				break;
			}
			DisplayLine(4, "#" + String(nCurrentMacro) + String(" Wait: ") + String(wait / 10) + "." + String(wait % 10) + " Repeat: " + String(nMacroRepeatsLeft - 1));
			delay(100);
		}
	}
	bCancelMacro = false;
}

// like run, but doesn't restore settings
void LoadMacro(MenuItem* menu)
{
	MacroLoadRun(menu, false);
}

void MacroLoadRun(MenuItem* menu, bool save)
{
	bool oldShowBuiltins;
	if (save) {
		oldShowBuiltins = bShowBuiltInTests;
		SettingsSaveRestore(true, 1);
	}
	bRunningMacro = true;
	bRecordingMacro = false;
	String line = String(nCurrentMacro) + ".miw";
	if (!ProcessConfigFile(line)) {
		line += " not found";
		WriteMessage(line, true);
	}
	bRunningMacro = false;
	if (save) {
		// need to handle if the builtins was changed
		if (oldShowBuiltins != bShowBuiltInTests) {
			ToggleFilesBuiltin(NULL);
		}
		SettingsSaveRestore(false, 1);
	}
}

void DeleteMacro(MenuItem* menu)
{
	WriteOrDeleteConfigFile(String(nCurrentMacro), true, false);
}

// show some LED's with and without white balance adjust
void ShowWhiteBalance(MenuItem* menu)
{
	for (int ix = 0; ix < 32; ++ix) {
		SetPixel(ix, CRGB(255, 255, 255));
	}
	FastLED.setTemperature(CRGB(255, 255, 255));
	FastLED.show();
	delay(2000);
	FastLED.clear(true);
	delay(50);
	FastLED.setTemperature(CRGB(whiteBalance.r, whiteBalance.g, whiteBalance.b));
	FastLED.show();
	delay(3000);
	FastLED.clear(true);
}

// reverse the strip index order for the lower strip, the upper strip is normal
// also check to make sure it isn't out of range
int AdjustStripIndex(int ix)
{
	if (ix < NUM_LEDS) {
		ix = (NUM_LEDS - 1 - ix);
	}
	ix = max(0, ix);
	ix = min(STRIPLENGTH - 1, ix);
	return ix;
}

// write a pixel to the correct location
// pixel doubling is handled here
// e.g. pixel 0 will be 0 and 1, 1 will be 2 and 3, etc
// if upside down n will be n and n-1, n-1 will be n-1 and n-2
void IRAM_ATTR SetPixel(int ix, CRGB pixel)
{
	if (bUpsideDown) {
		if (bDoublePixels) {
			leds[AdjustStripIndex(STRIPLENGTH - 1 - 2 * ix)] = pixel;
			leds[AdjustStripIndex(STRIPLENGTH - 2 - 2 * ix)] = pixel;
		}
		else {
			leds[AdjustStripIndex(STRIPLENGTH - 1 - ix)] = pixel;
		}
	}
	else {
		if (bDoublePixels) {
			leds[AdjustStripIndex(2 * ix)] = pixel;
			leds[AdjustStripIndex(2 * ix + 1)] = pixel;
		}
		else {
			leds[AdjustStripIndex(ix)] = pixel;
		}
	}
}

#define Fbattery    3700  //The default battery is 3700mv when the battery is fully charged.

float XS = 0.00225;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;
// read and display the battery voltage
void ReadBattery(MenuItem* menu)
{
	//tft.clear();
	uint16_t bat = analogRead(A4);
	Serial.println("bat: " + String(bat));
	DisplayLine(0, "Battery: " + String(bat));
	delay(1000);
	//uint16_t c = analogRead(13) * XS * MUL;
	////uint16_t d  =  (analogRead(13)*XS*MUL*MMUL)/Fbattery;
	//Serial.println(analogRead(13));
	////Serial.println((String)d);
 //  // Serial.printf("%x",analogRead(13));
	//Heltec.display->drawString(0, 0, "Remaining battery still has:");
	//Heltec.display->drawString(0, 10, "VBAT:");
	//Heltec.display->drawString(35, 10, (String)c);
	//Heltec.display->drawString(60, 10, "(mV)");
	//// Heltec.display->drawString(90, 10, (String)d);
	//// Heltec.display->drawString(98, 10, ".";
	//// Heltec.display->drawString(107, 10, "%");
	//Heltec.display->display();
	//delay(2000);
	//Heltec.display->clear();
 //Battery voltage read pin changed from GPIO13 to GPI37
	////adcStart(37);
	////while (adcBusy(37))
	////	;
	////Serial.printf("Battery power in GPIO 37: ");
	////Serial.println(analogRead(37));
	////uint16_t c1 = analogRead(37) * XS * MUL;
	////adcEnd(37);
	////Serial.println("Vbat: " + String(c1));

	////delay(100);

	//adcStart(36);
	//while (adcBusy(36));
	//Serial.printf("voltage input on GPIO 36: ");
	//Serial.println(analogRead(36));
	//uint16_t c2 = analogRead(36) * 0.769 + 150;
	//adcEnd(36);
	//Serial.println("-------------");
	// uint16_t c  =  analogRead(13)*XS*MUL;
	// Serial.println(analogRead(13));
	////Heltec.display->drawString(0, 0, "Vbat = ");
	////Heltec.display->drawString(33, 0, (String)c1);
	////Heltec.display->drawString(60, 0, "(mV)");

	//Heltec.display->drawString(0, 10, "Vin   = ");
	//Heltec.display->drawString(33, 10, (String)c2);
	//Heltec.display->drawString(60, 10, "(mV)");

	// Heltec.display->drawString(0, 0, "Remaining battery still has:");
	// Heltec.display->drawString(0, 10, "VBAT:");
	// Heltec.display->drawString(35, 10, (String)c);
	// Heltec.display->drawString(60, 10, "(mV)");
	////Heltec.display->display();
	////delay(2000);
}

// grow and shrink a rainbow type pattern
#define PI_SCALE 2
#define TWO_HUNDRED_PI (628*PI_SCALE)
void RainbowPulse()
{
	int element = 0;
	int last_element = 0;
	int highest_element = 0;
	//Serial.println("second: " + String(bSecondStrip));
	//Serial.println("Len: " + String(STRIPLENGTH));
	for (int i = 0; i < TWO_HUNDRED_PI; i++) {
		element = round((STRIPLENGTH - 1) / 2 * (-cos(i / (PI_SCALE * 100.0)) + 1));
		if (element > last_element) {
			//Serial.println("el: " + String(element));
			SetPixel(element, CHSV(element * nRainbowPulseColorScale + nRainbowPulseStartColor, nRainbowPulseSaturation, 255));
			FastLED.show();
			highest_element = max(highest_element, element);
		}
		if (CheckCancel()) {
			return;
		}
		delayMicroseconds(nRainbowPulsePause * 10);
		if (element < last_element) {
			// cleanup the highest one
			SetPixel(highest_element, CRGB::Black);
			SetPixel(element, CRGB::Black);
			FastLED.show();
		}
		last_element = element;
	}
}

/*
	Write a wedge in time, from the middle out
*/
void TestWedge()
{
	int midPoint = STRIPLENGTH / 2 - 1;
	for (int ix = 0; ix < STRIPLENGTH / 2; ++ix) {
		SetPixel(midPoint + ix, CRGB(nWedgeRed, nWedgeGreen, nWedgeBlue));
		SetPixel(midPoint - ix, CRGB(nWedgeRed, nWedgeGreen, nWedgeBlue));
		if (!bWedgeFill) {
			if (ix > 1) {
				SetPixel(midPoint + ix - 1, CRGB::Black);
				SetPixel(midPoint - ix + 1, CRGB::Black);
			}
			else {
				SetPixel(midPoint, CRGB::Black);
			}
		}
		FastLED.show();
		delay(nFrameHold);
		if (CheckCancel()) {
			return;
		}
	}
	FastLED.clear(true);
}

//#define NUM_LEDS 22
//#define DATA_PIN 5
//#define TWO_HUNDRED_PI 628
//#define TWO_THIRDS_PI 2.094
//
//void loop()
//{
//	int val1 = 0;
//	int val2 = 0;
//	int val3 = 0;
//	for (int i = 0; i < TWO_HUNDRED_PI; i++) {
//		val1 = round(255 / 2.0 * (sin(i / 100.0) + 1));
//		val2 = round(255 / 2.0 * (sin(i / 100.0 + TWO_THIRDS_PI) + 1));
//		val3 = round(255 / 2.0 * (sin(i / 100.0 - TWO_THIRDS_PI) + 1));
//
//		leds[7] = CHSV(0, 255, val1);
//		leds[8] = CHSV(96, 255, val2);
//		leds[9] = CHSV(160, 255, val3);
//
//		FastLED.show();
//
//		delay(1);
//	}
//}
// #########################################################################
// Fill screen with a rainbow pattern
// #########################################################################
byte red = 31;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11; // Colour order is RGB 5+6+5 bits each

void rainbow_fill()
{
	// The colours and state are not initialised so the start colour changes each time the function is called

	for (int i = 319; i >= 0; i--) {
		// Draw a vertical line 1 pixel wide in the selected colour
		tft.drawFastHLine(0, i, tft.width(), colour); // in this example tft.width() returns the pixel width of the display
		// This is a "state machine" that ramps up/down the colour brightnesses in sequence
		switch (state) {
		case 0:
			green++;
			if (green == 64) {
				green = 63;
				state = 1;
			}
			break;
		case 1:
			red--;
			if (red == 255) {
				red = 0;
				state = 2;
			}
			break;
		case 2:
			blue++;
			if (blue == 32) {
				blue = 31;
				state = 3;
			}
			break;
		case 3:
			green--;
			if (green == 255) {
				green = 0;
				state = 4;
			}
			break;
		case 4:
			red++;
			if (red == 32) {
				red = 31;
				state = 5;
			}
			break;
		case 5:
			blue--;
			if (blue == 255) {
				blue = 0;
				state = 0;
			}
			break;
		}
		colour = red << 11 | green << 5 | blue;
	}
}

// draw a progress bar
void DrawProgressBar(int x, int y, int dx, int dy, int percent)
{
	tft.drawRoundRect(x, y, dx, dy, 2, TFT_WHITE);
	int fill = (dx - 2) * percent / 100;
	// fill the filled part
	tft.fillRect(x + 1, y + 1, fill, dy - 2, TFT_GREEN);
	// blank the empty part
	tft.fillRect(x + 1 + fill, y + 1, dx - 2 - fill, dy - 2, TFT_BLACK);
}
