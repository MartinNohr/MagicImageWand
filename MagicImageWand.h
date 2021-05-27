#pragma once

char* myVersion = "1.15";

// ***** Various switchs for options are set here *****
// 1 for standard SD library, 0 for the new exFat library which allows > 32GB SD cards
#define USE_STANDARD_SD 0
// *****
#define DIAL_BTN 15
#define FRAMEBUTTON 22
// reverse A and B for some PCB or wired versions, this is set for the new PCB, 0 for older PCB
#define LATEST_PCB 0
#if LATEST_PCB
    #define DIAL_A 12
    #define DIAL_B 13
#else
    #define DIAL_A 13
    #define DIAL_B 12
#endif

//#include <ArduinoJson.h>

//#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEServer.h>
//#include <BLE2902.h>

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
String wifiMacs = "MIW-" + WiFi.macAddress();
const char *ssid = wifiMacs.c_str();
const char *password = "12345678"; // not critical stuff, therefore simple password is enough
WebServer server(80);
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
#include <TFT_eSPI.h>
#include <vector>
#include <stack>
//#include <fonts/GFXFF/FreeMono18pt7b.h>

// definitions for preferences
char* prefsName = "MIW";
char* prefsVars = "vars";
char* prefsVersion = "version";
char* prefsBuiltInInfo = "builtininfo";
char* prefsImgInfo = "imginfo";
char* prefsLedInfo = "ledinfo";
char* prefsSystemInfo = "systeminfo";
char* prefsAutoload = "autoload";
// rotary dial values
char* prefsLongPressTimer = "longpress";
char* prefsDialSensitivity = "dialsense";
char* prefsDialSpeed = "dialspeed";
char* prefsDialReverse = "dialreverse";

#if USE_STANDARD_SD
  SPIClass spiSDCard;
#else
  //SPIClass spi1(VSPI);
  SdFs SD; // fat16/32 and exFAT
#endif

// the display
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define BTN_SELECT  CRotaryDialButton::BTN_CLICK
#define BTN_NONE    CRotaryDialButton::BTN_NONE
#define BTN_LEFT    CRotaryDialButton::BTN_LEFT
#define BTN_RIGHT   CRotaryDialButton::BTN_RIGHT
#define BTN_LONG    CRotaryDialButton::BTN_LONGPRESS


// functions
void ShowLeds(int mode = 0, CRGB colorval = TFT_BLACK, int imgHeight = 144);
void SetDisplayBrightness(int val);
void DisplayCurrentFile(bool path = true);
void DisplayLine(int line, String text, int16_t color = TFT_WHITE, int16_t backColor = TFT_BLACK);
void DisplayMenuLine(int line, int displine, String text);
void fixRGBwithGamma(byte* rp, byte* gp, byte* bp);
void WriteMessage(String txt, bool error = false, int wait = 2000);
void BarberPole();
void TestBouncingBalls();
void CheckerBoard();
void RandomBars();
void RunningDot();
void OppositeRunningDots();
void TestTwinkle();
void TestMeteor();
void TestCylon();
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
void ReportCouldNotCreateFile();
void handleFileUpload();
void append_page_header();
void append_page_footer();
void HomePage();
void File_Download();
void SD_file_download();
void File_Upload();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void SelectInput();
void ReportFileNotPresent();
void ReportSDNotPresent();
void IncreaseRepeatButton();
void DecreaseRepeatButton();
bool SaveLoadSettings(bool save = false, bool autoloadonly = false, bool ledonly = false, bool nodisplay = false);
CRotaryDialButton::Button ReadButton();
bool CheckCancel();

bool bAutoLoadSettings = false;

// LED controller pins
#define DATA_PIN1 2
#define DATA_PIN2 17
// The array of leds, up to 512
CRGB* leds;
// 0 feed from center, 1 serial from end, 2 from outsides
enum LED_STRIPS_WIRING_MODE { STRIPS_MIDDLE_WIRED = 0, STRIPS_CHAINED, STRIPS_OUTSIDE_WIRED };
struct LED_INFO {
    bool bSecondController = false;
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
    int nStripMaxCurrent = 2000;              // maximum milliamps to allow
};
typedef LED_INFO LED_INFO;
RTC_DATA_ATTR LED_INFO LedInfo;

