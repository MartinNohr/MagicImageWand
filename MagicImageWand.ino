/*
 Name:		MagicImageWand.ino
 Created:	12/18/2020 6:12:01 PM
 Author:	Martin Nohr
*/

#include <TFT_eSPI.h>
#include "MagicImageWand.h"
#include "fonts.h"
#include <nvs_flash.h>

RTC_DATA_ATTR int nBootCount = 0;

// some forward references that Arduino IDE needs
int readByte(bool clear);
void ReadAndDisplayFile(bool doingFirstHalf);
uint16_t readInt();
uint32_t readLong();
void FileSeekBuf(uint32_t place);
int FileCountOnly(int start = 0);

//static const char* TAG = "lightwand";
//esp_timer_cb_t oneshot_timer_callback(void* arg)
void oneshot_LED_timer_callback(void* arg)
{
	g_bStripWaiting = false;
	//int64_t time_since_boot = esp_timer_get_time();
	//Serial.println("in isr");
	//ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
}

// timer called every second
void periodic_Second_timer_callback(void* arg)
{
	if (sleepTimer)
		--sleepTimer;
	if (SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_TIME && displayDimTimer) {
		--displayDimTimer;
		if (displayDimTimer == 0) {
			displayDimNow = true;
		}
	}
	if (g_nB0Pressed)
		--g_nB0Pressed;
	if (g_nB1Pressed)
		--g_nB1Pressed;
}

constexpr int TFT_ENABLE = 4;
// use these to control the LCD brightness
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
TFT_eSprite LineSprite = TFT_eSprite(&tft);  // Create Sprite object "LineSprite" with pointer to "tft" object
#define BATTERY_BAR_HEIGHT 5
TFT_eSprite BatterySprite = TFT_eSprite(&tft);  // Create Sprite object "BatterySprite" with pointer to "tft" object
TFT_eSprite FileCountSprite = TFT_eSprite(&tft);	// shows the file position and count

void setup()
{
	// init the display
	tft.init();
	tft.fillScreen(TFT_BLACK);
	Serial.begin(115200);
	while (!Serial.availableForWrite()) {
		delay(10);
	}
	//Serial.println("flash:" + String(ESP.getFlashChipSize()));
	//Serial.print("setup() is running on core ");
	//Serial.println(xPortGetCoreID());
	// create a mutex
	macroMutex = xSemaphoreCreateMutex();

	// configure LCD PWM functionalitites
	pinMode(TFT_ENABLE, OUTPUT);
	digitalWrite(TFT_ENABLE, 1);
	ledcSetup(ledChannel, freq, resolution);
	// attach the channel to the GPIO to be controlled
	ledcAttachPin(TFT_ENABLE, ledChannel);
#if TTGO_T == 1
	CRotaryDialButton::begin((gpio_num_t)DIAL_A, (gpio_num_t)DIAL_B, (gpio_num_t)DIAL_BTN, (gpio_num_t)0, (gpio_num_t)35, (gpio_num_t)-1, (gpio_num_t)-1, &SystemInfo.DialSettings);
#elif TTGO_T == 4
	CRotaryDialButton::begin((gpio_num_t)DIAL_A, (gpio_num_t)DIAL_B, (gpio_num_t)DIAL_BTN, (gpio_num_t)0, (gpio_num_t)-1, (gpio_num_t)38, (gpio_num_t)39, &SystemInfo.DialSettings);
#endif
	setupSDcard();
	//gpio_set_direction((gpio_num_t)LED, GPIO_MODE_OUTPUT);
	//digitalWrite(LED, HIGH);
	gpio_set_direction((gpio_num_t)FRAMEBUTTON_GPIO, GPIO_MODE_INPUT);
	gpio_set_pull_mode((gpio_num_t)FRAMEBUTTON_GPIO, GPIO_PULLUP_ONLY);
	// init the onboard buttons
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
	gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);

	oneshot_LED_timer_args = {
				oneshot_LED_timer_callback,
				/* argument specified here will be passed to timer callback function */
				(void*)0,
				ESP_TIMER_TASK,
				"one-shotLED"
	};
	esp_timer_create(&oneshot_LED_timer_args, &oneshot_LED_timer);

	periodic_Second_timer_args = {
				periodic_Second_timer_callback,
				/* argument specified here will be passed to timer callback function */
				(void*)0,
				ESP_TIMER_TASK,
				"second timer"
	};
	esp_timer_create(&periodic_Second_timer_args, &periodic_Second_timer);
	esp_timer_start_periodic(periodic_Second_timer, (int64_t)1000 * 1000);

	SystemInfo.bCriticalBatteryLevel = false;
	tft.setFreeFont(&Dialog_bold_16);
#if TTGO_T == 1
	SystemInfo.nDisplayRotation = 3;
#elif TTGO_T == 4
	SystemInfo.nDisplayRotation = 0;
#endif
	tft.setTextSize(1);
	tft.setTextPadding(tft.width());
	SetScreenRotation(SystemInfo.nDisplayRotation);
	//ClearScreen();
	SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	// see if the button is down, if so clear all settings
	if (gpio_get_level((gpio_num_t)DIAL_BTN) == 0) {
		Preferences prefs;
		prefs.begin(prefsName);
		prefs.clear();
		prefs.end();
		WriteMessage("Factory Reset");
	}
	String msg;
	// see if we can read the settings
	if (SaveLoadSettings(false, true, false, true)) {
		if ((nBootCount == 0) && bAutoLoadSettings) {
			SaveLoadSettings(false, false, false, true);
			msg = "Settings Loaded";
		}
	}
	else {
		// set the dial type
		CheckRotaryDialType();
		// see if there is a light sensor, read it until it is stable
		int lastVal = 0;
		int val = 0;
		for (int i = 0; i < 50; ++i) {
			val = ReadLightSensor();
			//Serial.println("read sensor: " + String(val));
			if (lastVal >= val)
				break;
			lastVal = val;
			delay(10);
		}
		SystemInfo.bHasLightSensor = val < 4094;
		// must not be anything there, so save it
		SaveLoadSettings(true, false, false, true);
	}
	// in case the saved ones were different
	SetScreenRotation(SystemInfo.nDisplayRotation);
	//ClearScreen();
	SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	//WiFi
	if (SystemInfo.bRunWebServer) {
		WiFi.softAP(ssid, password);
		IPAddress myIP = WiFi.softAPIP();
		// save for the menu system
		strncpy(localIpAddress, myIP.toString().c_str(), sizeof(localIpAddress));
		Serial.print("AP IP address: ");
		Serial.println(myIP);
		server.begin();
		Serial.println("Server started");
		for (OnServerItem item : OnServerList) {
			server.on(item.path, item.function);
		}
		server.on("/fupload", HTTP_POST, []() { server.send(200); }, handleFileUpload);
		//server.on("/settings/increpeat", HTTP_GET, []() { server.send(200); }, IncRepeat);
		//server.on("/settings/increpeat", HTTP_GET, IncRepeat);
		/////////////////////////// End of Request commands
		server.begin();
	}
	if (nBootCount) {
		// see if we need to get the path back
		if (strlen(sleepFolder))
			currentFolder = sleepFolder;
	}
#if !HAS_BATTERY_LEVEL
	SystemInfo.bShowBatteryLevel = false;
