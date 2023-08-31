#pragma once

const char* MIW_Version = "2.85";

const char* StartFileName = "START.MIW";
#include "MIWconfig.h"
#include <ArtnetNodeWifi.h>
#include <ArduinoJson.h>

const int JSON_DOC_SIZE = 5000;
const char* MACRO_JSON_FILE = "/macro.json";

//#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEServer.h>
//#include <BLE2902.h>
#include <Update.h>

#include <time.h>
#if USE_STANDARD_SD
    #include "SD.h"
#else
    #include <SdFatConfig.h>
    #include <sdfat.h>
#endif
#include "SPI.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include "WebServerX.h"
String wifiMacs = "MIW-" + WiFi.macAddress();
const char *ssid = wifiMacs.c_str();
const char *password = "12345678"; // not critical stuff, therefore simple password is enough

// we need to change the file handling for exFat, the name function is different than standard SD lib
#if USE_STANDARD_SD
WebServer server(80);
#else
WebServerX server(80);
#endif

String webpage = "";
char localIpAddress[16];

String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}

#include <FastLED.h>
#include <Preferences.h>
#include "RotaryDialButton.h"
#if TTGO_T == 1
    #include <TFT_eSPI.h>
#elif TTGO_T == 4
    #include <TFT_eSPI.h>
#endif
#include <vector>
#include <stack>
//#include <fonts/GFXFF/FreeMono18pt7b.h>

// definitions for preferences
const char* prefsName = "MIW";
const char* prefsVars = "vars";
const char* prefsVersion = "version";
const char* prefsBuiltInInfo = "builtininfo";
const char* prefsImgInfo = "imginfo";
const char* prefsLedInfo = "ledinfo";
const char* prefsBuiltinInfo = "builtininfo";
const char* prefsSystemInfo = "systeminfo";
const char* prefsAutoload = "autoload";
// rotary dial values
const char* prefsLongPressTimer = "longpress";
const char* prefsDialSensitivity = "dialsense";
const char* prefsDialSpeed = "dialspeed";
const char* prefsDialReverse = "dialreverse";

#if USE_STANDARD_SD
  SPIClass spiSDCard;
#else
  //SPIClass spi1(VSPI);
  SdFs SD; // fat16/32 and exFAT
#endif

// the display
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define BTN_SELECT      CRotaryDialButton::BTN_CLICK
#define BTN_NONE        CRotaryDialButton::BTN_NONE
#define BTN_LEFT        CRotaryDialButton::BTN_LEFT
#define BTN_LEFT_LONG   CRotaryDialButton::BTN_LEFT_LONG
#define BTN_RIGHT       CRotaryDialButton::BTN_RIGHT
#define BTN_RIGHT_LONG  CRotaryDialButton::BTN_RIGHT_LONG
#define BTN_LONG        CRotaryDialButton::BTN_LONGPRESS
#define BTN_B0_CLICK    CRotaryDialButton::BTN0_CLICK
#define BTN_B0_LONG     CRotaryDialButton::BTN0_LONGPRESS
#define BTN_B1_CLICK    CRotaryDialButton::BTN1_CLICK
#define BTN_B1_LONG     CRotaryDialButton::BTN1_LONGPRESS
#define BTN_B2_LONG     CRotaryDialButton::BTN2_LONGPRESS
#define BTN_LEFT_RIGHT_LONG CRotaryDialButton::BTN_LEFT_RIGHT_LONG

// functions
void ShowLeds(int mode = 0, CRGB colorval = TFT_BLACK, int imgHeight = 144);
void SetDisplayBrightness(int val);
void DisplayCurrentFile(bool bShowPath = true, bool bFirstOnly = false);
void DisplayLine(int line, String text, int16_t color = TFT_WHITE, int16_t backColor = TFT_BLACK);
void DisplayMenuLine(int line, int displine, String text);
void fixRGBwithGamma(byte* rp, byte* gp, byte* bp);
void WriteMessage(String txt, bool error = false, int wait = 2000, bool process = false);
void BarberPole();
void TestBouncingBalls();
void CheckerBoard();
void RandomBars();
void RunningDot();
void RandomDot();
void OppositeRunningDots();
void TestTwinkle();
void TestMeteor();
void TestCylon();
void TestCircles();
void TestRainbow();
void TestJuggle();
void TestSine();
void TestBpm();
void TestConfetti();
void DisplayLedLightBar();
void TestStripes();
void TestLines();
void RainbowPulse();
void TestWedge();
void ReportCouldNotCreateFile(String target);
void handleFileUpload();
void append_page_header();
void append_page_footer();
void HomePage();
void File_Download();
void SD_file_download(String filename);
void File_Upload();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void SelectInput(String heading1, String command, String arg_calling_name);
void ReportFileNotPresent(String target);
void ReportSDNotPresent();
void IncreaseRepeatButton();
void DecreaseRepeatButton();
bool SaveLoadSettings(bool save = false, bool autoloadonly = false, bool ledonly = false, bool nodisplay = false);
CRotaryDialButton::Button ReadButton();
bool CheckCancel(bool bLeaveButton = false);

bool bAutoLoadSettings = false;

// The array of leds, up to 512
CRGB* leds;
// 0 feed from center, 1 serial from end, 2 from outsides
enum LED_STRIPS_WIRING_MODE { STRIPS_MIDDLE_WIRED = 0, STRIPS_CHAINED, STRIPS_OUTSIDE_WIRED };
const char* StripsWiringText[] = { "Middle","Serial","Outside" };
const char* DisplayRotationText[] = { "90","180","270","0" };
typedef struct LED_INFO {
    bool bSecondController = false; // enable the second controller
    bool bSwapControllers = false;  // swap the 1st and 2nd LED controller output pins
    int nLEDBrightness = 25;
    int nTotalLeds = 144;
    bool bGammaCorrection = true;
    int stripsMode = STRIPS_MIDDLE_WIRED;
    // white balance values, really only 8 bits, but menus need ints
    struct {
        int r;
        int g;
        int b;
    } whiteBalance = { 255,255,255 };
    //int nPixelMaxCurrent = 48;              // maximum milliamps to allow on any pixel
};
LED_INFO LedInfo;

enum DIAL_IMG_FUNCTIONS { DIAL_IMG_NONE = 0, DIAL_IMG_BRIGHTNESS, DIAL_IMB_SPEED };
const char* DialImgText[] = { "None","Brightness","Speed" };

// image settings
typedef struct IMG_INFO {
    int nFrameHold = 10;                      // default for the frame delay
    bool bFixedTime = false;                  // set to use imagetime instead of framehold, the frame time will be calculated
    int nFixedImageTime = 5;                  // time to display image when fixedtime is used, in seconds
    int nFramePulseCount = 0;                 // advance frame when button pressed this many times, 0 means ignore
    bool bManualFrameAdvance = false;         // advance frame by clicking or rotating button
    bool bReverseImage = false;               // read the file lines in reverse
    bool bUpsideDown = false;                 // play the image upside down
    bool bRotate180 = false;                  // the image is rotated 180 degrees, example: MagiLight images
    bool bDoublePixels = false;               // double the image line, to go from 144 to 288
    bool bMirrorPlayImage = false;            // play the file twice, 2nd time reversed
    int nMirrorDelay = 0;                     // pause between the two halves of the image
    bool bChainFiles = false;                 // set to run all the files from current to the last one in the current folder
    int nChainRepeats = 1;                    // how many times to repeat the chain
    int nChainDelay = 0;                      // number of 1/10 seconds to delay between chained files
    bool bChainWaitKey = false;               // wait for keypress to advance to next chained file
    bool bScaleHeight = false;                // scale the Y values to fit the number of pixels
    int nFadeInOutFrames = 0;                 // number of frames to use for fading in and out
    int startDelay = 0;                       // Variable for delay between button press and start of light sequence, in seconds
    //bool bRepeatForever = false;              // Variable to select auto repeat (until select button is pressed again)
    int repeatDelay = 0;                      // Variable for delay between repeats, 0.1 seconds
    int repeatCount = 1;                      // Variable to keep track of number of repeats
	int nCurrentMacro = 0;                    // the number of the macro to select or run
	int nRepeatWaitMacro = 0;                 // time between macro repeats, in 1/10 seconds
	int nRepeatCountMacro = 1;                // repeat count for macros
    bool bShowBuiltInTests = false;           // list the internal file instead of the SD card
    int nDialDuringImgAction = DIAL_IMG_NONE; // dial action during image display
    int nDialDuringImgInc = 1;                // how much to change by during image display
};
RTC_DATA_ATTR IMG_INFO ImgInfo;

