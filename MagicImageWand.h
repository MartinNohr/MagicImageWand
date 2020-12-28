#pragma once

String myVersion = "1.02";

// ***** Various switchs for options are set here *****
// 1 for standard SD library, 0 for the new exFat library
#define USE_STANDARD_SD 0
// *****
#define DIAL_BTN 15
#define DIAL_A 12
#define DIAL_B 13
#define FRAMEBUTTON 22

#include <ArduinoJson.h>

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
#include <EEPROM.h>
#include "RotaryDialButton.h"
#include <TFT_eSPI.h>
#include <vector>
#include <stack>
//#include <fonts/GFXFF/FreeMono18pt7b.h>

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
void DisplayCurrentFile(bool path = true);
void DisplayLine(int line, String text, int16_t color = TFT_WHITE);
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


bool bPauseDisplay = false; // set this so DisplayLine and Progress won't update display
CRotaryDialButton::Button ReadButton();
bool CheckCancel();

// eeprom values
// the signature is saved first in eeprom, followed by the autoload flag, all other values follow
char signature[] = { "MIW102" };   // set to make sure saved values are valid, change when savevalues is changed, nice to keep in sync with version from .ino file
RTC_DATA_ATTR bool bAutoLoadSettings = false;     // set to automatically load saved settings from eeprom
bool SaveSettings(bool save, bool bOnlySignature = false, bool bAutoloadFlag = false);

// settings
RTC_DATA_ATTR int nDisplayBrightness = 100;           // this is in %
bool bSdCardValid = false;              // set to true when card is found
// strip leds
#define DATA_PIN1 2
#define DATA_PIN2 17
#define NUM_LEDS 144
// Define the array of leds, up to 288
CRGB leds[NUM_LEDS * 2];
RTC_DATA_ATTR bool bSecondStrip = false;                // set true when two strips installed
#define STRIPLENGTH (NUM_LEDS*(1+(bSecondStrip?1:0)))
int AdjustStripIndex(int ix);
// get the real LED strip index from the desired index
void SetPixel(int ix, CRGB pixel);
RTC_DATA_ATTR int nStripBrightness = 20;                // Variable and default for the Brightness of the strip, from 1 to 255
RTC_DATA_ATTR int startDelay = 0;                       // Variable for delay between button press and start of light sequence, in seconds
//bool bRepeatForever = false;                           // Variable to select auto repeat (until select button is pressed again)
RTC_DATA_ATTR int repeatDelay = 0;                      // Variable for delay between repeats, 0.1 seconds
RTC_DATA_ATTR int repeatCount = 1;                      // Variable to keep track of number of repeats
int nRepeatsLeft;                         // countdown while repeating, used for BLE also
int g = 0;                                // Variable for the Green Value
int b = 0;                                // Variable for the Blue Value
int r = 0;                                // Variable for the Red Value
//CRGB whiteBalance = CRGB::White;
// white balance values, really only 8 bits, but menus need ints
struct {
    int r;
    int g;
    int b;
} whiteBalance = { 255,255,255 };
// settings
int charHeight = 19;
RTC_DATA_ATTR uint16_t menuLineColor = TFT_CYAN;
RTC_DATA_ATTR uint16_t menuLineActiveColor = TFT_WHITE;
#define NEXT_FOLDER_CHAR '>'
#define PREVIOUS_FOLDER_CHAR '<'
String currentFolder = "/";
RTC_DATA_ATTR int CurrentFileIndex = 0;
int lastFileIndex = 0;                                  // save between switching of internal and SD
String lastFolder = "/";
std::vector<String> FileNames;
bool bSettingsMode = false;                             // set true when settings are displayed
RTC_DATA_ATTR int nFrameHold = 10;                      // default for the frame delay
RTC_DATA_ATTR bool bFixedTime = false;                  // set to use imagetime instead of framehold, the frame time will be calculated
RTC_DATA_ATTR int nFixedImageTime = 5;                  // time to display image when fixedtime is used
RTC_DATA_ATTR int nFramePulseCount = 0;                 // advance frame when button pressed this many times, 0 means ignore
RTC_DATA_ATTR bool bManualFrameAdvance = false;         // advance frame by clicking or rotating button
RTC_DATA_ATTR bool bGammaCorrection = true;             // set to use the gamma table
RTC_DATA_ATTR bool bShowBuiltInTests = false;           // list the internal file instead of the SD card
RTC_DATA_ATTR bool bReverseImage = false;               // read the file lines in reverse
RTC_DATA_ATTR bool bUpsideDown = false;                 // play the image upside down
RTC_DATA_ATTR bool bDoublePixels = false;               // double the image line, to go from 144 to 288
RTC_DATA_ATTR bool bMirrorPlayImage = false;            // play the file twice, 2nd time reversed
RTC_DATA_ATTR int nMirrorDelay = 0;                     // pause between the two halves of the image
RTC_DATA_ATTR bool bChainFiles = false;                 // set to run all the files from current to the last one in the current folder
RTC_DATA_ATTR int nChainRepeats = 1;                    // how many times to repeat the chain
RTC_DATA_ATTR int nChainDelay = 0;                      // number of 1/10 seconds to delay between chained files
RTC_DATA_ATTR bool bShowProgress = true;                // show the progress bar
RTC_DATA_ATTR bool bShowFolder = true;                  // show the path in front of the file
RTC_DATA_ATTR bool bScaleHeight = false;                // scale the Y values to fit the number of pixels
bool bCancelRun = false;                  // set to cancel a running job
bool bCancelMacro = false;                // set to cancel a running macro
RTC_DATA_ATTR bool bAllowMenuWrap = false;              // allows menus to wrap around from end and start instead of pinning
RTC_DATA_ATTR bool bShowNextFiles = true;               // show the next files in the main display
RTC_DATA_ATTR int nCurrentMacro = 0;                    // the number of the macro to select or run
bool bRecordingMacro = false;             // set while recording
bool bRunningMacro = false;               // set while running
RTC_DATA_ATTR int nRepeatWaitMacro = 0;                 // time between macro repeats, in 1/10 seconds
RTC_DATA_ATTR int nRepeatCountMacro = 1;                // repeat count for macros
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
// show percentage
int nProgress = 0;

