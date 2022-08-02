# esp32-st7789-display
ST7789 TFT display driver for esp-idf based on https://github.com/nopnop2002/esp-idf-st7789 with perfomance improvements and new functions.
Include tests for drawing lines, fonts, rectangles.

# Software requirements
esp-idf v4.4 or later.

# Hardware Required

* An ESP development board.
* An SPI-interfaced LCD on ST7789 controller/driver.
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