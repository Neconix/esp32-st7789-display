# esp32-st7789-display
ST7789 TFT display driver for esp-idf based on https://github.com/nopnop2002/esp-idf-st7789 with perfomance improvements and new functions.
Include tests for drawing lines, fonts, rectangles.

# Software requirements
esp-idf v4.4 or later.

# Hardware Required

* An ESP development board. (Tested with ESP32 NodeMCU)
* An SPI-interfaced LCD on ST7789 controller/driver. (Tested with ST7789 240x240 display)
* An USB cable for power supply and programming.

### Hardware Connection

The connection between ESP Board and the LCD is as follows:

```
      ESP Board                            LCD Screen
      +---------+              +---------------------------------+
      |         |              |                                 |
      |     3V3 +--------------+ VCC   +----------------------+  |
      |         |              |       |                      |  |
      |     GND +--------------+ GND   |                      |  |
      |         |              |       |                      |  |
      |   DATA0 +--------------+ MOSI  |                      |  |
      |         |              |       |                      |  |
      |    PCLK +--------------+ SCK   |                      |  |
      |         |              |       |                      |  |
      |      CS +--------------+ CS    |                      |  |
      |         |              |       |                      |  |
      |     D/C +--------------+ D/C   |                      |  |
      |         |              |       |                      |  |
      |     RST +--------------+ RST   |                      |  |
      |         |              |       |                      |  |
      |BK_LIGHT +--------------+ BCKL  +----------------------+  |
      |         |              |                                 |
      +---------+              +---------------------------------+
```

The GPIO number used by this example can be changed in [main.c](main/main.c), where:

| GPIO number              | LCD pin |
| ------------------------ | ------- |
| CONFIG_SCLK_GPIO         | SCK     |
| CONFIG_CS_GPIO           | CS      |
| CONFIG_DC_GPIO           | DC      |
| CONFIG_RESET_GPIO        | RST     |
| CONFIG_MOSI_GPIO         | MOSI    |
| CONFIG_BL_GPIO           | BCKL    |

Especially, please pay attention to the level used to turn on the LCD backlight, some LCD module needs a low level to turn it on, while others take a high level.

# Build and flash

## ESP-IDF from command line

```shell
git clone https://github.com/Neconix/esp32-st7789-display.git
cd esp32-st7789-display
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash # where /dev/ttyUSB0 is a port to connected ESP32 board
idf.py -p /dev/ttyUSB0 monitor # used to see output from ESP_LOG
```
With some Linux distributions, you may get the `Failed to open port /dev/ttyUSB0` error message when flashing the ESP32. Run something like this to add current user to `dialout` group:

```shell
sudo usermod -a -G dialout $USER
```
## VSCode

In VSCode with installed [ESP-IDF extension](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md):

- git clone https://github.com/Neconix/esp32-st7789-display.git
- open project folder in VSCode and run from command pallete (Ctrl + Shift + P):
- ESP-IDF: Add vscode configuration folder
- ESP-IDF: SDK configuration editor (menuconfig)
- ESP-IDF: Select port to use
- ESP-IDF: Set espressif device target
- ESP-IDF: Build your project
- ESP-IDF: Flash (UART) your project
- ESP-IDF: Monitor your device

Or run ESP-IDF: Build, Flash and start a monitor on your device.

# Supported functions

Current supported functions list:

```C
void lcdInit(TFT_t *dev, display_config_t *display_config);
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color);
void lcdDrawMultiPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t size, uint16_t * colors);
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t color);
void lcdDisplayOff(TFT_t * dev);
void lcdDisplayOn(TFT_t * dev);
void lcdFillScreen(TFT_t * dev, uint16_t color);
void lcdDrawHLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t length, uint16_t color);
void lcdDrawHLineT(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t length, uint16_t b, uint16_t color);
void lcdDrawVLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t height, uint16_t color);
void lcdDrawVLineT(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t height, uint16_t b, uint16_t color);
void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDrawRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t color);
void lcdDrawRectT(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t b, uint16_t color);
void lcdDrawRectAngle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);
void lcdDrawTriangle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);
void lcdDrawCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawFillCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawRoundRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color);
void lcdDrawArrow(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color);
void lcdDrawFillArrow(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color);
int lcdDrawChar(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color);
int lcdDrawString(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, char *ascii, uint16_t color);
int lcdDrawCode(TFT_t * dev, FontxFile *fx, uint16_t x,uint16_t y,uint8_t code,uint16_t color);
void lcdSetFontDirection(TFT_t * dev, uint16_t);
void lcdSetFontFill(TFT_t * dev, uint16_t color);
void lcdUnsetFontFill(TFT_t * dev);
void lcdSetFontUnderLine(TFT_t * dev, uint16_t color);
void lcdUnsetFontUnderLine(TFT_t * dev);
void lcdBacklightOff(TFT_t * dev);
void lcdBacklightOn(TFT_t * dev);
void lcdInversionOff(TFT_t * dev);
void lcdInversionOn(TFT_t * dev);
uint16_t rgb565_conv(uint16_t r, uint16_t g, uint16_t b);
```
See [st7789.h](main/st7789.h) and [st7789.c](main/st7789.c)   

# Docs
esp-idf: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/

source st7789 lib: https://github.com/nopnop2002/esp-idf-st7789