// image settings
struct IMG_INFO {
    int nFrameHold = 10;                      // default for the frame delay
    bool bFixedTime = false;                  // set to use imagetime instead of framehold, the frame time will be calculated
    int nFixedImageTime = 5;                  // time to display image when fixedtime is used, in seconds
    int nFramePulseCount = 0;                 // advance frame when button pressed this many times, 0 means ignore
    bool bManualFrameAdvance = false;         // advance frame by clicking or rotating button
    bool bReverseImage = false;               // read the file lines in reverse
    bool bUpsideDown = false;                 // play the image upside down
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
    //bool bRepeatForever = false;                           // Variable to select auto repeat (until select button is pressed again)
    int repeatDelay = 0;                      // Variable for delay between repeats, 0.1 seconds
    int repeatCount = 1;                      // Variable to keep track of number of repeats
    int nCurrentMacro = 0;                    // the number of the macro to select or run
    int nRepeatWaitMacro = 0;                 // time between macro repeats, in 1/10 seconds
    int nRepeatCountMacro = 1;                // repeat count for macros
};
typedef IMG_INFO IMG_INFO;
RTC_DATA_ATTR IMG_INFO ImgInfo;

RTC_DATA_ATTR int CurrentFileIndex = 0;

struct SYSTEM_INFO {
    bool bShowBuiltInTests = false;             // list the internal file instead of the SD card
    uint16_t menuTextColor = TFT_BLUE;
    bool bMenuStar = false;
    bool bHiLiteCurrentFile = true;
    int nPreviewScrollCols = 120;               // now many columns to scroll with dial during preview
    bool bShowProgress = true;                  // show the progress bar
    bool bShowFolder = true;                    // show the path in front of the file
    int nDisplayBrightness = 50;                // this is in %
    bool bAllowMenuWrap = false;                // allows menus to wrap around from end and start instead of pinning
    bool bShowNextFiles = true;                 // show the next files in the main display
    bool bShowDuringBmpFile = false;            // set this to display the bmp on the LCD and the LED during BMP display
    int nSidewayScrollSpeed = 25;               // mSec for pixel scroll
    int nSidewaysScrollPause = 20;              // how long to wait at each end
    int nSidewaysScrollReverse = 3;             // reverse speed multiplier
    bool bMacroUseCurrentSettings = false;      // ignore settings in macro files when this is true
    int nBatteryFullLevel = 3150;               // 100% battery
    int nBatteryEmptyLevel = 2210;              // 0% battery, should cause a shutdown to save the batteries
};
typedef SYSTEM_INFO SYSTEM_INFO;
RTC_DATA_ATTR SYSTEM_INFO SystemInfo;

struct BUILTIN_INFO {
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
    // display all color
    bool bAllowRollover = true;       // lets 255->0 and 0->255
    bool bDisplayAllRGB = false;    // true for RGB, else HSV
    int nDisplayAllRed = 255;
    int nDisplayAllGreen = 255;
    int nDisplayAllBlue = 255;
    int nDisplayAllHue = 0;
    int nDisplayAllSaturation = 255;
    int nDisplayAllBrightness = 255;
    int nDisplayAllPixelCount = 288;
    bool bDisplayAllFromMiddle = true;
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
};
typedef BUILTIN_INFO BUILTIN_INFO;
RTC_DATA_ATTR BUILTIN_INFO BuiltinInfo;