enum eDisplayOperation {
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
    eTerminate,         // must be last in a menu
};

// we need to have a pointer reference to this in the MenuItem, the full declaration follows later
struct BuiltInItem;

struct MenuItem {
    enum eDisplayOperation op;
    bool valid;                         // set to true if displayed for use
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
    // flag is 1 for first time, 0 for changes, and -1 for last call
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
void SetMenuColors(MenuItem* menu);
void UpdateStripBrightness(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceR(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceG(MenuItem* menu, int flag);
void UpdateStripWhiteBalanceB(MenuItem* menu, int flag);
bool WriteOrDeleteConfigFile(String filename, bool remove, bool startfile);
void RunMacro(MenuItem* menu);
void LoadMacro(MenuItem* menu);
void SaveMacro(MenuItem* menu);
void DeleteMacro(MenuItem* menu);
void LightBar(MenuItem* menu);
void Sleep(MenuItem* menu);
void ReadBattery(MenuItem* menu);
void ShowBmp(MenuItem* menu);

// SD details
#define SDcsPin    33  // GPIO33
#define SDSckPin   25  // GIPO25
#define SDMisoPin  27  // GPIO27
#define SDMosiPin  26  // GPIO26

// adjustment values for builtins
RTC_DATA_ATTR uint8_t gHue = 0; // rotating "base color" used by many of the patterns
// bouncing balls
RTC_DATA_ATTR int nBouncingBallsCount = 4;
RTC_DATA_ATTR int nBouncingBallsDecay = 1000;
RTC_DATA_ATTR int nBouncingBallsFirstColor = 0;   // first color, wraps to get all 32
RTC_DATA_ATTR int nBouncingBallsChangeColors = 0; // how many 100 count cycles to wait for change
// cylon eye
RTC_DATA_ATTR int nCylonEyeSize = 10;
RTC_DATA_ATTR int nCylonEyeRed = 255;
RTC_DATA_ATTR int nCylonEyeGreen = 0;
RTC_DATA_ATTR int nCylonEyeBlue = 0;
// random bars
RTC_DATA_ATTR bool bRandomBarsBlacks = true;
RTC_DATA_ATTR int nRandomBarsHoldframes = 10;
// meteor
RTC_DATA_ATTR int nMeteorSize = 10;
RTC_DATA_ATTR int nMeteorRed = 255;
RTC_DATA_ATTR int nMeteorGreen = 255;
RTC_DATA_ATTR int nMeteorBlue = 255;
// display all color
RTC_DATA_ATTR bool bAllowRollover = true;       // lets 255->0 and 0->255
RTC_DATA_ATTR bool bDisplayAllRGB = false;    // true for RGB, else HSV
RTC_DATA_ATTR int nDisplayAllRed = 255;
RTC_DATA_ATTR int nDisplayAllGreen = 255;
RTC_DATA_ATTR int nDisplayAllBlue = 255;
RTC_DATA_ATTR int nDisplayAllHue = 0;
RTC_DATA_ATTR int nDisplayAllSaturation = 255;
RTC_DATA_ATTR int nDisplayAllBrightness = 255;
// rainbow
RTC_DATA_ATTR int nRainbowHueDelta = 4;
RTC_DATA_ATTR int nRainbowInitialHue = 0;
RTC_DATA_ATTR int nRainbowFadeTime = 10;       // fade in out 0.1 Sec
RTC_DATA_ATTR bool bRainbowAddGlitter = false;
RTC_DATA_ATTR bool bRainbowCycleHue = false;
// twinkle
RTC_DATA_ATTR bool bTwinkleOnlyOne = false;
// confetti
RTC_DATA_ATTR bool bConfettiCycleHue = false;
// juggle

// sine
RTC_DATA_ATTR int nSineStartingHue = 0;
RTC_DATA_ATTR bool bSineCycleHue = false;
RTC_DATA_ATTR int nSineSpeed = 13;
// bpm
RTC_DATA_ATTR int nBpmBeatsPerMinute = 62;
RTC_DATA_ATTR bool bBpmCycleHue = false;
// checkerboard/bars
RTC_DATA_ATTR int nCheckerboardHoldframes = 10;
RTC_DATA_ATTR int nCheckboardBlackWidth = 12;
RTC_DATA_ATTR int nCheckboardWhiteWidth = 12;
RTC_DATA_ATTR bool bCheckerBoardAlternate = true;
RTC_DATA_ATTR int nCheckerboardAddPixels = 0;
// stripes

// black and white lines
RTC_DATA_ATTR int nLinesWhite = 5;
RTC_DATA_ATTR int nLinesBlack = 5;
// rainbow pulse settings
RTC_DATA_ATTR int nRainbowPulseColorScale = 10;
RTC_DATA_ATTR int nRainbowPulsePause = 5;
RTC_DATA_ATTR int nRainbowPulseSaturation = 255;
RTC_DATA_ATTR int nRainbowPulseStartColor = 0;
// wedge data
RTC_DATA_ATTR bool bWedgeFill = false;
RTC_DATA_ATTR int nWedgeRed = 255;
RTC_DATA_ATTR int nWedgeGreen = 255;
RTC_DATA_ATTR int nWedgeBlue = 255;

struct saveValues {
    void* val;
    int size;
};
// these values are saved in eeprom, the signature is first
const saveValues saveValueList[] = {
    {signature,sizeof(signature)},                      // first
    {&bAutoLoadSettings, sizeof(bAutoLoadSettings)},    // this must be second
    {&nStripBrightness, sizeof(nStripBrightness)},
    {&nFrameHold, sizeof(nFrameHold)},
    {&bFixedTime,sizeof(bFixedTime)},
    {&nFixedImageTime,sizeof(nFixedImageTime)},
    {&nFramePulseCount, sizeof(nFramePulseCount)},
    {&bManualFrameAdvance, sizeof(bManualFrameAdvance)},
    {&startDelay, sizeof(startDelay)},
    //{&bRepeatForever, sizeof(bRepeatForever)},
    {&repeatCount, sizeof(repeatCount)},
    {&repeatDelay, sizeof(repeatDelay)},
    {&bGammaCorrection, sizeof(bGammaCorrection)},
    {&bSecondStrip, sizeof(bSecondStrip)},
    //{&nBackLightSeconds, sizeof(nBackLightSeconds)},
    //{&nMaxBackLight, sizeof(nMaxBackLight)},
    {&CurrentFileIndex,sizeof(CurrentFileIndex)},
    {&bShowBuiltInTests,sizeof(bShowBuiltInTests)},
    {&bScaleHeight,sizeof(bScaleHeight)},
    {&bChainFiles,sizeof(bChainFiles)},
    {&bReverseImage,sizeof(bReverseImage)},
    {&bMirrorPlayImage,sizeof(bMirrorPlayImage)},
    {&nMirrorDelay,sizeof(nMirrorDelay)},
    {&bUpsideDown,sizeof(bUpsideDown)},
    {&bDoublePixels,sizeof(bDoublePixels)},
    {&nChainRepeats,sizeof(nChainRepeats)},
    {&nChainDelay,sizeof(nChainDelay)},
    {&whiteBalance,sizeof(whiteBalance)},
    {&bShowProgress,sizeof(bShowProgress)},
    {&bShowFolder,sizeof(bShowFolder)},
    {&bAllowMenuWrap,sizeof(bAllowMenuWrap)},
    {&bShowNextFiles,sizeof(bShowNextFiles)},
    {&CRotaryDialButton::m_bReverseDial,sizeof(CRotaryDialButton::m_bReverseDial)},
    {&CRotaryDialButton::m_nDialSensitivity,sizeof(CRotaryDialButton::m_nDialSensitivity)},
    {&CRotaryDialButton::m_nDialSpeed,sizeof(CRotaryDialButton::m_nDialSpeed)},
    {&CRotaryDialButton::m_nLongPressTimerValue,sizeof(CRotaryDialButton::m_nLongPressTimerValue)},
    {&nDisplayBrightness,sizeof(nDisplayBrightness)},
    {&menuLineColor,sizeof(menuLineColor)},
    {&menuLineActiveColor,sizeof(menuLineActiveColor)},

    // the built-in values
    // display all color
    {&bAllowRollover,sizeof(bAllowRollover)},
    {&bDisplayAllRGB,sizeof(bDisplayAllRGB)},
    {&nDisplayAllRed,sizeof(nDisplayAllRed)},
    {&nDisplayAllGreen,sizeof(nDisplayAllGreen)},
    {&nDisplayAllBlue,sizeof(nDisplayAllBlue)},
    {&nDisplayAllHue,sizeof(nDisplayAllHue)},
    {&nDisplayAllSaturation,sizeof(nDisplayAllSaturation)},
    {&nDisplayAllBrightness,sizeof(nDisplayAllBrightness)},
    // bouncing balls
    {&nBouncingBallsCount,sizeof(nBouncingBallsCount)},
    {&nBouncingBallsDecay,sizeof(nBouncingBallsDecay)},
    {&nBouncingBallsFirstColor,sizeof(nBouncingBallsFirstColor)},
    {&nBouncingBallsChangeColors,sizeof(nBouncingBallsChangeColors)},
    // cylon eye
    {&nCylonEyeSize,sizeof(nCylonEyeSize)},
    {&nCylonEyeRed,sizeof(nCylonEyeRed)},
    {&nCylonEyeGreen,sizeof(nCylonEyeGreen)},
    {&nCylonEyeBlue,sizeof(nCylonEyeBlue)},
    // random bars
    {&bRandomBarsBlacks,sizeof(bRandomBarsBlacks)},
    {&nRandomBarsHoldframes,sizeof(nRandomBarsHoldframes)},
    // meteor
    {&nMeteorSize,sizeof(nMeteorSize)},
    {&nMeteorRed,sizeof(nMeteorRed)},
    {&nMeteorGreen,sizeof(nMeteorGreen)},
    {&nMeteorBlue,sizeof(nMeteorBlue)},
    // rainbow
    {&nRainbowHueDelta,sizeof(nRainbowHueDelta)},
    {&nRainbowInitialHue,sizeof(nRainbowInitialHue)},
    {&nRainbowFadeTime,sizeof(nRainbowFadeTime)},
    {&bRainbowAddGlitter,sizeof(bRainbowAddGlitter)},
    {&bRainbowCycleHue,sizeof(bRainbowCycleHue)},
    // twinkle
    {&bTwinkleOnlyOne,sizeof(bTwinkleOnlyOne)},
    // confetti
    {&bConfettiCycleHue,sizeof(bConfettiCycleHue)},
    // juggle

    // sine
    {&nSineStartingHue,sizeof(nSineStartingHue)},
    {&bSineCycleHue,sizeof(bSineCycleHue)},
    {&nSineSpeed,sizeof(nSineSpeed)},
    // bpm
    {&nBpmBeatsPerMinute,sizeof(nBpmBeatsPerMinute)},
    {&bBpmCycleHue,sizeof(bBpmCycleHue)},
    // checkerboard/bars
    {&nCheckerboardHoldframes,sizeof(nCheckerboardHoldframes)},
    {&nCheckboardBlackWidth,sizeof(nCheckboardBlackWidth)},
    {&nCheckboardWhiteWidth,sizeof(nCheckboardWhiteWidth)},
    {&bCheckerBoardAlternate,sizeof(bCheckerBoardAlternate)},
    {&nCheckerboardAddPixels,sizeof(nCheckerboardAddPixels)},
    {&nCurrentMacro,sizeof(nCurrentMacro)},
    {&nRepeatCountMacro,sizeof(nRepeatCountMacro)},
    {&nRepeatWaitMacro,sizeof(nRepeatWaitMacro)},
    // lines values
    {&nLinesWhite,sizeof(nLinesWhite)},
    {&nLinesBlack,sizeof(nLinesBlack)},
    // rainbow pulse
    {&nRainbowPulseColorScale,sizeof(nRainbowPulseColorScale)},
    {&nRainbowPulsePause,sizeof(nRainbowPulsePause)},
    {&nRainbowPulseSaturation,sizeof(nRainbowPulseSaturation)},
    {&nRainbowPulseStartColor,sizeof(nRainbowPulseStartColor)},
    // wedge
    {&bWedgeFill,sizeof(bWedgeFill)},
    {&nWedgeBlue,sizeof(nWedgeBlue)},
    {&nWedgeRed,sizeof(nWedgeRed)},
    {&nWedgeGreen,sizeof(nWedgeGreen)},
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
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Ball Count: %d",GetIntegerValue,&nBouncingBallsCount,1,32},
    {eTextInt,false,"Decay (500-10000): %d",GetIntegerValue,&nBouncingBallsDecay,500,10000},
    {eTextInt,false,"First Color: %d",GetIntegerValue,&nBouncingBallsFirstColor,0,31},
    {eTextInt,false,"Change Color Rate: %d",GetIntegerValue,&nBouncingBallsChangeColors,0,10,0},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem CheckerBoardMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Hold Frames: %d",GetIntegerValue,&nCheckerboardHoldframes,1,100},
    {eTextInt,false,"Black Width (pixels): %d",GetIntegerValue,&nCheckboardBlackWidth,1,288},
    {eTextInt,false,"White Width (pixels): %d",GetIntegerValue,&nCheckboardWhiteWidth,1,288},
    {eTextInt,false,"Add Pixels per Cycle: %d",GetIntegerValue,&nCheckerboardAddPixels,0,144},
    {eBool,false,"Alternate per Cycle: %s",ToggleBool,&bCheckerBoardAlternate,0,0,0,"Yes","No"},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Fade Time (S): %d.%d",GetIntegerValue,&nRainbowFadeTime,0,100,1},
    {eTextInt,false,"Starting Hue: %d",GetIntegerValue,&nRainbowInitialHue,0,255},
    {eBool,false,"Cycle Hue: %s",ToggleBool,&bRainbowCycleHue,0,0,0,"Yes","No"},
    {eTextInt,false,"Hue Delta Size: %d",GetIntegerValue,&nRainbowHueDelta,1,255},
    {eBool,false,"Add Glitter: %s",ToggleBool,&bRainbowAddGlitter,0,0,0,"Yes","No"},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowPulseMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Step Pause: %d",GetIntegerValue,&nRainbowPulsePause,0,1000},
    {eTextInt,false,"Color Rate Scale: %d",GetIntegerValue,&nRainbowPulseColorScale,0,256},
    {eTextInt,false,"Start Color: %d",GetIntegerValue,&nRainbowPulseStartColor,0,255},
    {eTextInt,false,"Color Saturation: %d",GetIntegerValue,&nRainbowPulseSaturation,0,255},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem ConfettiMenu[] = {
    {eExit,false,"Previous Menu"},
    {eBool,false,"Cycle Hue: %s",ToggleBool,&bConfettiCycleHue,0,0,0,"Yes","No"},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem TwinkleMenu[] = {
    {eExit,false,"Previous Menu"},
    {eBool,false,"One or Many: %s",ToggleBool,&bTwinkleOnlyOne,0,0,0,"One","Many"},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem WedgeMenu[] = {
    {eExit,false,"Previous Menu"},
    {eBool,false,"Fill Wedge: %s",ToggleBool,&bWedgeFill,0,0,0,"Solid","<"},
    {eTextInt,false,"Red: %d",GetIntegerValue,&nWedgeRed,0,255},
    {eTextInt,false,"Green: %d",GetIntegerValue,&nWedgeGreen,0,255},
    {eTextInt,false,"Blue: %d",GetIntegerValue,&nWedgeBlue,0,255},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem SineMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Starting Hue: %d",GetIntegerValue,&nSineStartingHue,0,255},
    {eBool,false,"Cycle Hue: %s",ToggleBool,&bSineCycleHue,0,0,0,"Yes","No"},
    {eTextInt,false,"Speed: %d",GetIntegerValue,&nSineSpeed,1,500},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem BpmMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Beats per minute: %d",GetIntegerValue,&nBpmBeatsPerMinute,1,300},
    {eBool,false,"Cycle Hue: %s",ToggleBool,&bBpmCycleHue,0,0,0,"Yes","No"},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem LinesMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"White Pixels: %d",GetIntegerValue,&nLinesWhite,0,NUM_LEDS},
    {eTextInt,false,"Black Pixels: %d",GetIntegerValue,&nLinesBlack,0,NUM_LEDS},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem CylonEyeMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Eye Size:  %d",GetIntegerValue,&nCylonEyeSize,1,100},
    {eTextInt,false,"Eye Red:   %d",GetIntegerValue,&nCylonEyeRed,0,255},
    {eTextInt,false,"Eye Green: %d",GetIntegerValue,&nCylonEyeGreen,0,255},
    {eTextInt,false,"Eye Blue:  %d",GetIntegerValue,&nCylonEyeBlue,0,255},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MeteorMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Meteor Size:  %d",GetIntegerValue,&nMeteorSize,1,100},
    {eTextInt,false,"Meteor Red:   %d",GetIntegerValue,&nMeteorRed,0,255},
    {eTextInt,false,"Meteor Green: %d",GetIntegerValue,&nMeteorGreen,0,255},
    {eTextInt,false,"Meteor Blue:  %d",GetIntegerValue,&nMeteorBlue,0,255},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem LedLightBarMenu[] = {
    {eExit,false,"Previous Menu"},
    {eBool,false,"Allow rollover: %s",ToggleBool,&bAllowRollover,0,0,0,"Yes","No"},
    {eBool,false,"Color Mode: %s",ToggleBool,&bDisplayAllRGB,0,0,0,"RGB","HSL"},
    {eIfEqual,false,"",NULL,&bDisplayAllRGB,true},
        {eTextInt,false,"Red: %d",GetIntegerValue,&nDisplayAllRed,0,255},
        {eTextInt,false,"Green: %d",GetIntegerValue,&nDisplayAllGreen,0,255},
        {eTextInt,false,"Blue: %d",GetIntegerValue,&nDisplayAllBlue,0,255},
    {eElse},
        {eTextInt,false,"Hue: %d",GetIntegerValue,&nDisplayAllHue,0,255},
        {eTextInt,false,"Saturation: %d",GetIntegerValue,&nDisplayAllSaturation,0,255},
        {eTextInt,false,"Brightness: %d",GetIntegerValue,&nDisplayAllBrightness,0,255},
    {eEndif},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RandomBarsMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Hold Frames: %d",GetIntegerValue,&nRandomBarsHoldframes,1,100},
    {eBool,false,"Alternating Black: %s",ToggleBool,&bRandomBarsBlacks,0,0,0,"Yes","No"},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem SystemMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Display Bright: %d",GetIntegerValue,&nDisplayBrightness,1,100,0,NULL,NULL,UpdateDisplayBrightness},
    {eText,false,"Set Menu Colors",SetMenuColors},
    {eBool,false,"Menu Wrap: %s",ToggleBool,&bAllowMenuWrap,0,0,0,"Yes","No"},
    {eBool,false,"Show More Files: %s",ToggleBool,&bShowNextFiles,0,0,0,"Yes","No"},
    {eBool,false,"Show Folder: %s",ToggleBool,&bShowFolder,0,0,0,"Yes","No"},
    {eBool,false,"Progress Bar: %s",ToggleBool,&bShowProgress,0,0,0,"Yes","No"},
    {eMenu,false,"Light Bar Settings",{.menu = LedLightBarMenu}},
    {eBool,false,"Dial: %s",ToggleBool,&CRotaryDialButton::m_bReverseDial,0,0,0,"Reverse","Normal"},
    {eTextInt,false,"Dial Sensitivity: %d",GetIntegerValue,&CRotaryDialButton::m_nDialSensitivity,1,5},
    {eTextInt,false,"Dial Speed: %d",GetIntegerValue,&CRotaryDialButton::m_nDialSpeed,100,1000},
    {eTextInt,false,"Long Press count: %d",GetIntegerValue,&CRotaryDialButton::m_nLongPressTimerValue,2,200,0,NULL,NULL},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem ImageMenu[] = {
    {eExit,false,"Previous Menu"},
    {eBool,false,"Timing Type: %s",ToggleBool,&bFixedTime,0,0,0,"Image","Column"},
    {eIfEqual,false,"",NULL,&bFixedTime,false},
        {eTextInt,false,"Column Time (mS): %d",GetIntegerValue,&nFrameHold,0,500},
    {eElse},
        {eTextInt,false,"Image Time (S): %d",GetIntegerValue,&nFixedImageTime,1,120},
    {eEndif},
    {eTextInt,false,"Start Delay (S): %d.%d",GetIntegerValue,&startDelay,0,100,1},
    {eBool,false,"Upside Down: %s",ToggleBool,&bUpsideDown,0,0,0,"Yes","No"},
    {eIfEqual,false,"",NULL,&bShowBuiltInTests,false},
        {eBool,false,"Walk: %s",ToggleBool,&bReverseImage,0,0,0,"Left-Right","Right-Left"},
        {eBool,false,"Play Mirror Image: %s",ToggleBool,&bMirrorPlayImage,0,0,0,"Yes","No"},
        {eIfEqual,false,"",NULL,&bMirrorPlayImage,true},
            {eTextInt,false,"Mirror Delay (S): %d.%d",GetIntegerValue,&nMirrorDelay,0,10,1},
        {eEndif},
        {eBool,false,"Scale Height to Fit: %s",ToggleBool,&bScaleHeight,0,0,0,"On","Off"},
    {eEndif},
    {eIfEqual,false,"",NULL,&bSecondStrip,true},
        {eBool,false,"144 to 288 Pixels: %s",ToggleBool,&bDoublePixels,0,0,0,"Yes","No"},
    {eEndif},
    {eIfEqual,false,"",NULL,&bShowBuiltInTests,false},
        {eBool,false,"Frame Advance: %s",ToggleBool,&bManualFrameAdvance,0,0,0,"Step","Auto"},
        {eIfEqual,false,"",NULL,&bManualFrameAdvance,true},
            {eTextInt,false,"Frame Counter: %d",GetIntegerValue,&nFramePulseCount,0,32},
        {eEndif},
    {eEndif},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem StripMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Strip Brightness: %d",GetIntegerValue,&nStripBrightness,1,255,0,NULL,NULL,UpdateStripBrightness},
    {eBool,false,"LED strips: %s",ToggleBool,&bSecondStrip,0,0,0,"2","1"},
    {eBool,false,"Gamma Correction: %s",ToggleBool,&bGammaCorrection,0,0,0,"On","Off"},
    {eTextInt,false,"White Balance R: %3d",GetIntegerValue,&whiteBalance.r,0,255,0,NULL,NULL,UpdateStripWhiteBalanceR},
    {eTextInt,false,"White Balance G: %3d",GetIntegerValue,&whiteBalance.g,0,255,0,NULL,NULL,UpdateStripWhiteBalanceG},
    {eTextInt,false,"White Balance B: %3d",GetIntegerValue,&whiteBalance.b,0,255,0,NULL,NULL,UpdateStripWhiteBalanceB},
    {eText,false,"Show White Balance",ShowWhiteBalance},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem AssociatedFileMenu[] = {
    {eExit,false,"MIW Files Menu"},
    {eTextCurrentFile,false,"Save  %s.MIW",SaveAssociatedFile},
    {eTextCurrentFile,false,"Load  %s.MIW",LoadAssociatedFile},
    {eTextCurrentFile,false,"Erase %s.MIW",EraseAssociatedFile},
    {eExit,false,"MIW Files Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem StartFileMenu[] = {
    {eExit,false,"Previous Menu"},
    {eText,false,"Save  START.MIW",SaveStartFile},
    {eText,false,"Load  START.MIW",LoadStartFile},
    {eText,false,"Erase START.MIW",EraseStartFile},
    {eMenu,false,"Associated Files",{.menu = AssociatedFileMenu}},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RepeatMenu[] = {
    {eExit,false,"Previous Menu"},
    {eTextInt,false,"Repeat Count: %d",GetIntegerValue,&repeatCount,1,100},
    {eTextInt,false,"Repeat Delay (S): %d.%d",GetIntegerValue,&repeatDelay,0,100,1},
    {eIfEqual,false,"",NULL,&bShowBuiltInTests,false},
        {eBool,false,"Chain Files: %s",ToggleBool,&bChainFiles,0,0,0,"On","Off"},
        {eIfEqual,false,"",NULL,&bChainFiles,true},
            {eTextInt,false,"Chain Repeats: %d",GetIntegerValue,&nChainRepeats,1,100},
            {eTextInt,false,"Chain Delay (S): %d.%d",GetIntegerValue,&nChainDelay,0,100,1},
        {eEndif},
    {eEndif},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem EepromMenu[] = {
    {eExit,false,"Previous Menu"},
    {eBool,false,"Autoload Saved: %s",ToggleBool,&bAutoLoadSettings,0,0,0,"On","Off"},
    {eText,false,"Save Current Settings",SaveEepromSettings},
    {eText,false,"Load Saved Settings",LoadEepromSettings},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroSelectMenu[] = {
    //{eExit,false,"Previous Menu"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,0,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,1,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,2,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,3,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,4,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,5,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,6,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,7,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,8,0,0,"Used","Empty"},
    {eList,false,"Macro: #%d %s",NULL,&nCurrentMacro,9,0,0,"Used","Empty"},
    //{eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroMenu[] = {
    {eExit,false,"Previous Menu"},
    //{eTextInt,false,"Macro #: %d",GetIntegerValue,&nCurrentMacro,0,9},
    {eIfEqual,false,"",NULL,&bRecordingMacro,false},
        {eMenu,false,"Select Macro: #%d",{.menu = MacroSelectMenu},&nCurrentMacro},
        {eText,false,"Run: #%d",RunMacro,&nCurrentMacro},
    {eElse},
        {eText,false,"Recording Macro: #%d",NULL,&nCurrentMacro},
    {eEndif},
    {eBool,false,"Record: %s",ToggleBool,&bRecordingMacro,0,0,0,"On","Off"},
    {eIfEqual,false,"",NULL,&bRecordingMacro,false},
        {eTextInt,false,"Repeat Count: %d",GetIntegerValue,&nRepeatCountMacro,1,100},
        {eTextInt,false,"Repeat Delay (S): %d.%d",GetIntegerValue,&nRepeatWaitMacro,0,100,1},
        {eText,false,"Load: #%d",LoadMacro,&nCurrentMacro},
        {eText,false,"Save: #%d",SaveMacro,&nCurrentMacro},
        {eText,false,"Delete: #%d",DeleteMacro,&nCurrentMacro},
    {eEndif},
    {eExit,false,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MainMenu[] = {
    {eIfEqual,false,"",NULL,&bShowBuiltInTests,true},
        {eBool,false,"Switch to SD Card",ToggleFilesBuiltin,&bShowBuiltInTests,0,0,0,"On","Off"},
    {eElse},
        {eBool,false,"Switch to Built-ins",ToggleFilesBuiltin,&bShowBuiltInTests,0,0,0,"On","Off"},
        {eText,false,"Preview BMP",ShowBmp},
    {eEndif},
    {eMenu,false,"File Image Settings",{.menu = ImageMenu}},
    {eMenu,false,"Repeat Settings",{.menu = RepeatMenu}},
    {eMenu,false,"LED Strip Settings",{.menu = StripMenu}},
    {eIfEqual,false,"",NULL,&bShowBuiltInTests,true},
        {eBuiltinOptions,false,"%s Options",{.builtin = BuiltInFiles}},
    {eElse},
        {eMenu,false,"MIW File Operations",{.menu = StartFileMenu}},
    {eEndif},
    {eMenu,false,"Macros: #%d",{.menu = MacroMenu},&nCurrentMacro},
    {eMenu,false,"Saved Settings",{.menu = EepromMenu}},
    {eMenu,false,"System Settings",{.menu = SystemMenu}},
    {eText,false,"Light Bar",LightBar},
    //{eText,false,"Battery",ReadBattery},
    {eText,false,"IP: %s",NULL,&localIpAddress},
    {eText,false,"Sleep",Sleep},
    {eReboot,false,"Reboot"},
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

char FileToShow[40];
// save and load variables from MIW files
enum SETVARTYPE {
    vtInt,
    vtBool,
    vtRGB,
    vtShowFile,     // run a file on the display, the file has the path which is used to set the current path
    vtBuiltIn,      // bool for builtins or SD
};
struct SETTINGVAR {
    char* name;
    void* address;
    enum SETVARTYPE type;
    int min, max;
};
struct SETTINGVAR SettingsVarList[] = {
    {"SECOND STRIP",&bSecondStrip,vtBool},
    {"STRIP BRIGHTNESS",&nStripBrightness,vtInt,1,255},
    {"REPEAT COUNT",&repeatCount,vtInt},
    {"REPEAT DELAY",&repeatDelay,vtInt},
    {"FRAME TIME",&nFrameHold,vtInt},
    {"USE FIXED TIME",&bFixedTime,vtBool},
    {"FIXED IMAGE TIME",&nFixedImageTime,vtInt},
    {"START DELAY",&startDelay,vtInt},
    {"REVERSE IMAGE",&bReverseImage,vtBool},
    {"UPSIDE DOWN IMAGE",&bUpsideDown,vtBool},
    {"MIRROR PLAY IMAGE",&bMirrorPlayImage,vtBool},
    {"MIRROR PLAY DELAY",&nMirrorDelay,vtInt},
    {"CHAIN FILES",&bChainFiles,vtBool},
    {"CHAIN REPEATS",&nChainRepeats,vtInt},
    {"CHAIN DELAY",&nChainDelay,vtInt},
    {"WHITE BALANCE",&whiteBalance,vtRGB},
    {"DISPLAY BRIGHTNESS",&nDisplayBrightness,vtInt,0,100},
    {"DISPLAY MENULINE COLOR",&menuLineColor,vtInt},
    {"DISPLAY MENULINE ACTIVE COLOR",&menuLineActiveColor,vtInt},
    {"GAMMA CORRECTION",&bGammaCorrection,vtBool},
    {"SELECT BUILTINS",&bShowBuiltInTests,vtBuiltIn},       // this must be before the SHOW FILE command
    {"SHOW FILE",&FileToShow,vtShowFile},
    {"REVERSE DIAL",&CRotaryDialButton::m_bReverseDial,vtBool},
};
