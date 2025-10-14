//#define PIN_USER_BUTTON     9   // X1-13 active-low user buton

/////RELIABLE DOCK PINS ///
/////////////////////////////////////////////
///////          User Settings         //////
/////////////////////////////////////////////
//
//// Board Type
//// Options:                 ESP32_C3, ESP32_C6
//#define BOARD_TYPE          ESP32_C3
//
//// LED Strip Type:
//// Options:                 WS2811, WS2815, WS2818
//#define LED_STRIP_TYPE      WS2815
//
//// Total number of LEDs on the strip
//// Options: 1 - 10000. More leds will make the system slower, but memory limit is around 10k
//#define LED_STRIP_NUM_LEDS_MAX   1000
//
/////////////////////////////////////////////
/////// Not recommended to touch below //////
/////////////////////////////////////////////
//// Board pins
//#if   (BOARD_TYPE == ESP32_C3)
//  #define PIN_LED_STRIP       0   // X1-8  WS2815 data
//  #define PIN_LED_INDICATOR   1   // X1-7  SK6812
//  #define PIN_JACK_PRESENT    6   // X1-12 active-low JACK detect
//  #define PIN_USER_BUTTON     7   // X1-13 active-low user buton
//  #define PIN_SD_CS          20   // X1-17 SPI2 bus chip select for SD
//  #define PIN_SD_MOSI        10   // X1-16 SPI2 bus MOSI
//  #define PIN_SD_SCK          9   // X1-15 SPI2 bus SCK
//  #define PIN_SD_MISO         8   // X1-14 SPI2 bus MISO
//  #define PIN_STRIP_ISENSE    5   // X1-11 ADC
//#elif (BOARD_TYPE == ESP32_C6)
//  #define PIN_LED_STRIP      14   // X1-8  WS2815 data
//  #define PIN_LED_INDICATOR  15   // X1-7  SK6812
//  #define PIN_JACK_PRESENT   17   // X1-12 active-low JACK detect
//  #define PIN_USER_BUTTON     0   // X1-13 active-low user buton
//  #define PIN_SD_CS           4   // X1-17 SPI2 bus chip select for SD
//  #define PIN_SD_MOSI         3   // X1-16 SPI2 bus MOSI
//  #define PIN_SD_SCK          2   // X1-15 SPI2 bus SCK
//  #define PIN_SD_MISO         1   // X1-14 SPI2 bus MISO
//  #define PIN_STRIP_ISENSE    5   // X1-18 ADC
//#else
//  #error "Board not supported"
//#endif
//
//#define LED_INDICATOR_TYPE          SK6812
//#define LED_INDICATOR_COLOR_ORDER   GRB
//
//// LED color order
//#if   (LED_STRIP_TYPE == WS2811)
//  #define LED_STRIP_COLOR_ORDER   BRG
//#elif (LED_STRIP_TYPE == WS2815)
//  #define LED_STRIP_COLOR_ORDER   GRB   // (double-check on your hardware; many WS2815 strips are {G,R,B})
//#elif (LED_STRIP_TYPE == WS2818)
//  #define LED_STRIP_COLOR_ORDER   GRB   // (double-check)
//#else
//  #error "LED_STRIP_TYPE not supported"
//#endif
