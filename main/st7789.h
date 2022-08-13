#include "driver/spi_master.h"
#include "hal/gpio_types.h"
#include "fontx.h"

#define DIRECTION0		0
#define DIRECTION90		1
#define DIRECTION180	2
#define DIRECTION270	3

typedef struct {
	uint16_t _width;
	uint16_t _height;
	uint16_t maxX;
	uint16_t maxY;
	uint16_t _offsetx;
	uint16_t _offsety;
	uint16_t _font_direction;
	uint16_t _font_fill;
	uint16_t _font_fill_color;
	uint16_t _font_underline;
	uint16_t _font_underline_color;
	int16_t _dc;
	int16_t _bl;
	uint16_t diplayBufferLen;
	spi_device_handle_t _SPIHandle;
} TFT_t;

typedef struct {
	uint16_t width;
	uint16_t height;
    gpio_num_t pinMOSI;
    gpio_num_t pinSCLK;
    gpio_num_t pinCS;
    gpio_num_t pinDC;
    gpio_num_t pinRESET;
    gpio_num_t pinBL;
	spi_host_device_t spiHost;
	int spiFrequency;
} display_config_t;

typedef struct {
	uint8_t MH;  ///< Display Data Latch Data Order: “0” = LCD Refresh Left to Right; “1” = LCD Refresh Right to Left.
	uint8_t RGB; ///< RGB/BGR Order: “0” = RGB; “1” = BGR.
	uint8_t ML;  ///< Line Address Order: “0” = LCD Refresh Top to Bottom; “1” = LCD Refresh Bottom to Top.
	uint8_t MV;	 ///< Page/Column Order: “0” = Normal Mode. “1” = Reverse Mode.
	uint8_t MX;  ///< Column Address Order: “0” = Left to Right; “1” = Right to Left.
	uint8_t MY;  ///< Page Address Order: “0” = Top to Bottom. “1” = Bottom to Top.
} mad_ctl_t;

void lcdInit(TFT_t *dev, display_config_t *display_config);
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color);
void lcdDrawPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *pixels, uint16_t size);
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
uint8_t  lcdDrawChar(TFT_t *dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t charCode, uint16_t color, uint16_t bgColor);
uint16_t lcdDrawString(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bgColor);
void lcdSetFontDirection(TFT_t * dev, uint16_t);
void lcdSetFontFill(TFT_t * dev, uint16_t color);
void lcdUnsetFontFill(TFT_t * dev);
void lcdSetFontUnderLine(TFT_t * dev, uint16_t color);
void lcdUnsetFontUnderLine(TFT_t * dev);
void lcdBacklightOff(TFT_t * dev);
void lcdBacklightOn(TFT_t * dev);
void lcdInversionOff(TFT_t * dev);
void lcdInversionOn(TFT_t * dev);
esp_err_t lcdReadMemoryDataAccessControl(TFT_t *dev, mad_ctl_t *mad_ctl);
uint16_t rgb565_conv(uint16_t r, uint16_t g, uint16_t b);
uint16_t rgb24to16(uint32_t color);