// a stack to hold the file indexes as we navigate folders, put it in RTC memory for waking from sleep
typedef struct FILEINDEXINFO {
    int nFileIndex;     // current file index
    int nFileCursor;    // used when scrolling cursor, IE current file not always on top
};
#define FILEINDEXSTACKSIZE 10
RTC_DATA_ATTR FILEINDEXINFO FileIndexStack[FILEINDEXSTACKSIZE];
RTC_DATA_ATTR int FileIndexStackSize = 0;
// hold the current information
RTC_DATA_ATTR FILEINDEXINFO currentFileIndex = { 0,0 };

// functions that btn0 long press can map to
enum BTN_LONG_FUNCTIONS { BTN_LONG_ROTATION = 0, BTN_LONG_LIGHTBAR };
const char* BtnLongText[] = { "DisplayRotate","LightBar" };

// display dim modes, make sure sensor mode is last
enum DISPLAY_DIM_MODES { DISPLAY_DIM_MODE_NONE, DISPLAY_DIM_MODE_TIME, DISPLAY_DIM_MODE_SENSOR };
const char* DisplayDimModeText[] = { "None","Timer","Sensor" };

typedef struct SYSTEM_INFO {
    uint16_t menuTextColor = TFT_BLUE;
    bool bMenuStar = false;
    bool bHiLiteCurrentFile = true;
    int nPreviewScrollCols = 20;                // now many columns to scroll with dial during preview
    bool bShowProgress = true;                  // show the progress bar
    bool bShowFolder = false;                   // show the path in front of the file
#if TTGO_T == 1
    int nDisplayBrightness = 50;                // this is in %
#elif TTGO_T == 4
    int nDisplayBrightness = 75;                // this is in %
#endif
    bool bAllowMenuWrap = false;                // allows menus to wrap around from end and start instead of pinning
    bool bShowNextFiles = true;                 // show the next files in the main display
    bool bShowDuringBmpFile = false;            // set this to display the bmp on the LCD and the LED during BMP display
    int nSidewayScrollSpeed = 25;               // mSec for pixel scroll
    int nSidewaysScrollPause = 20;              // how long to wait at each end
    int nSidewaysScrollReverse = 3;             // reverse speed multiplier
    bool bMacroUseCurrentSettings = false;      // ignore settings in macro files when this is true
    int nBatteryFullLevel = 1760;               // 100% battery
    int nBatteryEmptyLevel = 1230;              // 0% battery, should cause a shutdown to save the batteries
    int bShowBatteryLevel = HAS_BATTERY_LEVEL;  // display the battery level on the bottom line
    int bSleepOnLowBattery = false;             // sleep on low battery
    int bCriticalBatteryLevel = false;          // set true if battery too low
    //int bShowBatteryLevel = 0;  // display the battery level on the bottom line
    int nBatteries = 2;                         // how many batteries
    CRotaryDialButton::ROTARY_DIAL_SETTINGS DialSettings;
    int nSleepTime = 0;                         // value in minutes before going to sleep, 0 means never
    int eDisplayDimMode = DISPLAY_DIM_MODE_NONE;// 0 is none, 1 is dimtime, 2 is light sensor
    int nDisplayDimTime = 0;                    // seconds before lcd is dimmed
    int nDisplayDimValue = 10;                  // the value to dim to
#if TTGO_T == 1
    int nDisplayRotation = 1;                   // rotates display 0, 180, 90, 270
#elif TTGO_T == 4
    int nDisplayRotation = 0;                   // rotates display 0, 180, 90, 270
#endif
    int nBtn0LongFunction = BTN_LONG_ROTATION;  // function that long btn0 performs
    int nBtn1LongFunction = BTN_LONG_LIGHTBAR;  // function that long btn1 performs
    bool bSimpleMenu = false;                   // full or simple menu
    int nLightSensorDim = 4000;                 // value for the dimmest setting
    int nLightSensorBright = 100;               // value for the brightest setting
    bool bHasLightSensor = false;               // set to true if the light sensor is detected
    int nPreviewAutoScroll = 0;                 // mSec for preview autoscroll, 0 means no scroll
    int nPreviewAutoScrollAmount = 1;           // now many pixels to auto scroll
    bool bPreviewScrollFiles = false;           // set for preview to scroll files instead of sideways
#if TTGO_T == 1
    int nPreviewStartOffset = 5;                // how many pixels to offset the start, the display is only 135, not 144
#elif TTGO_T == 4
    int nPreviewStartOffset = 0;                // how many pixels to offset the start, the display is only 135, not 144
#endif
    bool bKeepFileOnTopLine = false;            // keep the active file on the top line
    bool bInitTest = true;                      // test the LED's on boot
    // ArtNet DMX variables
    bool bRunArtNetDMX = false;                 // set to run the ArtNet WiFi DMX protocol
    char cArtNetName[20] = "MIW_ARTNET_DMX";    // ArtNet name
    char cNetworkName[33] = "";                 // the network to connect to
    char cNetworkPassword[65] = "";             // network password
    bool bStartUniverseOne = true;              // some controllers need 1, others 0 (false)
    bool bRunWebServer = false;                 // run the web server
    //
};
RTC_DATA_ATTR SYSTEM_INFO SystemInfo;

// Art-Net variables
ArtnetnodeWifi artnet;
// Art-Net settings
String ArtNetLocalIP;
bool bArtNetActive = false;

enum LIGHT_BAR_MODES { LBMODE_HSV = 0, LBMODE_RGB, LBMODE_KELVIN };
const char* LightBarModeText[] = { "HSV","RGB","Kelvin" };
uint32_t LightBarColorList[] = {
    Candle,
    Tungsten40W,
    Tungsten100W,
    Halogen,
    CarbonArc,
    HighNoonSun,
    DirectSunlight,
    OvercastSky,
    ClearBlueSky,
    WarmFluorescent,
    StandardFluorescent,
    CoolWhiteFluorescent,
    FullSpectrumFluorescent,
    GrowLightFluorescent,
    BlackLightFluorescent,
    MercuryVapor,
    SodiumVapor,
    MetalHalide,
    HighPressureSodium, 
};
const char* LightBarColorKelvinText[] = {
    "Candle 1900K",
    "Tungsten 40W 2600K",
    "Tungsten 100W 2850K",
    "Halogen 3200K",
    "Carbon Arc 5200K",
    "High Noon Sun 5400K",
    "Direct Sunlight 6000K",
    "Overcast Sky 7000K",
    "Clear Blue Sky 20000K",
    "Warm Fluorescent",
    "Standard Fluorescent",
    "Cool White Fluorescent",
    "Full Spectrum Fluorescent",
    "Grow Light Fluorescent",
    "Black Light Fluorescent",
    "Mercury Vapor",
    "Sodium Vapor",
    "Metal Halide",
    "High Pressure Sodium",
};