// settings
bool bSdCardValid = false;              // set to true when card is found
bool bControllerReboot = false;                         // set this when controllers or led count changed
int AdjustStripIndex(int ix);
// get the real LED strip index from the desired index
void IRAM_ATTR SetPixel(int ix, CRGB pixel, int column = 0, int totalColumns = 1);
int nRepeatsLeft;                         // countdown while repeating, used for BLE also
int g = 0;                                // Variable for the Green Value
int b = 0;                                // Variable for the Blue Value
int r = 0;                                // Variable for the Red Value
// settings
constexpr auto NEXT_FOLDER_CHAR = '>';
constexpr auto PREVIOUS_FOLDER_CHAR = '<';
String currentFolder = "/";
int lastFileIndex = 0;                                  // save between switching of internal and SD
String lastFolder = "/";
std::vector<String> FileNames;
bool bSettingsMode = false;                             // set true when settings are displayed
bool bCancelRun = false;                  // set to cancel a running job
bool bCancelMacro = false;                // set to cancel a running macro
bool bRecordingMacro = false;             // set while recording
char FileToShow[100];
int recordingTime;                        // shows the time for each part
bool bRunningMacro = false;               // set while running
int nMacroRepeatsLeft = 1;                // set during macro running
volatile int nTimerSeconds;
// set this to the delay time while we get the next frame, also used for delay timers
volatile bool bStripWaiting = false;
esp_timer_handle_t oneshot_LED_timer;
esp_timer_create_args_t oneshot_LED_timer_args;
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
    eExit,              // closes this menu
    eIfEqual,           // start skipping menu entries if match with data value
    eElse,              // toggles the skipping
    eEndif,             // ends an if block
    eBuiltinOptions,    // use an internal settings menu if available, see the internal name,function list below (BuiltInFiles[])
    eReboot,            // reboot the system
    eList,              // used to make a selection from multiple choices
};

// we need to have a pointer reference to this in the MenuItem, the full declaration follows later
struct BuiltInItem;
std::vector<bool> bMenuValid;   // set to indicate menu item  is valid
struct MenuItem {
    enum eDisplayOperation op;
    const char* text;                   // text to display
    union {
        void(*function)(MenuItem*);     // called on click
        MenuItem* menu;                 // jump to another menu
        BuiltInItem* builtin;           // builtin items
    };
    const void* value;                  // associated variable
    long min;                           // the minimum value, also used for ifequal
    long max;                           // the maximum value, also size to compare for if
    int decimals;                       // 0 for int, 1 for 0.1
    char* on;                           // text for boolean true
    char* off;                          // text for boolean false
    // flag is 1 for first time, 0 for changes, and -1 for last call, bools only call this with -1
    void(*change)(MenuItem*, int flag); // call for each change, example: brightness change show effect
};
typedef MenuItem MenuItem;

// builtins
// built-in "files"
struct BuiltInItem {
    const char* text;
    void(*function)();
    MenuItem* menu;
};
typedef BuiltInItem BuiltInItem;
extern BuiltInItem BuiltInFiles[];

