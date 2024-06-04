/* 
	Editor: https://www.visualmicro.com/
			This file is for intellisense purpose only.
			Visual micro (and the arduino ide) ignore this code during compilation. This code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			The contents of the _vm sub folder can be deleted prior to publishing a project
			All non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			Note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: LilyGo T-Display(esp32_lilygo_t_display), Platform=esp32, Package=esp32
*/

#if defined(_VMICRO_INTELLISENSE)

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __ESP32_esp32__ 1
#define __ESP32_ESP32__ 1
#define HAVE_CONFIG_H 1
#define MBEDTLS_CONFIG_FILE "mbedtls/esp_config.h"
#define UNITY_INCLUDE_CONFIG_H 1
#define WITH_POSIX 1
#define _GNU_SOURCE 1
#define IDF_VER "v4.4.7-dirty"
#define ESP_PLATFORM 1
#define _POSIX_READER_WRITER_LOCKS 1
#define F_CPU 240000000L
#define ARDUINO 108010
#define ARDUINO_LILYGO_T_DISPLAY 1
#define ARDUINO_ARCH_ESP32 1
#define ARDUINO_BOARD "LILYGO_T_DISPLAY"
#define ARDUINO_VARIANT "lilygo_t_display"
#define ARDUINO_PARTITION_default 1
#define ESP32 1
#define CORE_DEBUG_LEVEL 0
#define ARDUINO_RUNNING_CORE 1
#define ARDUINO_EVENT_RUNNING_CORE 1
#define ARDUINO_USB_CDC_ON_BOOT 0
#define __cplusplus 201103L

#define _Pragma(x)
#undef __cplusplus
#define __cplusplus 201103L

#define __STDC__
#define __ARM__
#define __arm__
#define __inline__
#define __asm__(...)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __volatile__

#define __ASM
#define __INLINE
#define __attribute__(noinline)

//#define _STD_BEGIN
//#define EMIT
#define WARNING
//#define _Lockit
#define __CLR_OR_THIS_CALL
#define C4005
//#define _NEW

typedef bool _bool;
typedef int _read;
typedef int _seek;
typedef int _write;
typedef int _close;
typedef int __cleanup;

//#define inline 

#define __builtin_clz
#define __builtin_clzl
#define __builtin_clzll
#define __builtin_labs
#define __builtin_va_list
typedef int __gnuc_va_list;

#define __ATOMIC_ACQ_REL

#define __CHAR_BIT__
#define _EXFUN()

typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}

//typedef long __INTPTR_TYPE__ ;
#define __INTPTR_TYPE__ unsigned int
typedef long __UINTPTR_TYPE__ ;
//typedef long __SIZE_TYPE__ 	;
#define __SIZE_TYPE__ unsigned int
#define __INT32_TYPE__ long
typedef long __PTRDIFF_TYPE__;

typedef long pthread_t;
typedef long pthread_key_t;
typedef long pthread_once_t;
typedef long pthread_mutex_t;
typedef long pthread_mutex_t;
typedef long pthread_cond_t;

#define __CHAR_BIT__ 1

// Ensure ArduinoJSON Lib Intellisense works correctly
#define ARDUINOJSON_ENABLE_STD_STREAM 0

class VM_DBG {
public:
	// Send a Message to the Serial Monitor via WiFi Connection 
	void sendUserMessage(const char* theMessage) {};
} MicroDebug;

class Lockit { public: int _Lockit() {} };

#include <arduino.h>
#include <pins_arduino.h> 

#define interrupts() sei()
#define noInterrupts() cli()

#define ESP_LOGI(tag, ...)

// Read Value from Register
#define READ_PERI_REG(addr)
// Write Value to Register
#define WRITE_PERI_REG(addr,val)
#include "..\MagicImageWand.ino"
#endif
#endif