typedef struct BUILTIN_INFO {
    // adjustment values for builtins
    uint8_t gHue = 0; // rotating "base color" used by many of the patterns
    // bouncing balls
    int nBouncingBallsCount = 4;
    int nBouncingBallsDecay = 1000;
    int nBouncingBallsFirstColor = 0;   // first color, wraps to get all 32
    int nBouncingBallsChangeColors = 0; // how many 100 count cycles to wait for change
    // cylon eye
    int nCylonEyeSize = 10;
    int nCylonEyeRed = 255;
    int nCylonEyeGreen = 0;
    int nCylonEyeBlue = 0;
    // random bars
    bool bRandomBarsBlacks = true;
    int nRandomBarsHoldframes = 10;
    // meteor
    int nMeteorSize = 10;
    int nMeteorRed = 255;
    int nMeteorGreen = 255;
    int nMeteorBlue = 255;
    // display all color, AKA lightbar
    bool bAllowRollover = true;       // lets 255->0 and 0->255
    int nLightBarMode = LBMODE_HSV;
    int nDisplayAllRed = 255;
    int nDisplayAllGreen = 255;
    int nDisplayAllBlue = 255;
    int nDisplayAllHue = 0;
    int nDisplayAllSaturation = 255;
    int nDisplayAllBrightness = 255;
    int nColorTemperature = 0;
    int nDisplayAllPixelCount = 288;
    bool bDisplayAllFromMiddle = true;
    int nDisplayAllChangeTime = 0;   // mS when changing steps between HUE, etc
    int nDisplayAllIncrement = 10;
    // rainbow
    int nRainbowHueDelta = 4;
    int nRainbowInitialHue = 0;
    int nRainbowFadeTime = 10;       // fade in out 0.1 Sec
    bool bRainbowAddGlitter = false;
    bool bRainbowCycleHue = false;
    // twinkle
    bool bTwinkleOnlyOne = false;
    // confetti
    bool bConfettiCycleHue = false;
    // juggle

    // sine
    int nSineStartingHue = 0;
    bool bSineCycleHue = false;
    int nSineSpeed = 13;
    // bpm
    int nBpmBeatsPerMinute = 62;
    bool bBpmCycleHue = false;
    // checkerboard/bars
    int nCheckerboardHoldframes = 10;
    int nCheckboardBlackWidth = 12;
    int nCheckboardWhiteWidth = 12;
    bool bCheckerBoardAlternate = true;
    int nCheckerboardAddPixels = 0;
    // stripes

    // black and white lines
    int nLinesWhite = 5;
    int nLinesBlack = 5;
    // rainbow pulse settings
    int nRainbowPulseColorScale = 10;
    int nRainbowPulsePause = 5;
    int nRainbowPulseSaturation = 255;
    int nRainbowPulseStartColor = 0;
    // wedge data
    bool bWedgeFill = false;
    int nWedgeRed = 255;
    int nWedgeGreen = 255;
    int nWedgeBlue = 255;
    // circles data
    int nCirclesDiameter = 144;
    int nCirclesHue = 0;
    int nCirclesSaturation = 255;
    int nCirclesCount = 1;
    int nCirclesGap = 0;        // space after each circle
    bool bCirclesFill = true;   // fill the circle
};
RTC_DATA_ATTR BUILTIN_INFO BuiltinInfo;

// settings
bool bSdCardValid = false;              // set to true when card is found
bool bControllerReboot = false;         // set this when controllers or led count changed
int AdjustStripIndex(int ix);
// get the real LED strip index from the desired index
void SetPixel(int ix, CRGB pixel, int column = 0, int totalColumns = 1);
int nRepeatsLeft;                         // countdown while repeating, used for BLE also
int g = 0;                                // Variable for the Green Value
int b = 0;                                // Variable for the Blue Value
int r = 0;                                // Variable for the Red Value
// settings
constexpr auto NEXT_FOLDER_CHAR = '>';
constexpr auto PREVIOUS_FOLDER_CHAR = '<';
String currentFolder = "/";
RTC_DATA_ATTR char sleepFolder[50];       // a place to save the folder during sleeping
FILEINDEXINFO lastFileIndex = { 0,0 };    // save between switching of internal and SD
String lastFolder = "/";
std::vector<String> FileNames;
String nameFilter;
bool bnameFilter = false;                 // set this true to enable the filters set in nameFilter
bool bSettingsMode = false;               // set true when settings are displayed
bool bCancelRun = false;                  // set to cancel a running job
bool bCancelMacro = false;                // set to cancel a running macro
bool bWebRunning = false;                 // set while running from web
bool bRecordingMacro = false;             // set while recording
char FileToShow[100];
unsigned long recordingTime;              // shows the time for each part
bool bRunningMacro = false;               // set while running
unsigned long nMacroColumnsDone = 0;      // how many pixel columns in the macro
int nMacroRepeatsLeft = 1;                // set during macro running
volatile int nTimerSeconds;

// esp timers
// set this to the delay time while we get the next frame, also used for delay timers
volatile bool bStripWaiting = false;
esp_timer_handle_t oneshot_LED_timer;
esp_timer_create_args_t oneshot_LED_timer_args;
// use this timer for seconds countdown
volatile int sleepTimer = 0;
// seconds before dimming the display
volatile int displayDimTimer = 30;
volatile bool displayDimNow = false;
esp_timer_handle_t periodic_Second_timer;
esp_timer_create_args_t periodic_Second_timer_args;

#if USE_STANDARD_SD
SDFile dataFile;
#else
FsFile dataFile;
#endif
// system state, idle or running
bool bIsRunning = false;

enum eDisplayOperation {
	eTerminate = 0,     // must be last in a menu, (or use {})
    eText,              // handle text with optional %s value
    eTextInt,           // handle text with optional %d value
    eTextCurrentFile,   // adds current basefilename for %s in string
    eBool,              // handle bool using %s and on/off values
    eMenu,              // load another menu
    eExit,              // closes this menu, handles optional %d or %s in string
    eIfEqual,           // start skipping menu entries if match with boolean data value
    eIfIntEqual,        // start skipping menu entries if match with int data value
    eElse,              // toggles the skipping
    eEndif,             // ends an if block
    eBuiltinOptions,    // use an internal settings menu if available, see the internal name,function list below (BuiltInFiles[])
    eReboot,            // reboot the system
    eMacroList,         // used to make a selection from the macro list
    eList,              // used to rotate selection from a list of choices
};

// we need to have a pointer reference to this in the MenuItem, the full declaration follows later
struct BuiltInItem;
std::vector<bool> bMenuValid;   // set to indicate menu item  is valid
typedef struct MenuItem {
    enum eDisplayOperation op;
    const char* text;                   // text to display
    union {
        void(*function)(MenuItem*);     // called on click
        MenuItem* menu;                 // jump to another menu
        BuiltInItem* builtin;           // builtin items
    };
    const void* value;                  // associated variable
    long min;                           // the minimum value, also used for ifequal, min length for string
    long max;                           // the maximum value, also size to compare for if, max length for string
    int decimals;                       // 0 for int, 1 for 0.1
    const char* on;                     // text for boolean true
    const char* off;                    // text for boolean false
    // flag is 1 for first time, 0 for changes, and -1 for last call, bools only call this with -1
    void(*change)(MenuItem*, int flag); // call for each change, example: brightness change show effect, can be NULL
    const char** nameList;              // used for multichoice of items, example wiring mode, .max should be count-1 and .min=0
    const char* cHelpText;              // a place to put some menu help
};

// builtins
// built-in "files"
typedef struct BuiltInItem {
    const char* text;
    void(*function)();
    MenuItem* menu;
};
extern BuiltInItem BuiltInFiles[];

// some menu functions using menus
void CheckUpdateBin(MenuItem* menu);
void FactorySettings(MenuItem* menu);
void EraseFlash(MenuItem* menu);
void EraseStartFile(MenuItem* menu);
void SaveStartFile(MenuItem* menu);
void EraseAssociatedFile(MenuItem* menu);
void SaveAssociatedFile(MenuItem* menu);
void LoadAssociatedFile(MenuItem* menu);
void LoadStartFile(MenuItem* menu);
void SaveEepromSettings(MenuItem* menu);
void LoadEepromSettings(MenuItem* menu);
void ShowWhiteBalance(MenuItem* menu);
void GetIntegerValue(MenuItem* menu);
void GetIntegerValueHue(MenuItem* menu);
void GetSelectChoice(MenuItem* menu);
void ToggleBool(MenuItem* menu);
void ToggleArtNet(MenuItem* menu);
void ToggleWebServer(MenuItem* menu);
void ToggleFilesBuiltin(MenuItem* menu);
void UpdateDisplayBrightness(MenuItem* menu, int flag);
void UpdateBatteries(MenuItem* menu, int flag);
void UpdateDisplayRotation(MenuItem* menu, int flag);
void UpdateDisplayDimMode(MenuItem* menu, int flag);
void SetMenuColor(MenuItem* menu);
void UpdateKeepOnTop(MenuItem* menu, int flag);
void UpdateTotalLeds(MenuItem* menu, int flag);
void UpdateControllers(MenuItem* menu, int flag);
void UpdateWiringMode(MenuItem* menu, int flag);
void UpdateStripBrightness(MenuItem* menu, int flag);
void UpdateStripHue(MenuItem* menu, int flag);
//void UpdatePixelMaxCurrent(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceR(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceG(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceB(MenuItem* menu, int flag);
bool WriteOrDeleteConfigFile(String filename, bool remove, bool startfile, bool bMacro);
void RunMacro(MenuItem* menu);
void LoadMacro(MenuItem* menu);
void InfoMacro(MenuItem* menu);
void SaveMacro(MenuItem* menu);
void DeleteMacro(MenuItem* menu);
void DeleteMacroJson(MenuItem* menu);
void LightBar(MenuItem* menu);
void Sleep(MenuItem* menu);
//void ReadBattery(MenuItem* menu);
void ShowBmp(MenuItem* menu);
void ShowBattery(MenuItem* menu);
void SetFilter(MenuItem* menu);
void ShowLightSensor(MenuItem* menu);
//void UpdateFilter(MenuItem* menu, int flag);
void GetStringName(MenuItem* menu);
void GetNetworkName(MenuItem* menu);
void ChangeNetCredentials(MenuItem* menu);

