# MagicImageWand
Similar to the commercial PixelStick or MagiLight products, but designed for DIY.
This is a collaboration project with dirkessl (and possibly others) on a new and improved version of the LED Image Painter project.
It displays bmp files and built-in patterns in the air to be captured with time exposures.
The images are displayed using one or two 144 LED RGB pixel arrays.
The processor is currently the TTGO with the TFT display. T-Display. Software only support has been added for the TTGO T4 with the larger display.
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
NOTE: TTGO Magic Image Wand Remote.pdf is used for a remote control camera shutter release designed by dirkessl. It is not part of the MIW project. Support for this concept will be added in the future. It will be possible then to have the MIW actually control the camera shutter.
  NOTE: The JSON library is no longer needed for this project, although it may come back someday! It was used by the bluetooth library for the Phone App. This was deprecated in favor of using a web server.
