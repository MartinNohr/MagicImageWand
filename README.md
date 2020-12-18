# MagicImageWand
Similar to the commercial PixelStick, but designed for DIY.
This is a collaboration project with dirkessl (and likely others) on a new and improved version of the LED Image Painter project.
This is a similar device to the commercial PixelStick. It displays bmp files and built-in patterns in the air to be captured with time exposures.
The images are displayed using one or two 144 LED RGB pixel arrays.
The processor is currently the TTGO with the TFT display. T-Display.
It has a nice menu system and single button for control.
This device can also be used as a simple light wand with color, saturation, and brightness controls.
It has optional exFat reading to support SD cards >32GB. This is enabled with a switch in the .h file, USE_STANDARD_SD, set to 0. 1 Uses the basic SD library.
For exFat support this library must be included: https://github.com/greiman/SdFat
The display is handled with this library: https://github.com/Bodmer/TFT_eSPI
Put both these in the Arduino libraries folder, remove the -master from the folder name that is in the downloaded zip file.
Two files need to be modified, user_setup.h and user_setup_select.h.

<p>In user_setup.h:
<p>Comment line: //#define ILI9341_DRIVER
<p>uncomment line: #define ST7789_DRIVER  
<p>comment these lines:
<p>//#define TFT_CS   PIN_D8  // Chip select control pin D8
<p>//#define TFT_DC   PIN_D3  // Data Command control pin
<p>//#define TFT_RST  PIN_D4  // Reset pin (could connect to NodeMCU RST, see next line)
<p>Comment this line:
<p>//#define SPI_FREQUENCY  27000000

<br>In user_setup_select.h:
<br>uncomment:
<br>#include &ltUser_Setups/Setup135_ST7789.h&gt
