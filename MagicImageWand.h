#pragma once

String myVersion = "1.09";
#define MY_EEPROM_VERSION "MIW109"

// ***** Various switchs for options are set here *****
// 1 for standard SD library, 0 for the new exFat library
#define USE_STANDARD_SD 0
// *****
#define DIAL_BTN 15
#define FRAMEBUTTON 22
// reverse A and B for some PCB or wired versions, this is set for the new PCB, 0 for older PCB
#define LATEST_PCB 1
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

bool bPauseDisplay = false; // set this so DisplayLine and Progress won't update display
CRotaryDialButton::Button ReadButton();
bool CheckCancel();

// eeprom values
// the signature is saved first in eeprom, followed by the autoload flag, all other values follow
char signature[] = { MY_EEPROM_VERSION };   // set to make sure saved values are valid, change when savevalues is changed, nice to keep in sync with version from .ino file
RTC_DATA_ATTR bool bAutoLoadSettings = false;     // set to automatically load saved settings from eeprom
bool SaveSettings(bool save, bool bOnlySignature = false, bool bAutoloadFlag = false);

// settings
RTC_DATA_ATTR int nDisplayBrightness = 50;           // this is in %
bool bSdCardValid = false;              // set to true when card is found
// strip leds
#define DATA_PIN1 2
#define DATA_PIN2 17
#define NUM_LEDS 144
// Define the array of leds, up to 288
enum STRIPS_MODE { STRIPS_MIDDLE_WIRED = 0, STRIPS_CHANGED, STRIPS_OUTSIDE_WIRED };
RTC_DATA_ATTR STRIPS_MODE stripsMode = STRIPS_MIDDLE_WIRED;
CRGB leds[NUM_LEDS * 2];
RTC_DATA_ATTR bool bSecondStrip = false;                // set true when two strips installed
#define STRIPLENGTH (NUM_LEDS*(1+(bSecondStrip?1:0)))
int AdjustStripIndex(int ix);
// get the real LED strip index from the desired index
void IRAM_ATTR SetPixel(int ix, CRGB pixel, int column = 0, int totalColumns = 1);
RTC_DATA_ATTR int nStripBrightness = 25;                // Variable and default for the Brightness of the strip, from 1 to 255
RTC_DATA_ATTR int nStripMaxCurrent = 2000;              // maximum milliamps to allow
RTC_DATA_ATTR int nFadeInOutFrames = 0;                 // number of frames to use for fading in and out
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
RTC_DATA_ATTR uint16_t menuTextColor = TFT_BLUE;
RTC_DATA_ATTR bool bMenuStar = false;
RTC_DATA_ATTR bool bHiLiteCurrentFile = true;
#define NEXT_FOLDER_CHAR '>'
#define PREVIOUS_FOLDER_CHAR '<'
String currentFolder = "/";
RTC_DATA_ATTR int CurrentFileIndex = 0;
int lastFileIndex = 0;                                  // save between switching of internal and SD
String lastFolder = "/";
std::vector<String> FileNames;
bool bSettingsMode = false;                             // set true when settings are displayed
RTC_DATA_ATTR int nPreviewScrollCols = 120;             // now many columns to scroll with dial during preview
RTC_DATA_ATTR int nFrameHold = 10;                      // default for the frame delay
RTC_DATA_ATTR bool bFixedTime = false;                  // set to use imagetime instead of framehold, the frame time will be calculated
RTC_DATA_ATTR int nFixedImageTime = 5;                  // time to display image when fixedtime is used, in seconds
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
void SetMenuColor(MenuItem* menu);
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
RTC_DATA_ATTR int nDisplayAllPixelCount = 144;
RTC_DATA_ATTR bool bDisplayAllFromMiddle = true;
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
	{&nStripMaxCurrent, sizeof(nStripMaxCurrent)},
    {&nFadeInOutFrames, sizeof(nFadeInOutFrames)},
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
    {&stripsMode, sizeof(stripsMode)},
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
    {&menuTextColor,sizeof(menuTextColor)},
    {&bMenuStar,sizeof(bMenuStar)},
    {&bHiLiteCurrentFile,sizeof(bHiLiteCurrentFile)},
    {&nPreviewScrollCols,sizeof(nPreviewScrollCols)},
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
    {&nDisplayAllPixelCount,sizeof(nDisplayAllPixelCount)},
    {&bDisplayAllFromMiddle,sizeof(bDisplayAllFromMiddle)},
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
    {eExit,"Previous Menu"},
    {eTextInt,"Ball Count: %d",GetIntegerValue,&nBouncingBallsCount,1,32},
    {eTextInt,"Decay (500-10000): %d",GetIntegerValue,&nBouncingBallsDecay,500,10000},
    {eTextInt,"First Color: %d",GetIntegerValue,&nBouncingBallsFirstColor,0,31},
    {eTextInt,"Change Color Rate: %d",GetIntegerValue,&nBouncingBallsChangeColors,0,10,0},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem CheckerBoardMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Hold Frames: %d",GetIntegerValue,&nCheckerboardHoldframes,1,100},
    {eTextInt,"Black Width (pixels): %d",GetIntegerValue,&nCheckboardBlackWidth,1,288},
    {eTextInt,"White Width (pixels): %d",GetIntegerValue,&nCheckboardWhiteWidth,1,288},
    {eTextInt,"Add Pixels per Cycle: %d",GetIntegerValue,&nCheckerboardAddPixels,0,144},
    {eBool,"Alternate per Cycle: %s",ToggleBool,&bCheckerBoardAlternate,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Fade Time (S): %d.%d",GetIntegerValue,&nRainbowFadeTime,0,100,1},
    {eTextInt,"Starting Hue: %d",GetIntegerValue,&nRainbowInitialHue,0,255},
    {eBool,"Cycle Hue: %s",ToggleBool,&bRainbowCycleHue,0,0,0,"Yes","No"},
    {eTextInt,"Hue Delta Size: %d",GetIntegerValue,&nRainbowHueDelta,1,255},
    {eBool,"Add Glitter: %s",ToggleBool,&bRainbowAddGlitter,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RainbowPulseMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Step Pause: %d",GetIntegerValue,&nRainbowPulsePause,0,1000},
    {eTextInt,"Color Rate Scale: %d",GetIntegerValue,&nRainbowPulseColorScale,0,256},
    {eTextInt,"Start Color: %d",GetIntegerValue,&nRainbowPulseStartColor,0,255},
    {eTextInt,"Color Saturation: %d",GetIntegerValue,&nRainbowPulseSaturation,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem ConfettiMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Cycle Hue: %s",ToggleBool,&bConfettiCycleHue,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem TwinkleMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"One or Many: %s",ToggleBool,&bTwinkleOnlyOne,0,0,0,"One","Many"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem WedgeMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Fill Wedge: %s",ToggleBool,&bWedgeFill,0,0,0,"Solid","<"},
    {eTextInt,"Red: %d",GetIntegerValue,&nWedgeRed,0,255},
    {eTextInt,"Green: %d",GetIntegerValue,&nWedgeGreen,0,255},
    {eTextInt,"Blue: %d",GetIntegerValue,&nWedgeBlue,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem SineMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Starting Hue: %d",GetIntegerValue,&nSineStartingHue,0,255},
    {eBool,"Cycle Hue: %s",ToggleBool,&bSineCycleHue,0,0,0,"Yes","No"},
    {eTextInt,"Speed: %d",GetIntegerValue,&nSineSpeed,1,500},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem BpmMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Beats per minute: %d",GetIntegerValue,&nBpmBeatsPerMinute,1,300},
    {eBool,"Cycle Hue: %s",ToggleBool,&bBpmCycleHue,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem LinesMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"White Pixels: %d",GetIntegerValue,&nLinesWhite,0,NUM_LEDS},
    {eTextInt,"Black Pixels: %d",GetIntegerValue,&nLinesBlack,0,NUM_LEDS},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem CylonEyeMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Eye Size:  %d",GetIntegerValue,&nCylonEyeSize,1,100},
    {eTextInt,"Eye Red:   %d",GetIntegerValue,&nCylonEyeRed,0,255},
    {eTextInt,"Eye Green: %d",GetIntegerValue,&nCylonEyeGreen,0,255},
    {eTextInt,"Eye Blue:  %d",GetIntegerValue,&nCylonEyeBlue,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MeteorMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Meteor Size:  %d",GetIntegerValue,&nMeteorSize,1,100},
    {eTextInt,"Meteor Red:   %d",GetIntegerValue,&nMeteorRed,0,255},
    {eTextInt,"Meteor Green: %d",GetIntegerValue,&nMeteorGreen,0,255},
    {eTextInt,"Meteor Blue:  %d",GetIntegerValue,&nMeteorBlue,0,255},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem LedLightBarMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Allow rollover: %s",ToggleBool,&bAllowRollover,0,0,0,"Yes","No"},
    {eBool,"Color Mode: %s",ToggleBool,&bDisplayAllRGB,0,0,0,"RGB","HSL"},
    {eIfEqual,"",NULL,&bDisplayAllRGB,true},
        {eTextInt,"Red: %d",GetIntegerValue,&nDisplayAllRed,0,255},
        {eTextInt,"Green: %d",GetIntegerValue,&nDisplayAllGreen,0,255},
        {eTextInt,"Blue: %d",GetIntegerValue,&nDisplayAllBlue,0,255},
    {eElse},
        {eTextInt,"Hue: %d",GetIntegerValue,&nDisplayAllHue,0,255},
        {eTextInt,"Saturation: %d",GetIntegerValue,&nDisplayAllSaturation,0,255},
        {eTextInt,"Brightness: %d",GetIntegerValue,&nDisplayAllBrightness,0,255},
    {eEndif},
    {eTextInt,"Pixels: %d",GetIntegerValue,&nDisplayAllPixelCount,1,144},
    {eBool,"From: %s",ToggleBool,&bDisplayAllFromMiddle,0,0,0,"Middle","End"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem RandomBarsMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Hold Frames: %d",GetIntegerValue,&nRandomBarsHoldframes,1,100},
    {eBool,"Alternating Black: %s",ToggleBool,&bRandomBarsBlacks,0,0,0,"Yes","No"},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem SystemMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Display Bright: %d%%",GetIntegerValue,&nDisplayBrightness,1,100,0,NULL,NULL,UpdateDisplayBrightness},
    {eText,"Set Text Color",SetMenuColor},
    {eBool,"Menu Wrap: %s",ToggleBool,&bAllowMenuWrap,0,0,0,"Yes","No"},
    {eBool,"Menu Select: %s",ToggleBool,&bMenuStar,0,0,0,"*","Color"},
    {eBool,"Current File: %s",ToggleBool,&bHiLiteCurrentFile,0,0,0,"Color","Normal"},
    {eBool,"Show More Files: %s",ToggleBool,&bShowNextFiles,0,0,0,"Yes","No"},
    {eBool,"Show Folder: %s",ToggleBool,&bShowFolder,0,0,0,"Yes","No"},
    {eBool,"Progress Bar: %s",ToggleBool,&bShowProgress,0,0,0,"Yes","No"},
    {eMenu,"Light Bar Settings",{.menu = LedLightBarMenu}},
    {eTextInt,"Preview Scroll: %d px",GetIntegerValue,&nPreviewScrollCols,1,240},
    {eBool,"Dial: %s",ToggleBool,&CRotaryDialButton::m_bReverseDial,0,0,0,"Reverse","Normal"},
    {eTextInt,"Dial Sensitivity: %d",GetIntegerValue,&CRotaryDialButton::m_nDialSensitivity,1,5},
    {eTextInt,"Dial Speed: %d",GetIntegerValue,&CRotaryDialButton::m_nDialSpeed,100,1000},
    {eTextInt,"Long Press count: %d",GetIntegerValue,&CRotaryDialButton::m_nLongPressTimerValue,2,200},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem ImageMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Timing Type: %s",ToggleBool,&bFixedTime,0,0,0,"Image","Column"},
    {eIfEqual,"",NULL,&bFixedTime,false},
        {eTextInt,"Column Time (mS): %d",GetIntegerValue,&nFrameHold,0,500},
    {eElse},
        {eTextInt,"Image Time (S): %d",GetIntegerValue,&nFixedImageTime,1,120},
    {eEndif},
    {eTextInt,"Start Delay (S): %d.%d",GetIntegerValue,&startDelay,0,100,1},
    {eTextInt,"Fade I/O Columns : %d",GetIntegerValue,&nFadeInOutFrames,0,255},
    {eBool,"Upside Down: %s",ToggleBool,&bUpsideDown,0,0,0,"Yes","No"},
    {eIfEqual,"",NULL,&bShowBuiltInTests,false},
        {eBool,"Walk: %s",ToggleBool,&bReverseImage,0,0,0,"Left-Right","Right-Left"},
        {eBool,"Play Mirror Image: %s",ToggleBool,&bMirrorPlayImage,0,0,0,"Yes","No"},
        {eIfEqual,"",NULL,&bMirrorPlayImage,true},
            {eTextInt,"Mirror Delay (S): %d.%d",GetIntegerValue,&nMirrorDelay,0,10,1},
        {eEndif},
        {eBool,"Scale Height to Fit: %s",ToggleBool,&bScaleHeight,0,0,0,"On","Off"},
    {eEndif},
    {eIfEqual,"",NULL,&bSecondStrip,true},
        {eBool,"144 to 288 Pixels: %s",ToggleBool,&bDoublePixels,0,0,0,"Yes","No"},
    {eEndif},
    {eIfEqual,"",NULL,&bShowBuiltInTests,false},
        {eBool,"Frame Advance: %s",ToggleBool,&bManualFrameAdvance,0,0,0,"Step","Auto"},
        {eIfEqual,"",NULL,&bManualFrameAdvance,true},
            {eTextInt,"Frame Counter: %d",GetIntegerValue,&nFramePulseCount,0,32},
        {eEndif},
    {eEndif},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem StripMenu[] = {
    {eExit,"Previous Menu"},
    {eTextInt,"Strip Bright: %d/255",GetIntegerValue,&nStripBrightness,1,255,0,NULL,NULL,UpdateStripBrightness},
    {eBool,"LED strips: %s",ToggleBool,&bSecondStrip,0,0,0,"2","1"},
    {eBool,"Gamma Correction: %s",ToggleBool,&bGammaCorrection,0,0,0,"On","Off"},
    {eTextInt,"White Balance R: %3d",GetIntegerValue,&whiteBalance.r,0,255,0,NULL,NULL,UpdateStripWhiteBalanceR},
    {eTextInt,"White Balance G: %3d",GetIntegerValue,&whiteBalance.g,0,255,0,NULL,NULL,UpdateStripWhiteBalanceG},
    {eTextInt,"White Balance B: %3d",GetIntegerValue,&whiteBalance.b,0,255,0,NULL,NULL,UpdateStripWhiteBalanceB},
    {eText,"Show White Balance",ShowWhiteBalance},
    {eTextInt,"LED Wiring Mode: %d",GetIntegerValue,&stripsMode,0,2},
    {eTextInt,"Max mAmp: %d",GetIntegerValue,&nStripMaxCurrent,100,10000},
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
    {eTextInt,"Repeat Count: %d",GetIntegerValue,&repeatCount,1,100},
    {eTextInt,"Repeat Delay (S): %d.%d",GetIntegerValue,&repeatDelay,0,100,1},
    {eIfEqual,"",NULL,&bShowBuiltInTests,false},
        {eBool,"Chain Files: %s",ToggleBool,&bChainFiles,0,0,0,"On","Off"},
        {eIfEqual,"",NULL,&bChainFiles,true},
            {eTextInt,"Chain Repeats: %d",GetIntegerValue,&nChainRepeats,1,100},
            {eTextInt,"Chain Delay (S): %d.%d",GetIntegerValue,&nChainDelay,0,100,1},
        {eEndif},
    {eEndif},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem EepromMenu[] = {
    {eExit,"Previous Menu"},
    {eBool,"Autoload Saved: %s",ToggleBool,&bAutoLoadSettings,0,0,0,"On","Off"},
    {eText,"Save Current Settings",SaveEepromSettings},
    {eText,"Load Saved Settings",LoadEepromSettings},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroSelectMenu[] = {
    //{eExit,"Previous Menu"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,0,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,1,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,2,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,3,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,4,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,5,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,6,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,7,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,8,0,0,"Used","Empty"},
    {eList,"Macro: #%d %s",NULL,&nCurrentMacro,9,0,0,"Used","Empty"},
    //{eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MacroMenu[] = {
    {eExit,"Previous Menu"},
    //{eTextInt,"Macro #: %d",GetIntegerValue,&nCurrentMacro,0,9},
    {eIfEqual,"",NULL,&bRecordingMacro,false},
        {eMenu,"Select Macro: #%d",{.menu = MacroSelectMenu},&nCurrentMacro},
        {eTextInt,"Run: #%d",RunMacro,&nCurrentMacro},
    {eElse},
        {eTextInt,"Recording Macro: #%d",NULL,&nCurrentMacro},
    {eEndif},
    {eBool,"Record: %s",ToggleBool,&bRecordingMacro,0,0,0,"On","Off"},
    {eIfEqual,"",NULL,&bRecordingMacro,false},
        {eTextInt,"Repeat Count: %d",GetIntegerValue,&nRepeatCountMacro,1,100},
        {eTextInt,"Repeat Delay (S): %d.%d",GetIntegerValue,&nRepeatWaitMacro,0,100,1},
        {eTextInt,"Load: #%d",LoadMacro,&nCurrentMacro},
        {eTextInt,"Save: #%d",SaveMacro,&nCurrentMacro},
        {eTextInt,"Delete: #%d",DeleteMacro,&nCurrentMacro},
    {eEndif},
    {eExit,"Previous Menu"},
    // make sure this one is last
    {eTerminate}
};
MenuItem MainMenu[] = {
    {eIfEqual,"",NULL,&bShowBuiltInTests,true},
        {eBool,"Switch to SD Card",ToggleFilesBuiltin,&bShowBuiltInTests,0,0,0,"On","Off"},
    {eElse},
        {eBool,"Switch to Built-ins",ToggleFilesBuiltin,&bShowBuiltInTests,0,0,0,"On","Off"},
        {eText,"Preview BMP",ShowBmp},
    {eEndif},
    {eMenu,"File Image Settings",{.menu = ImageMenu}},
    {eMenu,"Repeat Settings",{.menu = RepeatMenu}},
    {eMenu,"LED Strip Settings",{.menu = StripMenu}},
    {eIfEqual,"",NULL,&bShowBuiltInTests,true},
        {eBuiltinOptions,"%s Options",{.builtin = BuiltInFiles}},
    {eElse},
        {eMenu,"MIW File Operations",{.menu = StartFileMenu}},
    {eEndif},
    {eMenu,"Macros: #%d",{.menu = MacroMenu},&nCurrentMacro},
    {eMenu,"Saved Settings",{.menu = EepromMenu}},
    {eMenu,"System Settings",{.menu = SystemMenu}},
    {eText,"Light Bar",LightBar},
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

char FileToShow[100];
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
    {"FADE IN/OUT FRAMES",&nFadeInOutFrames,vtInt,0,255},
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
    {"DISPLAY MENULINE COLOR",&menuTextColor,vtInt},
    {"MENU STAR",&bMenuStar,vtBool},
    {"HILITE FILE",&bHiLiteCurrentFile,vtBool},
    {"GAMMA CORRECTION",&bGammaCorrection,vtBool},
    {"SELECT BUILTINS",&bShowBuiltInTests,vtBuiltIn},       // this must be before the SHOW FILE command
    {"SHOW FILE",&FileToShow,vtShowFile},
    {"REVERSE DIAL",&CRotaryDialButton::m_bReverseDial,vtBool},
    {"PREVIEW SCROLL",&nPreviewScrollCols,vtInt},
};