struct saveValues {
    void* val;
    int size;
};
// these values are saved during macro runs
const saveValues saveValueList[] = {
    {&LedInfo,sizeof(LedInfo)},
    {&ImgInfo,sizeof(ImgInfo)},
    {&BuiltinInfo,sizeof(BuiltinInfo)},
    {&SystemInfo,sizeof(SystemInfo)},
    {&currentFileIndex.nFileIndex,sizeof(currentFileIndex.nFileIndex)},
};

// Gramma Correction (Defalt Gamma = 2.8)
const uint8_t gammaR[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
    2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,
    5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,
    9,  9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14,
   15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
   23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33,
   33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46,
   46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
   62, 63, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 80,
   81, 83, 84, 85, 87, 88, 89, 91, 92, 94, 95, 97, 98, 99,101,102,
  104,105,107,109,110,112,113,115,116,118,120,121,123,125,127,128,
  130,132,134,135,137,139,141,143,145,146,148,150,152,154,156,158,
  160,162,164,166,168,170,172,174,177,179,181,183,185,187,190,192,
  194,196,199,201,203,206,208,210,213,215,218,220,223,225,227,230 };

const uint8_t gammaG[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


const uint8_t gammaB[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
    4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  7,  8,
    8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 11, 11, 12, 12, 12, 13,
   13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 19,
   20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28,
   29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
   40, 41, 42, 43, 44, 44, 45, 46, 47, 48, 49, 50, 51, 51, 52, 53,
   54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 69, 70,
   71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
   90, 92, 93, 94, 96, 97, 98,100,101,103,104,106,107,109,110,112,
  113,115,116,118,119,121,122,124,126,127,129,131,132,134,136,137,
  139,141,143,144,146,148,150,152,153,155,157,159,161,163,165,167,
  169,171,173,175,177,179,181,183,185,187,189,191,193,196,198,200 };

const char* PreviousMenu = "Back";
MenuItem BouncingBallsMenu[] = {
    {eExit,"Bouncing Balls"},
    {eTextInt,"Ball Count: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsCount,1,32},
    {eTextInt,"Decay: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsDecay,100,10000},
    {eTextInt,"First Color: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsFirstColor,0,31},
    {eTextInt,"Change Color Rate: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsChangeColors,0,10,0},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem CheckerBoardMenu[] = {
    {eExit,"Checker Board"},
    {eTextInt,"Hold Frames: %d",GetIntegerValue,&BuiltinInfo.nCheckerboardHoldframes,1,100},
    {eTextInt,"Black Width: %d pixels",GetIntegerValue,&BuiltinInfo.nCheckboardBlackWidth,1,288},
    {eTextInt,"White Width: %d pixels",GetIntegerValue,&BuiltinInfo.nCheckboardWhiteWidth,1,288},
    {eTextInt,"Add Pixels per Cycle: %d",GetIntegerValue,&BuiltinInfo.nCheckerboardAddPixels,0,144},
    {eBool,"Alternate per Cycle: %s",ToggleBool,&BuiltinInfo.bCheckerBoardAlternate,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowMenu[] = {
    {eExit,"Rainbow"},
    {eTextInt,"Fade Time: %d.%d S",GetIntegerValue,&BuiltinInfo.nRainbowFadeTime,0,100,1},
    {eTextInt,"Starting Hue: %d",GetIntegerValueHue,&BuiltinInfo.nRainbowInitialHue,0,255,0,NULL,NULL,UpdateStripHue},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bRainbowCycleHue,0,0,0,"Yes","No"},
    {eTextInt,"Hue Delta Size: %d",GetIntegerValue,&BuiltinInfo.nRainbowHueDelta,1,255},
    {eBool,"Add Glitter: %s",ToggleBool,&BuiltinInfo.bRainbowAddGlitter,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowPulseMenu[] = {
    {eExit,"Rainbow Pulse"},
    {eTextInt,"Step Pause: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulsePause,0,1000},
    {eTextInt,"Color Rate Scale: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulseColorScale,0,256},
    {eTextInt,"Start Color: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulseStartColor,0,255},
    {eTextInt,"Color Saturation: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulseSaturation,0,255},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem ConfettiMenu[] = {
    {eExit,"Confetti"},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bConfettiCycleHue,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem TwinkleMenu[] = {
    {eExit,"Twinkle"},
    {eBool,"One or Many: %s",ToggleBool,&BuiltinInfo.bTwinkleOnlyOne,0,0,0,"One","Many"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem WedgeMenu[] = {
    {eExit,"Wedge"},
    {eBool,"Fill Wedge: %s",ToggleBool,&BuiltinInfo.bWedgeFill,0,0,0,"Solid","<"},
    {eTextInt,"Red: %d",GetIntegerValue,&BuiltinInfo.nWedgeRed,0,255},
    {eTextInt,"Green: %d",GetIntegerValue,&BuiltinInfo.nWedgeGreen,0,255},
    {eTextInt,"Blue: %d",GetIntegerValue,&BuiltinInfo.nWedgeBlue,0,255},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem SineMenu[] = {
    {eExit,"Sine"},
    {eTextInt,"Starting Hue: %d",GetIntegerValueHue,&BuiltinInfo.nSineStartingHue,0,255,0,NULL,NULL,UpdateStripHue},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bSineCycleHue,0,0,0,"Yes","No"},
    {eTextInt,"Speed: %d",GetIntegerValue,&BuiltinInfo.nSineSpeed,1,500},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem BpmMenu[] = {
    {eExit,"Beats"},
    {eTextInt,"Beats per minute: %d",GetIntegerValue,&BuiltinInfo.nBpmBeatsPerMinute,1,300},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bBpmCycleHue,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem CirclesMenu[] = {
    {eExit,"Circles"},
    {eTextInt,"Diameter: %d",GetIntegerValue,&BuiltinInfo.nCirclesDiameter,0,LedInfo.nTotalLeds},
	{eTextInt,"Hue: %d",GetIntegerValueHue,&BuiltinInfo.nCirclesHue,0,255,0,NULL,NULL,UpdateStripHue},
    {eTextInt,"Saturation: %d",GetIntegerValue,&BuiltinInfo.nCirclesSaturation,0,255},
    {eTextInt,"Count: %d",GetIntegerValue,&BuiltinInfo.nCirclesCount,1,255},
    {eTextInt,"Gap Columns: %d",GetIntegerValue,&BuiltinInfo.nCirclesGap,0,100},
    {eBool,"Fill Circle: %s",ToggleBool,&BuiltinInfo.bCirclesFill,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem LinesMenu[] = {
    {eExit,"Lines"},
    {eTextInt,"White Pixels: %d",GetIntegerValue,&BuiltinInfo.nLinesWhite,0,LedInfo.nTotalLeds},
    {eTextInt,"Black Pixels: %d",GetIntegerValue,&BuiltinInfo.nLinesBlack,0,LedInfo.nTotalLeds},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem CylonEyeMenu[] = {
    {eExit,"Cylon Eye"},
    {eTextInt,"Eye Size:  %d",GetIntegerValue,&BuiltinInfo.nCylonEyeSize,1,100},
    {eTextInt,"Eye Red:   %d",GetIntegerValue,&BuiltinInfo.nCylonEyeRed,0,255},
    {eTextInt,"Eye Green: %d",GetIntegerValue,&BuiltinInfo.nCylonEyeGreen,0,255},
    {eTextInt,"Eye Blue:  %d",GetIntegerValue,&BuiltinInfo.nCylonEyeBlue,0,255},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem MeteorMenu[] = {
    {eExit,"Meteor"},
    {eTextInt,"Meteor Size:  %d",GetIntegerValue,&BuiltinInfo.nMeteorSize,1,100},
    {eTextInt,"Meteor Red:   %d",GetIntegerValue,&BuiltinInfo.nMeteorRed,0,255},
    {eTextInt,"Meteor Green: %d",GetIntegerValue,&BuiltinInfo.nMeteorGreen,0,255},
    {eTextInt,"Meteor Blue:  %d",GetIntegerValue,&BuiltinInfo.nMeteorBlue,0,255},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem LedLightBarMenu[] = {
    {eExit,"Light Bar Settings"},
    {eBool,"Allow rollover: %s",ToggleBool,&BuiltinInfo.bAllowRollover,0,0,0,"Yes","No"},
    {eTextInt,"Change Delay: %d mS",GetIntegerValue,&BuiltinInfo.nDisplayAllChangeTime,0,1000},
    {eList,"Color Mode: %s",GetSelectChoice,&BuiltinInfo.nLightBarMode,0,sizeof(LightBarModeText) / sizeof(*LightBarModeText) - 1,0,NULL,NULL,NULL,LightBarModeText},
    {eIfIntEqual,"",NULL,&BuiltinInfo.nLightBarMode,LBMODE_HSV},
        {eTextInt,"Hue: %d",GetIntegerValueHue,&BuiltinInfo.nDisplayAllHue,0,255,0,NULL,NULL,UpdateStripHue},
        {eTextInt,"Saturation: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllSaturation,0,255},
        {eTextInt,"Brightness: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllBrightness,0,255,0,NULL,NULL,UpdateStripBrightness},
    {eEndif},
    {eIfIntEqual,"",NULL,&BuiltinInfo.nLightBarMode,LBMODE_RGB},
        {eTextInt,"Red: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllRed,0,255},
        {eTextInt,"Green: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllGreen,0,255},
        {eTextInt,"Blue: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllBlue,0,255},
    {eEndif},
    {eIfIntEqual,"",NULL,&BuiltinInfo.nLightBarMode,LBMODE_KELVIN},
		{eList,"Temp: %s",GetSelectChoice,&BuiltinInfo.nColorTemperature,0,sizeof(LightBarColorKelvinText) / sizeof(*LightBarColorKelvinText) - 1,0,NULL,NULL,NULL,LightBarColorKelvinText},
    {eEndif},
    {eTextInt,"Pixels: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllPixelCount,1,288},
    {eBool,"From: %s",ToggleBool,&BuiltinInfo.bDisplayAllFromMiddle,0,0,0,"Middle","End"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RandomBarsMenu[] = {
    {eExit,"Random Color Bars"},
    {eTextInt,"Hold Frames: %d",GetIntegerValue,&BuiltinInfo.nRandomBarsHoldframes,1,100},
    {eBool,"Alternating Black: %s",ToggleBool,&BuiltinInfo.bRandomBarsBlacks,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem BatteryMenu[] = {
    {eExit,"Battery"},
    {eBool,"Low Battery Sleep: %s",ToggleBool,&SystemInfo.bSleepOnLowBattery,0,0,0,"Yes","No"},
    {eBool,"Show Battery: %s",ToggleBool,&SystemInfo.bShowBatteryLevel,0,0,0,"Yes","No"},
    {eText,"Read Battery",ShowBattery},
    {eTextInt,"100%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryFullLevel,900,4200},
    {eTextInt,"0%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryEmptyLevel,500,3000},
    {eTextInt,"Battery Count: %d",GetIntegerValue,&SystemInfo.nBatteries,1,4,0,NULL,NULL,UpdateBatteries},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem LightSensorMenu[] = {
    {eExit,"Light Sensor"},
    {eText,"Read Light Sensor",ShowLightSensor},
    {eTextInt,"Dim Value: %d",GetIntegerValue,&SystemInfo.nLightSensorDim,1000,5000},
    {eTextInt,"Bright Value: %d",GetIntegerValue,&SystemInfo.nLightSensorBright,0,1000},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem SidewaysScrollMenu[] = {
    {eExit,"Sideways Scrolling"},
    {eTextInt,"Sideways Scroll Speed: %d mS",GetIntegerValue,&SystemInfo.nSidewayScrollSpeed,1,1000},
    {eTextInt,"Sideways Scroll Pause: %d",GetIntegerValue,&SystemInfo.nSidewaysScrollPause,1,100},
    {eTextInt,"Sideways Scroll Reverse: %dx",GetIntegerValue,&SystemInfo.nSidewaysScrollReverse,1,20},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem DialMenu[] = {
    {eExit,"Rotary Dial Settings"},
    {eBool,"Direction: %s",ToggleBool,&SystemInfo.DialSettings.m_bReverseDial,0,0,0,"Reverse","Normal"},
    {eTextInt,"Pulse Count: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseCount,1,5},
    {eTextInt,"Pulse Timer: %d mS",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseTimer,100,1000},
    {eTextInt,"Long Press count: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nLongPressTimerValue,2,200},
    {eList,"Btn0 Long: %s",GetSelectChoice,&SystemInfo.nBtn0LongFunction,0,sizeof(BtnLongText) / sizeof(*BtnLongText) - 1,0,NULL,NULL,NULL,BtnLongText},
    {eList,"Btn1 Long: %s",GetSelectChoice,&SystemInfo.nBtn1LongFunction,0,sizeof(BtnLongText) / sizeof(*BtnLongText) - 1,0,NULL,NULL,NULL,BtnLongText},
    {eBool,"Rotate Dial Type: %s",ToggleBool,&SystemInfo.DialSettings.m_bToggleDial,0,0,0,"Toggle","Pulse"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem HomeScreenMenu[] = {
    {eExit,"Run Screen Settings"},
    {eBool,"Show BMP on LCD: %s",ToggleBool,&SystemInfo.bShowDuringBmpFile,0,0,0,"Yes","No"},
    {eBool,"Current File: %s",ToggleBool,&SystemInfo.bHiLiteCurrentFile,0,0,0,"Color","Normal"},
    {eBool,"Show More Files: %s",ToggleBool,&SystemInfo.bShowNextFiles,0,0,0,"Yes","No"},
    {eBool,"File on Top Line: %s",ToggleBool,&SystemInfo.bKeepFileOnTopLine,0,0,0,"Yes","No",UpdateKeepOnTop},
    {eBool,"Show Folder: %s",ToggleBool,&SystemInfo.bShowFolder,0,0,0,"Yes","No"},
    {eBool,"Progress Bar: %s",ToggleBool,&SystemInfo.bShowProgress,0,0,0,"On","Off"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
#define MAX_DIM_MODE (sizeof(DisplayDimModeText) / sizeof(*DisplayDimModeText) - 1)
MenuItem DisplayMenu[] = {
    {eExit,"Display Settings"},
    {eList, "Display Rotation: %s", GetSelectChoice, &SystemInfo.nDisplayRotation, 0, sizeof(DisplayRotationText) / sizeof(*DisplayRotationText) - 1, 0, NULL, NULL, UpdateDisplayRotation, DisplayRotationText},
    {eList, "Dimming Mode: %s", GetSelectChoice, &SystemInfo.eDisplayDimMode, 0, MAX_DIM_MODE, 0, NULL, NULL, UpdateDisplayDimMode, DisplayDimModeText},
    {eTextInt,"Bright Value: %d%%",GetIntegerValue,&SystemInfo.nDisplayBrightness,1,100,0,NULL,NULL,UpdateDisplayBrightness},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_NONE},
    {eElse},
        {eTextInt,"Dim Value: %d%%",GetIntegerValue,&SystemInfo.nDisplayDimValue,1,100},
    {eEndif},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_TIME},
        {eTextInt,"Display Dim Time: %d S",GetIntegerValue,&SystemInfo.nDisplayDimTime,0,120},
    {eEndif},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_SENSOR},
        {eMenu,"Light Sensor",{.menu = LightSensorMenu}},
    {eEndif},
    {eMenu,"Sideways Scroll Settings",{.menu = SidewaysScrollMenu}},
    {eBool,"Menu Choice: %s",ToggleBool,&SystemInfo.bMenuStar,0,0,0,"*","Color"},
    {eText,"Text Color",SetMenuColor},
    {eBool,"Menu Wrap: %s",ToggleBool,&SystemInfo.bAllowMenuWrap,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem PreviewMenu[] = {
    {eExit,"Preview Settings"},
    {eBool,"Scroll Mode: %s",ToggleBool,&SystemInfo.bPreviewScrollFiles,0,0,0,"Files","Sideways"},
    {eTextInt,"Top Start Offset: %d px",GetIntegerValue,&SystemInfo.nPreviewStartOffset,0,10},
    {eTextInt,"Dial Scroll Pixels: %d px",GetIntegerValue,&SystemInfo.nPreviewScrollCols,1,240},
    {eTextInt,"Auto Scroll Time: %d mS",GetIntegerValue,&SystemInfo.nPreviewAutoScroll,0,1000},
    {eTextInt,"Auto Scroll Pixels: %d px",GetIntegerValue,&SystemInfo.nPreviewAutoScrollAmount,1,240},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem WiFiMenu[] = {
    {eExit,"WiFi Settings"},
    {eBool,"MIW Web Server: %s",ToggleWebServer,&SystemInfo.bRunWebServer,0,0,0,"On","Off"},
    {eIfEqual,"",NULL,&SystemInfo.bRunWebServer,true},
        {eText,"MIW Net: %s",NULL,ssid},
        {eText,"Homepage: %s",NULL,localIpAddress},
    {eEndif},
    {eBool,"Art-Net DMX: %s",ToggleArtNet,&SystemInfo.bRunArtNetDMX,0,0,0,"On","Off"},
    {eIfEqual,"",NULL,&SystemInfo.bRunArtNetDMX,true},
        {eBool,"DMX Status: %s",NULL,&bArtNetActive,0,0,0,"Running","Stopped"},
        {eText,"DMX IP: %s",NULL,ArtNetLocalIP.c_str()},
        {eText,"Name: %s",ChangeNetCredentials,SystemInfo.cArtNetName,0,sizeof(SystemInfo.cArtNetName)},
        {eText,"Network: %s",ChangeNetCredentials,SystemInfo.cNetworkName,0,sizeof(SystemInfo.cNetworkName)},
        {eText,"Choose Network",GetNetworkName,SystemInfo.cNetworkName,0,sizeof(SystemInfo.cNetworkName)},
        {eText,"Password: %s",ChangeNetCredentials,SystemInfo.cNetworkPassword,0,sizeof(SystemInfo.cNetworkPassword)},
        {eBool,"Universe Start: %s",ToggleBool,&SystemInfo.bStartUniverseOne,0,0,0,"1","0"},
    {eEndif},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem SystemMenu[] = {
    {eExit,"System Settings"},
    {eMenu,"Display Settings",{.menu = DisplayMenu}},
    {eMenu,"Run Screen Settings",{.menu = HomeScreenMenu}},
    {eMenu,"Dial & Button Settings",{.menu = DialMenu}},
    {eMenu,"Preview Settings",{.menu = PreviewMenu}},
#if HAS_BATTERY_LEVEL
    {eMenu,"Battery Settings",{.menu = BatteryMenu}},
#endif
    {eTextInt,"Sleep Time: %d Min",GetIntegerValue,&SystemInfo.nSleepTime,0,120},
    {eBool,"Startup LED Test: %s",ToggleBool,&SystemInfo.bInitTest,0,0,0,"On","Off"},
    {eMenu,"WiFi Settings",{.menu = WiFiMenu}},
    {eText,"New Version BIN file",CheckUpdateBin},
    {eText,"Reset All Settings",FactorySettings},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
const char* HelpImageColumnTime = "How many mSeconds to display each column on the LEDS";
const char* HelpImageTime = "How many seconds to display the entire image on the LEDS";
const char* HelpImageFade = "Columns that fade up at start and down at end";
MenuItem ImageMenu[] = {
    {eExit,"Image Settings"},
    {eBool,"Timing Type: %s",ToggleBool,&ImgInfo.bFixedTime,0,0,0,"Image","Column"},
    {eIfEqual,"",NULL,&ImgInfo.bFixedTime,false},
        {eTextInt,"Column Time: %d mS",GetIntegerValue,&ImgInfo.nFrameHold,0,500,0,NULL,NULL,NULL,NULL,HelpImageColumnTime},
    {eElse},
        {eTextInt,"Image Time: %d S",GetIntegerValue,&ImgInfo.nFixedImageTime,1,120,0,NULL,NULL,NULL,NULL,HelpImageTime},
    {eEndif},
    {eTextInt,"Start Delay: %d.%d S",GetIntegerValue,&ImgInfo.startDelay,0,100,1},
    {eTextInt,"Fade I/O Columns: %d",GetIntegerValue,&ImgInfo.nFadeInOutFrames,0,255,0,NULL,NULL,NULL,NULL,HelpImageFade},
    {eBool,"Upside Down: %s",ToggleBool,&ImgInfo.bUpsideDown,0,0,0,"Yes","No"},
    {eBool,"Rotate 180: %s",ToggleBool,&ImgInfo.bRotate180,0,0,0,"Yes","No"},
    {eList,"Running Dial: %s",GetSelectChoice,&ImgInfo.nDialDuringImgAction,0,sizeof(DialImgText) / sizeof(*DialImgText) - 1,0,NULL,NULL,NULL,DialImgText},
	{eIfIntEqual,"",NULL,&ImgInfo.nDialDuringImgAction,DIAL_IMG_NONE},
    {eElse},
        {eTextInt,"Image Dial Step: %d",GetIntegerValue,&ImgInfo.nDialDuringImgInc,1,255},
    {eEndif},
    {eIfEqual,"",NULL,&ImgInfo.bShowBuiltInTests,false},
        {eBool,"Walk: %s",ToggleBool,&ImgInfo.bReverseImage,0,0,0,"Left-Right","Right-Left"},
        {eBool,"Play Mirror Image: %s",ToggleBool,&ImgInfo.bMirrorPlayImage,0,0,0,"Yes","No"},
        {eIfEqual,"",NULL,&ImgInfo.bMirrorPlayImage,true},
            {eTextInt,"Mirror Delay: %d.%d S",GetIntegerValue,&ImgInfo.nMirrorDelay,0,10,1},
        {eEndif},
        {eBool,"Scale Height to Fit: %s",ToggleBool,&ImgInfo.bScaleHeight,0,0,0,"On","Off"},
    {eEndif},
    {eBool,"144 to 288 Pixels: %s",ToggleBool,&ImgInfo.bDoublePixels,0,0,0,"Yes","No"},
    {eIfEqual,"",NULL,&ImgInfo.bShowBuiltInTests,false},
        {eBool,"Frame Advance: %s",ToggleBool,&ImgInfo.bManualFrameAdvance,0,0,0,"Step","Auto"},
        {eIfEqual,"",NULL,&ImgInfo.bManualFrameAdvance,true},
            {eTextInt,"Frame Counter: %d",GetIntegerValue,&ImgInfo.nFramePulseCount,0,32},
        {eEndif},
    {eEndif},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem StripMenu[] = {
    {eExit,"LED Strip Settings"},
    {eTextInt,"Max Brightness: %d/255",GetIntegerValue,&LedInfo.nLEDBrightness,1,255,0,NULL,NULL,UpdateStripBrightness},
    //{eTextInt,"Max Pixel Current: %d mA",GetIntegerValue,&LedInfo.nPixelMaxCurrent,1,2000,0,NULL,NULL,UpdatePixelMaxCurrent},
    {eBool,"LED Controllers: %s",ToggleBool,&LedInfo.bSecondController,0,0,0,"2","1",UpdateControllers},
    {eBool,"Controllers: %s First",ToggleBool,&LedInfo.bSwapControllers,0,0,0,"LED2","LED1",UpdateControllers},
    {eTextInt,"Total LEDs: %d",GetIntegerValue,&LedInfo.nTotalLeds,1,512,0,NULL,NULL,UpdateTotalLeds},
	{eList,"LED Wiring: %s",GetSelectChoice,&LedInfo.stripsMode,0,sizeof(StripsWiringText) / sizeof(*StripsWiringText) - 1,0,NULL,NULL,UpdateWiringMode,StripsWiringText},
    {eBool,"Gamma Correction: %s",ToggleBool,&LedInfo.bGammaCorrection,0,0,0,"On","Off"},
    {eTextInt,"White Balance R: %3d",GetIntegerValue,&LedInfo.whiteBalance.r,0,255,0,NULL,NULL,UpdateStripWhiteBalanceR},
    {eTextInt,"White Balance G: %3d",GetIntegerValue,&LedInfo.whiteBalance.g,0,255,0,NULL,NULL,UpdateStripWhiteBalanceG},
    {eTextInt,"White Balance B: %3d",GetIntegerValue,&LedInfo.whiteBalance.b,0,255,0,NULL,NULL,UpdateStripWhiteBalanceB},
    {eText,"Show White Balance",ShowWhiteBalance},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem AssociatedFileMenu[] = {
    {eExit,"Associated Files"},
    {eTextCurrentFile,"Save  %s.MIW",SaveAssociatedFile},
    {eTextCurrentFile,"Load  %s.MIW",LoadAssociatedFile},
    {eTextCurrentFile,"Erase %s.MIW",EraseAssociatedFile},
    {eExit,"MIW Files Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem StartFileMenu[] = {
    {eExit,"Start File"},
    {eText,"Save  START.MIW",SaveStartFile},
    {eText,"Load  START.MIW",LoadStartFile},
    {eText,"Erase START.MIW",EraseStartFile},
    {eMenu,"Associated Files",{.menu = AssociatedFileMenu}},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RepeatMenu[] = {
    {eExit,"Repeat/Chain Settings"},
    {eTextInt,"Repeat Count: %d",GetIntegerValue,&ImgInfo.repeatCount,1,100},
    {eTextInt,"Repeat Delay: %d.%d S",GetIntegerValue,&ImgInfo.repeatDelay,0,100,1},
    {eIfEqual,"",NULL,&ImgInfo.bShowBuiltInTests,false},
        {eBool,"Chain Images: %s",ToggleBool,&ImgInfo.bChainFiles,0,0,0,"On","Off"},
        {eIfEqual,"",NULL,&ImgInfo.bChainFiles,true},
            {eTextInt,"Chain Repeats: %d",GetIntegerValue,&ImgInfo.nChainRepeats,1,100},
            {eTextInt,"Chain Delay: %d.%d S",GetIntegerValue,&ImgInfo.nChainDelay,0,100,1},
            {eBool,"Chain Wait Key: %s",ToggleBool,&ImgInfo.bChainWaitKey,0,0,0,"Yes","No"},
        {eEndif},
    {eEndif},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem EepromMenu[] = {
    {eExit,"Saved Settings"},
    {eBool,"Autoload Settings: %s",ToggleBool,&bAutoLoadSettings,0,0,0,"On","Off"},
    {eText,"Save Current Settings",SaveEepromSettings},
    {eText,"Load Saved Settings",LoadEepromSettings},
    {eText,"Reset All Settings",FactorySettings},
    {eText,"Format EEPROM",EraseFlash},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroSelectMenu[] = {
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,0},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,1},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,2},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,3},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,4},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,5},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,6},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,7},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,8},
    {eMacroList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,9},
    // make sure this one is last
    {eTerminate}
};
//MenuItem MacroInfoMenu[] = {
//    {eExit,"Macro Information: %d",NULL,&ImgInfo.nCurrentMacro},
//    //{eText,"Desc: %s",SetFilter,&bnameFilter,0,0,0,"Enabled","Disabled"/*,UpdateFilter*/},
//    {eExit,PreviousMenu},
//    // make sure this one is last
//    {eTerminate}
//};
MenuItem MacroMenu[] = {
    {eExit,"Macro Operations"},
    //{eTextInt,"Macro #: %d",GetIntegerValue,&nCurrentMacro,0,9},
    {eIfEqual,"",NULL,&bRecordingMacro,false},
        {eMenu,"Select Macro: #%d",{.menu = MacroSelectMenu},&ImgInfo.nCurrentMacro},
        {eTextInt,"Run Macro: #%d",RunMacro,&ImgInfo.nCurrentMacro},
        {eBool,"Override Settings: %s",ToggleBool,&SystemInfo.bMacroUseCurrentSettings,0,0,0,"Yes","No"},
    {eElse},
        {eTextInt,"Recording Macro: #%d",NULL,&ImgInfo.nCurrentMacro},
    {eEndif},
    {eBool,"Record Macro: %s",ToggleBool,&bRecordingMacro,0,0,0,"On","Off"},
    {eIfEqual,"",NULL,&bRecordingMacro,false},
        {eTextInt,"Repeat Count: %d",GetIntegerValue,&ImgInfo.nRepeatCountMacro,1,100},
        {eTextInt,"Repeat Delay: %d.%d S",GetIntegerValue,&ImgInfo.nRepeatWaitMacro,0,100,1},
        {eTextInt,"Information: #%d",InfoMacro,&ImgInfo.nCurrentMacro},
       //{eTextInt,"Information: #%d",{.menu = MacroInfoMenu},&ImgInfo.nCurrentMacro},
        {eTextInt,"Load: #%d",LoadMacro,&ImgInfo.nCurrentMacro},
        {eTextInt,"Save: #%d",SaveMacro,&ImgInfo.nCurrentMacro},
        {eTextInt,"Delete: #%d",DeleteMacro,&ImgInfo.nCurrentMacro},
        {eTextInt,"Delete: Macro JSON File",DeleteMacroJson},
    {eEndif},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroMenuSimple[] = {
    {eExit,"Macros"},
    {eTextInt,"Run Macro: #%d",RunMacro,&ImgInfo.nCurrentMacro},
    {eMenu,"Select Macro: #%d",{.menu = MacroSelectMenu},&ImgInfo.nCurrentMacro},
    {eTextInt,"Information: #%d",InfoMacro,&ImgInfo.nCurrentMacro},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
const char* HelpMainMain = "Simple menu shows only commonly used commands";
const char* HelpMainImages = "Toggles between SD files and built-in patterns";
const char* HelpMainPreview = "Displays the BMP";
MenuItem MainMenu[] = {
    {eBool,"Main Menu: %s",ToggleFilesBuiltin,&SystemInfo.bSimpleMenu,0,0,0,"Simple","Full",NULL,NULL,HelpMainMain},
    {eBool,"Images: %s",ToggleFilesBuiltin,&ImgInfo.bShowBuiltInTests,0,0,0,"Built-Ins","SD Card BMP",NULL,NULL,HelpMainImages},
    {eIfEqual,"",NULL,&ImgInfo.bShowBuiltInTests,false},
        {eIfEqual,"",NULL,&SystemInfo.bSimpleMenu,false},
            {eText,"Preview BMP",ShowBmp,NULL,0,0,0,NULL,NULL,NULL,NULL,HelpMainPreview},
        {eEndif},
    {eEndif},
    {eIfEqual,"",NULL,&SystemInfo.bSimpleMenu,true},
        {eTextInt,"Column Time: %d mS",GetIntegerValue,&ImgInfo.nFrameHold,0,500,0,NULL,NULL,NULL,NULL,HelpImageColumnTime},
        {eTextInt,"Brightness: %d/255",GetIntegerValue,&LedInfo.nLEDBrightness,1,255,0,NULL,NULL,UpdateStripBrightness},
        {eBool,"Upside Down: %s",ToggleBool,&ImgInfo.bUpsideDown,0,0,0,"Yes","No"},
        {eBool,"Rotate 180: %s",ToggleBool,&ImgInfo.bRotate180,0,0,0,"Yes","No"},
        {eMenu,"Macros: #%d",{.menu = MacroMenuSimple},&ImgInfo.nCurrentMacro},
        {eIfEqual,"",NULL,&ImgInfo.bShowBuiltInTests,true},
            {eBuiltinOptions,"%s Options",{.builtin = BuiltInFiles}},
        {eEndif},
        {eText,"Sleep",Sleep},
    {eElse},
        {eBool,"Name Filter: %s",SetFilter,&bnameFilter,0,0,0,"Enabled","Disabled"/*,UpdateFilter*/},
        {eMenu,"Image Settings",{.menu = ImageMenu}},
        {eMenu,"Repeat/Chain Settings",{.menu = RepeatMenu}},
        {eMenu,"LED Strip Settings",{.menu = StripMenu}},
        {eIfEqual,"",NULL,&ImgInfo.bShowBuiltInTests,true},
            {eBuiltinOptions,"%s Options",{.builtin = BuiltInFiles}},
        {eElse},
            {eMenu,"MIW File Operations",{.menu = StartFileMenu}},
        {eEndif},
        {eMenu,"Macros: #%d",{.menu = MacroMenu},&ImgInfo.nCurrentMacro},
        {eMenu,"Saved Settings",{.menu = EepromMenu}},
        {eMenu,"System Settings",{.menu = SystemMenu}},
        {eText,"Light Bar",LightBar},
        {eMenu,"Light Bar Settings",{.menu = LedLightBarMenu}},
        {eReboot,"Reboot"},
        {eText,"Sleep",Sleep},
    {eEndif},
        // make sure this one is last
    {eTerminate}
};

BuiltInItem BuiltInFiles[] = {
    {"Barber Pole",BarberPole},
    {"Beats",TestBpm,BpmMenu},
    {"Bouncy Balls",TestBouncingBalls,BouncingBallsMenu},
    {"CheckerBoard",CheckerBoard,CheckerBoardMenu},
    {"Circles",TestCircles,CirclesMenu},
    {"Confetti",TestConfetti,ConfettiMenu},
    {"Cylon Eye",TestCylon,CylonEyeMenu},
    {"Juggle",TestJuggle},
    {"Lines",TestLines,LinesMenu},
    {"Meteor",TestMeteor,MeteorMenu},
    {"One Dot",RunningDot},
    {"Rainbow",TestRainbow,RainbowMenu},
    {"Rainbow Pulse",RainbowPulse,RainbowPulseMenu},
    {"Random Bars",RandomBars,RandomBarsMenu},
    {"Random Motion Dot",RandomDot},
    {"Sine Trails",TestSine,SineMenu},
    {"Solid Color",DisplayLedLightBar,LedLightBarMenu},
    {"Stripes",TestStripes},
    {"Twinkle",TestTwinkle,TwinkleMenu},
    {"Two Dots",OppositeRunningDots},
    {"Wedge",TestWedge,WedgeMenu},
};

// a stack for menus so we can find our way back
typedef struct MenuInfo {
    int index;      // active entry
    int offset;     // scrolled amount
    int menucount;  // how many entries in this menu
    MenuItem* menu; // pointer to the menu
};
MenuInfo* menuPtr;
std::stack<MenuInfo*> MenuStack;

bool bMenuChanged = true;

// save and load variables from MIW files
enum SETVARTYPE {
    vtInt,
    vtBool,
    vtRGB,
    vtShowFile,         // run a file on the display, the file has the path which is used to set the current path
    vtBuiltIn,          // bool for builtins or SD
    vtMacroTime,        // holds the estimated time to run this macro in mSec
};
struct SETTINGVAR {
    const char* name;
    void* address;
    enum SETVARTYPE type;
    int min, max;
};
struct SETTINGVAR SettingsVarList[] = {
    {"MACRO TIME",&recordingTime,vtMacroTime},
    {"STRIP BRIGHTNESS",&LedInfo.nLEDBrightness,vtInt,1,255},
    {"GAMMA CORRECTION",&LedInfo.bGammaCorrection,vtBool},
    {"WHITE BALANCE",&LedInfo.whiteBalance,vtRGB},
    {"FADE IN/OUT FRAMES",&ImgInfo.nFadeInOutFrames,vtInt,0,255},
    {"REPEAT COUNT",&ImgInfo.repeatCount,vtInt},
    {"REPEAT DELAY",&ImgInfo.repeatDelay,vtInt},
    {"FRAME TIME",&ImgInfo.nFrameHold,vtInt},
    {"USE FIXED TIME",&ImgInfo.bFixedTime,vtBool},
    {"FIXED IMAGE TIME",&ImgInfo.nFixedImageTime,vtInt},
    {"START DELAY",&ImgInfo.startDelay,vtInt},
    {"REVERSE IMAGE",&ImgInfo.bReverseImage,vtBool},
    {"UPSIDE DOWN IMAGE",&ImgInfo.bUpsideDown,vtBool},
    {"MIRROR PLAY IMAGE",&ImgInfo.bMirrorPlayImage,vtBool},
    {"MIRROR PLAY DELAY",&ImgInfo.nMirrorDelay,vtInt},
    {"CHAIN FILES",&ImgInfo.bChainFiles,vtBool},
    {"CHAIN REPEATS",&ImgInfo.nChainRepeats,vtInt},
    {"CHAIN DELAY",&ImgInfo.nChainDelay,vtInt},
    {"CHAIN WAIT FOR KEY",&ImgInfo.bChainWaitKey,vtBool},
    {"DISPLAY BRIGHTNESS",&SystemInfo.nDisplayBrightness,vtInt,0,100},
    {"DISPLAY MENULINE COLOR",&SystemInfo.menuTextColor,vtInt},
    {"SHOW BMP ON LCD",&SystemInfo.bShowDuringBmpFile,vtBool},
    {"MENU STAR",&SystemInfo.bMenuStar,vtBool},
    {"HILITE FILE",&SystemInfo.bHiLiteCurrentFile,vtBool},
    {"SELECT BUILTINS",&ImgInfo.bShowBuiltInTests,vtBuiltIn},       // this must be before the SHOW FILE command
    {"SHOW FILE",&FileToShow,vtShowFile},   // used in macros
};

RTC_DATA_ATTR int nMenuLineCount = 7;

// keep the display lines in here so we can scroll sideways if necessary
struct TEXTLINES {
    String Line;
    // the pixels length of this line
    int nRollLength;
    // current scroll pixel offsets
    int nRollOffset;
    // colors
    uint16_t foreColor, backColor;
    // whether we are going up or down
    int nRollDirection;
};
std::vector<struct TEXTLINES> TextLines;

typedef struct MACRO_INFO {
	String description;             // description of the file
	int mSeconds;                   // total time in mSeconds
	int pixels;                     // width in pixels
	float length;                   // how many meters based on 1:1 ratio with nTotalLeds
	std::vector<String> fileNames;  // list of all the filenames in this macro
};
MACRO_INFO MacroInfo[10];
SemaphoreHandle_t macroMutex;
// task for LED test on startup and then for sideways scrolling
TaskHandle_t TaskLEDTest;
TaskHandle_t TaskArtNet;
// global percent so we can send to web pages
volatile int g_nPercentDone;
// enums for what to fill the web page dropdowns with
enum WEB_PAGE_DROP_DOWNS {
    WPDD_FILES,     // image file types
    WPDD_MACROS,
};
typedef WEB_PAGE_DROP_DOWNS WebPageDropDowns;

void RebootSystem();
void VerifyRebootSystem();
void DoFileDelete();
void VerifyFileDelete();
void UtilitiesPage();
void WebToggleFileBuiltins();
void WebCancel();
void WebRunMacro();
void WebRunImage();
void WebChangeMacro();
void WebChangeBuiltinSettings();
void WebBuiltinSettings();
void WebChangeFile();
void WebToggleFilesBuiltins();
void WebChangeSettings();
void WebShowSettings();

struct ON_SERVER_ITEM {
    char* path;
    void(*function)();
};
typedef ON_SERVER_ITEM OnServerItem;
OnServerItem OnServerList[] = {
    {"/", HomePage},
    {"/download", File_Download},
    {"/upload", File_Upload},
    {"/settings", WebShowSettings},
    {"/changesettings", WebChangeSettings},
    {"/changefile", WebChangeFile},
    {"/builtinsettings", WebBuiltinSettings},
    {"/changebuiltinsettings", WebChangeBuiltinSettings},
    {"/changemacro", WebChangeMacro},
    {"/runimage", WebRunImage},
    {"/runmacro", WebRunMacro},
    {"/cancel", WebCancel},
    {"/togglefilesbuiltins", WebToggleFilesBuiltins},
    {"/utilities", UtilitiesPage},
    {"/verifyfiledelete", VerifyFileDelete},
    {"/dofiledelete", DoFileDelete},
    {"/verifyrebootsystem", VerifyRebootSystem},
    {"/rebootsystem", RebootSystem},
};

String MenuToHtml(MenuItem* menu, bool bActive = true, int nLevel = 0);
