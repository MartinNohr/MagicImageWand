# MagicImageWand
Similar to the commercial PixelStick, but designed for DIY.
This is a collaboration project with dirkessl (and likely others) on a new and improved version of the LED Image Painter project.
This is a similar device to the commercial PixelStick. It displays bmp files and built-in patterns in the air to be captured with time exposures.
The images are displayed using one or two 144 LED RGB pixel arrays.
The processor is currently the TTGO with the TFT display. T-Display.
It has a nice menu system and single rotary dial button for control.
This device can also be used as a simple light wand with color, saturation, and brightness controls.
It has optional exFat reading to support SD cards >32GB. This is enabled with a switch in the .h file, USE_STANDARD_SD, set to 0. 1 Uses the basic SD library.
<p>For exFat support this library must be included: https://github.com/greiman/SdFat
<br>The display is handled with this library: https://github.com/Bodmer/TFT_eSPI
<p>The file user_setup_select.h needs two lines modified.
<br>uncomment:
<br>#include &ltUser_Setups/Setup25_TTGO_T_Display.h&gt 
<br>comment:
<br>//#include &ltUser_Setup.h&gt

A more detailed setup Instruction for Arduino IDE is available in the Wiki https://github.com/MartinNohr/MagicImageWand/wiki/9.-Compiling-from-source
