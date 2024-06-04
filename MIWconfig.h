#pragma once
// ***** Various switches for options are set here *****

// use TTGO_T 1, 3 or 4, uncomment only one of these
#define TTGO_T 1
// choose TTGO T-Display for compile type (or T1 if that doesn't work)
//#define TTGO_T 3	// for S3 chip
// choose LilyGo T-Display S3 for compile type
//#define TTGO_T 4
// choose LilyGo T4 for compile type

// **** NOTE ****
// If you compile for TTGO 1 and load on S3 the S3 will not run and the boot loader will be disabled
// To fix this: Press and hold the BOOT button, press and release reset, and then release BOOT.
// Upload the firmware and then press the reset button. After this all will function normally again.

// also remember to change User_Setup_Select.h correctly
// use one of these in that file
//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT
//#include <User_Setups/Setup22_TTGO_T4.h>           // Setup file for ESP32 and TTGO T4 version 1.2
//#include <User_Setups/Setup22_TTGO_T4_v1.3.h>      // Setup file for ESP32 and TTGO T4 version 1.3
// make sure this one is commented out
//#include <User_Setup.h>           // Default setup is root library folder

// 1 for standard SD library, 0 for the new exFat library which allows > 32GB SD cards
#define USE_STANDARD_SD 0

#if TTGO_T == 1
	// onboard buttons
	#define ONBBTN0 GPIO_NUM_0
	#define ONBBTN1 GPIO_NUM_35
	// TFT PWM control pin
	#define TFT_PWM_PIN 4
	// SD details
	#define SDcsPin    33  // GPIO33
	#define SDSckPin   25  // GIPO25
	#define SDMisoPin  27  // GPIO27
	#define SDMosiPin  26  // GPIO26
	// LED controller pins
	#define DATA_PIN1 2
	#define DATA_PIN2 17
	// battery level
	#define HAS_BATTERY_LEVEL 1
	// the wheel frame advance gpio port
	#define FRAMEBUTTON_GPIO 32
	// battery sensor GPIO
	#define BATTERY_SENSOR_GPIO 36
	// the optional light sensor GPIO to control the LCD brightness
	#define LIGHT_SENSOR_GPIO 39
	// The push button setting, set to 1 for onboard PS version 1.4 (NOTE: PCB not ever created)
	#if 0
		#define DIAL_BTN 37
	#else
		#define DIAL_BTN 15
	#endif
	// default dial direction GPIO ports
	#define DIAL_A 13
	#define DIAL_B 12
#elif TTGO_T == 3
	// onboard buttons
	#define ONBBTN0 GPIO_NUM_0
	#define ONBBTN1 GPIO_NUM_14
	// TFT PWM control pin
	#define TFT_PWM_PIN 38
	// SD details
	#define SDcsPin    10
	#define SDSckPin   12
	#define SDMisoPin  11
	#define SDMosiPin  13
	// LED controller pins
	#define DATA_PIN1 21
	#define DATA_PIN2 16
	// battery level
	#define HAS_BATTERY_LEVEL 1
	// the wheel frame advance gpio port
	#define FRAMEBUTTON_GPIO 18
	// battery sensor GPIO
	#define BATTERY_SENSOR_GPIO 4
	// the optional light sensor GPIO to control the LCD brightness
	#define LIGHT_SENSOR_GPIO 18
	// set the push button GPIO port
	#define DIAL_BTN 3
	#define DIAL_A 1
	#define DIAL_B 2
#elif TTGO_T == 4
	// onboard buttons
	#define ONBBTN0 GPIO_NUM_0
	#define ONBBTN1 GPIO_NUM_35
	// TFT PWM control pin
	#define TFT_PWM_PIN 4
	// SD details
	#define SDcsPin    13
	#define SDSckPin   14
	#define SDMisoPin  2
	#define SDMosiPin  15
	// LED controller pins
	#define DATA_PIN1 19
	#define DATA_PIN2 25
	// battery level
	#define HAS_BATTERY_LEVEL 0
	// the wheel frame advance gpio port
	#define FRAMEBUTTON_GPIO 0
	// battery sensor GPIO
	#define BATTERY_SENSOR_GPIO 34
	// the optional light sensor GPIO to control the LCD brightness
	#define LIGHT_SENSOR_GPIO 35
	// set the push button GPIO port
	#define DIAL_BTN 37
	#define DIAL_A -1
	#define DIAL_B -1
#endif