// some menu functions using menus
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
void GetIntegerValue(MenuItem*);
void ToggleBool(MenuItem*);
void ToggleFilesBuiltin(MenuItem* menu);
void UpdateDisplayBrightness(MenuItem* menu, int flag);
void SetMenuColor(MenuItem* menu);
void UpdateTotalLeds(MenuItem* menu, int flag);
void UpdateControllers(MenuItem* menu, int flag);
void UpdateStripsMode(MenuItem* menu, int flag);
void UpdateStripBrightness(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceR(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceG(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceB(MenuItem* menu, int flag);
bool WriteOrDeleteConfigFile(String filename, bool remove, bool startfile);
void RunMacro(MenuItem* menu);
void LoadMacro(MenuItem* menu);
void InfoMacro(MenuItem* menu);
void SaveMacro(MenuItem* menu);
void DeleteMacro(MenuItem* menu);
void LightBar(MenuItem* menu);
void Sleep(MenuItem* menu);
void ReadBattery(MenuItem* menu);
void ShowBmp(MenuItem* menu);
void ShowBattery(MenuItem* menu);

// SD details
#define SDcsPin    33  // GPIO33
#define SDSckPin   25  // GIPO25
#define SDMisoPin  27  // GPIO27
#define SDMosiPin  26  // GPIO26

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
    //{&bAutoLoadSettings, sizeof(bAutoLoadSettings)},

    //{&bRepeatForever, sizeof(bRepeatForever)},
    //{&nBackLightSeconds, sizeof(nBackLightSeconds)},
    //{&nMaxBackLight, sizeof(nMaxBackLight)},
    {&CurrentFileIndex,sizeof(CurrentFileIndex)},
    //{&CRotaryDialButton::m_bReverseDial,sizeof(CRotaryDialButton::m_bReverseDial)},
    //{&CRotaryDialButton::m_nDialSensitivity,sizeof(CRotaryDialButton::m_nDialSensitivity)},
    //{&CRotaryDialButton::m_nDialSpeed,sizeof(CRotaryDialButton::m_nDialSpeed)},
    //{&CRotaryDialButton::m_nLongPressTimerValue,sizeof(CRotaryDialButton::m_nLongPressTimerValue)},
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

MenuItem BouncingBallsMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Ball Count: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsCount,1,32},
    {eTextInt,"Decay: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsDecay,100,10000},
    {eTextInt,"First Color: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsFirstColor,0,31},
    {eTextInt,"Change Color Rate: %d",GetIntegerValue,&BuiltinInfo.nBouncingBallsChangeColors,0,10,0},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem CheckerBoardMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Hold Frames: %d",GetIntegerValue,&BuiltinInfo.nCheckerboardHoldframes,1,100},
    {eTextInt,"Black Width (pixels): %d",GetIntegerValue,&BuiltinInfo.nCheckboardBlackWidth,1,288},
    {eTextInt,"White Width (pixels): %d",GetIntegerValue,&BuiltinInfo.nCheckboardWhiteWidth,1,288},
    {eTextInt,"Add Pixels per Cycle: %d",GetIntegerValue,&BuiltinInfo.nCheckerboardAddPixels,0,144},
    {eBool,"Alternate per Cycle: %s",ToggleBool,&BuiltinInfo.bCheckerBoardAlternate,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Fade Time (S): %d.%d",GetIntegerValue,&BuiltinInfo.nRainbowFadeTime,0,100,1},
    {eTextInt,"Starting Hue: %d",GetIntegerValue,&BuiltinInfo.nRainbowInitialHue,0,255},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bRainbowCycleHue,0,0,0,"Yes","No"},
    {eTextInt,"Hue Delta Size: %d",GetIntegerValue,&BuiltinInfo.nRainbowHueDelta,1,255},
    {eBool,"Add Glitter: %s",ToggleBool,&BuiltinInfo.bRainbowAddGlitter,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowPulseMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Step Pause: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulsePause,0,1000},
    {eTextInt,"Color Rate Scale: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulseColorScale,0,256},
    {eTextInt,"Start Color: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulseStartColor,0,255},
    {eTextInt,"Color Saturation: %d",GetIntegerValue,&BuiltinInfo.nRainbowPulseSaturation,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem ConfettiMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bConfettiCycleHue,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem TwinkleMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"One or Many: %s",ToggleBool,&BuiltinInfo.bTwinkleOnlyOne,0,0,0,"One","Many"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem WedgeMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Fill Wedge: %s",ToggleBool,&BuiltinInfo.bWedgeFill,0,0,0,"Solid","<"},
    {eTextInt,"Red: %d",GetIntegerValue,&BuiltinInfo.nWedgeRed,0,255},
    {eTextInt,"Green: %d",GetIntegerValue,&BuiltinInfo.nWedgeGreen,0,255},
    {eTextInt,"Blue: %d",GetIntegerValue,&BuiltinInfo.nWedgeBlue,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem SineMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Starting Hue: %d",GetIntegerValue,&BuiltinInfo.nSineStartingHue,0,255},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bSineCycleHue,0,0,0,"Yes","No"},
    {eTextInt,"Speed: %d",GetIntegerValue,&BuiltinInfo.nSineSpeed,1,500},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem BpmMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Beats per minute: %d",GetIntegerValue,&BuiltinInfo.nBpmBeatsPerMinute,1,300},
    {eBool,"Cycle Hue: %s",ToggleBool,&BuiltinInfo.bBpmCycleHue,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem LinesMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"White Pixels: %d",GetIntegerValue,&BuiltinInfo.nLinesWhite,0,LedInfo.nTotalLeds},
    {eTextInt,"Black Pixels: %d",GetIntegerValue,&BuiltinInfo.nLinesBlack,0,LedInfo.nTotalLeds},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem CylonEyeMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Eye Size:  %d",GetIntegerValue,&BuiltinInfo.nCylonEyeSize,1,100},
    {eTextInt,"Eye Red:   %d",GetIntegerValue,&BuiltinInfo.nCylonEyeRed,0,255},
    {eTextInt,"Eye Green: %d",GetIntegerValue,&BuiltinInfo.nCylonEyeGreen,0,255},
    {eTextInt,"Eye Blue:  %d",GetIntegerValue,&BuiltinInfo.nCylonEyeBlue,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MeteorMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Meteor Size:  %d",GetIntegerValue,&BuiltinInfo.nMeteorSize,1,100},
    {eTextInt,"Meteor Red:   %d",GetIntegerValue,&BuiltinInfo.nMeteorRed,0,255},
    {eTextInt,"Meteor Green: %d",GetIntegerValue,&BuiltinInfo.nMeteorGreen,0,255},
    {eTextInt,"Meteor Blue:  %d",GetIntegerValue,&BuiltinInfo.nMeteorBlue,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem LedLightBarMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Allow rollover: %s",ToggleBool,&BuiltinInfo.bAllowRollover,0,0,0,"Yes","No"},
    {eBool,"Color Mode: %s",ToggleBool,&BuiltinInfo.bDisplayAllRGB,0,0,0,"RGB","HSL"},
    {eIfEqual,"",NULL,&BuiltinInfo.bDisplayAllRGB,true},
        {eTextInt,"Red: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllRed,0,255},
        {eTextInt,"Green: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllGreen,0,255},
        {eTextInt,"Blue: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllBlue,0,255},
    {eElse},
        {eTextInt,"Hue: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllHue,0,255},
        {eTextInt,"Saturation: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllSaturation,0,255},
        {eTextInt,"Brightness: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllBrightness,0,255},
    {eEndif},
    {eTextInt,"Pixels: %d",GetIntegerValue,&BuiltinInfo.nDisplayAllPixelCount,1,288},
    {eBool,"From: %s",ToggleBool,&BuiltinInfo.bDisplayAllFromMiddle,0,0,0,"Middle","End"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RandomBarsMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Hold Frames: %d",GetIntegerValue,&BuiltinInfo.nRandomBarsHoldframes,1,100},
    {eBool,"Alternating Black: %s",ToggleBool,&BuiltinInfo.bRandomBarsBlacks,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem SystemMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Display Bright: %d%%",GetIntegerValue,&SystemInfo.nDisplayBrightness,1,100,0,NULL,NULL,UpdateDisplayBrightness},
    {eBool,"Show BMP on LCD: %s",ToggleBool,&SystemInfo.bShowDuringBmpFile,0,0,0,"Yes","No"},
    {eBool,"Progress Bar: %s",ToggleBool,&SystemInfo.bShowProgress,0,0,0,"On","Off"},
    {eBool,"Current File: %s",ToggleBool,&SystemInfo.bHiLiteCurrentFile,0,0,0,"Color","Normal"},
    {eBool,"Show More Files: %s",ToggleBool,&SystemInfo.bShowNextFiles,0,0,0,"Yes","No"},
    {eBool,"Show Folder: %s",ToggleBool,&SystemInfo.bShowFolder,0,0,0,"Yes","No"},
    {eText,"Set Text Color",SetMenuColor},
    {eBool,"Menu Wrap: %s",ToggleBool,&SystemInfo.bAllowMenuWrap,0,0,0,"Yes","No"},
    {eBool,"Menu Choice: %s",ToggleBool,&SystemInfo.bMenuStar,0,0,0,"*","Color"},
    {eTextInt,"Sideways Scroll Speed: %d mS",GetIntegerValue,&SystemInfo.nSidewayScrollSpeed,1,1000},
    {eTextInt,"Sideways Scroll Pause: %d",GetIntegerValue,&SystemInfo.nSidewaysScrollPause,1,100},
    {eTextInt,"Sideways Scroll Reverse: %dx",GetIntegerValue,&SystemInfo.nSidewaysScrollReverse,1,20},
    {eTextInt,"Preview Scroll: %d px",GetIntegerValue,&SystemInfo.nPreviewScrollCols,1,240},
    {eBool,"Dial: %s",ToggleBool,&CRotaryDialButton::m_bReverseDial,0,0,0,"Reverse","Normal"},
    {eTextInt,"Dial Sensitivity: %d",GetIntegerValue,&CRotaryDialButton::m_nDialSensitivity,1,5},
    {eTextInt,"Dial Speed: %d",GetIntegerValue,&CRotaryDialButton::m_nDialSpeed,100,1000},
    {eTextInt,"Long Press count: %d",GetIntegerValue,&CRotaryDialButton::m_nLongPressTimerValue,2,200},
    {eText,"Read Battery",ShowBattery},
    {eTextInt,"100%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryFullLevel,2000,4000},
    {eTextInt,"0%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryEmptyLevel,1000,3000},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem ImageMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Timing Type: %s",ToggleBool,&ImgInfo.bFixedTime,0,0,0,"Image","Column"},
    {eIfEqual,"",NULL,&ImgInfo.bFixedTime,false},
        {eTextInt,"Column Time (mS): %d",GetIntegerValue,&ImgInfo.nFrameHold,0,500},
    {eElse},
        {eTextInt,"Image Time (S): %d",GetIntegerValue,&ImgInfo.nFixedImageTime,1,120},
    {eEndif},
    {eTextInt,"Start Delay (S): %d.%d",GetIntegerValue,&ImgInfo.startDelay,0,100,1},
    {eTextInt,"Fade I/O Columns : %d",GetIntegerValue,&ImgInfo.nFadeInOutFrames,0,255},
    {eBool,"Upside Down: %s",ToggleBool,&ImgInfo.bUpsideDown,0,0,0,"Yes","No"},
    {eIfEqual,"",NULL,&SystemInfo.bShowBuiltInTests,false},
        {eBool,"Walk: %s",ToggleBool,&ImgInfo.bReverseImage,0,0,0,"Left-Right","Right-Left"},
        {eBool,"Play Mirror Image: %s",ToggleBool,&ImgInfo.bMirrorPlayImage,0,0,0,"Yes","No"},
        {eIfEqual,"",NULL,&ImgInfo.bMirrorPlayImage,true},
            {eTextInt,"Mirror Delay (S): %d.%d",GetIntegerValue,&ImgInfo.nMirrorDelay,0,10,1},
        {eEndif},
        {eBool,"Scale Height to Fit: %s",ToggleBool,&ImgInfo.bScaleHeight,0,0,0,"On","Off"},
    {eEndif},
    {eBool,"144 to 288 Pixels: %s",ToggleBool,&ImgInfo.bDoublePixels,0,0,0,"Yes","No"},
    {eIfEqual,"",NULL,&SystemInfo.bShowBuiltInTests,false},
        {eBool,"Frame Advance: %s",ToggleBool,&ImgInfo.bManualFrameAdvance,0,0,0,"Step","Auto"},
        {eIfEqual,"",NULL,&ImgInfo.bManualFrameAdvance,true},
            {eTextInt,"Frame Counter: %d",GetIntegerValue,&ImgInfo.nFramePulseCount,0,32},
        {eEndif},
    {eEndif},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem StripMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Strip Bright: %d/255",GetIntegerValue,&LedInfo.nLEDBrightness,1,255,0,NULL,NULL,UpdateStripBrightness},
    {eTextInt,"Max mAmp: %d",GetIntegerValue,&LedInfo.nStripMaxCurrent,100,10000},
    {eBool,"LED Controllers: %s",ToggleBool,&LedInfo.bSecondController,0,0,0,"2","1",UpdateControllers},
    {eTextInt,"Total LEDs: %d",GetIntegerValue,&LedInfo.nTotalLeds,1,512,0,NULL,NULL,UpdateTotalLeds},
    {eTextInt,"LED Wiring Mode: %d",GetIntegerValue,&LedInfo.stripsMode,0,2,0,NULL,NULL,UpdateStripsMode},
    {eBool,"Gamma Correction: %s",ToggleBool,&LedInfo.bGammaCorrection,0,0,0,"On","Off"},
    {eTextInt,"White Balance R: %3d",GetIntegerValue,&LedInfo.whiteBalance.r,0,255,0,NULL,NULL,UpdateStripWhiteBalanceR},
    {eTextInt,"White Balance G: %3d",GetIntegerValue,&LedInfo.whiteBalance.g,0,255,0,NULL,NULL,UpdateStripWhiteBalanceG},
    {eTextInt,"White Balance B: %3d",GetIntegerValue,&LedInfo.whiteBalance.b,0,255,0,NULL,NULL,UpdateStripWhiteBalanceB},
    {eText,"Show White Balance",ShowWhiteBalance},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem AssociatedFileMenu[] = {
    {eExit,"MIW Files Menu"},
    {eTextCurrentFile,"Save  %s.MIW",SaveAssociatedFile},
    {eTextCurrentFile,"Load  %s.MIW",LoadAssociatedFile},
    {eTextCurrentFile,"Erase %s.MIW",EraseAssociatedFile},
    {eExit,"MIW Files Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem StartFileMenu[] = {
    {eExit,"Previous Menu"},
    {eText,"Save  START.MIW",SaveStartFile},
    {eText,"Load  START.MIW",LoadStartFile},
    {eText,"Erase START.MIW",EraseStartFile},
    {eMenu,"Associated Files",{.menu = AssociatedFileMenu}},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RepeatMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Repeat Count: %d",GetIntegerValue,&ImgInfo.repeatCount,1,100},
    {eTextInt,"Repeat Delay (S): %d.%d",GetIntegerValue,&ImgInfo.repeatDelay,0,100,1},
    {eIfEqual,"",NULL,&SystemInfo.bShowBuiltInTests,false},
        {eBool,"Chain Files: %s",ToggleBool,&ImgInfo.bChainFiles,0,0,0,"On","Off"},
        {eIfEqual,"",NULL,&ImgInfo.bChainFiles,true},
            {eTextInt,"Chain Repeats: %d",GetIntegerValue,&ImgInfo.nChainRepeats,1,100},
            {eTextInt,"Chain Delay (S): %d.%d",GetIntegerValue,&ImgInfo.nChainDelay,0,100,1},
            {eBool,"Chain Wait Key: %s",ToggleBool,&ImgInfo.bChainWaitKey,0,0,0,"Yes","No"},
        {eEndif},
    {eEndif},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem EepromMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Autoload Settings: %s",ToggleBool,&bAutoLoadSettings,0,0,0,"On","Off"},
    {eText,"Save Current Settings",SaveEepromSettings},
    {eText,"Load Saved Settings",LoadEepromSettings},
    {eText,"Reset All Settings",FactorySettings},
    {eText,"Format EEPROM",EraseFlash},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroSelectMenu[] = {
    //{eExit,"Previous Menu"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,0,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,1,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,2,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,3,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,4,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,5,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,6,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,7,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,8,0,0,"Used","Empty"},
    {eList,"#%d %s",NULL,&ImgInfo.nCurrentMacro,9,0,0,"Used","Empty"},
    //{eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroMenu[] = {
    {eExit,"Previous Menu"},
    //{eTextInt,"Macro #: %d",GetIntegerValue,&nCurrentMacro,0,9},
    {eIfEqual,"",NULL,&bRecordingMacro,false},
        {eMenu,"Select Macro: #%d",{.menu = MacroSelectMenu},&ImgInfo.nCurrentMacro},
        {eTextInt,"Run: #%d",RunMacro,&ImgInfo.nCurrentMacro},
        {eBool,"Override Settings: %s",ToggleBool,&SystemInfo.bMacroUseCurrentSettings,0,0,0,"Yes","No"},
    {eElse},
        {eTextInt,"Recording Macro: #%d",NULL,&ImgInfo.nCurrentMacro},
    {eEndif},
    {eBool,"Record: %s",ToggleBool,&bRecordingMacro,0,0,0,"On","Off"},
    {eIfEqual,"",NULL,&bRecordingMacro,false},
        {eTextInt,"Repeat Count: %d",GetIntegerValue,&ImgInfo.nRepeatCountMacro,1,100},
        {eTextInt,"Repeat Delay (S): %d.%d",GetIntegerValue,&ImgInfo.nRepeatWaitMacro,0,100,1},
        {eTextInt,"Info: #%d",InfoMacro,&ImgInfo.nCurrentMacro},
        {eTextInt,"Load: #%d",LoadMacro,&ImgInfo.nCurrentMacro},
        {eTextInt,"Save: #%d",SaveMacro,&ImgInfo.nCurrentMacro},
        {eTextInt,"Delete: #%d",DeleteMacro,&ImgInfo.nCurrentMacro},
    {eEndif},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MainMenu[] = {
    //{eText,"This is a very long dummy menu item for testing"},
    {eIfEqual,"",NULL,&SystemInfo.bShowBuiltInTests,true},
        {eBool,"Switch to SD Card",ToggleFilesBuiltin,&SystemInfo.bShowBuiltInTests,0,0,0,"On","Off"},
    {eElse},
        {eBool,"Switch to Built-ins",ToggleFilesBuiltin,&SystemInfo.bShowBuiltInTests,0,0,0,"On","Off"},
        {eText,"Preview BMP",ShowBmp},
    {eEndif},
    //{eText,"Another long menu item for testing"},
    {eMenu,"File Image Settings",{.menu = ImageMenu}},
    {eMenu,"Repeat/Chain Settings",{.menu = RepeatMenu}},
    {eMenu,"LED Strip Settings",{.menu = StripMenu}},
    {eIfEqual,"",NULL,&SystemInfo.bShowBuiltInTests,true},
        {eBuiltinOptions,"%s Options",{.builtin = BuiltInFiles}},
    {eElse},
        {eMenu,"MIW File Operations",{.menu = StartFileMenu}},
    {eEndif},
    {eMenu,"Macros: #%d",{.menu = MacroMenu},&ImgInfo.nCurrentMacro},
    {eMenu,"Saved Settings",{.menu = EepromMenu}},
    {eMenu,"System Settings",{.menu = SystemMenu}},
    {eText,"Light Bar",LightBar},
    {eMenu,"Light Bar Settings",{.menu = LedLightBarMenu}},
    //{eText,"Battery",ReadBattery},
    {eText,"IP: %s",NULL,&localIpAddress},
    {eText,"Sleep",Sleep},
    {eReboot,"Reboot"},
    // make sure this one is last
    {eTerminate}
};

BuiltInItem BuiltInFiles[] = {
    {"Barber Pole",BarberPole},
    {"Beats",TestBpm,BpmMenu},
    {"Bouncy Balls",TestBouncingBalls,BouncingBallsMenu},
    {"CheckerBoard",CheckerBoard,CheckerBoardMenu},
    {"Confetti",TestConfetti,ConfettiMenu},
    {"Cylon Eye",TestCylon,CylonEyeMenu},
    {"Juggle",TestJuggle},
    {"Lines",TestLines,LinesMenu},
    {"Meteor",TestMeteor,MeteorMenu},
    {"One Dot",RunningDot},
    {"Rainbow",TestRainbow,RainbowMenu},
    {"Rainbow Pulse",RainbowPulse,RainbowPulseMenu},
    {"Random Bars",RandomBars,RandomBarsMenu},
    {"Sine Trails",TestSine,SineMenu},
    {"Solid Color",DisplayLedLightBar,LedLightBarMenu},
    {"Stripes",TestStripes},
    {"Twinkle",TestTwinkle,TwinkleMenu},
    {"Two Dots",OppositeRunningDots},
    {"Wedge",TestWedge,WedgeMenu},
};

// a stack to hold the file indexes as we navigate folders
std::stack<int> FileIndexStack;

// a stack for menus so we can find our way back
struct MENUINFO {
    int index;      // active entry
    int offset;     // scrolled amount
    int menucount;  // how many entries in this menu
    MenuItem* menu; // pointer to the menu
};
typedef MENUINFO MenuInfo;
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
    vtMacroTime,        // holds the estimated time to run this macro
};
struct SETTINGVAR {
    char* name;
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
    {"SELECT BUILTINS",&SystemInfo.bShowBuiltInTests,vtBuiltIn},       // this must be before the SHOW FILE command
    {"SHOW FILE",&FileToShow,vtShowFile},   // used in macros
};

#define MENU_LINES 7

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
struct TEXTLINES TextLines[MENU_LINES];