#endif
	tft.setFreeFont(&Dialog_bold_16);
	tft.setTextColor(SystemInfo.menuTextColor);
	// get our text line sprite ready
	LineSprite.setColorDepth(16);
	LineSprite.createSprite(tft.width(), tft.fontHeight());
	LineSprite.fillSprite(TFT_BLACK);
	LineSprite.setFreeFont(&Dialog_bold_16);
	LineSprite.setTextPadding(tft.width());
	// get our Battery sprite ready
	BatterySprite.setColorDepth(16);
	BatterySprite.createSprite(100, tft.fontHeight() + BATTERY_BAR_HEIGHT);
	BatterySprite.fillSprite(TFT_BLACK);
	BatterySprite.setFreeFont(&Dialog_bold_16);
	BatterySprite.setTextPadding(tft.width());
	// get the file count sprite ready
	FileCountSprite.setColorDepth(16);
	FileCountSprite.createSprite(110, tft.fontHeight() + 2);
	FileCountSprite.fillSprite(TFT_BLACK);
	FileCountSprite.setFreeFont(&Dialog_bold_16);
	FileCountSprite.setTextPadding(tft.width());
	// get the menu system ready
	menuPtr = new MenuInfo;
	MenuStack.push(menuPtr);
	MenuStack.top()->menu = MainMenu;
	MenuStack.top()->index = 0;
	MenuStack.top()->offset = 0;
	leds = (CRGB*)calloc(LedInfo.nTotalLeds, sizeof(*leds));
	tmpLeds = (CRGB*)calloc(LedInfo.nTotalLeds, sizeof(*leds));
	if (LedInfo.bSwapControllers)
		FastLED.addLeds<NEOPIXEL, DATA_PIN2>(leds, 0, LedInfo.bSecondController ? LedInfo.nTotalLeds / 2 : LedInfo.nTotalLeds);
	else
		FastLED.addLeds<NEOPIXEL, DATA_PIN1>(leds, 0, LedInfo.bSecondController ? LedInfo.nTotalLeds / 2 : LedInfo.nTotalLeds);
	//FastLED.addLeds<NEOPIXEL, DATA_PIN2>(leds, 0, NUM_LEDS);	// to test parallel second strip
	// create the second led controller
	if (LedInfo.bSecondController) {
		if (LedInfo.bSwapControllers)
			FastLED.addLeds<NEOPIXEL, DATA_PIN1>(leds, LedInfo.nTotalLeds / 2, LedInfo.nTotalLeds / 2);
		else
			FastLED.addLeds<NEOPIXEL, DATA_PIN2>(leds, LedInfo.nTotalLeds / 2, LedInfo.nTotalLeds / 2);
	}
	FastLED.setTemperature(CRGB(LedInfo.whiteBalance.r, LedInfo.whiteBalance.g, LedInfo.whiteBalance.b));
	FastLED.setBrightness(LedInfo.nLEDBrightness);
	//FastLED.setMaxPowerInVoltsAndMilliamps(5, LedInfo.nPixelMaxCurrent);
	if (nBootCount == 0) {
		// this must run on same task as main or only the first few LEDs light using FastLED 3.5, don't know why, 3.3 worked
		xTaskCreatePinnedToCore(TaskInitTestLed, "LEDTEST", 10000, NULL, 1, &TaskLEDTest, xPortGetCoreID());
		if (SystemInfo.bRunArtNetDMX) {
			xTaskCreatePinnedToCore(TaskRunArtNet, "ARTNET", 10000, NULL, 1, &TaskArtNet, xPortGetCoreID());
		}
		tft.setTextColor(SystemInfo.menuTextColor);
		//grey_fill();
		rainbow_fill();
		tft.setTextColor(TFT_BLACK);
		tft.setFreeFont(&Irish_Grover_Regular_24);
		tft.drawRect(0, 0, tft.width() - 1, tft.height() - 1, SystemInfo.menuTextColor);
		tft.drawRect(1, 1, tft.width() - 2, tft.height() - 2, SystemInfo.menuTextColor);
		tft.drawString("Magic Image Wand", 5, 10);
		tft.setFreeFont(&Dialog_bold_16);
		tft.drawString(String("Version ") + MIW_Version, 20, 70);
		tft.setTextSize(1);
		tft.drawString(__DATE__, 20, 90);
		if (msg.length()) {
			tft.drawString(msg, 20, 110);
		}
	}
	// clear the button buffer
	CRotaryDialButton::clear();
	nBootCount = 0;
	// load the sleep timer
	sleepTimer = SystemInfo.nSleepTime * 60;
	GetFileNamesFromSDorBuiltins(currentFolder);
	// read the macro data
	if (bSdCardValid)
		ReadMacroInfo();
	for (int cnt = 0; cnt < 400; ++cnt) {
		if (ReadButton() != BTN_NONE) {
			break;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	ClearScreen();
	DisplayCurrentFile();
	//// wait for led test to finish
	//eTaskState state = eTaskGetState(TaskLEDTest);
	//for (; state != eReady; delay(10)) {
	//	state = eTaskGetState(TaskLEDTest);
	//}
}

// task to run ArtNet
void TaskRunArtNet(void* parameter)
{
	artnet.setName(SystemInfo.cArtNetName);
	artnet.setNumPorts(1);
	artnet.enableDMXOutput(0);
	//artnet.disableDMXOutput(0);
	artnet.setStartingUniverse(SystemInfo.bStartUniverseOne ? 1 : 0);
	// use for ArtNetWiFi
	ConnectWifi();
	artnet.begin();
	// this will be called for each packet received
	artnet.setArtDmxCallback(onDmxFrame);
	while (true) {
		artnet.read();
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

// task to test the LEDS on start
void TaskInitTestLed(void* parameter)
{
	FastLED.clear(true);
	if (SystemInfo.bInitTest)
		TestLEDs(500);
	vTaskDelete(NULL);
}

// check and handle the rotary dial type
// if either A or B is 0, then this is a toggle dial
// else
// tell user to rotate one click
// delay
// if A or B is 0, then this is a toggle
// else it is a pulse dial
void CheckRotaryDialType()
{
	bool bA, bB;
	WriteMessage("checking dial type...", false, 1000);
	bA = gpio_get_level((gpio_num_t)DIAL_A);
	bB = gpio_get_level((gpio_num_t)DIAL_B);
	//Serial.println("ab " + String(bA) + String(bB));
	if (!bA && !bB) {
		// if both low must be a toggle
		SystemInfo.DialSettings.m_bToggleDial = true;
	}
	else {
		WriteMessage("Rotate dial 1 click", false, 10);
		// wait for rotate, they were both high before if we got this far, so just look at A
		while (gpio_get_level((gpio_num_t)DIAL_A))
			delay(10);
		// wait for button bounce
		delay(250);
		// read them again
		bA = gpio_get_level((gpio_num_t)DIAL_A);
		bB = gpio_get_level((gpio_num_t)DIAL_B);
		//Serial.println("ab " + String(bA) + String(bB));
		// if both low must be a toggle
		SystemInfo.DialSettings.m_bToggleDial = !bA && !bB;
		// we shouldn't need this again
	}
	if (!SystemInfo.DialSettings.m_bToggleDial)
		SystemInfo.DialSettings.m_nDialPulseCount = 2;
	WriteMessage(String("Dial Type: ") + (SystemInfo.DialSettings.m_bToggleDial ? "Toggle" : "Pulse"), false, 1000);
}

// read the macro info from the files if we didn't find the json file first
void ReadMacroInfo()
{
	if (SD.exists(MACRO_JSON_FILE)) {
		// read the file
#if USE_STANDARD_SD
		SDFile file;
		file = SD.open(MACRO_JSON_FILE);
		if (file) {
#else
		FsFile file;
		file = SD.open(MACRO_JSON_FILE);
		if (file.getError() == 0) {
#endif
		//WriteMessage("Reading: " + String(MACRO_JSON_FILE), false, 1000);
			//StaticJsonDocument<JSON_DOC_SIZE> doc;
			DynamicJsonDocument doc(JSON_DOC_SIZE);
			String input = file.readString();
			//Serial.println("json size: " + String(input.length()));
			DeserializationError err = deserializeJson(doc, input);
			if (err) {
				WriteMessage(String("failed to parse: ") + MACRO_JSON_FILE, true);
				//Serial.print(F("deserializeJson() failed with code "));
				//Serial.println(err.f_str());
			}
			else {
				// read the json into the macroinfo
				for (int ix = 0; ix < 10; ++ix) {
					MacroInfo[ix].description = String(doc[ix]["description"].as<const char*>());
					MacroInfo[ix].mSeconds = doc[ix]["mSeconds"];
					MacroInfo[ix].length = doc[ix]["length"].as<int>();
					MacroInfo[ix].pixels = doc[ix]["pixels"].as<int>();
					JsonArray ja = doc[ix]["images"];
					for (String str : ja) {
						MacroInfo[ix].fileNames.push_back(str);
					}
				}
			}
			file.close();
		}
		else {
#if USE_STANDARD_SD
			WriteMessage(String("failed to open: ") + MACRO_JSON_FILE, true);
#else
			WriteMessage(String("failed to open: ") + MACRO_JSON_FILE + " error: " + String(file.getError()), true);
#endif
		}
	}
	else {
		int fileCount, pixelWidth;
		WriteMessage(String("Creating: ") + MACRO_JSON_FILE, false, 1000);
		for (int ix = 0; ix < 10; ++ix) {
			String fn = MakeMIWFilename(String(ix), true);
			MacroInfo[ix].fileNames.clear();
			MacroInfo[ix].mSeconds = MacroTime("/" + fn, &fileCount, &pixelWidth, &MacroInfo[ix].fileNames);
			MacroInfo[ix].description = SD.exists("/" + fn) ? "Used" : "Empty";
			MacroInfo[ix].length = (float)pixelWidth / (float)LedInfo.nTotalLeds;
			MacroInfo[ix].pixels = pixelWidth;
		}
		// save the info since we just created it
		SaveMacroInfo();
	}
}
// save the macro info in json to a file called macro.json
void SaveMacroInfo()
{
#if USE_STANDARD_SD
	SDFile file;
	file = SD.open(MACRO_JSON_FILE, FILE_WRITE);
	if (file) {
#else
	FsFile file;
	file = SD.open(MACRO_JSON_FILE, O_WRITE | O_CREAT | O_TRUNC);
	if (file.getError() == 0) {
#endif
		DynamicJsonDocument doc(JSON_DOC_SIZE);
		//StaticJsonDocument<JSON_DOC_SIZE> doc;
		for (int ix = 0; ix < 10; ++ix) {
			doc[ix]["ID"] = ix;
			doc[ix]["description"] = MacroInfo[ix].description;
			doc[ix]["length"] = MacroInfo[ix].length;
			doc[ix]["mSeconds"] = MacroInfo[ix].mSeconds;
			doc[ix]["pixels"] = MacroInfo[ix].pixels;
			//doc[ix]["filecount"] = MacroInfo[ix].fileNames.size();
			JsonArray ja = doc[ix]["images"].to<JsonArray>();
			int fix = 0;
			// add the filenames in an array
			for (String fname : MacroInfo[ix].fileNames) {
				ja[fix] = fname;
				++fix;
			}
		}
		char output[JSON_DOC_SIZE];
		serializeJsonPretty(doc, output);
		file.write((uint8_t*)output, strlen(output));
		file.close();
	}
	else {
		// something went wrong
#if USE_STANDARD_SD
		WriteMessage(String("failed to open: ") + MACRO_JSON_FILE, true);
#else
		WriteMessage(String("failed to open: ") + MACRO_JSON_FILE + " error: " + String(file.getError()), true);
#endif
	}
}

void ResetSleepAndDimTimers() {
	sleepTimer = SystemInfo.nSleepTime * 60;
	displayDimTimer = SystemInfo.nDisplayDimTime;
	if (SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_TIME && SystemInfo.nDisplayDimTime) {
		SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	}
}

// scroll the long menu lines
// this also checks the light sensor if enabled
void MenuTextScrollSideways()
{
	// this handles sideways scrolling of really long menu items
	static unsigned long menuUpdateTime = 0;
	static unsigned long ledUpdateTime = 0;
	if (SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_SENSOR && millis() > ledUpdateTime + 100) {
		ledUpdateTime = millis();
		LightSensorLedBrightness();
	}
	if (millis() > menuUpdateTime + SystemInfo.nSidewayScrollSpeed) {
		menuUpdateTime = millis();
		for (int ix = 0; ix < nMenuLineCount; ++ix) {
			int offset = TextLines[ix].nRollOffset;
			if (TextLines[ix].nRollLength) {
				if (TextLines[ix].nRollOffset == 0 && TextLines[ix].nRollDirection == 0) {
					TextLines[ix].nRollDirection = SystemInfo.nSidewaysScrollPause;
					continue;
				}
				if (TextLines[ix].nRollDirection > 1) {
					--TextLines[ix].nRollDirection;
				}
				if (TextLines[ix].nRollDirection == 1) {
					++TextLines[ix].nRollOffset;
				}
				if (TextLines[ix].nRollOffset >= (TextLines[ix].nRollLength - tft.width()) && TextLines[ix].nRollDirection > 0) {
					TextLines[ix].nRollDirection = -SystemInfo.nSidewaysScrollPause;
				}
				if (TextLines[ix].nRollDirection < -1) {
					++TextLines[ix].nRollDirection;
				}
				if (TextLines[ix].nRollDirection == -1) {
					TextLines[ix].nRollOffset -= SystemInfo.nSidewaysScrollReverse;
					if (TextLines[ix].nRollOffset < 0) {
						TextLines[ix].nRollOffset = 0;
					}
					if (TextLines[ix].nRollOffset == 0) {
						TextLines[ix].nRollDirection = 0;
					}
				}
				if (offset != TextLines[ix].nRollOffset) {
					DisplayLine(ix, TextLines[ix].Line, TextLines[ix].foreColor, TextLines[ix].backColor);
				}
			}
		}
	}
}

// call the current setting for btn0 long press
void CallBtnLongFunction(int which)
{
	switch (which) {
	case BTN_LONG_ROTATION:
		SetScreenRotation(-1);
		// make the LED's upside down for mode 1.
		ImgInfo.bUpsideDown = SystemInfo.nDisplayRotation == 1;
		break;
	case BTN_LONG_LIGHTBAR:
		LightBar(NULL);
		break;
	}
}

void loop()
{
	static LED_INFO LedInfoSaved;
	static SYSTEM_INFO SystemInfoSaved;
	static BUILTIN_INFO BuiltinInfoSaved;
	static bool didsomething = false;
	static bool bLastSettingsMode = false;

	didsomething = bSettingsMode ? HandleMenus() : HandleRunMode();
	if (SystemInfo.nSleepTime && sleepTimer == 0) {
		// go to sleep
		Sleep(NULL);
	}
	if (bSettingsMode && !bLastSettingsMode) {
		memcpy(&SystemInfoSaved, &SystemInfo, sizeof(SystemInfo));
		memcpy(&LedInfoSaved, &LedInfo, sizeof(LedInfo));
		memcpy(&BuiltinInfoSaved, &BuiltinInfo, sizeof(BuiltinInfo));
	}
	if (!bSettingsMode && bLastSettingsMode) {
		if (memcmp(&SystemInfoSaved, &SystemInfo, sizeof(SystemInfo))) {
			// make sure that the lcd dim is less than the bright
			if (SystemInfo.nDisplayDimValue > SystemInfo.nDisplayBrightness)
				SystemInfo.nDisplayDimValue = SystemInfo.nDisplayBrightness;
			SaveLoadSettings(true, false, true, true);
		}
		if (memcmp(&BuiltinInfoSaved, &BuiltinInfo, sizeof(BuiltinInfo))) {
			SaveLoadSettings(true, false, true, true);
		}
	}
	bLastSettingsMode = bSettingsMode;
	if (!bSettingsMode && bControllerReboot) {
		if (memcmp(&LedInfo, &LedInfoSaved, sizeof(LedInfo)) || memcmp(&SystemInfoSaved, &SystemInfo, sizeof(SystemInfo))) {
			WriteMessage("Rebooting due to\nsystem change", false, 2000);
			SaveLoadSettings(true, false, true, true);
			ESP.restart();
		}
		else {
			bControllerReboot = false;
		}
	}
	if (SystemInfo.bRunWebServer) {
		server.handleClient();
	}
	// wait for no keys
	if (didsomething) {
		didsomething = false;
		delay(1);
	}
	if (!bSettingsMode) {
		// show file index if on
		if (SystemInfo.bShowFilePosition) {
			ShowFilePosition();
		}
		// show battery level if on
		if (SystemInfo.bShowBatteryLevel) {
			int raw;
			ReadBattery(&raw);
			//Serial.println(String("bat:") + String(raw));
			ShowBattery(NULL);
			if (raw > 900 && SystemInfo.bSleepOnLowBattery && SystemInfo.bCriticalBatteryLevel) {
				SystemInfo.bCriticalBatteryLevel = false;
				WriteMessage("Entering sleep mode\ndue to low battery", true, 10000);
				Sleep(NULL);
			}
		}
	}
}

// do something from the menu depending on the button argument
// only two buttons are actually handled, SELECT and HELP
void RunMenus(int button)
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
		if (!bMenuValid[ix]) {
			continue;	// and don't increment menix
		}
		if (menuix == MenuStack.top()->index) {
			gotmatch = true;
			switch (button) {
			case BTN_B0_LONG:	// handle help if there is any
				if (MenuStack.top()->menu[ix].cHelpText) {
					WriteMessage(MenuStack.top()->menu[ix].cHelpText, false, -1, true);
				}
				bMenuChanged = true;
				break;
			case BTN_SELECT:	// handle selection
				// got one, service it
				switch (MenuStack.top()->menu[ix].op) {
				case eTerminate:	// not used, tell compiler
				case eIfEqual:
				case eIfIntEqual:
				case eElse:
				case eEndif:
					break;
				case eText:
				case eTextInt:
				case eTextCurrentFile:
				case eBool:
				case eList:
					bMenuChanged = true;
					if (MenuStack.top()->menu[ix].change != NULL) {
						(*MenuStack.top()->menu[ix].change)(&MenuStack.top()->menu[ix], 1);
					}
					if (MenuStack.top()->menu[ix].function) {
						(*MenuStack.top()->menu[ix].function)(&MenuStack.top()->menu[ix]);
					}
					if (MenuStack.top()->menu[ix].change != NULL) {
						(*MenuStack.top()->menu[ix].change)(&MenuStack.top()->menu[ix], -1);
					}
					break;
				case eMacroList:
					bMenuChanged = true;
					if (MenuStack.top()->menu[ix].change != NULL) {
						(*MenuStack.top()->menu[ix].change)(&MenuStack.top()->menu[ix], 1);
					}
					if (MenuStack.top()->menu[ix].function) {
						(*MenuStack.top()->menu[ix].function)(&MenuStack.top()->menu[ix]);
					}
					if (MenuStack.top()->menu[ix].change != NULL) {
						(*MenuStack.top()->menu[ix].change)(&MenuStack.top()->menu[ix], -1);
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
						// check if the new menu is an eMacroList and if it has a value, if it does, set the index to it
						if (MenuStack.top()->menu->op == eMacroList && MenuStack.top()->menu->value) {
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
					if (BuiltInFiles[currentFileIndex.nFileIndex].menu != NULL) {
						MenuStack.top()->index = MenuStack.top()->index;
						MenuStack.push(new MenuInfo);
						MenuStack.top()->menu = BuiltInFiles[currentFileIndex.nFileIndex].menu;
						MenuStack.top()->index = 0;
						MenuStack.top()->offset = 0;
					}
					else {
						WriteMessage("No settings available for:\n" + String(BuiltInFiles[currentFileIndex.nFileIndex].text));
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
		}
		++menuix;
	}
	// if no match, and we are in a submenu, go back one level, or if bExit is set
	if (bExit || (!bMenuChanged && MenuStack.size() > 1)) {
		UpMenuLevel(false);
	}
	// see if the autoload flag changed
	if (bAutoLoadSettings != lastAutoLoadFlag) {
		// the flag is now true, so we should save the current settings
		SaveLoadSettings(true);
	}
}

// display the menu
// if MenuStack.top()->index is > MENU_LINES, then shift the lines up by enough to display them
// remember that we only have room for MENU_LINES lines
void ShowMenu(struct MenuItem* menu)
{
	MenuStack.top()->menucount = 0;
	int y = 0;
	int x = 0;
	// load with a false to start with
	std::stack<bool> skipStack;
	skipStack.push(false);
	// this is the active stack level, I.E. which level should be processed
	int skipLevel = 1;
	bool bSkipping = false;
	// loop through the menu
	for (int menix = 0; menu->op != eTerminate; ++menu, ++menix) {
		// make sure menu valid vector is big enough
		if (bMenuValid.size() < menix + 1) {
			bMenuValid.resize(menix + 1);
		}
		bMenuValid[menix] = false;
		switch ((menu->op)) {
		case eIfEqual:
			// skip the next one if match, this is boolean only
			skipStack.push(*(bool*)menu->value != (menu->min ? true : false));
			//Serial.println("ifequal test: skip: " + String(skip));
			if (!bSkipping) {
				++skipLevel;
				bSkipping = skipStack.top();
			}
			break;
		case eIfIntEqual:
			// skip the next one if match, this is int values
			skipStack.push(*(int*)menu->value != menu->min);
			//Serial.println("ifIntequal test: skip: " + String(skip));
			if (!bSkipping) {
				++skipLevel;
				bSkipping = skipStack.top();
			}
			break;
		case eElse:
			skipStack.top() = !skipStack.top();
			break;
		case eEndif:
			skipStack.pop();
			if (!bSkipping || skipLevel > skipStack.size()) {
				--skipLevel;
			}
			break;
		default:
			break;
		}
		bSkipping = skipLevel < skipStack.size() ? true : skipStack.top();
		if (bSkipping) {
			bMenuValid[menix] = false;
			continue;
		}
		char line[120]{}, xtraline[100]{};
		// only displayable menu items should be in this switch
		line[0] = '\0';
		int val;
		bool exists = false;
		switch (menu->op) {
		case eTextInt:
		case eText:
		case eTextCurrentFile:
			bMenuValid[menix] = true;
			if (menu->value) {
				val = *(int*)menu->value;
				if (menu->op == eText) {
					sprintf(line, menu->text, (char*)(menu->value));
				}
				else if (menu->op == eTextInt) {
					sprintf(line, menu->text, (int)(val / pow10(menu->decimals)), val % (int)(pow10(menu->decimals)));
				}
			}
			else {
				if (menu->op == eTextCurrentFile) {
					sprintf(line, menu->text, MakeMIWFilename(FileNames[currentFileIndex.nFileIndex], false).c_str());
				}
				else {
					strcpy(line, menu->text);
				}
			}
			// next line
			++y;
			break;
		case eMacroList:
			bMenuValid[menix] = true;
			// the list of macro files
			// min holds the macro number
			val = menu->min;
			//// see if the macro is there and append the text
			//exists = SD.exists("/" + String(val) + ".miw");
			//sprintf(line, menu->text, val, exists ? menu->on : menu->off);
			sprintf(line, menu->text, val, MacroInfo[val].description.c_str());
			// next line
			++y;
			break;
		case eList:
			bMenuValid[menix] = true;
			val = *(int*)menu->value;
			sprintf(line, menu->text, menu->nameList[val]);
			// next line
			++y;
			break;
		case eBool:
			bMenuValid[menix] = true;
			if (menu->value) {
				bool* pb = (bool*)menu->value;
				sprintf(line, menu->text, *pb ? menu->on : menu->off);
			}
			else {
				strcpy(line, menu->text);
			}
			// increment displayable lines
			++y;
			break;
		case eBuiltinOptions:
			// for builtins only show if available
			if (BuiltInFiles[currentFileIndex.nFileIndex].menu != NULL) {
				bMenuValid[menix] = true;
				sprintf(line, menu->text, BuiltInFiles[currentFileIndex.nFileIndex].text);
				++y;
			}
			break;
		case eMenu:
		case eExit:
		case eReboot:
			bMenuValid[menix] = true;
			if (menu->value) {
				// check for %d or %s in string, be lazy and assume %s if %d not there
				if (String(menu->text).indexOf("%d") != -1)
					sprintf(xtraline, menu->text, *(int*)menu->value);
				else
					sprintf(xtraline, menu->text, (char*)menu->value);
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
		default:
			break;
		}
		if (strlen(line) && y >= MenuStack.top()->offset) {
			DisplayMenuLine(y - 1, y - 1 - MenuStack.top()->offset, line);
		}
	}
	MenuStack.top()->menucount = y;
	// blank the rest of the lines
	for (int ix = y; ix < nMenuLineCount; ++ix) {
		DisplayLine(ix, "");
	}
	// show line if menu has been scrolled
	if (MenuStack.top()->offset > 0)
		tft.fillTriangle(0, 0, 2, 0, 0, tft.fontHeight() / 3, TFT_DARKGREY);
	//tft.drawLine(0, 0, 5, 0, menuLineActiveColor);TFT_DARKGREY
// show bottom line if last line is showing
	if (MenuStack.top()->offset + (nMenuLineCount - 1) < MenuStack.top()->menucount - 1) {
		int ypos = tft.height() - 2 - tft.fontHeight() / 3;
		tft.fillTriangle(0, ypos, 2, ypos, 0, ypos - tft.fontHeight() / 3, TFT_DARKGREY);
	}
	//if (MenuStack.top()->offset + (MENU_LINES - 1) < MenuStack.top()->menucount - 1)
	//	tft.drawLine(0, tft.height() - 1, 5, tft.height() - 1, menuLineActiveColor);
	//else
	//	tft.drawLine(0, tft.height() - 1, 5, tft.height() - 1, TFT_BLACK);
	// see if we need to clean up the end, like when the menu shrank due to a choice
	int extra = MenuStack.top()->menucount - MenuStack.top()->offset - nMenuLineCount;
	while (extra < 0) {
		DisplayLine(nMenuLineCount + extra, "");
		++extra;
	}
}

// switch between SD and built-ins
void ToggleFilesBuiltin(MenuItem* menu)
{
	// clear filenames list
	bool lastval = ImgInfo.bShowBuiltInTests;
	FILEINDEXINFO oldIndex = currentFileIndex;
	String oldFolder = currentFolder;
	if (menu != NULL) {
		ToggleBool(menu);
	}
	else {
		ImgInfo.bShowBuiltInTests = !ImgInfo.bShowBuiltInTests;
	}
	if (!ImgInfo.bShowBuiltInTests && !bSdCardValid) {
		// see if we can make it valid
		setupSDcard();
	}
	if (lastval != ImgInfo.bShowBuiltInTests) {
		currentFolder = lastFolder;
		GetFileNamesFromSDorBuiltins(currentFolder);
	}
	// restore indexes
	currentFileIndex = lastFileIndex;
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
		(*menu->change)(menu, 0);
	}
	ResetTextLines();
}

// choose from one of the values, update the index and wrap around if past max
void GetSelectChoice(MenuItem* menu)
{
	int* pVal = (int*)menu->value;
	++* pVal;
	*pVal %= menu->max + 1;
	if (menu->change != NULL) {
		(*menu->change)(menu, 0);
	}
	ResetTextLines();
}

void UpdateWiringMode(MenuItem* menu, int flag)
{
	bControllerReboot = true;
}

// get integer values
void GetIntegerValue(MenuItem* menu)
{
	GetIntegerValueHelper(menu, false);
}

// get integer values while showing HUE
void GetIntegerValueHue(MenuItem* menu)
{
	GetIntegerValueHelper(menu, true);
}

// get integer values and sometimes show other values like hue
void GetIntegerValueHelper(MenuItem* menu, bool bShowHue)
{
	ClearScreen();
	// -1 means to reset to original
	int stepSize = 1;
	int originalValue = *(int*)menu->value;
	//Serial.println("int: " + String(menu->text) + String(*(int*)menu->value));
	char line[50];
	CRotaryDialButton::Button button = BTN_NONE;
	bool done = false;
	const char* fmt = menu->decimals ? "%ld.%ld" : "%ld";
	char minstr[20], maxstr[20], valstr[20];
	sprintf(minstr, fmt, menu->min / (int)pow10(menu->decimals), menu->min % (int)pow10(menu->decimals));
	sprintf(maxstr, fmt, menu->max / (int)pow10(menu->decimals), menu->max % (int)pow10(menu->decimals));
	DisplayLine(1, String("Range: ") + String(minstr) + " to " + String(maxstr), SystemInfo.menuTextColor);
	DisplayLine(5, "Long Press B0 to reset", SystemInfo.menuTextColor);
	DisplayLine(6, "Long Press to Accept", SystemInfo.menuTextColor);
	int oldVal = *(int*)menu->value;
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
		case BTN_B0_CLICK:
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
		case BTN_B0_LONG:	// reset
			*(int*)menu->value = originalValue;
			stepSize = 1;
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
		tft.fillRect(0, 2 * tft.fontHeight(), tft.width() - 1, 6, TFT_BLACK);
		DrawProgressBar(0, 2 * tft.fontHeight() + 4, tft.width() - 1, 12, map(*(int*)menu->value, menu->min, menu->max, 0, 100), true);
		sprintf(line, menu->text, *(int*)menu->value / (int)pow10(menu->decimals), *(int*)menu->value % (int)pow10(menu->decimals));
		// get the hue value
		if (bShowHue) {
			CHSV hue = CHSV(*(int*)menu->value, 255, 255);
			CRGB rgb = CRGB(hue);
			DisplayLine(0, line, tft.color565(rgb.r, rgb.g, rgb.b));
		}
		else {
			DisplayLine(0, line, SystemInfo.menuTextColor);
		}
		sprintf(valstr, fmt, *(int*)menu->value / (int)pow10(menu->decimals), *(int*)menu->value % (int)pow10(menu->decimals));
		DisplayLine(3, String("Value: ") + valstr, SystemInfo.menuTextColor);
		sprintf(valstr, fmt, stepSize / (int)pow10(menu->decimals), stepSize % (int)pow10(menu->decimals));
		DisplayLine(4, stepSize == -1 ? "Reset: long press (Click +)" : "Step: " + String(valstr) + " (Click +)", SystemInfo.menuTextColor);
		if (menu->change != NULL && oldVal != *(int*)menu->value) {
			(*menu->change)(menu, 0);
			oldVal = *(int*)menu->value;
		}
		button = ReadButton();
	} while (!done);
}

//void UpdatePixelMaxCurrent(MenuItem* menu, int flag)
//{
//	FastLED.setMaxPowerInVoltsAndMilliamps(5, LedInfo.nPixelMaxCurrent);
//	switch (flag) {
//	case 1:		// first time
//		for (int ix = 0; ix < 64; ++ix) {
//			SetPixel(ix, CRGB::White);
//		}
//		FastLED.show();
//		break;
//	case 0:		// every change
//		FastLED.setBrightness(*(int*)menu->value);
//		FastLED.show();
//		break;
//	case -1:	// last time
//		FastLED.clear(true);
//		break;
//	}
//}

// called when brightness changed
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

// called when LED controller changes happen
void UpdateControllers(MenuItem* menu, int flag)
{
	static LED_INFO oldInfo;
	switch (flag) {
	case 1:		// first time
		oldInfo = LedInfo;
		break;
	case 0:		// every change
		if (oldInfo.bSecondController != LedInfo.bSecondController) {
			if (LedInfo.bSecondController)
				LedInfo.nTotalLeds *= 2;
			else
				LedInfo.nTotalLeds /= 2;
		}
		break;
	case -1:
		if (memcmp(&oldInfo, &LedInfo, sizeof(LedInfo)) != 0)
			bControllerReboot = true;
		break;
	}
}

void UpdateTotalLeds(MenuItem* menu, int flag)
{
	static int lastcount;
	switch (flag) {
	case 1:		// first time
		lastcount = LedInfo.nTotalLeds;
		break;
	case 0:		// every change
		break;
	case -1:	// last time, expand but don't shrink
		if (LedInfo.nTotalLeds != lastcount && !bControllerReboot) {
			//WriteMessage("Reboot needed\nto take effect", false, 1000);
			bControllerReboot = true;
		}
		break;
	}
}

void UpdateStripHue(MenuItem* menu, int flag)
{
	CHSV hue = CHSV(*(int*)menu->value, 255, 255);
	CRGB rgb = CRGB(hue);
	switch (flag) {
	case 1:		// first time
		for (int ix = 0; ix < 64; ++ix) {
			SetPixel(ix, rgb);
		}
		FastLED.show();
		break;
	case 0:		// every change
		FastLED.setTemperature(rgb);
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
		FastLED.setTemperature(CRGB(*(int*)menu->value, LedInfo.whiteBalance.g, LedInfo.whiteBalance.b));
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
		FastLED.setTemperature(CRGB(LedInfo.whiteBalance.r, *(int*)menu->value, LedInfo.whiteBalance.b));
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
		FastLED.setTemperature(CRGB(LedInfo.whiteBalance.r, LedInfo.whiteBalance.g, *(int*)menu->value));
		FastLED.show();
		break;
	case -1:	// last time
		FastLED.clear(true);
		break;
	}
}

// remember to clear the cursor spot or the display might act strange
void UpdateKeepOnTop(MenuItem* menu, int flag)
{
	currentFileIndex.nFileCursor = 0;
}

// update the batterie default settings, 1-4 batteries
void UpdateBatteries(MenuItem* menu, int flag)
{
	int batLo[4] = { 553, 1276, 1999, 2710 };
	int batHi[4] = { 809, 1790, 2763, 4094 };
	switch (flag) {
	case 1:		// first time
		break;
	case 0:		// every change
		break;
	case -1:	// last time, load the defaults
		SystemInfo.nBatteryEmptyLevel = batLo[SystemInfo.nBatteries - menu->min];
		SystemInfo.nBatteryFullLevel = batHi[SystemInfo.nBatteries - menu->min];
		break;
	}
}

void UpdateDisplayBrightness(MenuItem* menu, int flag)
{
	// control LCD brightness
	SetDisplayBrightness(*(int*)menu->value);
}

void UpdateDisplayRotation(MenuItem* menu, int flag)
{
	SetScreenRotation(SystemInfo.nDisplayRotation);
	tft.fillScreen(TFT_BLACK);
}

void UpdateDisplayDimMode(MenuItem* menu, int flag)
{
	switch (flag) {
	case 0:		// first time
		break;
	case 1:		// every change
		break;
	case -1:	// last time
		if (!SystemInfo.bHasLightSensor && SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_SENSOR) {
			SystemInfo.eDisplayDimMode = DISPLAY_DIM_MODE_NONE;
		}
		if (SystemInfo.eDisplayDimMode != DISPLAY_DIM_MODE_SENSOR) {
			SetDisplayBrightness(SystemInfo.nDisplayBrightness);
		}
	}
}

void SetDisplayBrightness(int val)
{
	ledcWrite(ledChannel, map(val, 0, 100, 0, 255));
}

uint16_t ColorList[] = {
	//TFT_NAVY,
	//TFT_MAROON,
	//TFT_OLIVE,
	TFT_WHITE,
	TFT_LIGHTGREY,
	TFT_BLUE,
	TFT_SKYBLUE,
	TFT_CYAN,
	TFT_RED,
	TFT_BROWN,
	TFT_GREEN,
	TFT_MAGENTA,
	TFT_YELLOW,
	TFT_ORANGE,
	TFT_GREENYELLOW,
	TFT_GOLD,
	TFT_SILVER,
	TFT_VIOLET,
	TFT_PURPLE,
};

// find the color in the list
int FindMenuColor(uint16_t col)
{
	int ix;
	int colors = sizeof(ColorList) / sizeof(*ColorList);
	for (ix = 0; ix < colors; ++ix) {
		if (col == ColorList[ix])
			break;
	}
	return constrain(ix, 0, colors - 1);
}

void SetMenuColor(MenuItem* menu)
{
	int maxIndex = sizeof(ColorList) / sizeof(*ColorList) - 1;
	int colorIndex = FindMenuColor(SystemInfo.menuTextColor);
	ClearScreen();
	DisplayLine(4, "Rotate change value", SystemInfo.menuTextColor);
	DisplayLine(5, "Long Press Exit", SystemInfo.menuTextColor);
	bool done = false;
	bool change = true;
	while (!done) {
		if (change) {
			DisplayLine(0, "Text Color", SystemInfo.menuTextColor);
			change = false;
		}
		switch (ReadButton()) {
		case CRotaryDialButton::BTN_LONGPRESS:
			done = true;
			break;
		case CRotaryDialButton::BTN_RIGHT:
			change = true;
			colorIndex = ++colorIndex;
			break;
		case CRotaryDialButton::BTN_LEFT:
			change = true;
			colorIndex = --colorIndex;
			break;
		}
		colorIndex = constrain(colorIndex, 0, maxIndex);
		SystemInfo.menuTextColor = ColorList[colorIndex];
	}
}

// go up one menu level, return true if anything done
// set gotoMain to go all the way back to the top
bool UpMenuLevel(bool gotoMain)
{
	if (gotoMain) {
		while (UpMenuLevel(false))
			;
	}
	else if (MenuStack.size() > 1) {
		bMenuChanged = true;
		menuPtr = MenuStack.top();
		MenuStack.pop();
		delete menuPtr;
		return true;
	}
	return false;
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
	//MenuItem* pCurrentMenu = &MenuStack.top()->menu[MenuStack.top()->index];
	int btnRepeatCount = 1;
	switch (button) {
	case BTN_LEFT_LONG:
	case BTN_RIGHT_LONG:
		btnRepeatCount = 5;
		break;
	}
	switch (button) {
	case BTN_B0_CLICK:	// go back a menu level if we can
		UpMenuLevel(false);
		break;
	case BTN_B0_LONG:	// look for help
		//UpMenuLevel(true);	// go back to the top menu
		RunMenus(button);
		bMenuChanged = true;
		break;
	case BTN_B1_LONG:
		button = BTN_SELECT;
		// yes, no break here
	case BTN_SELECT:
		RunMenus(button);
		bMenuChanged = true;
		break;
	case BTN_RIGHT_LONG:
	case BTN_RIGHT:
		while (btnRepeatCount--) {
			if (SystemInfo.bAllowMenuWrap || MenuStack.top()->index < MenuStack.top()->menucount - 1) {
				++MenuStack.top()->index;
			}
			if (MenuStack.top()->index >= MenuStack.top()->menucount) {
				MenuStack.top()->index = 0;
				bMenuChanged = true;
				MenuStack.top()->offset = 0;
			}
			// see if we need to scroll the menu
			if (MenuStack.top()->index - MenuStack.top()->offset > (nMenuLineCount - 1)) {
				if (MenuStack.top()->offset < MenuStack.top()->menucount - nMenuLineCount) {
					++MenuStack.top()->offset;
				}
			}
		}
		break;
	case BTN_LEFT_LONG:
	case BTN_LEFT:
		while (btnRepeatCount--) {
			if (SystemInfo.bAllowMenuWrap || MenuStack.top()->index > 0) {
				--MenuStack.top()->index;
			}
			if (MenuStack.top()->index < 0) {
				MenuStack.top()->index = MenuStack.top()->menucount - 1;
				bMenuChanged = true;
				MenuStack.top()->offset = MenuStack.top()->menucount - nMenuLineCount;
			}
			// see if we need to adjust the offset
			if (MenuStack.top()->offset && MenuStack.top()->index < MenuStack.top()->offset) {
				--MenuStack.top()->offset;
			}
		}
		break;
	case BTN_LONG:
		ClearScreen();
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
		// if recording off, save the new macro info
		if (bRecordingMacro) {
			// clear the new entry
			MacroInfo[ImgInfo.nCurrentMacro].description = "Used";
		}
		else {
			ClearScreen();
			WriteMessage("Saving macro #" + String(ImgInfo.nCurrentMacro), false, 0);
			// save timing and sizes
			int fileCount, pixelWidth;
			MacroInfo[ImgInfo.nCurrentMacro].mSeconds = MacroTime("/" + MakeMIWFilename(String(ImgInfo.nCurrentMacro), true), &fileCount, &pixelWidth, &MacroInfo[ImgInfo.nCurrentMacro].fileNames);
			MacroInfo[ImgInfo.nCurrentMacro].length = (float)pixelWidth / (float)LedInfo.nTotalLeds;
			MacroInfo[ImgInfo.nCurrentMacro].pixels = pixelWidth;
			SaveMacroInfo();
		}
		MenuStack.top()->index = 0;
		MenuStack.top()->offset = 0;
		bMenuChanged = true;
	}
	return didsomething;
}

// handle keys in run mode
bool HandleRunMode()
{
	bool bRedraw = false;
	bool didsomething = true;
	int maxMenuLine = nMenuLineCount - ((SystemInfo.bShowBatteryLevel || SystemInfo.bShowFilePosition) ? 2 : 1);
	CRotaryDialButton::Button button = ReadButton();
	int btnRepeatCount = 1;
	switch (button) {
	case BTN_LEFT_LONG:
	case BTN_RIGHT_LONG:
		btnRepeatCount = 5;
		break;
	}
	switch (button) {
	case BTN_SELECT:
		bCancelRun = bCancelMacro = false;
		ProcessFileOrBuiltin();
		DisplayCurrentFile();
		break;
	case BTN_RIGHT_LONG:
	case BTN_RIGHT:
		if (ImgInfo.bAutoColumnReset)
			ImgInfo.nStartCol = ImgInfo.nEndCol = 0;
		while (btnRepeatCount--) {
			if (!SystemInfo.bKeepFileOnTopLine && currentFileIndex.nFileCursor < maxMenuLine) {
				++currentFileIndex.nFileCursor;
				// pin to max
				currentFileIndex.nFileCursor = min(currentFileIndex.nFileCursor, (int)FileNames.size() - 1);
			}
			// increase the current file index
			if (SystemInfo.bAllowMenuWrap || (currentFileIndex.nFileIndex < FileNames.size() - 1))
				++currentFileIndex.nFileIndex;
			if (currentFileIndex.nFileIndex >= FileNames.size())
				currentFileIndex.nFileIndex = 0;
		}
		//if (oldFileIndex != CurrentFileIndex)
		DisplayCurrentFile();
		break;
	case BTN_LEFT_LONG:
	case BTN_LEFT:
		if (ImgInfo.bAutoColumnReset)
			ImgInfo.nStartCol = ImgInfo.nEndCol = 0;
		while (btnRepeatCount--) {
			if (!SystemInfo.bKeepFileOnTopLine && currentFileIndex.nFileCursor > 0) {
				--currentFileIndex.nFileCursor;
			}
			// decrease the current file index
			if (SystemInfo.bAllowMenuWrap || (currentFileIndex.nFileIndex > 0))
				--currentFileIndex.nFileIndex;
			if (currentFileIndex.nFileIndex < 0)
				currentFileIndex.nFileIndex = FileNames.size() - 1;
		}
		//if (oldFileIndex != CurrentFileIndex)
		DisplayCurrentFile();
		break;
		//case btnShowFiles:
		//	bShowBuiltInTests = !bShowBuiltInTests;
		//	GetFileNamesFromSDorBuiltins(currentFolder);
		//	DisplayCurrentFile();
		//	break;
	case BTN_LONG:
		ClearScreen();
		bSettingsMode = true;
		break;
	case BTN_B0_CLICK:
		if (IsFolder(currentFileIndex.nFileIndex)) {
			CRotaryDialButton::pushButton(BTN_SELECT);
		}
		else {
			ShowBmp(NULL);
			bRedraw = true;
		}
		break;
	case BTN_B0_LONG:
		CallBtnLongFunction(SystemInfo.nBtn0LongFunction);
		bRedraw = true;
		break;
	case BTN_B1_CLICK:
		break;
	case BTN_B1_LONG:
	case BTN_LEFT_RIGHT_LONG:
		CallBtnLongFunction(SystemInfo.nBtn1LongFunction);
		bRedraw = true;
		break;
	default:
		didsomething = false;
		break;
	}
	if (bRedraw) {
		ClearScreen();
		DisplayCurrentFile(SystemInfo.bShowFolder);
	}
	return didsomething;
}

// check buttons and return if one pressed
// check the rotation buttons during running
enum CRotaryDialButton::Button ReadButton()
{
	if (SystemInfo.bRunWebServer) {
		server.handleClient();
	}
	MenuTextScrollSideways();
	enum CRotaryDialButton::Button retValue = BTN_NONE;
	// read the next button, or NONE if none there
	retValue = CRotaryDialButton::dequeue();
	// reboot?
	if (retValue == BTN_B2_LONG)
		ESP.restart();
	//// turn the b1 button into a dial long click
	//if (retValue == BTN_B1_CLICK)
	//	retValue = BTN_LONG;
	if (retValue != BTN_NONE) {
		ResetSleepAndDimTimers();
	}
	else if (displayDimNow) {
		for (int val = SystemInfo.nDisplayBrightness - 1; val >= SystemInfo.nDisplayDimValue; --val) {
			SetDisplayBrightness(val);
			if (CRotaryDialButton::getCount()) {
				// if button pressed finish and get out of here
				SetDisplayBrightness(SystemInfo.nDisplayBrightness);
				break;
			}
			delay(10);
		}
		displayDimNow = false;
	}
	if (bIsRunning) {
		int amt = 0, newval = 0;
		switch (retValue) {
		case BTN_LEFT:
			amt = -ImgInfo.nDialDuringImgInc;
			break;
		case BTN_RIGHT:
			amt = ImgInfo.nDialDuringImgInc;
			break;
		}
		switch (ImgInfo.nDialDuringImgAction) {
		case DIAL_IMG_NONE:
			break;
		case DIAL_IMG_BRIGHTNESS:
			LedInfo.nLEDBrightness += amt;
			LedInfo.nLEDBrightness = constrain(LedInfo.nLEDBrightness, 1, 255);
			FastLED.setBrightness(LedInfo.nLEDBrightness);
			DisplayLine(nMenuLineCount - 1, "Brightness: " + String(LedInfo.nLEDBrightness), SystemInfo.menuTextColor);
			break;
		case DIAL_IMB_SPEED:
			ImgInfo.nFrameHold += amt;
			ImgInfo.nFrameHold = constrain(ImgInfo.nFrameHold, 0, 500);
			DisplayLine(nMenuLineCount - 1, "Frame Time: " + String(ImgInfo.nFrameHold) + " mS", SystemInfo.menuTextColor);
			break;
		}
	}
	return retValue;
}

// just check for longpress and cancel if it was there
bool CheckCancel(bool bLeaveButton)
{
	ResetSleepAndDimTimers();
	// if it has been set, just return true
	if (bCancelRun || bCancelMacro)
		return true;
	CRotaryDialButton::Button button = ReadButton();
	if (button) {
		if (button == BTN_LONG) {
			bCancelMacro = bCancelRun = true;
			return true;
		}
		else if (bLeaveButton) {
			CRotaryDialButton::pushButton(button);
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
}

// return the pixel
CRGB getRGBwithGamma() {
	if (LedInfo.bGammaCorrection) {
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
	if (LedInfo.bGammaCorrection) {
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

	BouncingColoredBalls(BuiltinInfo.nBouncingBallsCount, colors);
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

	long percent = 0;
	int colorChangeCounter = 0;
	bool done = false;
	while (!done) {
		if (CheckCancel()) {
			done = true;
			break;
		}
		for (int i = 0; i < balls; i++) {
			if (CheckCancel()) {
				done = true;
				break;
			}
			TimeSinceLastBounce[i] = millis() - ClockTimeSinceLastBounce[i];
			Height[i] = 0.5 * Gravity * pow(TimeSinceLastBounce[i] / BuiltinInfo.nBouncingBallsDecay, 2.0) + ImpactVelocity[i] * TimeSinceLastBounce[i] / BuiltinInfo.nBouncingBallsDecay;

			if (Height[i] < 0) {
				Height[i] = 0;
				ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
				ClockTimeSinceLastBounce[i] = millis();

				if (ImpactVelocity[i] < 0.01) {
					ImpactVelocity[i] = ImpactVelocityStart;
				}
			}
			Position[i] = round(Height[i] * (LedInfo.nTotalLeds - 1) / StartHeight);
		}

		for (int i = 0; i < balls; i++) {
			int ix;
			if (CheckCancel()) {
				done = true;
				break;
			}
			ix = (i + BuiltinInfo.nBouncingBallsFirstColor) % 32;
			SetPixel(Position[i], colors[ix]);
		}
		if (BuiltinInfo.nBouncingBallsChangeColors && colorChangeCounter++ > (BuiltinInfo.nBouncingBallsChangeColors * 100)) {
			++BuiltinInfo.nBouncingBallsFirstColor;
			colorChangeCounter = 0;
		}
		ShowLeds();
		delayMicroseconds(50);
		FastLED.clear();
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
	CRGB red, white, blue;
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
	bool done = false;
	for (int loop = 0; !done; ++loop) {
		if (CheckCancel()) {
			done = true;
			break;
		}
		for (int ledIx = 0; ledIx < LedInfo.nTotalLeds; ++ledIx) {
			if (CheckCancel()) {
				done = true;
				break;
			}
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
		ShowLeds();
		delay(ImgInfo.nFrameHold);
	}
}

// checkerboard
void CheckerBoard()
{
	int width = BuiltinInfo.nCheckboardBlackWidth + BuiltinInfo.nCheckboardWhiteWidth;
	int times = 0;
	CRGB color1 = CRGB::Black, color2 = CRGB::White;
	int addPixels = 0;
	bool done = false;
	while (!done) {
		for (int y = 0; y < LedInfo.nTotalLeds; ++y) {
			SetPixel(y, ((y + addPixels) % width) < BuiltinInfo.nCheckboardBlackWidth ? color1 : color2);
		}
		ShowLeds();
		int count = BuiltinInfo.nCheckerboardHoldframes;
		while (count-- > 0) {
			delay(ImgInfo.nFrameHold);
			if (CheckCancel()) {
				done = true;
				break;
			}
		}
		if (BuiltinInfo.bCheckerBoardAlternate && (times++ % 2)) {
			// swap colors
			CRGB temp = color1;
			color1 = color2;
			color2 = temp;
		}
		addPixels += BuiltinInfo.nCheckerboardAddPixels;
		if (CheckCancel()) {
			done = true;
			break;
		}
	}
}

void RandomBars()
{
	ShowRandomBars(BuiltinInfo.bRandomBarsBlacks);
}

// show random bars of lights with optional blacks between
void ShowRandomBars(bool blacks)
{
	time_t start = time(NULL);
	byte r, g, b;
	srand(millis());
	char line[40]{};
	bool done = false;
	for (int pass = 0; !done; ++pass) {
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
		int count = BuiltinInfo.nRandomBarsHoldframes;
		while (count-- > 0) {
			delay(ImgInfo.nFrameHold);
			if (CheckCancel()) {
				done = true;
				break;
			}
		}
	}
}

// running dot
void RunningDot()
{
	for (int colorvalue = 0; colorvalue <= 3; ++colorvalue) {
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
		for (int ix = 0; ix < LedInfo.nTotalLeds; ++ix) {
			if (CheckCancel()) {
				break;
			}
			if (ix > 0) {
				SetPixel(ix - 1, CRGB::Black);
			}
			SetPixel(ix, CRGB(r, g, b));
			ShowLeds();
			delay(max(1, ImgInfo.nFrameHold));
		}
		// remember the last one, turn it off
		SetPixel(LedInfo.nTotalLeds - 1, CRGB::Black);
		ShowLeds();
	}
	FastLED.clear(true);
	delay(2);
}

// random dot, moves back and forth with random timing
void RandomDot()
{
	// RGBW
	byte r, g, b;
	r = 255;
	g = 0;
	b = 0;
	long delayTime = 0;
	fixRGBwithGamma(&r, &g, &b);
	for (int ix = 0; ix < LedInfo.nTotalLeds; ++ix) {
		if (CheckCancel()) {
			break;
		}
		if (ix > 0) {
			SetPixel(ix - 1, CRGB::Black);
		}
		SetPixel(ix, CRGB(r, g, b));
		ShowLeds();
		if ((ix % 10) == 0)
			delayTime = random(0, ImgInfo.nFrameHold) * 2;
		delay(delayTime);
	}
	// remember the last one, turn it off
	SetPixel(LedInfo.nTotalLeds - 1, CRGB::Black);
	ShowLeds();
	FastLED.clear(true);
	delay(2);
}

void OppositeRunningDots()
{
	for (int mode = 0; mode <= 3; ++mode) {
		if (CheckCancel())
			break;;
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
		for (int ix = 0; ix < LedInfo.nTotalLeds; ++ix) {
			if (CheckCancel())
				return;
			if (ix > 0) {
				SetPixel(ix - 1, CRGB::Black);
				SetPixel(LedInfo.nTotalLeds - ix + 1, CRGB::Black);
			}
			SetPixel(LedInfo.nTotalLeds - ix, CRGB(r, g, b));
			SetPixel(ix, CRGB(r, g, b));
			ShowLeds();
			delay(max(1, ImgInfo.nFrameHold));
		}
	}
}

void Sleep(MenuItem* menu)
{
	esp_timer_stop(periodic_Second_timer);
	++nBootCount;
	//rtc_gpio_pullup_en(BTNPUSH);
	// save the current folder
	memset(sleepFolder, '\0', sizeof(sleepFolder));
	strncpy(sleepFolder, currentFolder.c_str(), sizeof(sleepFolder) - 1);
	esp_sleep_enable_ext0_wakeup((gpio_num_t)DIAL_BTN, LOW);
	esp_deep_sleep_start();
}

void LightBar(MenuItem* menu)
{
	ClearScreen();
	DisplayLightBarTitle(false);
	DisplayLine(4, "B0=+Inc, B0Long=+Delay, B1Long=-Delay", SystemInfo.menuTextColor);
	DisplayLine(5, "Click/Rotate Dial", SystemInfo.menuTextColor);
	DisplayLine(6, "Long Press Exit", SystemInfo.menuTextColor);
	DisplayLedLightBar();
	FastLED.clear(true);
	// these were set by CheckCancel() in DisplayAllColor() and need to be cleared
	bCancelMacro = bCancelRun = false;
}

// display the top line and the delay and inc settings of the iightbar
void DisplayLightBarTitle(bool bRun)
{
	DisplayLine(0, String(LightBarModeText[BuiltinInfo.nLightBarMode]) + " Light Bar" + (bRun ? " - Running" : ""), SystemInfo.menuTextColor);
	DisplayLine(3, "Delay: " + String(BuiltinInfo.nDisplayAllChangeTime) + " mS" + "  Inc: " + String(BuiltinInfo.nDisplayAllIncrement), SystemInfo.menuTextColor);
}

// utility for DisplayLedLightBar()
void FillLightBar()
{
	int offset = BuiltinInfo.bDisplayAllFromMiddle ? (LedInfo.nTotalLeds - BuiltinInfo.nDisplayAllPixelCount) / 2 : 0;
	if (!BuiltinInfo.bDisplayAllFromMiddle && ImgInfo.bUpsideDown)
		offset = LedInfo.nTotalLeds - BuiltinInfo.nDisplayAllPixelCount;
	FastLED.clear();
	CRGB color;
	for (int ix = 0; ix < BuiltinInfo.nDisplayAllPixelCount; ++ix) {
		switch (BuiltinInfo.nLightBarMode) {
		case LBMODE_HSV:
			SetPixel(ix + offset, CHSV(BuiltinInfo.nDisplayAllHue, BuiltinInfo.nDisplayAllSaturation, BuiltinInfo.nDisplayAllBrightness));
			break;
		case LBMODE_RGB:
			SetPixel(ix + offset, CRGB(BuiltinInfo.nDisplayAllRed, BuiltinInfo.nDisplayAllGreen, BuiltinInfo.nDisplayAllBlue));
			break;
		case LBMODE_KELVIN:
			color = CRGB((ColorTemperature)LightBarColorList[BuiltinInfo.nColorTemperature]);
			color.nscale8(BuiltinInfo.nDisplayAllBrightness);
			SetPixel(ix + offset, color);
			break;
		}
	}
	ShowLeds();
}

// Used LEDs as a light bar
void DisplayLedLightBar()
{
	unsigned int incIx = 1;
	int incList[] = { 1,10,100,256,256 };
	if (LedInfo.nTotalLeds > 256) {
		incList[4] = LedInfo.nTotalLeds;
	}
	else {
		incList[3] = LedInfo.nTotalLeds;
	}
	// these are used to handle the slow transition mode
	// it works by saving the increment, setting it to 1, and then saving the number in count
	CRotaryDialButton::Button btnSlowChange = BTN_NONE;
	int savedIncrement = 0;
	int nSlowChangeCount = 0;

	FillLightBar();
	// show until cancelled, but check for rotations of the knob
	CRotaryDialButton::Button btn;
	enum DAWHAT { DAWHAT_HRK = 0, DAWHAT_SGK, DAWHAT_BBB, DAWHAT_PIXELS, DAWHAT_FROM, DAWHAT_MAX };
	enum DAWHAT what = DAWHAT_HRK;	// 0 for hue, 1 for saturation, 2 for brightness, 3 for pixels, 4 for Kelvin
	BuiltinInfo.nDisplayAllIncrement = incList[incIx];
	bool bChange = true;

	while (true) {
		if (bChange) {
			String line;
			switch (what) {
			case DAWHAT_HRK:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					line = "HUE: " + String(BuiltinInfo.nDisplayAllHue);
					break;
				case LBMODE_RGB:
					line = "Red: " + String(BuiltinInfo.nDisplayAllRed);
					break;
				case LBMODE_KELVIN:
					line = /*"Temp: " +*/ String(LightBarColorKelvinText[BuiltinInfo.nColorTemperature]);
					break;
				}
				break;
			case DAWHAT_SGK:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					line = "Saturation: " + String(BuiltinInfo.nDisplayAllSaturation);
					break;
				case LBMODE_RGB:
					line = "Green: " + String(BuiltinInfo.nDisplayAllGreen);
					break;
				case LBMODE_KELVIN:
					break;
				}
				break;
			case DAWHAT_BBB:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					line = "Brightness: " + String(BuiltinInfo.nDisplayAllBrightness);
					break;
				case LBMODE_RGB:
					line = "Blue: " + String(BuiltinInfo.nDisplayAllBlue);
					break;
				case LBMODE_KELVIN:
					line = "Brightness: " + String(BuiltinInfo.nDisplayAllBrightness);
					break;
				}
				break;
			case DAWHAT_PIXELS:
				line = "Pixels: " + String(BuiltinInfo.nDisplayAllPixelCount);
				break;
			case DAWHAT_FROM:
				line = "From: " + String((BuiltinInfo.bDisplayAllFromMiddle ? "Middle" : "End"));
				break;
			case DAWHAT_MAX:	// keep the compiler happy
				break;
			}
			DisplayLine(1, line, SystemInfo.menuTextColor);
		}
		// see if there is a slow change going on
		if (nSlowChangeCount > 0) {
			DisplayLightBarTitle(true);
			delay(BuiltinInfo.nDisplayAllChangeTime);
			if (--nSlowChangeCount) {
				// push the button, we're still running
				CRotaryDialButton::pushButton(btnSlowChange);
			}
			else {
				// we're all done now
				BuiltinInfo.nDisplayAllIncrement = savedIncrement;
				DisplayLightBarTitle(false);
			}
		}
		btn = ReadButton();
		bChange = true;
		// first check for the slow mode cases
		switch (btn) {
		case BTN_RIGHT:
		case BTN_LEFT:
			// now check for HSV, so far the only ones allowed to do slow transitions
			switch (BuiltinInfo.nLightBarMode) {
			case LBMODE_HSV:
				if (nSlowChangeCount == 0 && BuiltinInfo.nDisplayAllChangeTime) {
					btnSlowChange = btn;
					savedIncrement = BuiltinInfo.nDisplayAllIncrement;
					// set how many to do, add one since we're starting over
					nSlowChangeCount = BuiltinInfo.nDisplayAllIncrement + 1;
					// only do one at a time during slow transition
					BuiltinInfo.nDisplayAllIncrement = 1;
					// go back and handle this new key
					continue;
				}
				break;
			}
			break;
		}
		switch (btn) {
			//**** B2 long is reserved for reboot
			//case BTN_B2_LONG:
			//	// next mode
			//	++BuiltinInfo.nLightBarMode;
			//	// wrap
			//	BuiltinInfo.nLightBarMode %= sizeof(LightBarModeText) / sizeof(*LightBarModeText);
			//	DisplayLightBarTitle(false);
			//	break;
		case BTN_B0_CLICK:	// change the inc
			++incIx;
			incIx %= sizeof(incList) / sizeof(*incList);
			BuiltinInfo.nDisplayAllIncrement = incList[incIx];
			break;
		case BTN_B0_LONG:	// increment delay
			BuiltinInfo.nDisplayAllChangeTime += BuiltinInfo.nDisplayAllIncrement;
			BuiltinInfo.nDisplayAllChangeTime = constrain(BuiltinInfo.nDisplayAllChangeTime, 0, 1000);
			break;
		case BTN_B1_LONG:	// decrement delay
			//ShowMenu(LedLightBarMenu);
			BuiltinInfo.nDisplayAllChangeTime -= BuiltinInfo.nDisplayAllIncrement;
			BuiltinInfo.nDisplayAllChangeTime = constrain(BuiltinInfo.nDisplayAllChangeTime, 0, 1000);
			break;
		case BTN_NONE:
			bChange = false;
			break;
		case BTN_RIGHT:
			switch (what) {
			case DAWHAT_HRK:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					BuiltinInfo.nDisplayAllHue += BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_RGB:
					BuiltinInfo.nDisplayAllRed += BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_KELVIN:
					++BuiltinInfo.nColorTemperature;
					break;
				}
				break;
			case DAWHAT_SGK:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					BuiltinInfo.nDisplayAllSaturation += BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_RGB:
					BuiltinInfo.nDisplayAllGreen += BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_KELVIN:
					break;
				}
				break;
			case DAWHAT_BBB:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					BuiltinInfo.nDisplayAllBrightness += BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_RGB:
					BuiltinInfo.nDisplayAllBlue += BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_KELVIN:
					BuiltinInfo.nDisplayAllBrightness += BuiltinInfo.nDisplayAllIncrement;
					break;
				}
				break;
			case DAWHAT_PIXELS:
				BuiltinInfo.nDisplayAllPixelCount += BuiltinInfo.nDisplayAllIncrement;
				if (BuiltinInfo.bAllowRollover && BuiltinInfo.nDisplayAllPixelCount > LedInfo.nTotalLeds) {
					BuiltinInfo.nDisplayAllPixelCount = 0;
				}
				break;
			case DAWHAT_FROM:
				BuiltinInfo.bDisplayAllFromMiddle = true;
				break;
			}
			break;
		case BTN_LEFT:
			switch (what) {
			case DAWHAT_HRK:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					BuiltinInfo.nDisplayAllHue -= BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_RGB:
					BuiltinInfo.nDisplayAllRed -= BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_KELVIN:
					--BuiltinInfo.nColorTemperature;
					break;
				}
				break;
			case DAWHAT_SGK:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					BuiltinInfo.nDisplayAllSaturation -= BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_RGB:
					BuiltinInfo.nDisplayAllGreen -= BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_KELVIN:
					break;
				}
				break;
			case DAWHAT_BBB:
				switch (BuiltinInfo.nLightBarMode) {
				case LBMODE_HSV:
					BuiltinInfo.nDisplayAllBrightness -= BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_RGB:
					BuiltinInfo.nDisplayAllBlue -= BuiltinInfo.nDisplayAllIncrement;
					break;
				case LBMODE_KELVIN:
					BuiltinInfo.nDisplayAllBrightness -= BuiltinInfo.nDisplayAllIncrement;
					break;
				}
				break;
			case DAWHAT_PIXELS:
				BuiltinInfo.nDisplayAllPixelCount -= BuiltinInfo.nDisplayAllIncrement;
				if (BuiltinInfo.bAllowRollover && BuiltinInfo.nDisplayAllPixelCount <= 0) {
					BuiltinInfo.nDisplayAllPixelCount = LedInfo.nTotalLeds;
				}
				break;
			case DAWHAT_FROM:
				BuiltinInfo.bDisplayAllFromMiddle = false;
				break;
			case DAWHAT_MAX:	// keep the compiler happy
				break;
			}
			break;
		case BTN_SELECT:
			// switch to the next selection, wrapping around if necessary
			what = (enum DAWHAT)((what + 1) % (DAWHAT_MAX - 1));
			// DAWHAT_SGK is not valid with Kelvin
			if (BuiltinInfo.nLightBarMode == LBMODE_KELVIN && what == DAWHAT_SGK) {
				what = DAWHAT_BBB;
			}
			break;
		case BTN_LONG:
			// put it back, we don't want it
			CRotaryDialButton::pushButton(btn);
			break;
		}
		if (CheckCancel(true))
			return;
		if (bChange) {
			DisplayLightBarTitle(nSlowChangeCount > 0);
			BuiltinInfo.nDisplayAllPixelCount = constrain(BuiltinInfo.nDisplayAllPixelCount, 1, LedInfo.nTotalLeds);
			//increment = constrain(increment, 1, 256);
			switch (BuiltinInfo.nLightBarMode) {
			case LBMODE_HSV:
				if (BuiltinInfo.bAllowRollover) {
					if (BuiltinInfo.nDisplayAllHue < 0)
						BuiltinInfo.nDisplayAllHue = RollDownRollOver(BuiltinInfo.nDisplayAllIncrement);
					if (BuiltinInfo.nDisplayAllHue > 255)
						BuiltinInfo.nDisplayAllHue = 0;
					if (BuiltinInfo.nDisplayAllSaturation < 0)
						BuiltinInfo.nDisplayAllSaturation = RollDownRollOver(BuiltinInfo.nDisplayAllIncrement);
					if (BuiltinInfo.nDisplayAllSaturation > 255)
						BuiltinInfo.nDisplayAllSaturation = 0;
				}
				else {
					BuiltinInfo.nDisplayAllHue = constrain(BuiltinInfo.nDisplayAllHue, 0, 255);
					BuiltinInfo.nDisplayAllSaturation = constrain(BuiltinInfo.nDisplayAllSaturation, 0, 255);
				}
				BuiltinInfo.nDisplayAllBrightness = constrain(BuiltinInfo.nDisplayAllBrightness, 0, 255);
				FillLightBar();
				break;
			case LBMODE_RGB:
				if (BuiltinInfo.bAllowRollover) {
					if (BuiltinInfo.nDisplayAllRed < 0)
						BuiltinInfo.nDisplayAllRed = RollDownRollOver(BuiltinInfo.nDisplayAllIncrement);
					if (BuiltinInfo.nDisplayAllRed > 255)
						BuiltinInfo.nDisplayAllRed = 0;
					if (BuiltinInfo.nDisplayAllGreen < 0)
						BuiltinInfo.nDisplayAllGreen = RollDownRollOver(BuiltinInfo.nDisplayAllIncrement);
					if (BuiltinInfo.nDisplayAllGreen > 255)
						BuiltinInfo.nDisplayAllGreen = 0;
					if (BuiltinInfo.nDisplayAllBlue < 0)
						BuiltinInfo.nDisplayAllBlue = RollDownRollOver(BuiltinInfo.nDisplayAllIncrement);
					if (BuiltinInfo.nDisplayAllBlue > 255)
						BuiltinInfo.nDisplayAllBlue = 0;
				}
				else {
					BuiltinInfo.nDisplayAllRed = constrain(BuiltinInfo.nDisplayAllRed, 0, 255);
					BuiltinInfo.nDisplayAllGreen = constrain(BuiltinInfo.nDisplayAllGreen, 0, 255);
					BuiltinInfo.nDisplayAllBlue = constrain(BuiltinInfo.nDisplayAllBlue, 0, 255);
				}
				FillLightBar();
				break;
			case LBMODE_KELVIN:
				BuiltinInfo.nColorTemperature %= sizeof(LightBarColorList) / sizeof(*LightBarColorList);
				BuiltinInfo.nDisplayAllBrightness = constrain(BuiltinInfo.nDisplayAllBrightness, 0, 255);
				FillLightBar();
				break;
			}
		}
		delay(10);
	}
}

// handle rollover when -ve
// inc 1 gives 255, inc 10 gives 250, inc 100 gives 200
int RollDownRollOver(int inc)
{
	if (inc == 1)
		return 255;
	int retval = 256;
	retval -= retval % inc;
	return retval;
}

void TestTwinkle() {
	TwinkleRandom(max(5, ImgInfo.nFrameHold), BuiltinInfo.bTwinkleOnlyOne);
}
void TwinkleRandom(int SpeedDelay, boolean OnlyOne) {
	time_t start = time(NULL);
	bool done = false;
	while (!done) {
		SetPixel(random(LedInfo.nTotalLeds - 1), CRGB(random(0, 255), random(0, 255), random(0, 255)));
		ShowLeds();
		delay(SpeedDelay);
		if (OnlyOne) {
			FastLED.clear(true);
		}
		if (CheckCancel()) {
			done = true;
			break;
		}
	}
}

void TestCylon()
{
	CylonBounce(BuiltinInfo.nCylonEyeRed, BuiltinInfo.nCylonEyeGreen, BuiltinInfo.nCylonEyeBlue, BuiltinInfo.nCylonEyeSize, max(5, ImgInfo.nFrameHold), 50);
}
void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
{
	for (int i = 0; i < LedInfo.nTotalLeds - EyeSize - 2; i++) {
		if (CheckCancel()) {
			break;
		}
		FastLED.clear();
		SetPixel(i, CRGB(red / 10, green / 10, blue / 10));
		for (int j = 1; j <= EyeSize; j++) {
			SetPixel(i + j, CRGB(red, green, blue));
		}
		SetPixel(i + EyeSize + 1, CRGB(red / 10, green / 10, blue / 10));
		ShowLeds();
		delay(SpeedDelay);
	}
	delay(ReturnDelay);
	for (int i = LedInfo.nTotalLeds - EyeSize - 2; i > 0; i--) {
		if (CheckCancel()) {
			break;
		}
		FastLED.clear();
		SetPixel(i, CRGB(red / 10, green / 10, blue / 10));
		for (int j = 1; j <= EyeSize; j++) {
			SetPixel(i + j, CRGB(red, green, blue));
		}
		SetPixel(i + EyeSize + 1, CRGB(red / 10, green / 10, blue / 10));
		ShowLeds();
		delay(SpeedDelay);
	}
	FastLED.clear(true);
}

void TestMeteor() {
	meteorRain(BuiltinInfo.nMeteorRed, BuiltinInfo.nMeteorGreen, BuiltinInfo.nMeteorBlue, BuiltinInfo.nMeteorSize, 64, true, 30);
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay)
{
	FastLED.clear(true);

	for (int i = 0; i < LedInfo.nTotalLeds + LedInfo.nTotalLeds; i++) {
		if (CheckCancel())
			break;;
		// fade brightness all LEDs one step
		for (int j = 0; j < LedInfo.nTotalLeds; j++) {
			if (CheckCancel())
				break;
			if ((!meteorRandomDecay) || (random(10) > 5)) {
				fadeToBlack(j, meteorTrailDecay);
			}
		}
		// draw meteor
		for (int j = 0; j < meteorSize; j++) {
			if (CheckCancel())
				break;
			if ((i - j < LedInfo.nTotalLeds) && (i - j >= 0)) {
				SetPixel(i - j, CRGB(red, green, blue));
			}
		}
		ShowLeds();
		delay(SpeedDelay);
	}
}

void TestConfetti()
{
	time_t start = time(NULL);
	BuiltinInfo.gHue = 0;
	bool done = false;
	while (!done) {
		EVERY_N_MILLISECONDS_I(timerObj, 0) {
			timerObj.setPeriod(max(5, ImgInfo.nFrameHold));
			if (BuiltinInfo.bConfettiCycleHue)
				++BuiltinInfo.gHue;
			confetti();
			ShowLeds();
		}
		if (CheckCancel()) {
			done = true;
			break;
		}
	}
	// wait for timeout so strip will be blank
	delay(100);
}

void confetti()
{
	// random colored speckles that blink in and fade smoothly
	fadeToBlackBy(leds, LedInfo.nTotalLeds, 10);
	int pos = random16(LedInfo.nTotalLeds - 1);
	leds[pos] += CHSV(BuiltinInfo.gHue + random8(64), 200, 255);
}

void TestJuggle()
{
	bool done = false;
	while (!done) {
		EVERY_N_MILLISECONDS_I(timerObj, 0) {
			timerObj.setPeriod(max(5, ImgInfo.nFrameHold));
			juggle();
			ShowLeds();
		}
		if (CheckCancel()) {
			done = true;
			break;
		}
	}
}

void juggle()
{
	// eight colored dots, weaving in and out of sync with each other
	fadeToBlackBy(leds, LedInfo.nTotalLeds, 20);
	byte dothue = 0;
	uint16_t index;
	for (int i = 0; i < 8; i++) {
		index = beatsin16(i + 7, 0, LedInfo.nTotalLeds - 1);
		// use AdjustStripIndex to get the right one
		SetPixel(index, leds[AdjustStripIndex(index)] | CHSV(dothue, 255, 255));
		//leds[beatsin16(i + 7, 0, STRIPLENGTH)] |= CHSV(dothue, 255, 255);
		dothue += 32;
	}
}

void TestSine()
{
	BuiltinInfo.gHue = BuiltinInfo.nSineStartingHue;
	bool done = false;
	while (!done) {
		EVERY_N_MILLISECONDS_I(timerObj, 0) {
			timerObj.setPeriod(max(5, ImgInfo.nFrameHold));
			sinelon();
			ShowLeds();
		}
		if (CheckCancel()) {
			done = true;
			break;
		}
	}
}
void sinelon()
{
	// a colored dot sweeping back and forth, with fading trails
	fadeToBlackBy(leds, LedInfo.nTotalLeds, 20);
	int pos = beatsin16(BuiltinInfo.nSineSpeed, 0, LedInfo.nTotalLeds - 1);
	leds[AdjustStripIndex(pos)] += CHSV(BuiltinInfo.gHue, 255, 192);
	if (BuiltinInfo.bSineCycleHue)
		++BuiltinInfo.gHue;
}

void TestBpm()
{
	BuiltinInfo.gHue = 0;
	bool done = false;
	while (!done) {
		EVERY_N_MILLISECONDS_I(timerObj, 0) {
			timerObj.setPeriod(ImgInfo.nFrameHold);
			bpm();
			ShowLeds();
		}
		if (CheckCancel()) {
			done = true;
			break;
		}
	}
	FastLED.clear(true);
}

void bpm()
{
	// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(BuiltinInfo.nBpmBeatsPerMinute, 64, 255);
	for (int i = 0; i < LedInfo.nTotalLeds; i++) { //9948
		SetPixel(i, ColorFromPalette(palette, BuiltinInfo.gHue + (i * 2), beat - BuiltinInfo.gHue + (i * 10)));
	}
	if (BuiltinInfo.bBpmCycleHue)
		++BuiltinInfo.gHue;
}

void FillRainbow(struct CRGB* pFirstLED, int numToFill, uint8_t initialhue, int deltahue)
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
	BuiltinInfo.gHue = BuiltinInfo.nRainbowInitialHue;
	FillRainbow(leds, LedInfo.nTotalLeds, BuiltinInfo.gHue, BuiltinInfo.nRainbowHueDelta);
	FadeInOut(BuiltinInfo.nRainbowFadeTime * 100, true);
	bool done = false;
	while (!done) {
		EVERY_N_MILLISECONDS_I(timerObj, 0) {
			timerObj.setPeriod(ImgInfo.nFrameHold);
			if (BuiltinInfo.bRainbowCycleHue)
				++BuiltinInfo.gHue;
			FillRainbow(leds, LedInfo.nTotalLeds, BuiltinInfo.gHue, BuiltinInfo.nRainbowHueDelta);
			if (BuiltinInfo.bRainbowAddGlitter)
				addGlitter(80);
			ShowLeds();
		}
		if (CheckCancel()) {
			done = true;
			FastLED.setBrightness(LedInfo.nLEDBrightness);
			break;
		}
	}
	FadeInOut(BuiltinInfo.nRainbowFadeTime * 100, false);
	FastLED.setBrightness(LedInfo.nLEDBrightness);
}

// create a user defined stripe set
// it consists of a list of stripes, each of which have a width and color
// there can be up to 10 of these
constexpr int NUM_STRIPES = 20;
struct {
	int start;
	int length;
	CHSV color;
} Stripes[NUM_STRIPES];

void TestStripes()
{
	FastLED.setBrightness(LedInfo.nLEDBrightness);
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
	ShowLeds();
	bool done = false;
	while (!done) {
		if (CheckCancel()) {
			done = true;
			break;
		}
		delay(1000);
	}
}

// alternating white and black lines
void TestLines()
{
	FastLED.setBrightness(LedInfo.nLEDBrightness);
	FastLED.clear(true);
	bool bWhite = true;
	for (int pix = 0; pix < LedInfo.nTotalLeds; ++pix) {
		// fill in each block of pixels
		for (int len = 0; len < (bWhite ? BuiltinInfo.nLinesWhite : BuiltinInfo.nLinesBlack); ++len) {
			SetPixel(pix++, bWhite ? CRGB::White : CRGB::Black);
		}
		bWhite = !bWhite;
	}
	ShowLeds();
	bool done = false;
	while (!done) {
		if (CheckCancel()) {
			done = true;
			break;
		}
		delay(100);
		// might make this work to toggle blacks and whites eventually
		//for (int ix = 0; ix < STRIPLENGTH; ++ix) {
		//	leds[ix] = (leds[ix] == CRGB::White) ? CRGB::Black : CRGB::White;
		//}
		ShowLeds();
	}
	FastLED.clear(true);
	delay(2);
}

// time is in mSec
void FadeInOut(int time, bool in)
{
	if (in) {
		for (int i = 0; i <= LedInfo.nLEDBrightness; ++i) {
			FastLED.setBrightness(i);
			ShowLeds();
			delay(time / LedInfo.nLEDBrightness);
		}
	}
	else {
		for (int i = LedInfo.nLEDBrightness; i >= 0; --i) {
			FastLED.setBrightness(i);
			ShowLeds();
			delay(time / LedInfo.nLEDBrightness);
		}
	}
}

void addGlitter(fract8 chanceOfGlitter)
{
	if (random8() < chanceOfGlitter) {
		leds[random16(LedInfo.nTotalLeds - 1)] += CRGB::White;
	}
}

void fadeToBlack(int ledNo, byte fadeValue) {
	// FastLED
	leds[ledNo].fadeToBlackBy(fadeValue);
}

// display circles
void TestCircles()
{
	bool done = false;
	for (int count = 0; count < BuiltinInfo.nCirclesCount; ++count) {
		for (int col = 0; col < BuiltinInfo.nCirclesDiameter; ) {
			EVERY_N_MILLISECONDS_I(timerObj, 0) {
				timerObj.setPeriod(ImgInfo.nFrameHold);
				// go get a column
				Circles(col++);
				ShowLeds();
			}
			if (CheckCancel()) {
				done = true;
				break;
			}
		}
		// see if we need any gaps
		for (int gap = 0; gap < BuiltinInfo.nCirclesGap; ) {
			EVERY_N_MILLISECONDS_I(timerObj, 0) {
				timerObj.setPeriod(ImgInfo.nFrameHold);
				for (uint16_t row = 0; row < LedInfo.nTotalLeds; ++row) {
					SetPixel(row, CRGB::Black);
				}
				ShowLeds();
				++gap;
			}
			if (CheckCancel()) {
				done = true;
				break;
			}
		}
	}
	FastLED.clear(true);
}

// calculate and fill a column
// h,k is col,row of circle center
// r is radius
void Circles(int col)
{
	int h, k;
	k = LedInfo.nTotalLeds / 2 - 1;
	h = BuiltinInfo.nCirclesDiameter / 2 - 1;
	int r = BuiltinInfo.nCirclesDiameter / 2 - 1;
	// find the circle boundaries from y=k +/- sqrt(r*r-(x-h)*(x-h))
	int root = sqrt(r * r - ((col - h) * (col - h)));
	for (uint16_t row = 0; row < LedInfo.nTotalLeds; ++row) {
		bool show;
		if (BuiltinInfo.bCirclesFill) {
			// light everything inside the circle
			show = (row >= (k - root) && row <= (k + root));
		}
		else {
			// only light the circle
			show = (row == (k - root) || row == (k + root));
		}
		SetPixel(row, show ? CRGB(CHSV(BuiltinInfo.nCirclesHue, BuiltinInfo.nCirclesSaturation, 255)) : CRGB::Black);
	}
}

// push file index and cursor offset on stack to save
void PushFileIndex()
{
	if (FileIndexStackSize < FILEINDEXSTACKSIZE) {
		FileIndexStack[FileIndexStackSize] = currentFileIndex;
		++FileIndexStackSize;
	}
}

// pop the file index and cursor offset from the saved stack
void PopFileIndex()
{
	if (FileIndexStackSize) {
		--FileIndexStackSize;
		currentFileIndex = FileIndexStack[FileIndexStackSize];
	}
}

// run file or built-in
void ProcessFileOrBuiltin()
{
	// clear the cancel flag
	bCancelRun = false;
	unsigned long recordingTimeStart = 0;                // holds the start time for the current recording part
	String line;
	// let's see if this is a folder command
	String tmp = FileNames[currentFileIndex.nFileIndex];
	if (tmp[0] == NEXT_FOLDER_CHAR) {
		PushFileIndex();
		// clear new one
		FILEINDEXINFO fix = { 0,0 };
		currentFileIndex = fix;
		tmp = tmp.substring(1);
		// change folder, reload files
		currentFolder += tmp + "/";
		GetFileNamesFromSDorBuiltins(currentFolder);
		DisplayCurrentFile();
		return;
	}
	else if (tmp[0] == PREVIOUS_FOLDER_CHAR) {
		tmp = currentFolder.substring(0, currentFolder.length() - 1);
		tmp = tmp.substring(0, tmp.lastIndexOf("/") + 1);
		// change folder, reload files
		currentFolder = tmp;
		GetFileNamesFromSDorBuiltins(currentFolder);
		PopFileIndex();
		DisplayCurrentFile();
		return;
	}
	if (bRecordingMacro) {
		// get the starting name, the index will change for chaining, but the macro file has to have the start
		strcpy(FileToShow, FileNames[currentFileIndex.nFileIndex].c_str());
		// tag the start time
		recordingTimeStart = millis();
		recordingTime = 0;
		//Serial.println("marking macro start time: " + String(recordingTimeStart));
	}
	bIsRunning = true;
	// clear the rest of the lines
	//if (!bRunningMacro) {
	//	for (int ix = 1; ix < nMenuLineCount; ++ix)
	//		DisplayLine(ix, "");
	//}
	ClearScreen();
	DisplayCurrentFile(true, true);
	if (ImgInfo.startDelay) {
		// set a timer
		nTimerSeconds = ImgInfo.startDelay;
		while (nTimerSeconds && !CheckCancel()) {
			line = "Start Delay: " + String(nTimerSeconds / 10) + "." + String(nTimerSeconds % 10);
			DisplayLine(2, line, SystemInfo.menuTextColor);
			delay(100);
			--nTimerSeconds;
		}
		DisplayLine(3, "");
	}
	int chainCount = ImgInfo.bChainFiles ? FileCountOnly(currentFileIndex.nFileIndex) : 1;
	int chainFileCount = chainCount;
	int chainRepeatCount = ImgInfo.bChainFiles ? ImgInfo.nChainRepeats : 1;
	int lastFileIndex = currentFileIndex.nFileIndex;
	// don't allow chaining for built-ins, although maybe we should
	if (ImgInfo.bShowBuiltInTests) {
		chainCount = 1;
		chainRepeatCount = 1;
	}
	// set the basic LED info
	FastLED.setTemperature(CRGB(LedInfo.whiteBalance.r, LedInfo.whiteBalance.g, LedInfo.whiteBalance.b));
	FastLED.setBrightness(LedInfo.nLEDBrightness);
	//FastLED.setMaxPowerInVoltsAndMilliamps(5, LedInfo.nPixelMaxCurrent);
	//Serial.println("mA: " + String(LedInfo.nStripMaxCurrent));
	line = "";
	while (chainRepeatCount-- > 0) {
		while (chainCount-- > 0) {
			DisplayCurrentFile(true, true);
			if (ImgInfo.bChainFiles && !ImgInfo.bShowBuiltInTests) {
				line = "Remaining: " + String(chainCount + 1);
				DisplayLine(4, line, SystemInfo.menuTextColor);
				if (currentFileIndex.nFileIndex < chainFileCount - 1) {
					line = "Next: " + FileNames[currentFileIndex.nFileIndex + 1];
				}
				else {
					line = "";
				}
				DisplayLine(5, line, SystemInfo.menuTextColor);
			}
			line = "";
			// process the repeats and waits for each file in the list
			for (nRepeatsLeft = ImgInfo.repeatCount; nRepeatsLeft > 0; nRepeatsLeft--) {
				// fill the progress bar
				if (!ImgInfo.bShowBuiltInTests && !bRunningMacro)
					ShowProgressBar(0);
				if (ImgInfo.repeatCount > 1) {
					line = "Repeats: " + String(nRepeatsLeft) + " ";
				}
				if (!ImgInfo.bShowBuiltInTests && ImgInfo.nChainRepeats > 1) {
					line += "Chains: " + String(chainRepeatCount + 1);
				}
				DisplayLine(3, line, SystemInfo.menuTextColor);
				if (ImgInfo.bShowBuiltInTests) {
					DisplayLine(4, "Running (long cancel)", SystemInfo.menuTextColor);
					if (SystemInfo.bShowLEDsOnLcdWhileRunning)
						ShowLeds(1);
					// run the test
					(*BuiltInFiles[currentFileIndex.nFileIndex].function)();
					if (SystemInfo.bShowLEDsOnLcdWhileRunning)
						ShowLeds(2);
				}
				else {
					if (ImgInfo.nRepeatCountMacro > 1 && bRunningMacro) {
						DisplayLine(4, String("Macro Repeats: ") + String(nMacroRepeatsLeft), SystemInfo.menuTextColor);
					}
					// output the file
					SendFile(FileNames[currentFileIndex.nFileIndex]);
				}
				if (bCancelRun) {
					break;
				}
				if (!ImgInfo.bShowBuiltInTests && !bRunningMacro)
					ShowProgressBar(0);
				if (nRepeatsLeft > 1) {
					if (ImgInfo.repeatDelay) {
						FastLED.clear(true);
						// start timer
						nTimerSeconds = ImgInfo.repeatDelay;
						while (nTimerSeconds > 0 && !CheckCancel()) {
							line = "Repeat Delay: " + String(nTimerSeconds / 10) + "." + String(nTimerSeconds % 10);
							DisplayLine(2, line, SystemInfo.menuTextColor);
							line = "";
							delay(100);
							--nTimerSeconds;
						}
						DisplayLine(2, "");
					}
				}
			}
			if (bCancelRun) {
				chainCount = 0;
				break;
			}
			if (ImgInfo.bShowBuiltInTests)
				break;
			// see if we are chaining, if so, get the next file, if a folder we're done
			if (ImgInfo.bChainFiles) {
				// grab the next file
				if (currentFileIndex.nFileIndex < FileNames.size() - 1)
					++currentFileIndex.nFileIndex;
				if (IsFolder(currentFileIndex.nFileIndex))
					break;
				// handle any chain delay
				for (int dly = ImgInfo.nChainDelay; dly > 0 && !CheckCancel(); --dly) {
					line = "Chain Delay: " + String(dly / 10) + "." + String(dly % 10);
					DisplayLine(2, line, SystemInfo.menuTextColor);
					delay(100);
				}
				// check for chain wait for keypress
				if (chainCount && ImgInfo.bChainWaitKey) {
					DisplayLine(2, "Click: " + FileNames[currentFileIndex.nFileIndex], SystemInfo.menuTextColor);
					bool waitNext = true;
					int wbtn;
					while (waitNext) {
						delay(10);
						wbtn = ReadButton();
						if (wbtn == BTN_NONE)
							continue;
						if (wbtn == BTN_LONG) {
							CRotaryDialButton::pushButton(CRotaryDialButton::BTN_LONGPRESS);
						}
						else {
							waitNext = false;
						}
						if (CheckCancel()) {
							waitNext = false;
						}
					}
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
		currentFileIndex.nFileIndex = lastFileIndex;
		chainCount = ImgInfo.bChainFiles ? FileCountOnly(currentFileIndex.nFileIndex) : 1;
		if (ImgInfo.repeatDelay && (nRepeatsLeft > 1) || chainRepeatCount >= 1) {
			FastLED.clear(true);
			// start timer
			nTimerSeconds = ImgInfo.repeatDelay;
			while (nTimerSeconds > 0 && !CheckCancel()) {
				line = "Repeat Delay: " + String(nTimerSeconds / 10) + "." + String(nTimerSeconds % 10);
				DisplayLine(2, line, SystemInfo.menuTextColor);
				line = "";
				delay(100);
				--nTimerSeconds;
			}
		}
	}
	if (ImgInfo.bChainFiles)
		currentFileIndex.nFileIndex = lastFileIndex;
	FastLED.clear(true);
	bIsRunning = false;
	if (!bRunningMacro) {
		ClearScreen();
		DisplayCurrentFile(true, true);
	}
	if (bRecordingMacro) {
		// write the time for this macro into the file
		time_t now = time(NULL);
		recordingTime = millis() - recordingTimeStart;
		WriteOrDeleteConfigFile(String(ImgInfo.nCurrentMacro), false, false, true);
		DisplayCurrentFile(SystemInfo.bShowFolder, true);
	}
	// clear buttons
	CRotaryDialButton::clear();
}

void SendFile(String Filename) {
	bool bSavedReverse = ImgInfo.bReverseImage;	// save in case we mess with it
	// see if there is an associated config file and if it is supposed to run
	if (SystemInfo.bAutoLoadFileOnRun) {
		String cfFile = MakeMIWFilename(Filename, true);
		SettingsSaveRestore(true, 0);
		ProcessConfigFile(cfFile);
	}
	String fn = currentFolder + Filename;
	dataFile = SD.open(fn);
	// if the file is available send it to the LED's
	if (dataFile.available()) {
		for (int cnt = 0; cnt < (ImgInfo.bMirrorPlayImage ? 2 : 1); ++cnt) {
			ReadAndDisplayFile(cnt == 0);
			ImgInfo.bReverseImage = !ImgInfo.bReverseImage; // note this will be restored by SettingsSaveRestore
			dataFile.seek(0);
			FastLED.clear(true);
			int wait = ImgInfo.nMirrorDelay;
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
	if (!bRunningMacro)
		ShowProgressBar(100);
	if (SystemInfo.bAutoLoadFileOnRun) {
		SettingsSaveRestore(false, 0);
	}
	ImgInfo.bReverseImage = bSavedReverse;
}

// some useful BMP constants
#define MYBMP_BF_TYPE           0x4D42	// "BM"
#define MYBMP_BI_RGB            0L
//#define MYBMP_BI_RLE8           1L
//#define MYBMP_BI_RLE4           2L
//#define MYBMP_BI_BITFIELDS      3L

void ReadAndDisplayFile(bool doingFirstHalf) {
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
		WriteMessage(String("Invalid BMP:\n") + currentFolder + FileNames[currentFileIndex.nFileIndex], true);
		return;
	}

	/* Read info header */
	uint32_t imgSize = readLong();
	uint32_t imgWidth = readLong();
	uint32_t imgHeight = readLong();
	// save the original length, we need it later since imgHeight might be changed due to column limits
	uint32_t imgOriginalHeight = imgHeight;
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
		imgBitCount != 24 || imgCompression != MYBMP_BI_RGB)
	{
		WriteMessage(String("Unsupported, must be 24bpp, 1 plane, uncompressed, and non-zero size:\n") + currentFolder + FileNames[currentFileIndex.nFileIndex], true);
		return;
	}
	int displayWidth = imgWidth;
	if (imgWidth > LedInfo.nTotalLeds) {
		displayWidth = LedInfo.nTotalLeds;           //only display the number of led's we have
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
	// init the fade settings in SetPixel
	SetPixel(0, TFT_BLACK, -1, (int)imgHeight);
	long secondsLeft = 0, lastSeconds = 0;
	char num[50];
	unsigned minLoopTime = 0; // the minimum time it takes to process a line
	bool bLoopTimed = false;
	if (SystemInfo.bShowLEDsOnLcdWhileRunning) {
		ShowLeds(1, TFT_BLACK, imgWidth);
	}
	bool bReverseImage = ImgInfo.bReverseImage != ImgInfo.bRotate180;
	// also remember that height and width are effectively swapped since we rotated the BMP image CCW for ease of reading and displaying here
	// if start and end columns are different we need to adjust the imgHeight value
	// first see if end needs to be fixed
	if (ImgInfo.nStartCol > ImgInfo.nEndCol) {
		// set it to the end
		ImgInfo.nEndCol = imgHeight - 1;
	}
	if (ImgInfo.nEndCol > ImgInfo.nStartCol) {
		imgHeight = _min(imgHeight, ImgInfo.nEndCol + 1) - ImgInfo.nStartCol;
	}
	for (int column = bReverseImage ? imgHeight - 1 : 0; bReverseImage ? column >= 0 : column < imgHeight; bReverseImage ? --column : ++column) {
		// approximate time left
		if (bReverseImage)
			secondsLeft = ((long)column * (ImgInfo.nFrameHold + ImgInfo.nStutterTime + minLoopTime) / 1000L) + 1;
		else
			secondsLeft = ((long)(imgHeight - column) * (ImgInfo.nFrameHold + ImgInfo.nStutterTime + minLoopTime) / 1000L) + 1;
		// mark the time for timing the loop
		if (!bLoopTimed) {
			minLoopTime = millis();
		}
		if (ImgInfo.bMirrorPlayImage) {
			if (totalSeconds == -1)
				totalSeconds = secondsLeft;
			if (doingFirstHalf) {
				secondsLeft += totalSeconds;
			}
		}
		if (bRunningMacro) {
			// count columns done
			++nMacroColumnsDone;
			g_nPercentDone = map(nMacroColumnsDone, 0, MacroInfo[ImgInfo.nCurrentMacro].pixels, 0, 100);
			static int lastSeconds;
			int currentSeconds = (MacroInfo[ImgInfo.nCurrentMacro].mSeconds * g_nPercentDone / 100 + 500) / 1000;
			if (lastSeconds != currentSeconds) {
				lastSeconds = currentSeconds;
				int sec = MacroInfo[ImgInfo.nCurrentMacro].mSeconds / 1000 - currentSeconds;
				if (sec < 0)
					sec = 0;
				sprintf(num, "Macro Seconds: %d", sec);
				DisplayLine(2, num, SystemInfo.menuTextColor);
			}
		}
		else {
			if (secondsLeft != lastSeconds) {
				lastSeconds = secondsLeft;
				sprintf(num, "File Seconds: %d", secondsLeft);
				DisplayLine(2, num, SystemInfo.menuTextColor);
			}
			g_nPercentDone = map(bReverseImage ? imgHeight - column : column, 0, imgHeight, 0, 100);
		}
		if (ImgInfo.bMirrorPlayImage) {
			g_nPercentDone /= 2;
			if (!doingFirstHalf) {
				g_nPercentDone += 50;
			}
		}
		if (g_nPercentDone < 2 || ((g_nPercentDone % 5) == 0) || g_nPercentDone > 90) {
			ShowProgressBar(g_nPercentDone);
		}
		int bufpos = 0;
		CRGB pixel;
		int startHere = bReverseImage ? imgOriginalHeight - ImgInfo.nStartCol - (imgHeight - column) : column + ImgInfo.nStartCol;
		//Serial.print(String("startHere:") + startHere + " col:" + column + "\n");
		FileSeekBuf((uint32_t)bmpOffBits + (startHere * lineLength));
		//uint32_t offset = (bmpOffBits + (y * lineLength));
		//dataFile.seekSet(offset);
		// add the column row data here
		for (int row = displayWidth - 1; row >= 0; --row) {
			// this reads three bytes
			pixel = getRGBwithGamma();
			// see if we want this one
			if (ImgInfo.bScaleHeight && (row * displayWidth) % imgWidth) {
				continue;
			}
			SetPixel(row, pixel, column);
			if (SystemInfo.bShowLEDsOnLcdWhileRunning) {
				ShowLeds(4, pixel);
			}
		}
		// see how long it took to get here
		if (!bLoopTimed) {
			minLoopTime = millis() - minLoopTime;
			bLoopTimed = true;
			// if fixed time then we need to calculate the framehold value
			if (ImgInfo.bFixedTime) {
				// divide the time by the number of frames
				ImgInfo.nFrameHold = 1000 * ImgInfo.nFixedImageTime / imgHeight;
				ImgInfo.nFrameHold -= minLoopTime;
				ImgInfo.nFrameHold = max(ImgInfo.nFrameHold, 0);
			}
		}
		// wait for timer to expire before we show the next frame
		while (g_bStripWaiting) {
			MenuTextScrollSideways();
			//yield();
			//delayMicroseconds(100);
			// we should maybe check the cancel key here to handle slow frame rates?
		}
		// now show the lights
		ShowLeds();
		// set a timer while we go ahead and load the next frame
		g_bStripWaiting = true;
		esp_timer_start_once(oneshot_LED_timer, static_cast<uint64_t>(ImgInfo.nFrameHold) * 1000);
		// check keys
		if (CheckCancel())
			break;
		if (ImgInfo.bManualFrameAdvance) {
			// check if frame advance button requested
			if (ImgInfo.nFramePulseCount) {
				for (int ix = ImgInfo.nFramePulseCount; ix; --ix) {
					// wait for press
					while (digitalRead(FRAMEBUTTON_GPIO)) {
						if (CheckCancel())
							break;
						delay(10);
					}
					// wait for release
					while (!digitalRead(FRAMEBUTTON_GPIO)) {
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
						CRotaryDialButton::pushButton(BTN_LONG);
					else if (btn == BTN_LEFT) {
						// backup a line, use 2 because the for loop does one when we're done here
						if (bReverseImage) {
							column += 2;
							if (column > imgHeight)
								column = imgHeight;
						}
						else {
							column -= 2;
							if (column < -1)
								column = -1;
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
	if (SystemInfo.bShowLEDsOnLcdWhileRunning) {
		ShowLeds(2);
	}
}

// get some file information from the BMP, namely the width and height
void GetBmpSize(String fileName, uint32_t* width, uint32_t* height)
{
#if USE_STANDARD_SD
	SDFile bmpFile;
#else
	FsFile bmpFile;
#endif
	bmpFile = SD.open(fileName);
	// if the file is available send it to the LED's
	if (!bmpFile) {
		WriteMessage("failed to open: " + fileName, true);
		return;
	}
#pragma pack(1)
	struct {
		uint16_t bmpType;
		uint32_t bmpSize;
		uint16_t bmpReserved1;
		uint16_t bmpReserved2;
		uint32_t bmpOffBits;
		uint32_t imgSize;
		uint32_t imgWidth;
		uint32_t imgHeight;
		uint16_t imgPlanes;
		uint16_t imgBitCount;
		uint32_t imgCompression;
		uint32_t imgSizeImage;
		uint32_t imgXPelsPerMeter;
		uint32_t imgYPelsPerMeter;
		uint32_t imgClrUsed;
		uint32_t imgClrImportant;
	} bmpHeader = { 0 };
#pragma pack()
	bmpFile.readBytes((char*)&bmpHeader, sizeof(bmpHeader));
	bmpFile.close();
	// return the sizes, remember the bmp file has been rotated
	*width = bmpHeader.imgHeight;
	*height = bmpHeader.imgWidth;
}

// put the current file on the display
// Note that MenuItem is not used
void ShowBmp(MenuItem*)
{
	if (ImgInfo.bShowBuiltInTests) {
		bCancelMacro = bCancelRun = false;
		if (BuiltInFiles[currentFileIndex.nFileIndex].function) {
			ShowLeds(1);    // get ready for preview
			ShowLeds(5);    // turn LEDs off
			(*BuiltInFiles[currentFileIndex.nFileIndex].function)();
			ShowLeds(2);    // go back to normal
		}
		bCancelMacro = bCancelRun = false;
		return;
	}
	// true until cancel selected
	uint16_t* scrBuf = NULL;
	bool bOldGamma = LedInfo.bGammaCorrection;
	LedInfo.bGammaCorrection = false;
	bool bKeepShowing = true;
	int tftTall = _min(144, tft.height());
	int tftWide = tft.width();
	while (bKeepShowing) {
		String fn = currentFolder + FileNames[currentFileIndex.nFileIndex];
		// make sure this is a bmp file, if not just quietly go away
		String tmp = fn.substring(fn.length() - 3);
		tmp.toLowerCase();
		// check if really an image file (BMP)
		if (tmp.compareTo("bmp") != 0) {
			break;
		}
		// get a buffer if we don't already have one
		uint32_t scrBufSize = tft.width() * tftTall * sizeof(uint16_t);
		if (scrBuf == NULL)
			scrBuf = (uint16_t*)calloc(tft.width() * tftTall, sizeof(uint16_t));
		if (scrBuf == NULL) {
			WriteMessage("Not enough memory", true, 5000);
			bKeepShowing = false;
			scrBufSize = 0;
			break;
		}
#if USE_STANDARD_SD
		if (dataFile)
#else
		if (dataFile.isOpen())
#endif
			dataFile.close();
		dataFile = SD.open(fn);
		// if the file is not available, give up
		if (!dataFile.available()) {
			WriteMessage("failed to open: " + currentFolder + FileNames[currentFileIndex.nFileIndex], true);
			bKeepShowing = false;
			break;
		}
		// clear the buffer and the screen
		memset(scrBuf, 0, scrBufSize);
		ClearScreen();
		// clear the file cache buffer
		readByte(true);
		uint16_t bmpType = readInt();
		uint32_t bmpSize = readLong();
		uint16_t bmpReserved1 = readInt();
		uint16_t bmpReserved2 = readInt();
		uint32_t bmpOffBits = readLong();

		/* Check file header */
		if (bmpType != MYBMP_BF_TYPE) {
			WriteMessage(String("Invalid BMP:\n") + currentFolder + FileNames[currentFileIndex.nFileIndex], true);
			bKeepShowing = false;
			break;
		}

		/* Read info header */
		uint32_t imgSize = readLong();
		uint32_t imgWidth = readLong();
		uint32_t imgHeight = readLong();
		//uint32_t imgOriginalHeight = imgHeight;	// keep around for later use, since imgHeight might get changed due to column restrictions
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
			imgBitCount != 24 || imgCompression != MYBMP_BI_RGB)
		{
			WriteMessage(String("Unsupported, must be 24bpp:\n") + currentFolder + FileNames[currentFileIndex.nFileIndex], true);
			bKeepShowing = false;
			break;
		}
		bool bHalfSize = false;
		// see if this is too tall for the TFT
		if (imgWidth > 144) {
			bHalfSize = true;
			// also divide the width (height in the file) by 2
			imgHeight /= 2;
			//imgOriginalHeight = imgHeight;
		}
		//if (SystemInfo.bShowCroppedView) {
		//	//this code limits the displayed range
		//	// now see if column restrictions are set, first see if end needs to be fixed
		//	if (ImgInfo.nStartCol > ImgInfo.nEndCol) {
		//		// set it to the end
		//		ImgInfo.nEndCol = imgHeight - 1;
		//	}
		//	if (ImgInfo.nEndCol > ImgInfo.nStartCol) {
		//		imgHeight = _min(imgHeight, ImgInfo.nEndCol + 1) - ImgInfo.nStartCol;
		//	}
		//}
		/* compute the line length */
		uint32_t lineLength = imgWidth * 3;
		// fix for padding to 4 byte words
		if ((lineLength % 4) != 0)
			lineLength = (lineLength / 4 + 1) * 4;
		bool bDone = false;
		bool bRedraw = true;
		bool bAllowScroll = imgHeight > tftWide;
		// column offset for showing the image
		int imgStartCol = 0;
		int oldImgStartCol = 0;
		// if column set, change the startcol
		if (ImgInfo.nStartCol != ImgInfo.nEndCol)
			oldImgStartCol = imgStartCol = ImgInfo.nStartCol;
		bool bShowingSize = false;
		unsigned long mSecAuto = millis();
		// this is the current vertical offset
		// TODO: init to the current value which might be between 0 and 10
		int startOffsetList[] = { 0,5,9 };
		int startOffsetIndex = -1;
#if TTGO_T == 4
		DisplayLine(10, currentFolder, SystemInfo.menuTextColor);
		DisplayLine(11, FileNames[currentFileIndex.nFileIndex], SystemInfo.menuTextColor);
		float walk = (float)imgHeight / (float)imgWidth;
		DisplayLine(13, String(imgWidth) + " x " + String(imgHeight) + " pixels", SystemInfo.menuTextColor);
		DisplayLine(14, String(walk, 1) + " (" + String(walk * 3.28084, 1) + ") meters(feet)", SystemInfo.menuTextColor);
		// calculate display time
		float dspTime = ImgInfo.bFixedTime ? ImgInfo.nFixedImageTime : (imgHeight * (ImgInfo.nFrameHold + ImgInfo.nStutterTime) / 1000.0 + imgHeight * .008);
		DisplayLine(15, "About " + String((int)round(dspTime)) + " Seconds", SystemInfo.menuTextColor);
#endif
		while (!bDone) {
			if (SystemInfo.nPreviewAutoScroll && (millis() > mSecAuto + SystemInfo.nPreviewAutoScroll)) {
				mSecAuto = millis();
				// make sure not too long
				int newColStart = min((int32_t)imgHeight - tftWide, imgStartCol + SystemInfo.nPreviewAutoScrollAmount);
				// if <= 0 we couldn't scroll
				if (newColStart > 0) {
					// if no change we must be at the end, so reset it
					if (newColStart == imgStartCol) {
						imgStartCol = 0;
					}
					else {
						// accept the new offset
						imgStartCol = newColStart;
					}
					bRedraw = true;
				}
			}
			if (bRedraw) {
				// this code has been optimized so on a sideways scroll it first moves the
				// existing pixels on the display by the move amount
				// following that the rest of the screen is read from the SD card
				// this doubled the display speed on average when sideways scrolling
				int startCol = 0;
				int endCol = (imgHeight > tftWide ? tftWide : imgHeight);
				uint16_t* base = scrBuf;
				int diff = abs(imgStartCol - oldImgStartCol);
				if (imgStartCol != oldImgStartCol) {
					if (imgStartCol > oldImgStartCol) {
						// move the display columns to the left
						startCol = tftWide - diff;
						for (int mrow = 0; mrow < tftTall; ++mrow, base += tftWide) {
							// move the row
							memmove(base, base + diff, (tftWide - diff) * sizeof(uint16_t));
						}
					}
					else {
						// move the display columns to the right
						startCol = 0;
						endCol = diff;
						for (int mrow = 0; mrow < tftTall; ++mrow, base += tftWide) {
							// move the row
							memmove(base + diff, base, (tftWide - diff) * sizeof(uint16_t));
						}
					}
				}
				// now we read the missing data from the SD card
				// loop through the image, col is the image width, and x is the image height
				for (int col = startCol; col < endCol; ++col) {
					int bufpos = 0;
					CRGB pixel;
					int startHere;
					//if (SystemInfo.bShowCroppedView) {
					//	// This is code to limit the display to the selected columns
					//	if (ImgInfo.bRotate180) {
					//		startHere = imgOriginalHeight - 1 - (col + imgStartCol) - ImgInfo.nStartCol;
					//	}
					//	else {
					//		startHere = col + imgStartCol + ImgInfo.nStartCol;
					//	}
					//}
					//else {
					// if the image is rotated we need to reverse the direction reading the file
					if (ImgInfo.bRotate180) {
						startHere = imgHeight - 1 - (col + imgStartCol);
					}
					else {
						startHere = col + imgStartCol;
					}
					//}
					// get to start of pixel data for this column
					FileSeekBuf((uint32_t)bmpOffBits + ((startHere * (bHalfSize ? 2 : 1)) * lineLength));
					//Serial.println(String("start: ") + startHere + " " + ImgInfo.nStartCol + " : " + ImgInfo.nEndCol);
					for (int x = 0; x < imgWidth; ++x) {
						// this reads a three byte pixel RGB
						pixel = getRGBwithGamma();
						// throw a pixel away if we're dividing by 2 for the 288 pixel image
						if (bHalfSize)
							pixel = getRGBwithGamma();
						// because tftTall (might be 135) row display is less than 144 image
						// if upsidedown xor (boolean !=) rotated180 we need to make it upside down
						int row = (ImgInfo.bUpsideDown != ImgInfo.bRotate180) ? (imgWidth - 1 - x) : x;
						row -= SystemInfo.nPreviewStartOffset;
						if (row >= 0 && row < tftTall) {
							uint16_t color = tft.color565(pixel.r, pixel.g, pixel.b);
							uint16_t sbcolor;
							// the memory image colors are byte swapped
							swab(&color, &sbcolor, 2);
							//if (SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) {
							if (ImgInfo.nStartCol != ImgInfo.nEndCol) {
								if (ImgInfo.bUpsideDown != ImgInfo.bRotate180) {
									if (startHere == imgHeight - 1 - ImgInfo.nStartCol || startHere == imgHeight - 1 - ImgInfo.nEndCol) {
										sbcolor = TFT_WHITE;
									}
								}
								else {
									if (startHere == ImgInfo.nStartCol || startHere == ImgInfo.nEndCol) {
										sbcolor = TFT_WHITE;
									}
								}
							}
							//}
							// add to the display memory, organized as rows from top to bottom in memory
							scrBuf[row * tftWide + col] = sbcolor;
						}
					}
				}
				oldImgStartCol = imgStartCol;
				// got it all, go show it
				tft.pushRect(0, 0, tftWide, tftTall, scrBuf);
				// don't draw it again until something changes
				bRedraw = false;
			}
			ResetSleepAndDimTimers();
			switch (ReadButton()) {
			case BTN_NONE:
			case BTN_B2_LONG:
				break;
			case BTN_RIGHT_LONG:
				if (!bShowingSize && bAllowScroll) {
					imgStartCol -= bHalfSize ? (SystemInfo.nPreviewScrollCols * 2) : SystemInfo.nPreviewScrollCols;
					imgStartCol = max(0, imgStartCol);
				}
				break;
			case BTN_RIGHT:
				if ((SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) && (g_nB0Pressed || g_nB1Pressed)) {
					if (g_nB0Pressed) {
						// move start
						++ImgInfo.nStartCol;
						g_nB0Pressed = SystemInfo.nB0B1SetColumnsTimer;
					}
					else {
						// move end column to the right
						if (ImgInfo.nEndCol < imgHeight - 1)
							++ImgInfo.nEndCol;
						// do we need adjust the view?
						if (ImgInfo.nEndCol >= imgStartCol + tftWide) {
							++imgStartCol;
							++oldImgStartCol;
						}
						g_nB1Pressed = SystemInfo.nB0B1SetColumnsTimer;
					}
					bRedraw = true;
				}
				else if (SystemInfo.nPreviewMode == PREVIEW_MODE_FILE) {
					if (ImgInfo.bAutoColumnReset) {
						ImgInfo.nStartCol = ImgInfo.nEndCol;
					}
					if (currentFileIndex.nFileIndex < FileNames.size() - 1) {
						// stop if this is a folder
						if (!IsFolder(currentFileIndex.nFileIndex + 1)) {
							++currentFileIndex.nFileIndex;
							bDone = true;
						}
					}
				}
				else if (SystemInfo.nPreviewMode == PREVIEW_MODE_SCROLL || SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) {
					if (!bShowingSize && bAllowScroll) {
						imgStartCol -= bHalfSize ? (SystemInfo.nPreviewScrollCols * 2) : SystemInfo.nPreviewScrollCols;
						imgStartCol = max(0, imgStartCol);
					}
				}
				break;
			case BTN_LEFT_LONG:
				if (!bShowingSize && bAllowScroll) {
					imgStartCol += SystemInfo.nPreviewScrollCols;
					imgStartCol = min((int)imgHeight - tftWide, imgStartCol);
				}
				break;
			case BTN_LEFT:
				if ((SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) && (g_nB0Pressed || g_nB1Pressed)) {
					if (g_nB0Pressed) {
						// move the start back
						if (ImgInfo.nStartCol)
							--ImgInfo.nStartCol;
						// see if we need to adjust the view
						if (ImgInfo.nStartCol < imgStartCol) {
							--imgStartCol;
							--oldImgStartCol;
						}
						g_nB0Pressed = SystemInfo.nB0B1SetColumnsTimer;
					}
					else {
						--ImgInfo.nEndCol;
						g_nB1Pressed = SystemInfo.nB0B1SetColumnsTimer;
					}
					bRedraw = true;
				}
				else if (SystemInfo.nPreviewMode == PREVIEW_MODE_FILE) {
					if (ImgInfo.bAutoColumnReset) {
						ImgInfo.nStartCol = ImgInfo.nEndCol;
					}
					if (currentFileIndex.nFileIndex > 0) {
						// stop if this is a folder
						if (!IsFolder(currentFileIndex.nFileIndex - 1)) {
							--currentFileIndex.nFileIndex;
							bDone = true;
						}
					}
				}
				else if (SystemInfo.nPreviewMode == PREVIEW_MODE_SCROLL || SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) {
					if (!bShowingSize && bAllowScroll) {
						imgStartCol += SystemInfo.nPreviewScrollCols;
						imgStartCol = min((int)imgHeight - tftWide, imgStartCol);
					}
				}
				break;
			case BTN_B0_CLICK:
				// if B0 was active, cancel it
				if (g_nB0Pressed) {
					g_nB0Pressed = 0;
					break;
				}
				if (SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) {
					ImgInfo.nStartCol = imgStartCol;
					// make sure that the end column is not to the left of the start column
					if (ImgInfo.nEndCol <= ImgInfo.nStartCol) {
						//Serial.println(String("start: ") + imgStartCol + " h: " + imgHeight);
						ImgInfo.nEndCol = min((int)imgHeight - 1, imgStartCol + tftWide - 1);
					}
					bRedraw = true;
					// set a timer to watch for fine rotation setting
					g_nB0Pressed = SystemInfo.nB0B1SetColumnsTimer;
				}
				else {
					bDone = true;
					bKeepShowing = false;
				}
				break;
			case BTN_B1_CLICK:
				// if B1 was active, cancel it
				if (g_nB1Pressed) {
					g_nB1Pressed = 0;
					break;
				}
				if (SystemInfo.nPreviewMode == PREVIEW_MODE_CROP_SELECT) {
					ImgInfo.nEndCol = min((int)imgHeight - 1, imgStartCol + tftWide - 1);
					bRedraw = true;
					// set a timer to watch for fine rotation setting
					g_nB1Pressed = SystemInfo.nB0B1SetColumnsTimer;
				}
				else {	// leave preview on btn1 also when not setting columns
					bDone = true;
					bKeepShowing = false;
				}
				break;
			case BTN_LONG:
				bDone = true;
				bKeepShowing = false;
				break;
			case BTN_B0_LONG:	// rotate row offsets
				startOffsetIndex = (++startOffsetIndex) % (sizeof(startOffsetList) / sizeof(*startOffsetList));
				SystemInfo.nPreviewStartOffset = startOffsetList[startOffsetIndex];
				bRedraw = true;
				break;
#if TTGO_T == 1
			case BTN_SELECT:	// show the bmp information
				if (bShowingSize) {
					bShowingSize = false;
					bRedraw = true;
					ClearScreen();
				}
				else {
					ClearScreen();
					DisplayLine(0, currentFolder, SystemInfo.menuTextColor);
					DisplayLine(1, FileNames[currentFileIndex.nFileIndex], SystemInfo.menuTextColor);
					int tmpHeight = imgHeight;
					// account for column limits
					if (ImgInfo.nEndCol > ImgInfo.nStartCol) {
						tmpHeight = _min(tmpHeight, ImgInfo.nEndCol + 1) - ImgInfo.nStartCol;
					}
					float walk = (float)tmpHeight / (float)imgWidth;
					DisplayLine(3, String(imgWidth) + " x " + String(tmpHeight) + " pixels", SystemInfo.menuTextColor);
					DisplayLine(4, String(walk, 1) + " (" + String(walk * 3.28084, 1) + ") meters(feet)", SystemInfo.menuTextColor);
					// calculate display time
					float dspTime = ImgInfo.bFixedTime ? ImgInfo.nFixedImageTime : (tmpHeight * (ImgInfo.nFrameHold + ImgInfo.nStutterTime) / 1000.0 + tmpHeight * .008);
					DisplayLine(5, "About " + String((int)round(dspTime)) + " Seconds", SystemInfo.menuTextColor);
					bShowingSize = true;
					bRedraw = false;
				}
				break;
#endif
			case BTN_LEFT_RIGHT_LONG:
			case BTN_B1_LONG:	// change scroll mode
				// choose next mode
				++SystemInfo.nPreviewMode;
				// wrap if needed
				SystemInfo.nPreviewMode %= sizeof(PreviewFileModeText) / sizeof(*PreviewFileModeText);
//				SystemInfo.bPreviewScrollFiles = !SystemInfo.bPreviewScrollFiles;
				bDone = true;
//				WriteMessage(SystemInfo.bPreviewScrollFiles ? "Dial: Browse Images" : "Dial: Sideways Scroll", false, 1000);
				WriteMessage(String("Dial: ") + PreviewFileModeText[SystemInfo.nPreviewMode], false, 1000);
				break;
			}
			if (oldImgStartCol != imgStartCol) {
				bRedraw = true;
			}
			delay(1);
		}
	}
	// all done
	if (scrBuf)
		free(scrBuf);
	dataFile.close();
	readByte(true);
	LedInfo.bGammaCorrection = bOldGamma;
	ClearScreen();
	// kill the cancel flag
	bCancelRun = bCancelMacro = false;
}

// display a line in selected colors and clear to the end of the line
void DisplayLine(int line, String text, int16_t color, int16_t backColor)
{
	if (line >= 0 && line < nMenuLineCount) {
		// don't show if running and displaying file on LCD
		if (!(bIsRunning && SystemInfo.bShowLEDsOnLcdWhileRunning)) {
			if (TextLines[line].Line != text || TextLines[line].backColor != backColor || TextLines[line].foreColor != color) {
				int pixels = tft.textWidth(text);
				if (pixels > tft.width()) {
					TextLines[line].nRollLength = pixels;
					TextLines[line].nRollOffset = 0;
				}
				else {
					TextLines[line].nRollOffset = TextLines[line].nRollLength = 0;
				}
				// save the line for scrolling purposes
				TextLines[line].Line = text;
				TextLines[line].foreColor = color;
				TextLines[line].backColor = backColor;
			}
			// push the sprite text to the display
			int y = line * tft.fontHeight();
			LineSprite.setTextColor(color, backColor);
			LineSprite.drawString(text, -TextLines[line].nRollOffset, 0);
			LineSprite.pushSprite(0, y);
		}
	}
}

void ClearScreen()
{
	tft.fillScreen(TFT_BLACK);
	ResetTextLines();
}

void ResetTextLines()
{
	for (int ix = 0; ix < nMenuLineCount; ++ix) {
		TextLines[ix].Line.clear();
		TextLines[ix].nRollOffset = 0;
		TextLines[ix].nRollLength = 0;
		TextLines[ix].foreColor = 0;
		TextLines[ix].backColor = 0;
		TextLines[ix].nRollDirection = 0;
	}
}

// active menu line is in reverse video or * at front depending on bMenuStar
void DisplayMenuLine(int line, int displine, String text)
{
	bool hilite = MenuStack.top()->index == line;
	String mline = (hilite && SystemInfo.bMenuStar ? "*" : " ") + text;
	if (displine >= 0 && displine < nMenuLineCount) {
		if (SystemInfo.bMenuStar) {
			DisplayLine(displine, mline, SystemInfo.menuTextColor, TFT_BLACK);
		}
		else {
			DisplayLine(displine, mline, hilite ? TFT_BLACK : SystemInfo.menuTextColor, hilite ? SystemInfo.menuTextColor : TFT_BLACK);
		}
	}
}

uint32_t readLong() {
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

uint16_t readInt() {
	byte incomingbyte;
	uint16_t retValue = 0;

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

int readByte(bool clear) {
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
void FileSeekBuf(uint32_t place)
{
	if (place < filePosition || place >= filePosition + filebufsize) {
		// we need to read some more
		filebufsize = 0;
		dataFile.seek(place);
	}
}

// count the actual files, at a given starting point
int FileCountOnly(int start)
{
	int count = 0;
	// ignore folders, at the end
	for (int files = start; files < FileNames.size(); ++files) {
		if (!IsFolder(files))
			++count;
	}
	return count;
}

// return true if file is folder
bool IsFolder(String name)
{
	return name[0] == NEXT_FOLDER_CHAR
		|| name[0] == PREVIOUS_FOLDER_CHAR;
}

// return true if current file is folder
bool IsFolder(int index)
{
	return FileNames[index][0] == NEXT_FOLDER_CHAR
		|| FileNames[index][0] == PREVIOUS_FOLDER_CHAR;
}

// show the current file
// bShowPath adds path in front, bFirstOnly does top line of current file only, used for running
void DisplayCurrentFile(bool bShowPath, bool bFirstOnly)
{
	bool bOldShowNextFiles;
	int nOldFileCursor;
	// save the current settings
	if (bFirstOnly) {
		nOldFileCursor = currentFileIndex.nFileCursor;
		currentFileIndex.nFileCursor = 0;
		bOldShowNextFiles = SystemInfo.bShowNextFiles;
		SystemInfo.bShowNextFiles = false;
	}
	// fix the offset in case no extra files are wanted on the display
	if (!SystemInfo.bShowNextFiles)
		currentFileIndex.nFileCursor = 0;
	bool bShowFolder = SystemInfo.bShowFolder;
	// always show the path for previous folder
	if (SystemInfo.bKeepFileOnTopLine && (FileNames[currentFileIndex.nFileIndex][0] == PREVIOUS_FOLDER_CHAR || currentFolder == "/"))
		bShowFolder = true;
	// display the current file with the correct hilite
	if (ImgInfo.bShowBuiltInTests) {
		if (SystemInfo.bHiLiteCurrentFile) {
			DisplayLine(currentFileIndex.nFileCursor, FileNames[currentFileIndex.nFileIndex], TFT_BLACK, SystemInfo.menuTextColor);
		}
		else {
			DisplayLine(currentFileIndex.nFileCursor, FileNames[currentFileIndex.nFileIndex], SystemInfo.menuTextColor, TFT_BLACK);
		}
	}
	else {
		if (bSdCardValid) {
			// show the current file
			String name = ((bShowPath && bShowFolder) ? currentFolder : "") + FileNames[currentFileIndex.nFileIndex] + (ImgInfo.bMirrorPlayImage ? "><" : "");
			// prepend the path if this is the scrolling type list for subfolders
			if (!SystemInfo.bKeepFileOnTopLine && currentFileIndex.nFileCursor == 0 && name[0] == PREVIOUS_FOLDER_CHAR) {
				name = currentFolder + name;
			}
			if (SystemInfo.bHiLiteCurrentFile) {
				DisplayLine(currentFileIndex.nFileCursor, name, TFT_BLACK, SystemInfo.menuTextColor);
			}
			else {
				DisplayLine(currentFileIndex.nFileCursor, name, SystemInfo.menuTextColor, TFT_BLACK);
			}
		}
		else {
			WriteMessage("No SD Card or Files", true);
			ImgInfo.bShowBuiltInTests = true;
			GetFileNamesFromSDorBuiltins("/");
			DisplayCurrentFile(bShowPath, bFirstOnly);
			return;
		}
	}
	// fill in the rest of the files if wanted
	if (!bIsRunning && SystemInfo.bShowNextFiles) {
		for (int ix = 0; ix < nMenuLineCount - ((SystemInfo.bShowBatteryLevel || SystemInfo.bShowFilePosition) ? 1 : 0); ++ix) {
			// skip the current one unless previous folder, we already did it above
			if (ix == currentFileIndex.nFileCursor)
				continue;
			String fn;
			if (currentFileIndex.nFileIndex + ix - currentFileIndex.nFileCursor < FileNames.size()) {
				fn = FileNames[currentFileIndex.nFileIndex + ix - currentFileIndex.nFileCursor];
				fn = (SystemInfo.bShowFolder || (ix == 0 && fn[0] == PREVIOUS_FOLDER_CHAR) ? currentFolder : "") + fn;
			}
			DisplayLine(ix, (SystemInfo.bKeepFileOnTopLine ? "  " : "") + fn, SystemInfo.menuTextColor);
		}
	}
	// if recording put a red digit at the top right
	if (bRecordingMacro) {
		tft.setTextColor(TFT_BLACK, TFT_RED);
		tft.drawString(String(ImgInfo.nCurrentMacro), tft.width() - 10, 1);
	}
	tft.setTextColor(SystemInfo.menuTextColor);
	// for debugging keypresses
	//DisplayLine(3, String(nButtonDowns) + " " + nButtonUps);
	if (bFirstOnly) {
		currentFileIndex.nFileCursor = nOldFileCursor;
		SystemInfo.bShowNextFiles = bOldShowNextFiles;
	}
}

void ShowProgressBar(int percent)
{
	//if (SystemInfo.bShowProgress && !(bIsRunning && SystemInfo.bShowDuringBmpFile)) {
	if (SystemInfo.bShowProgress) {
		static int lastpercent = 0;
		if (lastpercent && (lastpercent == percent))
			return;
		int x = tft.width() - 1;
		int y = SystemInfo.bShowLEDsOnLcdWhileRunning ? 0 : (tft.fontHeight() + 4);
		int h = SystemInfo.bShowLEDsOnLcdWhileRunning ? 4 : 8;
		if (percent == 0) {
			tft.fillRect(0, y, x, h, TFT_BLACK);
		}
		DrawProgressBar(0, y, x, h, percent, !SystemInfo.bShowLEDsOnLcdWhileRunning);
		lastpercent = percent;
	}
}

// insert newlines into a string so it doesn't wrap in the middle of words when displayed
// existing newlines are honored
String FormatMultiLine(String& input)
{
	String output;
	int lineWidth = 0;
	int lastInputSpace = 0;
	int lastOutputSpace = 0;
	int lastOutputStart = 0;
	// look for spaces and add words to the output, when each line is too long start a new line
	for (int inIx = 0; inIx < input.length(); ++inIx) {
		switch (input[inIx]) {
		case '\n':
			// flush the line
			output += '\n';
			lastOutputStart = output.length();
			break;
		case ' ':
			output += input[inIx];
			// mark the space location so we can go back
			lastInputSpace = inIx;
			lastOutputSpace = output.length();
			break;
		default:
			// check the width after adding this character
			output += input[inIx];
			lineWidth = tft.textWidth(output.substring(lastOutputStart));
			if (lineWidth > tft.width()) {
				// too wide, backup
				output = output.substring(0, lastOutputSpace);
				// add a newline to the output
				output += '\n';
				inIx = lastInputSpace;
				lastOutputStart = output.length();
			}
			break;
		}
	}
	return output;
}

// display message on first line, if wait is -1, wait for a key press
void WriteMessage(String txt, bool error, int wait, bool process)
{
	ClearScreen();
	if (process) {
		txt = FormatMultiLine(txt);
	}
	if (error) {
		txt = "**" + txt + "**";
		tft.setTextColor(TFT_RED);
	}
	else {
		tft.setTextColor(SystemInfo.menuTextColor);
	}
	tft.setCursor(0, tft.fontHeight());
	tft.setTextWrap(true);
	tft.print(txt);
	if (wait == -1) {
		// wait for a key
		while (ReadButton() == BTN_NONE)
			delay(10);
	}
	else {
		delay(wait);
	}
	tft.setTextColor(TFT_WHITE);
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
	for (auto& nm : FileNames) {
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
						// see if we want to ignore the settings
						if (!SystemInfo.bMacroUseCurrentSettings || SettingsVarList[which].type == vtShowFile) {
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
								bool bLastBuiltIn = ImgInfo.bShowBuiltInTests;
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
								int oldFileIndex = currentFileIndex.nFileIndex;
								// save the old folder if necessary
								String oldFolder;
								if (!ImgInfo.bShowBuiltInTests && !currentFolder.equalsIgnoreCase(folder)) {
									oldFolder = currentFolder;
									currentFolder = folder;
									GetFileNamesFromSDorBuiltins(folder);
								}
								// search for the file in the list
								int which = LookUpFile(name);
								if (which >= 0) {
									currentFileIndex.nFileIndex = which;
									// call the process routine
									strcpy(FileToShow, name.c_str());
									if (!bRunningMacro)
										ClearScreen();
									ProcessFileOrBuiltin();
								}
								if (oldFolder.length()) {
									currentFolder = oldFolder;
									GetFileNamesFromSDorBuiltins(currentFolder);
								}
								currentFileIndex.nFileIndex = oldFileIndex;
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
							case vtMacroTime:
								*(unsigned long*)(SettingsVarList[which].address) = args.toInt();
								break;
							default:
								break;
							}
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

// return the total time from the macro file
// nameList can be NULL to avoid saving the files
int MacroTime(String filepath, int* files, int* width, std::vector<String>* nameList)
{
	int retval = 0;
	int count = 0;
	int pixels = 0;
#if USE_STANDARD_SD
	SDFile rdfile;
#else
	FsFile rdfile;
#endif
	rdfile = SD.open(filepath);
	if (rdfile.available()) {
		String line, command, args;
		uint32_t width, height;
		while (line = rdfile.readStringUntil('\n'), line.length()) {
			// read the lines and do what they say
			int ix = line.indexOf('=', 0);
			if (ix > 0) {
				command = line.substring(0, ix);
				command.trim();
				command.toUpperCase();
				args = line.substring(ix + 1);
				args.trim();
				// loop through the var list looking for a match, only do the time ones
				for (int which = 0; which < sizeof(SettingsVarList) / sizeof(*SettingsVarList); ++which) {
					if (command.compareTo(SettingsVarList[which].name) == 0) {
						switch (SettingsVarList[which].type) {
						case vtShowFile:
							GetBmpSize(args, &width, &height);
							pixels += width;
							// save the name
							if (nameList) {
								nameList->push_back(args);
							}
							break;
						case vtInt:
						case vtBool:
						case vtBuiltIn:
						case vtRGB:
							break;
						case vtMacroTime:
						{
							retval += args.toInt();
							// older macros had seconds, newer ones need mS
							if (retval < 20)
								retval *= 1000;
							++count;
							// add a fudge factor to account for file opening/closing etc
							retval += 300;
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
	*files = count;
	*width = pixels;
	return retval;
}

bool MatchNameFilter(String name, String pattern)
{
	// split the patterns and handle each in turn
	String match;
	int ix = pattern.indexOf('|');
	do {
		//Serial.println("pattern:" + pattern + " match:" + match);
		if (ix == -1)
			match = pattern;
		else {
			match = pattern.substring(0);
			//  remove this part
			pattern = pattern.substring(ix + 1);
			int ixend = match.indexOf('|');
			if (ixend != -1)
				match = match.substring(0, ixend);
			//Serial.println("  pattern:" + pattern + " match:" + match);
		}
		if (name.indexOf(match) != -1) {
			//Serial.println("true");
			return true;
		}
	} while (pattern.length() && ix != -1);
	//Serial.println("false");
	return false;
}

// read the files from the card or list the built-ins
// look for start.MIW, and process it, but don't add it to the list
// the valid card flag is set here
void GetFileNamesFromSDorBuiltins(String dir) {
	bool worked = true;
	// start over
	// first empty the current file names
	FileNames.clear();
	if (nBootCount == 0)
		currentFileIndex.nFileIndex = 0;
	if (!ImgInfo.bShowBuiltInTests) {
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
			worked = false;
		}
		if (!root.isDirectory()) {
			//Serial.println("Not a directory: " + dir);
			worked = false;
		}
		if (worked) {
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
				char fname[100];
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
						//find files with our extension only
						if (uppername.endsWith(".BMP") && (!bnameFilter || MatchNameFilter(uppername, nameFilter))) {
							//Serial.println("name: " + CurrentFilename);
							FileNames.push_back(CurrentFilename);
						}
						else if (uppername == StartFileName) {
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
			bSdCardValid = true;
		}
		else {
			ImgInfo.bShowBuiltInTests = true;
			bSdCardValid = false;
		}
	}
	// if nothing read, switch to built-in
	if (FileNames.size() == 0) {
		ImgInfo.bShowBuiltInTests = true;
		bSdCardValid = false;
	}
	if (ImgInfo.bShowBuiltInTests) {
		for (int ix = 0; ix < (sizeof(BuiltInFiles) / sizeof(*BuiltInFiles)); ++ix) {
			FileNames.push_back(String(BuiltInFiles[ix].text));
		}
		currentFolder = "";
	}
	return;
}

// compare strings for sort ignoring case
bool CompareNames(const String& a, const String& b)
{
	String a1 = a, b1 = b;
	a1.toUpperCase();
	b1.toUpperCase();
	// force folders to sort last
	if (a1[0] == NEXT_FOLDER_CHAR)
		a1[0] = '\x7e';
	if (b1[0] == NEXT_FOLDER_CHAR)
		b1[0] = '\x7e';
	// force previous folder to sort first
	if (a1[0] == PREVIOUS_FOLDER_CHAR)
		a1[0] = '0' - 1;
	if (b1[0] == PREVIOUS_FOLDER_CHAR)
		b1[0] = '0' - 1;
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
		// calculate how many bytes we need
		size_t neededBytes = 0;
		for (int ix = 0; ix < (sizeof(saveValueList) / sizeof(*saveValueList)); ++ix) {
			neededBytes += saveValueList[ix].size;
		}
		memptr[set] = malloc(neededBytes);
		if (!memptr[set]) {
			return false;
		}
	}
	void* blockptr = memptr[set];
	if (memptr[set] == NULL) {
		return false;
	}
	for (int ix = 0; ix < (sizeof(saveValueList) / sizeof(*saveValueList)); ++ix) {
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
	if (GetYesNo("Erase START.MIW?"))
		WriteOrDeleteConfigFile("", true, true, false);
}

void SaveStartFile(MenuItem* menu)
{
	WriteOrDeleteConfigFile("", false, true, false);
}

void EraseAssociatedFile(MenuItem* menu)
{
	String filepath;
	filepath = currentFolder + MakeMIWFilename(FileNames[currentFileIndex.nFileIndex], true);

	if (GetYesNo(("Erase " + filepath + "?").c_str()))
		WriteOrDeleteConfigFile(FileNames[currentFileIndex.nFileIndex].c_str(), true, false, false);
}

void SaveAssociatedFile(MenuItem* menu)
{
	WriteOrDeleteConfigFile(FileNames[currentFileIndex.nFileIndex].c_str(), false, false, false);
}

void LoadAssociatedFile(MenuItem* menu)
{
	String name = FileNames[currentFileIndex.nFileIndex];
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
	String name = StartFileName;
	if (ProcessConfigFile(name)) {
		WriteMessage(String("Processed:\n") + name);
	}
	else {
		WriteMessage("Failed reading:\n" + name, true);
	}
}

// create the config file, or remove it
// startfile true makes it use the start.MIW file, else it handles the associated name file
bool WriteOrDeleteConfigFile(String filename, bool remove, bool startfile, bool bMacro)
{
	bool retval = true;
	String filepath;
	if (startfile) {
		filepath = currentFolder + String(StartFileName);
	}
	else {
		filepath = (bMacro ? String("/") : currentFolder) + MakeMIWFilename(filename, true);
	}
	//bool fileExists = SD.exists(filepath.c_str());
	if (remove) {
		if (!SD.exists(filepath.c_str()))
			WriteMessage(String("Not Found:\n") + filepath, true);
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
		SDFile file = SD.open(filepath.c_str(), bRecordingMacro ? FILE_APPEND : FILE_WRITE);
		if (file) {
#else
		FsFile file = SD.open(filepath.c_str(), bRecordingMacro ? (O_APPEND | O_WRITE | O_CREAT) : (O_WRITE | O_TRUNC | O_CREAT));
		if (file.getError() == 0) {
#endif
			//Serial.println("file: " + filepath + " " + String(remove) + " " + String(startfile) + " recording " + String(bRecordingMacro));
			// loop through the var list
			for (int ix = 0; ix < sizeof(SettingsVarList) / sizeof(*SettingsVarList); ++ix) {
				switch (SettingsVarList[ix].type) {
				case vtBuiltIn:
					line = String(SettingsVarList[ix].name) + "=" + String(*(bool*)(SettingsVarList[ix].address) ? "TRUE" : "FALSE");
					break;
				case vtShowFile:
					if (*(char*)(SettingsVarList[ix].address)) {
						line = String(SettingsVarList[ix].name) + "=" + (ImgInfo.bShowBuiltInTests ? "" : currentFolder) + String((char*)(SettingsVarList[ix].address));
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
				case vtMacroTime:
					line = String(SettingsVarList[ix].name) + "=" + String(*(int*)(SettingsVarList[ix].address));
					break;
				default:
					line = "";
					break;
				}
				if (line.length())
					file.println(line);
			}
			file.close();
			WriteMessage(String("Saved:\n") + filepath, false, 700);
		}
		else {
			retval = false;
			//Serial.println("error: " + String(file.getError()));
			WriteMessage(String("Failed to write:\n") + filepath, true);
		}
	}
	return retval;
}

// save the eeprom settings
void SaveEepromSettings(MenuItem* menu)
{
	SaveLoadSettings(true);
}

// load eeprom settings
void LoadEepromSettings(MenuItem* menu)
{
	SaveLoadSettings(false);
}

// save the macro with the current settings
void SaveMacro(MenuItem* menu)
{
	bRecordingMacro = true;
	WriteOrDeleteConfigFile(String(ImgInfo.nCurrentMacro), false, false, true);
	bRecordingMacro = false;
}

// saves and restores settings before running the macro
void RunMacro(MenuItem* menu)
{
	bCancelMacro = false;
	for (nMacroRepeatsLeft = ImgInfo.nRepeatCountMacro; nMacroRepeatsLeft; --nMacroRepeatsLeft) {
		MacroLoadRun(menu, true);
		if (bCancelMacro) {
			break;
		}
		ClearScreen();
		for (int wait = ImgInfo.nRepeatWaitMacro; nMacroRepeatsLeft > 1 && wait; --wait) {
			if (CheckCancel()) {
				nMacroRepeatsLeft = 0;
				break;
			}
			DisplayLine(5, "#" + String(ImgInfo.nCurrentMacro) + String(" Wait: ") + String(wait / 10) + "." + String(wait % 10) + " Repeat: " + String(nMacroRepeatsLeft - 1), SystemInfo.menuTextColor);
			delay(100);
		}
	}
	bCancelMacro = false;
}

// get a yes/no response
bool GetYesNo(String msg)
{
	int pad = tft.getTextPadding();
	tft.setTextPadding(0);
	bool retval = false;
	bool change = true;
	ClearScreen();
	DisplayLine(1, msg, SystemInfo.menuTextColor);
	DisplayLine(4, "Rotate to Change", SystemInfo.menuTextColor);
	DisplayLine(5, "Click to Select", SystemInfo.menuTextColor);
	CRotaryDialButton::Button button = BTN_NONE;
	bool done = false;
	while (!done) {
		button = ReadButton();
		switch (button) {
		case BTN_LEFT:
		case BTN_RIGHT:
			retval = !retval;
			change = true;
			break;
		case BTN_SELECT:
			//case BTN_LONG:
			done = true;
			break;
		default:
			break;
		}
		if (change) {
			// draw the buttons
			tft.fillRoundRect(60, 42, 60, 22, 8, SystemInfo.menuTextColor);
			tft.setTextColor(TFT_BLACK, SystemInfo.menuTextColor);
			tft.drawString(retval ? "Yes" : "No", 72, 46);
			change = false;
		}
		MenuTextScrollSideways();
	}
	tft.setTextPadding(pad);
	return retval;
}

// display some macro info
void InfoMacro(MenuItem* menu)
{
	int nMacroNum = *(int*)menu->value;
	// rotate dial to view files, else exit on click
	bool done = false;
	bool redraw = true;
	int offset = -1;
	String savedNameFilter;
	bool bMacroChanges = false;
	while (!done) {
		CRotaryDialButton::Button btn = BTN_NONE;
		if (redraw) {
			ClearScreen();
			DisplayLine(0, String(nMacroNum) + " : " + MacroInfo[nMacroNum].description, TFT_BLACK, SystemInfo.menuTextColor);
			DisplayLine(1, "Files: " + String(MacroInfo[nMacroNum].fileNames.size()), SystemInfo.menuTextColor);
			DisplayLine(2, "Time: " + String((MacroInfo[nMacroNum].mSeconds + 500) / 1000) + " Sec", SystemInfo.menuTextColor);
			DisplayLine(3, "Pixels: " + String(MacroInfo[nMacroNum].pixels) + " Pixels", SystemInfo.menuTextColor);
			float walk = (float)MacroInfo[nMacroNum].pixels / (float)LedInfo.nTotalLeds;
			DisplayLine(4, String(walk, 1) + " (" + String(walk * 3.28084, 1) + ") meters(feet)", SystemInfo.menuTextColor);
			DisplayLine(6, "Click=Edit Text, Long Press=Exit, Rotate=Show Files", SystemInfo.menuTextColor);
			redraw = false;
		}
		switch (btn = ReadButton()) {
		case BTN_NONE:
		case BTN_B1_CLICK:
		case BTN_B0_LONG:
		case BTN_B1_LONG:
		case BTN_B2_LONG:
			break;
		case BTN_B0_CLICK:
		case BTN_SELECT:
			// save the namefilter and use SetFilter for entering our description
			savedNameFilter = nameFilter;
			nameFilter = MacroInfo[nMacroNum].description;
			SetFilter(menu);
			if (MacroInfo[nMacroNum].description != nameFilter) {
				MacroInfo[nMacroNum].description = nameFilter;
				bMacroChanges = true;
			}
			nameFilter = savedNameFilter;
			redraw = true;
			break;
		case BTN_LONG:
			done = true;
			break;
		case BTN_RIGHT:
		case BTN_LEFT:
			// check if 0 files, nothing to do
			if (MacroInfo[nMacroNum].fileNames.size() == 0)
				break;
			if (offset == -1) {
				offset = -nMenuLineCount;
				ClearScreen();
			}
			// go to the next set
			offset += (btn == BTN_RIGHT ? nMenuLineCount : -nMenuLineCount);
			// range check
			offset = constrain(offset, 0, MacroInfo[nMacroNum].fileNames.size() - nMenuLineCount);
			// no point in scrolling if size isn't larger than the lines on the display
			if (MacroInfo[nMacroNum].fileNames.size() <= nMenuLineCount)
				offset = 0;
			//Serial.println("offset: " + String(offset));
			// show the files
			for (int lineNum = 0; lineNum < nMenuLineCount; ++lineNum) {
				DisplayLine(lineNum, (lineNum + offset) < MacroInfo[nMacroNum].fileNames.size() ? MacroInfo[nMacroNum].fileNames[lineNum + offset] : "", SystemInfo.menuTextColor);
			}
			break;
		}
	}
	if (bMacroChanges) {
		SaveMacroInfo();
	}
}

// like run, but doesn't restore settings
void LoadMacro(MenuItem* menu)
{
	MacroLoadRun(menu, false);
}

void MacroLoadRun(MenuItem*, bool save)
{
	bool oldShowBuiltins = false;
	if (save) {
		oldShowBuiltins = ImgInfo.bShowBuiltInTests;
		SettingsSaveRestore(true, 1);
	}
	bRunningMacro = true;
	nMacroColumnsDone = 0;
	//nMacroStartTime = millis();
//#define MIN_FRAME_TIME_ESTIMATE 10
	// calculate the time using the calculated or stored mSeconds
	//if (SystemInfo.bMacroUseCurrentSettings)
	//	nMacroTimemS = MacroInfo[ImgInfo.nCurrentMacro].pixels * (ImgInfo.nFrameHold == 0 ? MIN_FRAME_TIME_ESTIMATE : ImgInfo.nFrameHold) + (MacroInfo[ImgInfo.nCurrentMacro].fileNames.size() * 50);
	//else
	//	nMacroTimemS = MacroInfo[ImgInfo.nCurrentMacro].mSeconds;
	ClearScreen();
	bRecordingMacro = false;
	String line = "/" + MakeMIWFilename(String(ImgInfo.nCurrentMacro), true);
	if (!ProcessConfigFile(line)) {
		line += " not found";
		WriteMessage(line, true);
	}
	bRunningMacro = false;
	if (save) {
		// need to handle if the builtins was changed
		if (oldShowBuiltins != ImgInfo.bShowBuiltInTests) {
			ToggleFilesBuiltin(NULL);
		}
		SettingsSaveRestore(false, 1);
	}
	//nMacroStartTime = 0;
}

void DeleteMacro(MenuItem* menu)
{
	if (GetYesNo(("Delete Macro #" + String(*(int*)menu->value) + "?").c_str())) {
		WriteOrDeleteConfigFile(String(*(int*)menu->value), true, false, true);
		// remove the macroinfo entry
		MacroInfo[*(int*)menu->value].description = "Empty";
		MacroInfo[*(int*)menu->value].fileNames.clear();
		MacroInfo[*(int*)menu->value].pixels = 0;
		MacroInfo[*(int*)menu->value].length = 0;
		SaveMacroInfo();
	}
}

// remove the macro json file, it will be automatically rebuilt on reboot
void DeleteMacroJson(MenuItem* menu)
{
	if (GetYesNo(String("Delete ") + String(MACRO_JSON_FILE))) {
		SD.remove(MACRO_JSON_FILE);
	}
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
	FastLED.setTemperature(CRGB(LedInfo.whiteBalance.r, LedInfo.whiteBalance.g, LedInfo.whiteBalance.b));
	FastLED.show();
	delay(3000);
	FastLED.clear(true);
}

// reverse the strip index order for the lower strip, the upper strip is normal
// also check to make sure it isn't out of range
int AdjustStripIndex(int ix)
{
	int ledCount = LedInfo.bSecondController ? LedInfo.nTotalLeds / 2 : LedInfo.nTotalLeds;
	switch (LedInfo.stripsMode) {
	case STRIPS_MIDDLE_WIRED:	// bottom reversed, top normal, both wired in the middle
		if (ix < ledCount) {
			ix = (ledCount - 1 - ix);
		}
		break;
	case STRIPS_CHAINED:	// bottom and top normal, chained, so nothing to do
		break;
	case STRIPS_OUTSIDE_WIRED:	// top reversed, bottom normal, no connection in the middle
		if (ix >= ledCount) {
			ix = (LedInfo.nTotalLeds - 1 - ix) + ledCount;
		}
		break;
	}
	// make sure it isn't too big or too small
	ix = constrain(ix, 0, LedInfo.nTotalLeds - 1);
	return ix;
}

// write a pixel to the correct location
// pixel doubling is handled here
// e.g. pixel 0 will be 0 and 1, 1 will be 2 and 3, etc
// if upside down n will be n and n-1, n-1 will be n-1 and n-2
// column = -1 to init fade in/out values
void SetPixel(int ix, CRGB pixel, int column, int totalColumns)
{
	static int fadeStep;
	static int fadeColumns;
	static int lastColumn;
	static int maxColumn;
	static int fade;
	if (ImgInfo.nFadeInOutFrames) {
		// handle fading
		if (column == -1) {
			fadeColumns = min(totalColumns / 2, ImgInfo.nFadeInOutFrames);
			maxColumn = totalColumns;
			fadeStep = 255 / fadeColumns;
			//Serial.println("fadeStep: " + String(fadeStep) + " fadeColumns: " + String(fadeColumns) + " maxColumn: " + String(maxColumn));
			lastColumn = -1;
			fade = 255;
			return;
		}
		// when the column changes check if we are in the fade areas
		if (column != lastColumn) {
			int realColumn = (ImgInfo.bReverseImage != ImgInfo.bRotate180) ? maxColumn - 1 - column : column;
			if (realColumn <= fadeColumns) {
				// calculate the fade amount
				fade = realColumn * fadeStep;
				fade = constrain(fade, 0, 255);
				// fading up
				//Serial.println("UP col: " + String(realColumn) + " fade: " + String(fade));
			}
			else if (realColumn >= maxColumn - 1 - fadeColumns) {
				// calculate the fade amount
				fade = (maxColumn - 1 - realColumn) * fadeStep;
				fade = constrain(fade, 0, 255);
				// fading down
				//Serial.println("DOWN col: " + String(realColumn) + " fade: " + String(fade));
			}
			else
				fade = 255;
			lastColumn = column;
		}
	}
	else {
		// no fade
		fade = 255;
	}
	int ix1 = 0, ix2 = 0;
	if (ImgInfo.bUpsideDown != ImgInfo.bRotate180) {
		if (ImgInfo.bDoublePixels && !ImgInfo.bShowBuiltInTests) {
			ix1 = AdjustStripIndex(LedInfo.nTotalLeds - 1 - 2 * ix);
			ix2 = AdjustStripIndex(LedInfo.nTotalLeds - 2 - 2 * ix);
		}
		else {
			ix1 = AdjustStripIndex(LedInfo.nTotalLeds - 1 - ix);
		}
	}
	else {
		if (ImgInfo.bDoublePixels && !ImgInfo.bShowBuiltInTests) {
			ix1 = AdjustStripIndex(2 * ix);
			ix2 = AdjustStripIndex(2 * ix + 1);
		}
		else {
			ix1 = AdjustStripIndex(ix);
		}
	}
	if (fade != 255) {
		pixel = pixel.nscale8_video(fade);
		//Serial.println("col: " + String(column) + " fade: " + String(fade));
	}
	leds[ix1] = pixel;
	if (ImgInfo.bDoublePixels && !ImgInfo.bShowBuiltInTests)
		leds[ix2] = pixel;
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
		element = round(LedInfo.nTotalLeds / static_cast<double>(2) * (-cos(i / (PI_SCALE * 100.0)) + 1));
		//Serial.println("elements: " + String(element) + " " + String(last_element));
		if (element > last_element) {
			SetPixel(element, CHSV(element * BuiltinInfo.nRainbowPulseColorScale + BuiltinInfo.nRainbowPulseStartColor, BuiltinInfo.nRainbowPulseSaturation, 255));
			ShowLeds();
			highest_element = max(highest_element, element);
		}
		if (CheckCancel()) {
			break;
		}
		delayMicroseconds(BuiltinInfo.nRainbowPulsePause * 10);
		if (element < last_element) {
			// cleanup the highest one
			SetPixel(highest_element, CRGB::Black);
			SetPixel(element, CRGB::Black);
			ShowLeds();
		}
		last_element = element;
	}
}

/*
	Write a wedge in time, from the middle out
*/
void TestWedge()
{
	int midPoint = LedInfo.nTotalLeds / 2 - 1;
	for (int ix = 0; ix < LedInfo.nTotalLeds / 2; ++ix) {
		SetPixel(midPoint + ix, CRGB(BuiltinInfo.nWedgeRed, BuiltinInfo.nWedgeGreen, BuiltinInfo.nWedgeBlue));
		SetPixel(midPoint - ix, CRGB(BuiltinInfo.nWedgeRed, BuiltinInfo.nWedgeGreen, BuiltinInfo.nWedgeBlue));
		if (!BuiltinInfo.bWedgeFill) {
			if (ix > 1) {
				SetPixel(midPoint + ix - 1, CRGB::Black);
				SetPixel(midPoint - ix + 1, CRGB::Black);
			}
			else {
				SetPixel(midPoint, CRGB::Black);
			}
		}
		ShowLeds();
		delay(ImgInfo.nFrameHold);
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
void rainbow_fill()
{
	byte red = 31;
	byte green = 0;
	byte blue = 0;
	byte state = 0;
	unsigned int color = red << 11; // Colour order is RGB 5+6+5 bits each
	// The colours and state are not initialised so the start colour changes each time the function is called

	for (int i = tft.height() - 1; i >= 0; i--) {
		// Draw a vertical line 1 pixel wide in the selected colour
		tft.drawFastHLine(0, i, tft.width(), color); // in this example tft.width() returns the pixel width of the display
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
		color = red << 11 | green << 5 | blue;
	}
}

//// fill the screen with grey
//void grey_fill()
//{
//	//for (int ix = 0; ix < tft.width(); ++ix) {
//	//	if (ix % 2)
//	//		tft.drawFastVLine(ix, 0, tft.height(), TFT_LIGHTGREY);
//	//	else
//	//		tft.drawFastVLine(ix + 1, 0, tft.height(), TFT_DARKGREY);
//	//}
//	tft.fillRect(0, 0, tft.width(), tft.height(), TFT_DARKGREY);
//}

// draw a progress bar
void DrawProgressBar(int x, int y, int dx, int dy, int percent, bool rect)
{
	if (rect)
		tft.drawRoundRect(x, y, dx, dy, 2, SystemInfo.menuTextColor);
	int fill = (dx - 2) * percent / 100;
	// fill the filled part
	tft.fillRect(x + 1, y + 1, fill, dy - 2, TFT_DARKGREEN);
	// blank the empty part
	tft.fillRect(x + 1 + fill, y + 1, dx - 2 - fill, dy - 2, TFT_BLACK);
}

// save/load settings
// return false if not found or wrong version
bool SaveLoadSettings(bool save, bool autoloadonly, bool ledonly, bool nodisplay)
{
	bool retvalue = true;
	Preferences prefs;
	prefs.begin(prefsName, !save);
	if (save) {
		//Serial.println("saving");
		prefs.putString(prefsVersion, MIW_Version);
		prefs.putBool(prefsAutoload, bAutoLoadSettings);
		// save things
		if (!ledonly) {
			prefs.putBytes(prefsImgInfo, &ImgInfo, sizeof(ImgInfo));
			prefs.putBytes(prefsBuiltInInfo, &BuiltinInfo, sizeof(BuiltinInfo));
			if (!nodisplay)
				WriteMessage("Settings Saved", false, 500);
		}
		// we always do these since they are hardware related
		prefs.putBytes(prefsLedInfo, &LedInfo, sizeof(LedInfo));
		prefs.putBytes(prefsSystemInfo, &SystemInfo, sizeof(SystemInfo));
		prefs.putBytes(prefsBuiltinInfo, &BuiltinInfo, sizeof(BuiltinInfo));
	}
	else {
		// load things
		String vsn = prefs.getString(prefsVersion, "");
		if (vsn == MIW_Version) {
			if (autoloadonly) {
				bAutoLoadSettings = prefs.getBool(prefsAutoload, false);
				//Serial.println("getting autoload: " + String(bAutoLoadSettings));
			}
			else if (!ledonly) {
				prefs.getBytes(prefsImgInfo, &ImgInfo, sizeof(ImgInfo));
				prefs.getBytes(prefsBuiltInInfo, &BuiltinInfo, sizeof(BuiltinInfo));
				int savedFileIndex = currentFileIndex.nFileIndex;
				// we don't know the folder path, so just reset the folder level
				currentFolder = "/";
				setupSDcard();
				GetFileNamesFromSDorBuiltins(currentFolder);
				currentFileIndex.nFileIndex = savedFileIndex;
				// make sure file index isn't too big
				if (currentFileIndex.nFileIndex >= FileNames.size()) {
					currentFileIndex.nFileIndex = 0;
				}
				// set the brightness values since they might have changed
				SetDisplayBrightness(SystemInfo.nDisplayBrightness);
				if (!nodisplay)
					WriteMessage("Settings Loaded", false, 500);
			}
			// these are always done
			prefs.getBytes(prefsLedInfo, &LedInfo, sizeof(LedInfo));
			prefs.getBytes(prefsSystemInfo, &SystemInfo, sizeof(SystemInfo));
			prefs.getBytes(prefsBuiltinInfo, &BuiltinInfo, sizeof(BuiltinInfo));
		}
		else {
			retvalue = false;
			if (!nodisplay)
				WriteMessage("Settings not saved yet", true, 2000);
		}
	}
	prefs.end();
	return retvalue;
}

// delete saved settings
void FactorySettings(MenuItem* menu)
{
	if (GetYesNo("Reset All Settings? (reboot)")) {
		Preferences prefs;
		prefs.begin(prefsName);
		prefs.clear();
		prefs.end();
		ESP.restart();
	}
}

void EraseFlash(MenuItem* menu)
{
	if (GetYesNo("Format EEPROM? (factory reset)")) {
		nvs_flash_erase(); // erase the NVS partition and...
		nvs_flash_init(); // initialize the NVS partition.
		//SaveLoadSettings(true);
	}
}

void ReportCouldNotCreateFile(String target) {
	SendHTML_Header();
	webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
	webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}

#if USE_STANDARD_SD
SDFile UploadFile;
#else
FsFile UploadFile;
#endif

// upload a new file to the Filing system
void handleFileUpload()
{
	HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
											  // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
	if (uploadfile.status == UPLOAD_FILE_START)	{
		String filename = uploadfile.filename;
		String filepath = String("/");
		if (!filename.startsWith("/"))
			filename = "/" + filename;
		Serial.print("Upload File Name: " + filename);
		SD.remove(filename);   // Remove a previous version just to start clean
#if USE_STANDARD_SD
		UploadFile = SD.open(filename, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
#else
		UploadFile = SD.open(filename, O_WRITE | O_CREAT);
#endif
		filename = String();
	}
	else if (uploadfile.status == UPLOAD_FILE_WRITE)
	{
		if (UploadFile)
			UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
	}
	else if (uploadfile.status == UPLOAD_FILE_END)
	{
		if (UploadFile)          // If the file was successfully created
		{
			UploadFile.close();   // Close the file again
			//Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
			webpage = "";
			load_page_header(false);
			webpage += F("<h3>File was successfully uploaded</h3>");
			webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename + "</h2>";
			webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>";
			append_page_footer();
			server.send(200, "text/html", webpage);
			// reload the file list, if not showing built-ins
			if (!ImgInfo.bShowBuiltInTests) {
				GetFileNamesFromSDorBuiltins(currentFolder);
			}
		}
		else
		{
			ReportCouldNotCreateFile("upload");
		}
	}
}

void load_page_header(bool bRefresh) {
	webpage = "<!DOCTYPE html><html>";
	webpage += "<head>";
	webpage += "<title>MagicImageWand</title>";
	webpage += "<META name='viewport' content='width=device-width, initial-scale=1.0'>";
	if (bRefresh)
		webpage += "<META http-equiv='refresh' content='2'>";
	webpage += "<style>";
	webpage += "body{max-width:98%;margin:0 auto;font-family:arial;font-size:100%;text-align:center;color:black;background-color:#888888;}";
	webpage += "ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.17em;overflow:hidden;background-color:#EEEEEE;font-size:1em;}";
	webpage += "li{float:left;border-radius:0.17em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}";
	// fontsize was 65%, changed to 100 to make tabs easier to hit on a phone
	webpage += "li a{display: block;border-radius:0.17em;padding:0.44em 0.44em;text-decoration:none;font-size:100%}";
	webpage += "li a:hover{background-color:#DDDDDD;border-radius:0.17em;font-size:85%}";
	webpage += "section {font-size:0.88em;}";
	webpage += "h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#444444;}";
	webpage += "h2{color:orange;font-size:1.0em;}";
	webpage += "h3{font-size:0.8em;}";
	webpage += "table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:100%;}";
	webpage += "th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}";
	webpage += "tr:nth-child(odd) {background-color:#eeeeee;}";
	webpage += ".rcorners_n {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
	webpage += ".rcorners_m {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
	webpage += ".rcorners_w {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
	webpage += ".column{float:left;width:100%;height:100%;}";
	webpage += ".row:after{content:'';display:table;clear:both;}";
	webpage += "*{box-sizing:border-box;}";
	webpage += "footer{background-color:#AAAAAA; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}";
	webpage += "button{border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:100%;}";
	webpage += ".buttons {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:80%;}";
	webpage += ".buttonsm{border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%; color:white;font-size:70%;}";
	webpage += ".buttonm {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:70%;}";
	webpage += ".buttonw {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:70%;}";
	webpage += "a{font-size:75%;}";
	webpage += "p{font-size:75%;}";
	webpage += "</style></head><body>";
	webpage += "<body><h1>MIW Server<br>";
	webpage + "</h1>";
}

void append_page_footer() {
	webpage += "<ul>";
	webpage += "<li><a href='/'>Home</a></li>";
	webpage += "<li><a href='/download'>Download</a></li>";
	webpage += "<li><a href='/upload'>Upload</a></li>";
	webpage += "<li><a href='/settings'>Settings</a></li>";
	webpage += "<li><a href='/utilities'>Utilities</a></li>";
	webpage += "</ul>";
	webpage += "<footer>Magic Image Wand ";
	webpage += MIW_Version;
	webpage += "</footer>";
	webpage += "</body></html>";
}

void SendHTML_Header() {
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
	load_page_header(false);
	server.sendContent(webpage);
	webpage = "";
}

void SendHTML_Content() {
	server.sendContent(webpage);
	webpage = "";
}

void SendHTML_Stop() {
	server.sendContent("");
	server.client().stop(); // Stop is needed because no content length was sent
}

// set the current macro from its name
void SetMacroIndexFromName(String lookfor)
{
	// strip the "#n " from the start
	if (lookfor[0] == '#')
		lookfor = lookfor.substring(lookfor.indexOf(' ') + 1);
	for (int ix = 0; ix < (sizeof(MacroInfo) / sizeof(*MacroInfo)); ++ix) {
		if (lookfor == MacroInfo[ix].description) {
			ImgInfo.nCurrentMacro = ix;
			break;
		}
	}
}

// set the current file index from the name
void SetFileIndexFromName(String lookfor)
{
	int ix = LookUpFile(lookfor);
	currentFileIndex.nFileCursor = 0;
	currentFileIndex.nFileIndex = ix;
	if (IsFolder(ix)) {
		// change folders
		ProcessFileOrBuiltin();
	}
	else {
		DisplayCurrentFile();
	}
}

// change the current file from the webpage
void WebChangeFile()
{
	if (server.args()) {
		//Serial.println("arg: " + server.arg(0));
		// set the file and reload all the names
		SetFileIndexFromName(server.arg(0));
	}
	HomePage();
}

// change the current macro from the webpage
void WebChangeMacro()
{
	if (server.args()) {
		SetMacroIndexFromName(server.arg(0));
	}
	HomePage();
}

// display the homepage on the web browser
void HomePage() {
	g_nPercentDone = 0;
	bWebRunning = false;
	SendHTML_Header();
	//webpage += "<a href='/download'><button style=\"width:auto\">Download</button></a>";
	//webpage += "<a href='/upload'><button style=\"width:auto\">Upload</button></a>";
	//webpage += "<a href='/settings'><button style='width:auto'>Settings</button></a>";
	//webpage += "<a href='/utilities'><button style='width:auto'>Utilities</button></a>";
	webpage += "<br><h2>" + String("Folder: ") + currentFolder + "</h2>";
	webpage += "<a href='/runimage'><button style='width:90%;font-size:200%;color:#00ff00'>";
	webpage += "Run File:<br>" + FileNames[currentFileIndex.nFileIndex] + "</button></a>";
	webpage += "<br><br>";
	MakeFileForm("/changefile", "newfile", (ImgInfo.bShowBuiltInTests ? NULL : "Select Folder"), WPDD_FILES);
	webpage += "<br>";
	// now lets see if we need to add settings for a builtin
	// this is done by checking for a menu entry for the current
	if (ImgInfo.bShowBuiltInTests && BuiltInFiles[currentFileIndex.nFileIndex].menu) {
		webpage += "<a href='/builtinsettings'><button style='width:50%;font-size:200%;color:#00ff00'>";
		webpage += "Settings:<br>" + FileNames[currentFileIndex.nFileIndex] + "</button></a><br>";
	}
	webpage += "<br>";
	webpage += "<a href='/togglefilesbuiltins'><button style='font-size:150%'>";
	webpage += "Switch to: " + String(ImgInfo.bShowBuiltInTests ? "SD Files" : "Built-Ins") + "</button></a>";
	webpage += "<br><br><br>";
	webpage += "<a href='/runmacro'><button style='width:90%;font-size:200%;color:#00ff00'>";
	webpage += "Run Macro:<br>#" + String(ImgInfo.nCurrentMacro) + " " + MacroInfo[ImgInfo.nCurrentMacro].description + "</button></a>";
	webpage += "</button></a>";
	webpage += "<br><br>";
	MakeFileForm("/changemacro", "newmacro", NULL, WPDD_MACROS);
	webpage += "<br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}

// handle the builtin settings changes
void WebChangeBuiltinSettings()
{
	MenuItem* menu = BuiltInFiles[currentFileIndex.nFileIndex].menu;
	//Serial.println("builtin settings change");
	String str, line, name, stmp;
	double sfloat;
	for (int ix = 0; menu->op != eTerminate; ++ix, ++menu) {
		name = "bi_" + String(ix);
		//Serial.println("id:" + name);
		switch (menu->op) {
		case eText:
			//Serial.println("eText");
			break;
		case eTextInt:
			//Serial.println("eTextInt:" + name);
			if (server.hasArg(name))
				*(int*)(menu->value) = server.arg(name).toInt();
			break;
		case eBool:
			*(bool*)(menu->value) = server.arg(name).length() ? true : false;
			break;
		case eList:	// a dropdown list
			// figure out which one
			if (server.hasArg(name)) {
				for (int ix = 0; ix <= menu->max; ++ix) {
					if (server.arg(name) == String(menu->nameList[ix])) {
						*(int*)menu->value = ix;
					}
				}
			}
			//Serial.println("eList: " + String(server.arg(name)));
			break;
		case eIfEqual:
		case eIfIntEqual:
		case eElse:
		case eEndif:
		case eExit:	// do nothing
		case eTextCurrentFile:
		case eMenu:
		case eBuiltinOptions:
		case eReboot:
		case eMacroList:
		case eTerminate:
			//Serial.println("unsupported menutype from html: " + String(menu->op));
			break;
		default:
			break;
		}
	}
	WebBuiltinSettings();
}

// handle builtin web settings
void WebBuiltinSettings()
{
	load_page_header(false);
	webpage += "<form id='builtinsettings' onchange='document.forms[\"builtinsettings\"].submit()' action='/changebuiltinsettings' method='post'>";
	//webpage += "<form id='builtinsettings' action='/changebuiltinsettings' method='post'>";
	webpage += MenuToHtml(BuiltInFiles[currentFileIndex.nFileIndex].menu);
	webpage += "</form><br>";
	webpage += "<a href='/' style='font-size:125%;color:#f0f0f0'>[Back]</a><br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
}

// toggle files or builtins flag
void WebToggleFilesBuiltins()
{
	ToggleFilesBuiltin(NULL);
	DisplayCurrentFile();
	HomePage();
}

// create an html form to select a file
void MakeFileForm(String action, String name, const char* text, WebPageDropDowns dataType)
{
	webpage += "<form id='" + String(name) + "' action = '" + action + "' method = 'post'>";
	webpage += "<select onchange='document.forms[\"" + String(name) + "\"].submit()' name='" + name + "'>";
	int ix = 0;
	String macroName;
	switch (dataType) {
	case WPDD_FILES:	// fill with bmp filenames
		for (String nm : FileNames) {
			webpage += "<option ";
			if (currentFileIndex.nFileIndex == ix) {
				webpage += "selected='" + String(ix) + "' ";
			}
			webpage += "<value='" + String(ix) + "'>" + nm + "</option>";
			++ix;
		}
		webpage += "</select>";
		if (text)
			webpage += " <input type='submit' value='" + String(text) + "'>";
		webpage += "</form>";
		break;
	case WPDD_MACROS:	// handle macro filenames/descriptions
		for (int ix = 0; ix < (sizeof(MacroInfo) / sizeof(*MacroInfo)); ++ix) {
			webpage += "<option ";
			if (ImgInfo.nCurrentMacro == ix) {
				webpage += "selected='" + String(ix) + "' ";
			}
			// use the description if it is there, else the number
			macroName = MacroInfo[ix].description;
			if (MacroInfo[ix].mSeconds == 0)
				macroName = "Empty";
			macroName = "#" + String(ix) + " " + macroName;
			webpage += "<value='" + String(ix) + "'>" + macroName + "</option>";
		}
		webpage += "</select>";
		if (text)
			webpage += " <input type='submit' value='" + String(text) + "'>";
		webpage += "</form>";
		break;
	}
}

void SelectInput(String heading1, String command, String arg_calling_name)
{
	SendHTML_Header();
	webpage += "<h2>" + heading1 + "</h2>";
	MakeFileForm("/" + command, arg_calling_name, "Download File/Open Folder", WPDD_FILES);
	webpage += "<br><a href='/' style='font-size:125%'>[Back]</a><br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}

// This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
void File_Download()
{
	if (server.args() > 0) { // Arguments were received
		if (server.hasArg("download")) {
			String name = server.arg("download");
			// if this is a folder, we kind of start over by setting the current folder and calling SelectInput again
			// we have the name, but we need the index
			if (IsFolder(name)) {
				SetFileIndexFromName(name);
				SelectInput("Select file to download", "download", "download");
			}
			else {
				SD_file_download(currentFolder + name);
			}
		}
	}
	else
		SelectInput("Select filename to download", "download", "download");
}

void ReportFileNotPresent(String target)
{
	SendHTML_Header();
	webpage += "<h3>File does not exist</h3>";
	webpage += "<a href='/" + target + "'>[Back]</a><br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}

void ReportSDNotPresent() {
	SendHTML_Header();
	webpage += "<h3>No SD Card present</h3>";
	webpage += "<a href='/'>[Back]</a><br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}

void SD_file_download(String filename)
{
	if (bSdCardValid) {
#if USE_STANDARD_SD
		SDFile download = SD.open(filename);
#else
		FsFile download = SD.open(filename);
#endif
		if (filename[0] != '/')
			filename = '/' + filename;
		if (download) {
			server.sendHeader("Content-Type", "text/text");
			server.sendHeader("Content-Disposition", "attachment; filename="
				+ filename.substring(filename.lastIndexOf('/') + 1));
			server.sendHeader("Connection", "close");
			//server.streamFile(download, "application/octet-stream");
#if USE_STANDARD_SD
			server.streamFile(download, "image/bmp");
#else
			server.streamFileX(download, "image/bmp");
#endif
			download.close();
		}
		else ReportFileNotPresent("download");
	}
	else ReportSDNotPresent();
}

void IncreaseRepeatButton()
{
	// This can be for sure made into a universal function like IncreaseButton(Setting, Value)
	  //webpage += String("&nbsp;<a href='/settings/increpeat?var=5'><strong>&#8679;</strong></a>");
	webpage += String("&nbsp;<a href='/settings/increpeat?var=5'><strong>+;</strong></a>");
}

void DecreaseRepeatButton()
{
	// This can be for sure made into a universal function like DecreaseButton(Setting, Value)
	webpage += String("&nbsp;<a href='/settings/decrepeat'><strong>&#8681;</strong></a>");
}

void IncRepeat()
{
	String str = server.uri();
	Serial.println("uri: " + str);
}

#if 0
// display settings form
void FormSettings()
{
	form method = "get" enctype = "application/x-www-form-urlencoded" action = "/html/codes/html_form_handler.cfm" >

		<p>
		<label>Name
		<input type = "text" name = "customer_name" required>
		< / label>
		< / p>

		<p>
		<label>Phone
		<input type = "tel" name = "phone_number">
		< / label>
		< / p>

		<p>
		<label>Email
		<input type = "email" name = "email_address">
		< / label>
		< / p>

		<fieldset>
		<legend>Which taxi do you require ? < / legend>
		<p><label> <input type = "radio" name = "taxi" required value = "car"> Car < / label>< / p>
		<p><label> <input type = "radio" name = "taxi" required value = "van"> Van < / label>< / p>
		<p><label> <input type = "radio" name = "taxi" required value = "tuktuk"> Tuk Tuk < / label>< / p>
		< / fieldset>

		<fieldset>
		<legend>Extras< / legend>
		<p><label> <input type = "checkbox" name = "extras" value = "baby"> Baby Seat < / label>< / p>
		<p><label> <input type = "checkbox" name = "extras" value = "wheelchair"> Wheelchair Access < / label>< / p>
		<p><label> <input type = "checkbox" name = "extras" value = "tip"> Stock Tip < / label>< / p>
		< / fieldset>

		<p>
		<label>Pickup Date / Time
		<input type = "datetime-local" name = "pickup_time" required>
		< / label>
		< / p>

		<p>
		<label>Pickup Place
		<select id = "pickup_place" name = "pickup_place">
		<option value = "" selected = "selected">Select One< / option>
		<option value = "office" >Taxi Office< / option>
		<option value = "town_hall" >Town Hall< / option>
		<option value = "telepathy" >We'll Guess!</option>
		< / select >
		< / label>
		< / p>

		<p>
		<label>Dropoff Place
		<input type = "text" name = "dropoff_place" required list = "destinations">
		< / label>

		<datalist id = "destinations">
		<option value = "Airport">
		<option value = "Beach">
		<option value = "Fred Flintstone's House">
		< / datalist>
		< / p>

		<p>
		<label>Special Instructions
		<textarea name = "comments" maxlength = "500">< / textarea>
		< / label>
		< / p>

		<p><button>Submit Booking< / button>< / p>

		< / form>
}
#endif

// these are used for the list of things that can be set on the settings web page
enum WEB_SETTINGS_TYPE {
	WST_NUMBER,		// a number value, decimals 0 for integer
	WST_BOOL,		// boolean values
	WST_STRING,		// a string of characters
	WST_TEXT_ONLY,	// text that will display as H2, use to separate sections
	WST_SLIDER,		// a slider control
};
typedef WEB_SETTINGS_TYPE WEB_SETTINGS_TYPE;
struct WEB_SETTINGS {
	WEB_SETTINGS_TYPE type;		// what type of data
	bool* display;				// if not NULL, compare with displayTest to display this line
	bool displayTest;			// compare with display to see if this line should display or not
	char* text;					// show on page
	char* name;					// the data name
	void* data;					// a pointer to the data
	int width;					// how wide to make the field
	int decimals;				// decimals for floats, although stored as ints, also used for max string length
	int min, max;				// not used yet, TODO, but will limit range of numbers
};
typedef WEB_SETTINGS WebSettings;
WebSettings WebSettingsPage[] = {
	{WST_TEXT_ONLY,NULL,true,"Image Settings",NULL},
	{WST_BOOL,NULL,true,"Use Fixed Image Time","use_fixed_time",&ImgInfo.bFixedTime},
	{WST_NUMBER,&ImgInfo.bFixedTime,true,"Fixed Time Value (S)","fixed_time",&ImgInfo.nFixedImageTime,4,0},
	{WST_NUMBER,&ImgInfo.bFixedTime,false,"Column Time(mS)","column_time",&ImgInfo.nFrameHold,4,0},
	{WST_NUMBER,&ImgInfo.bFixedTime,false,"Stutter Time(mS)","stutter_time",&ImgInfo.nStutterTime,4,0},
	{WST_BOOL,NULL,true,"Stutter White","stutter_white",&ImgInfo.bStutterWhite},
	//{WST_SLIDER,&ImgInfo.bFixedTime,false,"Column Time(mS)","column_time_slider",&ImgInfo.nFrameHold,4,0,0,500},
	{WST_NUMBER,NULL,true,"Start Delay (S)","start_delay",&ImgInfo.startDelay,4,1},
	{WST_BOOL,NULL,true,"Upside Down","upside_down",&ImgInfo.bUpsideDown},
	{WST_BOOL,NULL,true,"Rotate 180","rotate_180",&ImgInfo.bRotate180},
	{WST_BOOL,NULL,true,"Reverse Walk (left-right)","reverse_walk",&ImgInfo.bReverseImage},
	{WST_BOOL,NULL,true,"Play Mirror Image","mirror_image",&ImgInfo.bMirrorPlayImage},
	{WST_NUMBER,&ImgInfo.bMirrorPlayImage,true,"Middle Mirror Delay (S)","mirror_delay",&ImgInfo.nMirrorDelay,4,1},
	{WST_BOOL,NULL,true,"Scale Height to Fit Pixels","scale_height",&ImgInfo.bScaleHeight},
	{WST_BOOL,NULL,true,"Double Pixels (144 to 288)","double_pixels",&ImgInfo.bDoublePixels},
	{WST_BOOL,NULL,true,"Chain Images","chain_images",&ImgInfo.bChainFiles},
	{WST_NUMBER,&ImgInfo.bChainFiles,true,"Chain Delay (S)","chain_delay",&ImgInfo.nChainDelay,4,1},
	{WST_NUMBER,&ImgInfo.bChainFiles,true,"Chain Repeats","chain_repeats",&ImgInfo.nChainRepeats,4,0},
	{WST_TEXT_ONLY,NULL,true,"File Repeat Settings",NULL},
	{WST_NUMBER,NULL,true,"Repeat Count","repeat_count",&ImgInfo.repeatCount,4,0},
	{WST_NUMBER,NULL,true,"Repeat Delay (S)","repeat_delay",&ImgInfo.repeatDelay,4,1},
	{WST_TEXT_ONLY,NULL,true,"Macro Repeat Settings",NULL},
	{WST_NUMBER,NULL,true,"Repeat Count","macro_repeat_count",&ImgInfo.nRepeatCountMacro,4,0},
	{WST_NUMBER,NULL,true,"Repeat Delay (S)","macro_repeat_delay",&ImgInfo.nRepeatWaitMacro,4,1},
	{WST_TEXT_ONLY,NULL,true,"LED Settings",NULL},
	{WST_NUMBER,NULL,true,"LED Brightness (1-255)","LED_brightness",&LedInfo.nLEDBrightness,4,0},
	{WST_BOOL,NULL,true,"Gamma Correction","gamma_correction",&LedInfo.bGammaCorrection},
	//{WST_TEXT_ONLY,NULL,true,"DMX512 Settings",NULL},
	//{WST_BOOL,NULL,true,"DMX Enabled","dmx_enabled",&SystemInfo.bRunArtNetDMX},
	//{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Art-Net Name","artnet_name",&SystemInfo.cArtNetName,14,sizeof(SystemInfo.cArtNetName)},
	//{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Network to Connect To","network_name",&SystemInfo.cNetworkName,20,sizeof(SystemInfo.cNetworkName)},
	//{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Password","password",&SystemInfo.cNetworkPassword,30,sizeof(SystemInfo.cNetworkPassword)},
	//{WST_BOOL,&SystemInfo.bRunArtNetDMX,true,"Universe Start 1 (off for 0)","universe_start",&SystemInfo.bStartUniverseOne},
};

// change the settings from the web page
void WebChangeSettings()
{
	if (server.args()) {
		void* lastData = NULL;
		void* thisData = NULL;
		//Serial.println("argcnt: " + String(server.args()));
		for (WebSettings val : WebSettingsPage) {
			thisData = val.data;
			//if (thisData == lastData)
			//	continue;
			lastData = thisData;
			if (val.type != WST_BOOL && !server.hasArg(val.name))
				continue;
			//Serial.println(String(val.name) + ": ~" + server.arg(val.name) + "~");
			switch (val.type) {
			case WST_NUMBER:
				*(int*)(val.data) = (int)(server.arg(val.name).toDouble() * pow10(val.decimals));
				break;
			case WST_SLIDER:
				//Serial.println("slider value:" + server.arg(val.name));
				*(int*)(val.data) = (int)(server.arg(val.name).toDouble() * pow10(val.decimals));
				break;
			case WST_BOOL:
				*(bool*)(val.data) = server.arg(val.name).length() ? true : false;
				break;
			case WST_STRING:
				memset(val.data, 0, val.decimals);
				strncpy((char*)(val.data), server.arg(val.name).c_str(), val.decimals - 1);
				break;
			case WST_TEXT_ONLY:
				break;
			}
		}
	}
	//Serial.println("fixed: " + String(server.arg("fixed_time")));
	WebShowSettings();
}

void WebShowSettings() {
	String stmp;
	double sfloat;
	bool bDoneFirst = false;
	load_page_header(false);
	//webpage += ".slidecontainer{  width: 100 %;	}";
	webpage += "<form id='allsettings' onchange='document.forms[\"allsettings\"].submit()' action='/changesettings' method='post'>";
	//webpage += "<form onchange='document.getElementById(\"settingssubmitbutton\").disabled=false' action='/changesettings' method='post'>";
	for (WebSettings val : WebSettingsPage) {
		if (val.display && (*(val.display) != val.displayTest))
			continue;
		if (bDoneFirst)
			webpage += "<br>";
		else
			bDoneFirst = true;
		switch (val.type) {
		case WST_NUMBER:
			webpage += "<label>" + String(val.text) + ": ";
			stmp = String(*(int*)(val.data));
			sfloat = stmp.toDouble() / pow10(val.decimals);
			stmp = String(sfloat, val.decimals);
			webpage += "<input type='text' name='" + String(val.name) + "' size='" + String(val.width) + "' value='" + stmp + "'>";
			webpage += "</label>";
			break;
		case WST_SLIDER:
			//webpage += "<label>" + String(val.text) + ": ";
			stmp = String(*(int*)(val.data));
			sfloat = stmp.toDouble() / pow10(val.decimals);
			stmp = String(sfloat, val.decimals);
			webpage += "<input type='range' name='" + String(val.name) + "' min='" + String(val.min) + "' max='" + String(val.max) + "' value='" + stmp + "' class='slider' id='" + val.name + "'>";
			//webpage += "</label>";
			break;
		case WST_BOOL:
			webpage += "<label>" + String(val.text) + ": ";
			webpage += "<input type='checkbox' name='" + String(val.name) + "' value='" + val.name + "'";
			if (*(bool*)(val.data))
				webpage += " checked='checked'";
			webpage += ">";
			webpage += "</label>";
			break;
		case WST_STRING:
			webpage += "<label>" + String(val.text) + ": ";
			webpage += "<input type='text' name='" + String(val.name) + "' size='" + String(val.width) + "' value='" + (char*)(val.data) + "'>";
			webpage += "</label>";
			break;
		case WST_TEXT_ONLY:
			webpage += "<h2>" + String(val.text) + "</h2>";
			bDoneFirst = false;
			break;
		}
	}
	//webpage += "<br><input type='range' name='test' min='1' max='255' value='50' class='slider' id='myRange'>";
	//webpage += "<br><br><input id='settingssubmitbutton' disabled type='submit' value='Update MIW'>";
	webpage += "</form><br>";
	//if (ImgInfo.bFixedTime) {
	//	webpage += String("<p>Fixed Image Time: ") + String(ImgInfo.nFixedImageTime) + " S";
	//}
	//else {
	//	webpage += String("<p>Column Time: ") + String(ImgInfo.nFrameHold) + " mS";
	//}
	////IncreaseRepeatButton();
	////DecreaseRepeatButton();
	append_page_footer();
	server.send(200, "text/html", webpage);
}

// interlock so repeat of webpage won't reboot, verifyreboot has to be called first
bool b_RebootArmed = false;
void UtilitiesPage()
{
	load_page_header(false);
	webpage += "<h2>Utilities</h2>";
	webpage += "<a href='/verifyfiledelete'><button style='width:80%;font-size:150%;color:#ffffff'>Delete File: " + FileNames[currentFileIndex.nFileIndex] + "</button></a>";
	webpage += "<br><br><a href='/verifyrebootsystem'><button style='width:50%;font-size:150%;color:#ffffff'>Reboot System</button></a>";
	webpage += "<br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
	b_RebootArmed = false;
}

// verify reboot
void VerifyRebootSystem()
{
	load_page_header(false);
	webpage += "<h2>Confirm System Reboot</h2>";
	webpage += "<a href='/utilities'><button style='width:30%;font-size:150%;color:#ffffff'>Cancel</button></a>";
	webpage += "<a href='/rebootsystem'><button style='width:30%;font-size:150%;color:#ffffff'> Reboot</button></a>";
	webpage += "<br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
	b_RebootArmed = true;
}

// reboot system from web page
void RebootSystem()
{
	if (b_RebootArmed) {
		ESP.restart();
	}
	else {
		UtilitiesPage();
	}
}

// verify file delete
void VerifyFileDelete()
{
	load_page_header(false);
	webpage += "<h2>Confirm File Delete: " + FileNames[currentFileIndex.nFileIndex] + "</h2>";
	webpage += "<a href='/utilities'><button style='width:30%;font-size:150%;color:#ffffff'>Cancel</button></a>";
	webpage += "<a href='/dofiledelete'><button style='width:30%;font-size:150%;color:#ffffff'> Delete</button></a>";
	webpage += "<br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
}

// do the actual file deletion
void DoFileDelete()
{
	SD.remove(currentFolder + FileNames[currentFileIndex.nFileIndex]);
	GetFileNamesFromSDorBuiltins(currentFolder);
	UtilitiesPage();
}

// run the current selected image from the web button
void WebRunMacro()
{
	if (!bRunningMacro && g_nPercentDone > 0) {
		HomePage();
		g_nPercentDone = 0;
		bWebRunning = false;
		return;
	}
	webpage = "";
	load_page_header(true);
	webpage += "<h2>Running: ";
	webpage += MacroInfo[ImgInfo.nCurrentMacro].description;
	webpage += " " + String(g_nPercentDone) + "%<br>";
	if (!bRunningMacro)
		webpage += MacroInfo[ImgInfo.nCurrentMacro].fileNames[0];
	else
		webpage += currentFolder + FileNames[currentFileIndex.nFileIndex];
	webpage += "</h2>";
	webpage += "<a href='/cancel'><button style='width:50%;font-size:200%;color:#ff0000'>";
	webpage += "Cancel</button></a>";
	webpage += "<br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
	// run the file
	if (!bWebRunning) {
		bWebRunning = true;
		g_nPercentDone = 0;
		bCancelRun = bCancelMacro = false;
		MacroLoadRun(NULL, true);
		DisplayCurrentFile();
	}
}

void WebCancel()
{
	bWebRunning = false;
	bCancelRun = bCancelMacro = true;
	HomePage();
}

// map a menuitem list to html
// recurses for each eIf
String MenuToHtml(MenuItem* pMenu, bool bActive, int nLevel)
{
	static String str;
	static int ix;
	static MenuItem* menu, *StartMenu;
	static String line, name, stmp;
	double sfloat;
	if (nLevel == 0) {
		StartMenu = pMenu;
		str = "";
		ix = 0;
	}
	else {
		++ix;
	}
	for (menu = &StartMenu[ix]; menu->op != eTerminate; ++ix, ++menu) {
		if (!bActive) {
			// skip if not one of the if else parts when not active
			switch (menu->op) {
			case eIfEqual:
			case eIfIntEqual:
			case eElse:
			case eEndif:
				break;
			default:
				continue;
				break;
			}
		}
		name = "bi_" + String(ix);
		//Serial.println("name: " + name);
		// keep the line without %x's
		line = menu->text;
		if (line.indexOf("%d.%d") >= 0) {
			line.remove(line.indexOf("%d.%d"), 5);
		}
		while (line.indexOf('%') >= 0) {
			line.remove(line.indexOf('%'), 2);
		}
		switch (menu->op) {
		case eText:
			str += String("<p>") + line + "</p>";
			break;
		case eTextInt:
			str += "<label>" + line;
			stmp = String(*(int*)(menu->value));
			sfloat = stmp.toDouble() / pow10(menu->decimals);
			stmp = String(sfloat, menu->decimals);
			str += "<input type='text' name='" + name + "' size='" + String(5) + "' value='" + stmp + "'>";
			str += "</label>";
			break;
		case eBool:	// show the two text choices in (...)
			str += "<label>" + line + " (" + menu->on + "&#x2611; | " + menu->off + "&#9633;)";
			str += "<input type='checkbox' name='" + name + "' value='" + name + "'";
			if (*(bool*)(menu->value))
				str += " checked='checked'";
			str += ">";
			str += "</label>";
			break;
		case eList:	// a dropdown list
			str += "<label>" + line;
			str += "<select name='" + name + "'>";
			for (int ix = 0; ix <= menu->max; ++ix) {
				str += "<option ";
				if (ix == *(int*)(menu->value))
					str += "selected='" + String(ix) + "' ";
				str += "<value='" + String(ix) + "'>" + menu->nameList[ix] + "</option>";
			}
			str += "</select>";
			str += "</label>";
			break;
		case eExit:
			break;
		case eIfEqual:
			MenuToHtml(NULL, *(bool*)(menu->value) == (menu->min ? true : false), nLevel + 1);
			continue;
			break;
		case eIfIntEqual:
			MenuToHtml(NULL, *(int*)(menu->value) == (menu->min), nLevel + 1);
			continue;
			break;
		case eElse:
			bActive = !bActive;
			break;
		case eEndif:
			return str;
			break;
		case eTextCurrentFile:
		case eMenu:
		case eBuiltinOptions:
		case eReboot:
		case eMacroList:
		case eTerminate:
			//Serial.println("unsupported menutype to html: " + String(menu->op));
			break;
		default:
			break;
		}
		if (bActive) {
			str += "<br>";
		}
	}
	return str;
}

// run the current selected image from the web button
void WebRunImage()
{
	if (bWebRunning && !bIsRunning) {
		HomePage();
		g_nPercentDone = 0;
		bWebRunning = false;
		return;
	}
	webpage = "";
	load_page_header(true);
	webpage += "<h2>Running: ";
	webpage += FileNames[currentFileIndex.nFileIndex];
	webpage += " " + String(g_nPercentDone) + "%";
	webpage += "</h2>";
	webpage += "<a href='/cancel'><button style='width:50%;font-size:200%;color:#ff0000'>";
	webpage += "Cancel</button></a>";
	webpage += "<br><br>";
	// now lets see if we need to add settings for a builtin
	// this is done by checking for a menu entry for the current
	if (ImgInfo.bShowBuiltInTests && BuiltInFiles[currentFileIndex.nFileIndex].menu == LedLightBarMenu) {
		webpage += MenuToHtml(BuiltInFiles[currentFileIndex.nFileIndex].menu);
	}
	append_page_footer();
	server.send(200, "text/html", webpage);
	// run the file
	if (!bWebRunning) {
		bWebRunning = true;
		g_nPercentDone = 0;
		bCancelRun = bCancelMacro = false;
		ProcessFileOrBuiltin();
		DisplayCurrentFile();
	}
}

void File_Upload()
{
	load_page_header(false);
	webpage += "<h2>Select File to Upload</h2>";
	webpage += "<FORM action='/fupload' method='post' enctype='multipart/form-data'>";
	webpage += "<input class='buttons' style='width:75%' type='file' name='fupload' id = 'fupload' value=''><br>";
	webpage += "<br><button class='buttons' style='width:75%' type='submit'>Upload File</button><br>";
	webpage += "<br><a href='/' style='font-size:125%'>[Back]</a><br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
}

// show on leds or display
// mode 0 is normal, mode 1 is prepare for LCD, mode 2 is reset to normal
// mode 3 writes to LCD, 4 adds a pixel to the buffer
// mode 5 turns the LED's off, used for previewing the built-ins
constexpr int BMP_LCD_SKIP_TOP = 4;
void ShowLeds(int mode, CRGB colorval, int imgHeight)
{
	static bool bNoLED = false;	// don't show the LED's
	static bool bSkipCol = false;
	static bool bSkipRow = false;
	static bool bHalfSize = false;
	static int row = 0;
	static int col = 0;
	static int top = 0;	// how much to ignore on the top of the column
	static uint16_t* scrBuf = nullptr;
	uint16_t color;
	uint16_t sbcolor = 0;
	int height;
	// reset the sleep timer
	ResetSleepAndDimTimers();
	switch (mode) {
	case 0:
		// see if need to do the stutter time
		if (ImgInfo.nStutterTime) {
			// copy the led array
			memcpy(tmpLeds, leds, sizeof(CRGB) * LedInfo.nTotalLeds);
			// fill with black or white
			memset(leds, ImgInfo.bStutterWhite ? 0xff : 0x00, sizeof(CRGB) * LedInfo.nTotalLeds);
			FastLED.show();
			// wait for the specified mSeconds
			unsigned long startmS = millis();
			while (millis() < startmS + ImgInfo.nStutterTime)
				ResetSleepAndDimTimers();
			// restore the leds
			memcpy(leds, tmpLeds, sizeof(CRGB) * LedInfo.nTotalLeds);
		}
		// send it to the LED's
		if (!bNoLED) {
			FastLED.show();
		}
		// check if LCD also needs updating
		if (scrBuf != nullptr) {
			ShowLeds(3);
		}
		break;
	case 1:	// initialize
		row = col = 0;
		bSkipCol = false;
		bSkipRow = false;
		tft.fillScreen(TFT_BLACK);
		FastLED.clearData();
		scrBuf = (uint16_t*)calloc(tft.height(), sizeof(uint16_t));
		if (SystemInfo.bShowProgress && bIsRunning) {
			// leave some room for the progress bar
			top = 3;
		}
		else {
			top = 0;
		}
		bHalfSize = imgHeight > 144;
		break;
	case 2:	// clean up and leave
		free(scrBuf);
		scrBuf = NULL;
		bNoLED = false;
		break;
	case 3:	// output to tft lCD using data from the LED buffer
		// sanity check, scrBuf must not be null
		if (scrBuf == nullptr)
			break;
		// get LCD column data from leds array
		height = tft.height();
		height = constrain(height, 0, LedInfo.nTotalLeds - BMP_LCD_SKIP_TOP);
		for (int ix = top; ix < height - top; ++ix) {
			color = tft.color565(leds[ix + BMP_LCD_SKIP_TOP].r, leds[ix + BMP_LCD_SKIP_TOP].g, leds[ix + BMP_LCD_SKIP_TOP].b);
			sbcolor;
			// the memory image colors are byte swapped
			swab(&color, &sbcolor, sizeof(uint16_t));
			scrBuf[ix - top] = sbcolor;
		}
		row = 0;
		if (bHalfSize) {
			bSkipCol = !bSkipCol;
		}
		if (!bSkipCol) {
			tft.pushRect(col, top, 1, tft.height() - top, scrBuf);
			++col;
			if (col >= tft.width()) {
				tft.fillRect(0, top, tft.width(), tft.height() - top, TFT_BLACK);
				col = 0;
			}
		}
		break;
	case 4:	// add to buffer
		if (bHalfSize) {
			bSkipRow = !bSkipRow;
		}
		if (!bSkipRow) {
			// skip some rows because the LCD is too small
			if (row >= BMP_LCD_SKIP_TOP + top) {
				int rix = row - (BMP_LCD_SKIP_TOP + top);
				rix = constrain(rix, 0, tft.height() - 1);
				color = tft.color565(colorval.r, colorval.g, colorval.b);
				// the memory image colors are byte swapped
				swab(&color, &sbcolor, sizeof(uint16_t));
				scrBuf[rix] = sbcolor;
			}
			++row;
		}
		break;
	case 5:	// turn LED's off
		bNoLED = true;
		break;
	default:
		break;
	}
}

// read the battery level, LiIon cells are 4.2 fully charged and should not be discharged below 2.75
// smoothing of the reading is done using an exponential moving average
int ReadBattery(int* raw)
{
	const float alpha = 0.9;
	static float eSmooth = 0.0;
	int percent;
	float nextLevel;
	for (int tries = 0; tries < 5; ++tries) {
		nextLevel = (float)analogRead(BATTERY_SENSOR_GPIO);
		// calculate the next value
		eSmooth = (alpha * eSmooth) + ((1 - alpha) * nextLevel);
		// calculate the %
		if (eSmooth >= SystemInfo.nBatteryFullLevel)
			percent = 100;
		else if (eSmooth <= SystemInfo.nBatteryEmptyLevel)
			percent = 0;
		else {
			percent = (eSmooth - SystemInfo.nBatteryEmptyLevel) * 100 / (SystemInfo.nBatteryFullLevel - SystemInfo.nBatteryEmptyLevel);
		}
		delay(2);
	}
	if (raw)
		*raw = (int)eSmooth;
	if (percent == 0) {
		static int count0 = 0;
		// we need at least 5 0's before claiming no power, it takes a few cycles to stabilize
		if (count0++ > 5) {
			SystemInfo.bCriticalBatteryLevel = true;
		}
	}
	return percent;
}

// this code shows the battery on the main display when menu is NULL
// otherwise it shows the current raw integer readings of the battery sensor
void ShowBattery(MenuItem* menu)
{
	static int percent = 0, raw = 0;
	if (menu)
		ClearScreen();
	static unsigned long showtime = 0;
	while (!menu || ReadButton() != BTN_LONG) {
		percent = ReadBattery(&raw);
		if (millis() > showtime + 1000) {
			if (menu) {
				DisplayLine(0, "Battery: " + String(percent) + "%", SystemInfo.menuTextColor);
				DisplayLine(1, "Raw Battery: " + String(raw), SystemInfo.menuTextColor);
				DisplayLine(3, "Long Press to Cancel", SystemInfo.menuTextColor);
			}
			else {
				int sh = BatterySprite.height();
				BatterySprite.fillSprite(TFT_BLACK);
				BatterySprite.setTextColor(SystemInfo.menuTextColor);
				// show % full
				BatterySprite.fillRect(0, sh - BATTERY_BAR_HEIGHT - 2, percent, BATTERY_BAR_HEIGHT, SystemInfo.menuTextColor);
				// thin line rest of line
				BatterySprite.fillRect(percent, sh - 4, 100 - percent, 2, SystemInfo.menuTextColor);
				// show the text above the bar
				String pc = String(percent) + "%";
				BatterySprite.drawString(pc, 100 - tft.textWidth(pc) - 2, 0);
				// push the sprite to the display
				BatterySprite.pushSprite(tft.width() - 101, tft.height() - sh + 2);
				//DisplayLine(MENU_LINES - 1, "          Battery: " + String(percent) + "%", SystemInfo.menuTextColor);
			}
			showtime = millis();
		}
		if (menu)
			delay(100);
		else
			break;
	}
}

// this code shows "the current file position and count on the main display when menu is NULL
void ShowFilePosition()
{
	FileCountSprite.fillSprite(TFT_BLACK);
	FileCountSprite.setTextColor(SystemInfo.menuTextColor);
	// see if this is the root, we need to modify the numbers if so
	int nAdjust = currentFolder.length() == 1 ? 0 : 1;
	// show the text with the current file index and total
	String pc = " " + String(currentFileIndex.nFileIndex + 1 - nAdjust) + " / " + String(FileNames.size() - nAdjust);
	FileCountSprite.drawString(pc, 0, 0);
	// push the sprite to the display
	FileCountSprite.pushSprite(0, tft.height() - FileCountSprite.height() + 2);
}

// load the filename filter, also used for setting macro names
void SetFilter(MenuItem* menu)
{
	if (menu->op == eBool) {
		*(bool*)menu->value = !*(bool*)menu->value;
	}
	if (menu->op != eBool || *(bool*)menu->value) {
		ClearScreen();
		String upperLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_@#$%^&|";
		String lowerLetters = "abcdefghijklmnopqrstuvwxyz0123456789 -_@#$%^&|";
		String letters = upperLetters;
		bool bUpper = true;
		CRotaryDialButton::Button button = BTN_NONE;
		bool done = false;
		if (menu->op == eBool) {
			DisplayLine(5, "Rotate dial to select, Click appends char, '|' separates OR fields", SystemInfo.menuTextColor);
			DisplayLine(6, "Long press dial exits, BTN0 deletes last char, BTN0 Long clears text", SystemInfo.menuTextColor);
		}
		else {
			DisplayLine(5, "Rotate dial to select, Click appends char, BTN1 Long toggles case", SystemInfo.menuTextColor);
			DisplayLine(6, "Long press dial exits, BTN0 deletes last char, BTN0 Long clears text", SystemInfo.menuTextColor);
		}
		int nLetterIndex = 0;
		// redraw screen only when necessary
		bool bRedraw = true;
		const int partA = 13;	// half the alphabet
		do {
			if (bRedraw) {
				DisplayLine(0, nameFilter, SystemInfo.menuTextColor);
				// draw the text
				DisplayLine(1, letters.substring(0, partA), SystemInfo.menuTextColor);
				DisplayLine(2, letters.substring(partA, partA * 2), SystemInfo.menuTextColor);
				DisplayLine(3, letters.substring(partA * 2), SystemInfo.menuTextColor);
				// figure out which letter to hilite
				int y = nLetterIndex / partA;
				y = constrain(y, 0, 2);
				int x = tft.textWidth(letters.substring(y * partA, nLetterIndex));
				char ch[2] = { 0 };
				ch[0] = letters[nLetterIndex];
				// the width calculation for ' ' is 0 (an error!), so we use something close
				if (ch[0] == ' ')
					ch[0] = '|';
				if (SystemInfo.bMenuStar) {
					tft.drawChar(x + 1, tft.fontHeight() * (y + 2) - 6, letters[nLetterIndex], TFT_WHITE, TFT_BLACK, 1);
				}
				else {
					tft.fillRect(x + 1, tft.fontHeight() * (y + 1) - 4, tft.textWidth(ch), (y + 2) + tft.fontHeight(), SystemInfo.menuTextColor);
					tft.drawChar(x + 1, tft.fontHeight() * (y + 2) - 6, letters[nLetterIndex], TFT_BLACK, TFT_BLACK, 1);
				}
				bRedraw = false;
			}
			button = ReadButton();
			switch (button) {
			case BTN_NONE:
			case BTN_B1_CLICK:
			case BTN_B2_LONG:
				break;
			case BTN_B1_LONG:
				if (menu->op != eBool) {
					bUpper = !bUpper;
					letters = bUpper ? upperLetters : lowerLetters;
					bRedraw = true;
				}
				break;
			case BTN_LEFT:
				if (nLetterIndex)
					--nLetterIndex;
				else
					nLetterIndex = letters.length() - 1;
				bRedraw = true;
				break;
			case BTN_RIGHT:
				if (nLetterIndex < letters.length() - 1)
					++nLetterIndex;
				else
					nLetterIndex = 0;
				bRedraw = true;
				break;
			case BTN_SELECT:
				nameFilter += letters[nLetterIndex];
				bRedraw = true;
				break;
			case BTN_B0_CLICK:
				if (nameFilter.length())
					nameFilter = nameFilter.substring(0, nameFilter.length() - 1);
				bRedraw = true;
				break;
			case BTN_B0_LONG:
				nameFilter.clear();
				bRedraw = true;
				break;
			case BTN_LONG:
				done = true;
				break;
			}
		} while (!done);
	}
	if (menu->op == eBool)
		GetFileNamesFromSDorBuiltins(currentFolder);
}

// set the screen rotation to the correct value, 0-3, allocate the screen memory
// -1 goes to the next one
void SetScreenRotation(int rot)
{
	if (rot == -1) {
		++SystemInfo.nDisplayRotation;
	}
	else {
		SystemInfo.nDisplayRotation = rot;
	}
	SystemInfo.nDisplayRotation %= 4;
	tft.setRotation(SystemInfo.nDisplayRotation);
	nMenuLineCount = tft.height() / tft.fontHeight();
	TextLines.resize(nMenuLineCount);
}

// read the light sensor
// smoothing of the reading is done using an exponential moving average
int ReadLightSensor()
{
	const float alpha = 0.9;
	static float eSmooth = 0.0;
	float nextLevel;
	for (int tries = 0; tries < 5; ++tries) {
		nextLevel = (float)analogRead(LIGHT_SENSOR_GPIO);
		// calculate the next value
		eSmooth = (alpha * eSmooth) + ((1 - alpha) * nextLevel);
		delay(2);
	}
	return (int)eSmooth;
}

// this code shows the light sensor value on the display
void ShowLightSensor(MenuItem* menu)
{
	static int nLightValue = 0;
	ClearScreen();
	DisplayLine(2, "Long Press to Cancel", SystemInfo.menuTextColor);
	while (!menu || ReadButton() != BTN_LONG) {
		nLightValue = ReadLightSensor();
		DisplayLine(0, "Light Sensor: " + String(nLightValue), SystemInfo.menuTextColor);
		delay(100);
	}
}

// calculate the LED brightness from the ambient light
void LightSensorLedBrightness()
{
	int sensor = ReadLightSensor();
	int percent = map(sensor, SystemInfo.nLightSensorDim, SystemInfo.nLightSensorBright, 0, 100);
	percent = constrain(percent, SystemInfo.nDisplayDimValue, SystemInfo.nDisplayBrightness);
	SetDisplayBrightness(percent);
	//Serial.println("led: " + String(percent));
}

//void UpdateFilter(MenuItem* menu, int flag)
//{
//}

// show the update bin file progress
void ShowUpdateProgress(size_t x, size_t total)
{
	ShowProgressBar(x * 100 / total);
}

// see if there is an update bin file in the SD slot
void CheckUpdateBin(MenuItem* menu)
{
	const char* binFileName = "/MagicImageWand.bin";
	if (SD.exists(binFileName)) {
		if (GetYesNo("Load New Firmware?")) {
			ClearScreen();
			DisplayLine(2, "loading...");
#if USE_STANDARD_SD
			SDFile binFile;
			binFile = SD.open(binFileName);
			if (binFileName) {
#else
			FsFile binFile;
			binFile = SD.open(binFileName);
			if (binFile.getError() == 0) {
#endif
				size_t binSize = binFile.size();
				//Serial.println("size: " + String(binSize));
				Update.begin(binSize);
				Update.onProgress(ShowUpdateProgress);
				size_t bytesWritten = Update.writeStream(binFile);
				//Serial.println("written: " + String(bytesWritten));
				Update.end();
				binFile.close();
				ClearScreen();
				if (GetYesNo("Delete BIN file?")) {
					SD.remove(binFileName);
				}
				ClearScreen();
				ESP.restart();
			}
		}
	}
	else {
		WriteMessage("MagicImageFile.BIN missing", true);
	}
}

void TestLEDs(int delay)
{
	for (int i = 0; i < LedInfo.nTotalLeds; i++) {
		leds[i] = CRGB(127, 0, 0);
	}
	ShowLeds();
	vTaskDelay(delay / portTICK_PERIOD_MS);
	for (int i = 0; i < LedInfo.nTotalLeds; i++) {
		leds[i] = CRGB(0, 127, 0);
	}
	ShowLeds();
	vTaskDelay(delay / portTICK_PERIOD_MS);
	for (int i = 0; i < LedInfo.nTotalLeds; i++) {
		leds[i] = CRGB(0, 0, 127);
	}
	ShowLeds();
	vTaskDelay(delay / portTICK_PERIOD_MS);
	for (int i = 0; i < LedInfo.nTotalLeds; i++) {
		leds[i] = CRGB(0, 0, 0);
	}
	ShowLeds();
}

// connect to wifi – returns true if successful or false if not
boolean ConnectWifi(void)
{
	boolean state = true;
	int i = 0;

	WiFi.begin(SystemInfo.cNetworkName, SystemInfo.cNetworkPassword);
	Serial.println("Connecting to WiFi for Art-Net DMX");
	Serial.println(SystemInfo.cNetworkName);
	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		vTaskDelay(500 / portTICK_PERIOD_MS);
		Serial.print(".");
		if (i > 20) {
			state = false;
			break;
		}
		i++;
	}
	if (state) {
		ArtNetLocalIP = WiFi.localIP().toString();
		Serial.println(String("Connected to ") + SystemInfo.cNetworkName);
		Serial.print("IP address: " + ArtNetLocalIP);
		Serial.println(" as '" + String(SystemInfo.cArtNetName) + "'");
		bArtNetActive = true;
	}
	else {
		String txt = String("Art-Net Connection to\n") + SystemInfo.cNetworkName + "\nfailed";
		WriteMessage(txt, true);
		Serial.println("");
		Serial.println("Connection failed.");
		bArtNetActive = false;
	}
	return state;
}

// handle ArtNet requests
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
	int startUniverse = SystemInfo.bStartUniverseOne ? 1 : 0;
	const int numLeds = 144; // CHANGE FOR YOUR SETUP
	const int numberOfChannels = numLeds * 3; // Total number of channels you want to receive (1 led = 3 channels)
	// Check if we got all universes
	const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
	bool universesReceived[maxUniverses];
	static bool sendFrame = 1;
	//Serial.println("universe: " + String(universe) + " len: " + String(length) + " seq: " + String(sequence)
	//	+ " data: " + String(data[0]) + " " + String(data[1]) + " " + String(data[2]));
	sendFrame = 1;
	// set brightness of the whole strip
	if (universe == 15)
	{
		FastLED.setBrightness(data[0]);
		FastLED.show();
	}

	// Store which universe has got in
	if ((universe - startUniverse) < maxUniverses) {
		universesReceived[universe - startUniverse] = 1;
	}

	for (int i = 0; i < maxUniverses; i++)
	{
		if (universesReceived[i] == 0)
		{
			Serial.println("Broke");
			sendFrame = 0;
			break;
		}
	}
	static int previousDataLength = 0;
	// read universe and put into the right part of the display buffer
	for (int i = 0; i < length / 3; i++)
	{
		int led = i + (universe - startUniverse) * (previousDataLength / 3);
		if (led < numLeds)
			leds[AdjustStripIndex(led)] = CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
	}
	previousDataLength = length;

	if (sendFrame)
	{
		FastLED.show();
		// Reset universeReceived to 0
		memset(universesReceived, 0, maxUniverses);
	}
}

// scan for networks
int ScanForNetworks()
{
	//Serial.println("scan start");
	WiFi.disconnect();
	// WiFi.scanNetworks will return the number of networks found
	int retval = WiFi.scanNetworks();
	//Serial.println("scan done");
	if (retval == 0) {
		Serial.println("no networks found");
	}
	else {
		Serial.print(retval);
		Serial.println(" networks found");
		for (int i = 0; i < retval; ++i) {
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
		}
	}
	return retval;
}

// get a string
void GetStringName(MenuItem* menu)
{
	String savedNameFilter = nameFilter;
	nameFilter = (char*)menu->value;
	SetFilter(menu);
	strncpy((char*)menu->value, nameFilter.c_str(), menu->max - 1);
	nameFilter = savedNameFilter;
}

// choose a network name from the first 5 found
void GetNetworkName(MenuItem* menu)
{
	ClearScreen();
	DisplayLine(0, "Scanning Networks...", SystemInfo.menuTextColor, TFT_BLACK);
	int nets = ScanForNetworks();
	// maximum 5 nets
	constexpr int maxNetworks = 5;
	nets = min(nets, maxNetworks);
	for (int ix = 0; ix < nets; ++ix) {
		DisplayLine(ix, WiFi.SSID(ix), SystemInfo.menuTextColor, TFT_BLACK);
	}
	// loop handling key presses
	int which = 0;
	bool done = false;
	DisplayLine(6, "Longpress=accept LongB0=cancel", SystemInfo.menuTextColor, TFT_BLACK);
	DisplayLine(which, WiFi.SSID(which), TFT_BLACK, SystemInfo.menuTextColor);
	while (!done) {
		CRotaryDialButton::Button btn = ReadButton();
		switch (btn) {
		case CRotaryDialButton::BTN_LONGPRESS:
			strncpy((char*)menu->value, WiFi.SSID(which).c_str(), menu->max - 1);
			bControllerReboot = true;
			done = true;
			break;
		case CRotaryDialButton::BTN0_LONGPRESS:
			done = true;
			break;
		case CRotaryDialButton::BTN_RIGHT:
			if (which < maxNetworks - 1) {
				DisplayLine(which, WiFi.SSID(which), SystemInfo.menuTextColor, TFT_BLACK);
				++which;
				DisplayLine(which, WiFi.SSID(which), TFT_BLACK, SystemInfo.menuTextColor);
			}
			break;
		case CRotaryDialButton::BTN_LEFT:
			if (which > 0) {
				DisplayLine(which, WiFi.SSID(which), SystemInfo.menuTextColor, TFT_BLACK);
				--which;
				DisplayLine(which, WiFi.SSID(which), TFT_BLACK, SystemInfo.menuTextColor);
			}
			break;
		default:
			break;
		}
	}
	delay(10);
}

// toggle ArtNet running, reboot if needed
void ToggleWebServer(MenuItem* menu)
{
	// save existing value
	bool bWas = *(bool*)menu->value;
	ToggleBool(menu);
	bControllerReboot = (bWas != *(bool*)menu->value);
}

// toggle ArtNet running, reboot if needed
void ToggleArtNet(MenuItem* menu)
{
	// save existing value
	bool bWas = *(bool*)menu->value;
	ToggleBool(menu);
	bControllerReboot = (bWas != *(bool*)menu->value);
}

// change network name or password or ArtNet name, reboot if necessary
void ChangeNetCredentials(MenuItem* menu)
{
	// save the values
	String oldArt = (char*)menu->value;
	String oldNet = (char*)menu->value;
	String oldPass = (char*)menu->value;
	GetStringName(menu);
	// check for changes
	if (oldArt != (char*)menu->value
		|| oldNet != (char*)menu->value
		|| oldPass != (char*)menu->value) {
		// only really matters if ArtNet is running
		if (SystemInfo.bRunArtNetDMX)
			bControllerReboot = true;
	}
}
