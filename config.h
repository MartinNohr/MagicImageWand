#pragma once
// ***** Various switches for options are set here *****

// battery level
#define HAS_BATTERY_LEVEL 1
// 1 for standard SD library, 0 for the new exFat library which allows > 32GB SD cards
#define USE_STANDARD_SD 0
// reverse A and B for some PCB or wired versions, this is set for rev 2 PCB, 0 for older PCB, and 0 for rev 3 pcb
#define ROTARY_DIAL_REVERSE 0
// The push button setting, set to 1 for onboard PS version 1.4
#define PUSH_BUTTON_PORT 0
// battery sensor GPIO
#define BATTERY_SENSOR_GPIO 36
// the optional light sensor GPIO to control the LCD brightness
#define LIGHT_SENSOR_GPIO 39
// the wheel frame advance gpio port
#define FRAMEBUTTON_GPIO 32

// set the push button GPIO port
#if PUSH_BUTTON_PORT
	#define DIAL_BTN 37
#else
	#define DIAL_BTN 15
#endif

// default dial direction GPIO ports
#if ROTARY_DIAL_REVERSE
	#define DIAL_A 12
	#define DIAL_B 13
#else
	#define DIAL_A 13
	#define DIAL_B 12
#endif
