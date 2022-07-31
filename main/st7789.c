#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <hal/spi_types.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "st7789.h"
#include "st7789_commands.h"

#define	_DEBUG_ 0

#define TAG "ST7789"

#define WRITE_BUFF_LEN 1024

#define SPI_CMD_MODE  0x00
#define SPI_DATA_MODE 0x01

void delayMS(int ms) {
	int _ms = ms + (portTICK_PERIOD_MS - 1);
	TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
	vTaskDelay(xTicksToDelay);
}

void spi_master_init(TFT_t *dev, display_config_t *display_config)
{
	esp_err_t ret;

	ESP_LOGI(TAG, "GPIO_CS=%d", display_config->pinCS);
	if ( display_config->pinCS >= 0 ) {
		gpio_reset_pin( display_config->pinCS );
		gpio_set_direction( display_config->pinCS, GPIO_MODE_OUTPUT );
		gpio_set_level( display_config->pinCS, 0 );
	}

	ESP_LOGI(TAG, "GPIO_DC=%d", display_config->pinDC);
	gpio_reset_pin( display_config->pinDC );
	gpio_set_direction( display_config->pinDC, GPIO_MODE_OUTPUT );
	gpio_set_level( display_config->pinDC, 0 );

	ESP_LOGI(TAG, "GPIO_RESET=%d", display_config->pinRESET);
	if ( display_config->pinRESET >= 0 ) {
		gpio_reset_pin( display_config->pinRESET );
		gpio_set_direction( display_config->pinRESET, GPIO_MODE_OUTPUT );
		gpio_set_level( display_config->pinRESET, 1 );
		delayMS(50);
		gpio_set_level( display_config->pinRESET, 0 );
		delayMS(50);
		gpio_set_level( display_config->pinRESET, 1 );
		delayMS(50);
	}

	ESP_LOGI(TAG, "GPIO_BL=%d", display_config->pinBL);
	if ( display_config->pinBL >= 0 ) {
		gpio_reset_pin(display_config->pinBL);
		gpio_set_direction(display_config->pinBL, GPIO_MODE_OUTPUT);
		gpio_set_level(display_config->pinBL, 0);
	}

	ESP_LOGI(TAG, "GPIO_MOSI=%d", display_config->pinMOSI);
	ESP_LOGI(TAG, "GPIO_SCLK=%d", display_config->pinSCLK);
	spi_bus_config_t buscfg = {
		.mosi_io_num = display_config->pinMOSI,
		.miso_io_num = -1,
		.sclk_io_num = display_config->pinSCLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 0,
		.flags = 0
	};

	ret = spi_bus_initialize( display_config->spiHost, &buscfg, SPI_DMA_CH_AUTO );
	ESP_LOGD(TAG, "spi_bus_initialize=%d",ret);
	assert(ret==ESP_OK);

	spi_device_interface_config_t devcfg;
	memset(&devcfg, 0, sizeof(devcfg));
	devcfg.clock_speed_hz = display_config->spiFrequency;
	devcfg.queue_size = 7;
	devcfg.mode = 2;
	devcfg.flags = SPI_DEVICE_NO_DUMMY;

	if ( display_config->pinCS >= 0 ) {
		devcfg.spics_io_num = display_config->pinCS;
	} else {
		devcfg.spics_io_num = -1;
	}
	
	spi_device_handle_t handle;
	ret = spi_bus_add_device( display_config->spiHost, &devcfg, &handle);
	ESP_LOGD(TAG, "spi_bus_add_device=%d",ret);
	assert(ret==ESP_OK);

	dev->_dc = display_config->pinDC;
	dev->_bl = display_config->pinBL;
	dev->_SPIHandle = handle;
}


bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	if ( DataLength > 0 ) {
		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = DataLength * 8;
		SPITransaction.tx_buffer = Data;
#if 1
		ret = spi_device_transmit( SPIHandle, &SPITransaction );
#else
		ret = spi_device_polling_transmit( SPIHandle, &SPITransaction );
#endif
		assert(ret==ESP_OK); 
	}

	return true;
}

bool spi_master_write_command(TFT_t * dev, uint8_t cmd)
{
	gpio_set_level(dev->_dc, SPI_CMD_MODE);
	return spi_master_write_byte(dev->_SPIHandle, &cmd, 1);
}

bool spi_master_write_data_byte(TFT_t * dev, uint8_t data)
{
	gpio_set_level( dev->_dc, SPI_DATA_MODE);
	return spi_master_write_byte(dev->_SPIHandle, &data, 1);
}


bool spi_master_write_data_word(TFT_t * dev, uint16_t data)
{
	static uint8_t Byte[2];
	Byte[0] = (data >> 8) & 0xFF;
	Byte[1] = data & 0xFF;
	gpio_set_level( dev->_dc, SPI_DATA_MODE );
	return spi_master_write_byte( dev->_SPIHandle, Byte, 2);
}

bool spi_master_write_addr(TFT_t * dev, uint16_t addr1, uint16_t addr2)
{
	static uint8_t Byte[4];
	Byte[0] = (addr1 >> 8) & 0xFF;
	Byte[1] = addr1 & 0xFF;
	Byte[2] = (addr2 >> 8) & 0xFF;
	Byte[3] = addr2 & 0xFF;
	gpio_set_level( dev->_dc, SPI_DATA_MODE );
	return spi_master_write_byte( dev->_SPIHandle, Byte, 4);
}

bool spi_master_write_color(TFT_t * dev, uint16_t color, uint16_t size)
{
	static uint8_t Byte[1024];
	int index = 0;
	for(int i=0;i<size;i++) {
		Byte[index++] = (color >> 8) & 0xFF;
		Byte[index++] = color & 0xFF;
	}
	gpio_set_level( dev->_dc, SPI_DATA_MODE );
	return spi_master_write_byte( dev->_SPIHandle, Byte, size*2);
}

// Add 202001
bool spi_master_write_colors(TFT_t * dev, uint16_t * colors, uint16_t size)
{
	static uint8_t Byte[1024];
	int index = 0;
	for(int i=0;i<size;i++) {
		Byte[index++] = (colors[i] >> 8) & 0xFF;
		Byte[index++] = colors[i] & 0xFF;
	}
	gpio_set_level( dev->_dc, SPI_DATA_MODE);
	return spi_master_write_byte( dev->_SPIHandle, Byte, size*2);
}

uint16_t spi_master_write_packet(TFT_t * dev, uint16_t color, uint16_t size)
{
	static uint16_t packet[WRITE_BUFF_LEN];
	// Swapping bytes in a word
	uint16_t color_word = color >> 8 | (color & 0xFF) << 8;

	gpio_set_level(dev->_dc, SPI_DATA_MODE);

	uint16_t len;
	uint16_t total = 0;
	for (uint16_t p = 0; p <= size; p += WRITE_BUFF_LEN) {
		len = (size - p) > WRITE_BUFF_LEN ? WRITE_BUFF_LEN : (size - p);

		for(int i = 0; i < len; i++) {
			packet[i] = color_word;
		}
		
		spi_transaction_t SPITransaction;

		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = len * 16; // bits in a packet
		SPITransaction.tx_buffer = packet;

		esp_err_t ret = spi_device_transmit( dev->_SPIHandle, &SPITransaction );
		assert(ret==ESP_OK);
		total += len;
	}

	return total;
}

void lcdInit(TFT_t *dev, display_config_t *display_config)
{
	dev->_width = display_config->width;
	dev->_height = display_config->height;
	dev->_offsetx = 0;
	dev->_offsety = 0;
	dev->_font_direction = DIRECTION0;
	dev->_font_fill = false;
	dev->_font_underline = false;
	dev->diplayBufferLen = WRITE_BUFF_LEN;

	spi_master_init(dev, display_config);

	spi_master_write_command(dev, 0x01);	//Software Reset
	delayMS(150);

	spi_master_write_command(dev, 0x11);	//Sleep Out
	delayMS(255);
	
	spi_master_write_command(dev,   0x3A);	//Interface Pixel Format
	
	spi_master_write_data_byte(dev, 0x55); // 0 101 0 101 - 16 bit/pixel, 65K RGB interface
	delayMS(10);
	
	spi_master_write_command(dev, 0x36);	//Memory Data Access Control
	spi_master_write_data_byte(dev, 0x00);

	spi_master_write_command(dev, 0x2A);	//Column Address Set
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0xF0);

	spi_master_write_command(dev, 0x2B);	//Row Address Set
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0xF0);

	spi_master_write_command(dev, 0x21);	//Display Inversion On
	delayMS(10);

	spi_master_write_command(dev, 0x13);	//Normal Display Mode On
	delayMS(10);

	spi_master_write_command(dev, 0x29);	//Display ON
	delayMS(255);

	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 1 );
	}
}


// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color){
	if (x >= dev->_width) return;
	if (y >= dev->_height) return;

	uint16_t _x = x + dev->_offsetx;
	uint16_t _y = y + dev->_offsety;

	spi_master_write_command(dev, 0x2A);	// set column(x) address
	spi_master_write_addr(dev, _x, _x);
	spi_master_write_command(dev, 0x2B);	// set Page(y) address
	spi_master_write_addr(dev, _y, _y);
	spi_master_write_command(dev, 0x2C);	//	Memory Write
	spi_master_write_data_word(dev, color);
}


// Draw multi pixel
// x:X coordinate
// y:Y coordinate
// size:Number of colors
// colors:colors
void lcdDrawMultiPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t size, uint16_t * colors) {
	if (x+size > dev->_width) return;
	if (y >= dev->_height) return;

	uint16_t _x1 = x + dev->_offsetx;
	uint16_t _x2 = _x1 + size;
	uint16_t _y1 = y + dev->_offsety;
	uint16_t _y2 = _y1;

	spi_master_write_command(dev, 0x2A);	// set column(x) address
	spi_master_write_addr(dev, _x1, _x2);
	spi_master_write_command(dev, 0x2B);	// set Page(y) address
	spi_master_write_addr(dev, _y1, _y2);
	spi_master_write_command(dev, 0x2C);	//	Memory Write
	spi_master_write_colors(dev, colors, size);
}

/**
 * @brief Draw a filled rectangle
 * 
 * @param dev 
 * @param x1 
 * @param y1 
 * @param width 
 * @param height 
 * @param color 
 */
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t color) 
{
	if (x1 > dev->_width - 1) return;
	if (y1 > dev->_height - 1) return;

	uint16_t _x1 = x1;
	uint16_t _x2 = x1 + width - 1;
	uint16_t _y1 = y1;
	uint16_t _y2 = y1 + height - 1;

	if (width == 0 || _x2 > dev->_width - 1) return;
	if (height == 0 || _y2 > dev->_height - 1) return;

	uint16_t size = width * height;

	spi_master_write_command(dev, LCD_CMD_CASET);	// set column(x) address
	spi_master_write_addr(dev, _x1, _x2);
	spi_master_write_command(dev, LCD_CMD_RASET);	// set Page(y) address
	spi_master_write_addr(dev, _y1, _y2);
	spi_master_write_command(dev, LCD_CMD_RAMWR);	//	Memory Write

	spi_master_write_packet(dev, color, size);
}

// Display OFF
void lcdDisplayOff(TFT_t * dev) {
	spi_master_write_command(dev, 0x28);	//Display off
}
 
// Display ON
void lcdDisplayOn(TFT_t * dev) {
	spi_master_write_command(dev, 0x29);	//Display on
}

// Fill screen
// color:color
void lcdFillScreen(TFT_t * dev, uint16_t color) {
	lcdDrawFillRect(dev, 0, 0, dev->_width, dev->_height, color);
}

/**
 * @brief Fast display a horizontal line
 * 
 * @param dev 
 * @param x1 
 * @param y1 
 * @param length 
 * @param color 
 */
void lcdDrawHLine(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t length, uint16_t color)
{
	if (length == 0) return;
	if (x1 > dev->_width - 1) return;
	if (y1 > dev->_height - 1) return;

	uint16_t x2 = x1 + length - 1;

	if (x2 > dev->_width - 1) return;

	spi_master_write_command(dev, LCD_CMD_CASET);	// set column(x) address
	spi_master_write_addr(dev, x1, x2);
	spi_master_write_command(dev, LCD_CMD_RASET);	// set Page(y) address
	spi_master_write_addr(dev, y1, y1+1);
	spi_master_write_command(dev, LCD_CMD_RAMWR);	//	Memory Write

	uint16_t total = spi_master_write_packet(dev, color, length);
}

/**
 * @brief * @brief Fast display a vertical line
 * 
 * @param dev 
 * @param x1 
 * @param y1 
 * @param height 
 * @param color 
 */
void lcdDrawVLine(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t height, uint16_t color)
{
	if (height == 0) return;
	if (x1 > dev->_width - 1) return;
	if (y1 > dev->_height - 1) return;

	uint16_t y2 = y1 + height - 1;

	if (y2 > dev->_height - 1) return;

	spi_master_write_command(dev, LCD_CMD_CASET);	// set column(x) address
	spi_master_write_addr(dev, x1, x1);
	spi_master_write_command(dev, LCD_CMD_RASET);	// set Page(y) address
	spi_master_write_addr(dev, y1, y2);
	spi_master_write_command(dev, LCD_CMD_RAMWR);	//	Memory Write

	spi_master_write_packet(dev, color, height);
}

// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color 
void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	int i;
	int dx,dy;
	int sx,sy;
	int E;

	/* distance between two points */
	dx = ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = ( y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
			y1 += sy;
			E -= 2 * dx;
		}
	}

	/* inclination >= 1 */
	} else {
		E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

// Draw rectangle
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// color:color
void lcdDrawRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	lcdDrawLine(dev, x1, y1, x2, y1, color);
	lcdDrawLine(dev, x2, y1, x2, y2, color);
	lcdDrawLine(dev, x2, y2, x1, y2, color);
	lcdDrawLine(dev, x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle :Angle of rectangle
// color :color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawRectAngle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) {
	double xd,yd,rd;
	int x1,y1;
	int x2,y2;
	int x3,y3;
	int x4,y4;
	rd = -angle * M_PI / 180.0;
	xd = 0.0 - w/2;
	yd = h/2;
	x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = w/2;
	yd = h/2;
	x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x4 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y4 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	lcdDrawLine(dev, x1, y1, x2, y2, color);
	lcdDrawLine(dev, x1, y1, x3, y3, color);
	lcdDrawLine(dev, x2, y2, x4, y4, color);
	lcdDrawLine(dev, x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle :Angle of triangle
// color :color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawTriangle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) {
	double xd,yd,rd;
	int x1,y1;
	int x2,y2;
	int x3,y3;
	rd = -angle * M_PI / 180.0;
	xd = 0.0;
	yd = h/2;
	x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = w/2;
	yd = 0.0 - yd;
	x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = 0.0 - w/2;
	x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	lcdDrawLine(dev, x1, y1, x2, y2, color);
	lcdDrawLine(dev, x1, y1, x3, y3, color);
	lcdDrawLine(dev, x2, y2, x3, y3, color);
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
	int x;
	int y;
	int err;
	int old_err;

	x=0;
	y=-r;
	err=2-2*r;
	do{
		lcdDrawPixel(dev, x0-x, y0+y, color); 
		lcdDrawPixel(dev, x0-y, y0-x, color); 
		lcdDrawPixel(dev, x0+x, y0-y, color); 
		lcdDrawPixel(dev, x0+y, y0+x, color); 
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;	 
	} while(y<0);
}

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawFillCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
	int x;
	int y;
	int err;
	int old_err;
	int ChangeX;

	x=0;
	y=-r;
	err=2-2*r;
	ChangeX=1;
	do{
		if(ChangeX) {
			lcdDrawLine(dev, x0-x, y0-y, x0-x, y0+y, color);
			lcdDrawLine(dev, x0+x, y0-y, x0+x, y0+y, color);
		} // endif
		ChangeX=(old_err=err)<=x;
		if (ChangeX)			err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;
	} while(y<=0);
} 

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// r:radius
// color:color
void lcdDrawRoundRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color) {
	int x;
	int y;
	int err;
	int old_err;
	unsigned char temp;

	if(x1>x2) {
		temp=x1; x1=x2; x2=temp;
	} // endif
	  
	if(y1>y2) {
		temp=y1; y1=y2; y2=temp;
	} // endif

	ESP_LOGD(TAG, "x1=%d x2=%d delta=%d r=%d",x1, x2, x2-x1, r);
	ESP_LOGD(TAG, "y1=%d y2=%d delta=%d r=%d",y1, y2, y2-y1, r);
	if (x2-x1 < r) return; // Add 20190517
	if (y2-y1 < r) return; // Add 20190517

	x=0;
	y=-r;
	err=2-2*r;

	do{
		if(x) {
			lcdDrawPixel(dev, x1+r-x, y1+r+y, color); 
			lcdDrawPixel(dev, x2-r+x, y1+r+y, color); 
			lcdDrawPixel(dev, x1+r-x, y2-r-y, color); 
			lcdDrawPixel(dev, x2-r+x, y2-r-y, color);
		} // endif 
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;	 
	} while(y<0);

	ESP_LOGD(TAG, "x1+r=%d x2-r=%d",x1+r, x2-r);
	lcdDrawLine(dev, x1+r,y1  ,x2-r,y1	,color);
	lcdDrawLine(dev, x1+r,y2  ,x2-r,y2	,color);
	ESP_LOGD(TAG, "y1+r=%d y2-r=%d",y1+r, y2-r);
	lcdDrawLine(dev, x1  ,y1+r,x1  ,y2-r,color);
	lcdDrawLine(dev, x2  ,y1+r,x2  ,y2-r,color);  
} 

// Draw arrow
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
void lcdDrawArrow(TFT_t * dev, uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint16_t color) {
	double Vx= x1 - x0;
	double Vy= y1 - y0;
	double v = sqrt(Vx*Vx+Vy*Vy);
	//	 printf("v=%f\n",v);
	double Ux= Vx/v;
	double Uy= Vy/v;

	uint16_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	//lcdDrawLine(x0,y0,x1,y1,color);
	lcdDrawLine(dev, x1, y1, L[0], L[1], color);
	lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);
}


// Draw arrow of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
void lcdDrawFillArrow(TFT_t * dev, uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint16_t color) {
	double Vx= x1 - x0;
	double Vy= y1 - y0;
	double v = sqrt(Vx*Vx+Vy*Vy);
	//printf("v=%f\n",v);
	double Ux= Vx/v;
	double Uy= Vy/v;

	uint16_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	lcdDrawLine(dev, x0, y0, x1, y1, color);
	lcdDrawLine(dev, x1, y1, L[0], L[1], color);
	lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);

	int ww;
	for(ww=w-1;ww>0;ww--) {
		L[0]= x1 - Uy*ww - Ux*v;
		L[1]= y1 + Ux*ww - Uy*v;
		R[0]= x1 + Uy*ww - Ux*v;
		R[1]= y1 - Ux*ww - Uy*v;
		//printf("Fill>L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);
		lcdDrawLine(dev, x1, y1, L[0], L[1], color);
		lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	}
}


// RGB565 conversion
// RGB565 is R(5)+G(6)+B(5)=16bit color format.
// Bit image "RRRRRGGGGGGBBBBB"
uint16_t rgb565_conv(uint16_t r,uint16_t g,uint16_t b) {
	return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
// color:color
int lcdDrawChar(TFT_t * dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color) {
	uint16_t xx,yy,bit,ofs;
	unsigned char fonts[128]; // font pattern
	unsigned char pw, ph;
	int h,w;
	uint16_t mask;
	bool rc;

	if(_DEBUG_)printf("_font_direction=%d\n",dev->_font_direction);
	rc = GetFontx(fxs, ascii, fonts, &pw, &ph);
	if(_DEBUG_)printf("GetFontx rc=%d pw=%d ph=%d\n",rc,pw,ph);
	if (!rc) return 0;

	int16_t xd1 = 0;
	int16_t yd1 = 0;
	int16_t xd2 = 0;
	int16_t yd2 = 0;
	uint16_t xss = 0;
	uint16_t yss = 0;
	int16_t xsd = 0;
	int16_t ysd = 0;
	int16_t next = 0;
	uint16_t x0  = 0;
	uint16_t x1  = 0;
	uint16_t y0  = 0;
	uint16_t y1  = 0;
	if (dev->_font_direction == 0) {
		xd1 = +1;
		yd1 = +1; //-1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y - (ph - 1);
		xsd =  1;
		ysd =  0;
		next = x + pw;

		x0	= x;
		y0	= y - (ph-1);
		x1	= x + (pw-1);
		y1	= y;
	} else if (dev->_font_direction == 2) {
		xd1 = -1;
		yd1 = -1; //+1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y + ph + 1;
		xsd =  1;
		ysd =  0;
		next = x - pw;

		x0	= x - (pw-1);
		y0	= y;
		x1	= x;
		y1	= y + (ph-1);
	} else if (dev->_font_direction == 1) {
		xd1 =  0;
		yd1 =  0;
		xd2 = -1;
		yd2 = +1; //-1;
		xss =  x + ph;
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = y + pw; //y - pw;

		x0	= x;
		y0	= y;
		x1	= x + (ph-1);
		y1	= y + (pw-1);
	} else if (dev->_font_direction == 3) {
		xd1 =  0;
		yd1 =  0;
		xd2 = +1;
		yd2 = -1; //+1;
		xss =  x - (ph - 1);
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = y - pw; //y + pw;

		x0	= x - (ph-1);
		y0	= y - (pw-1);
		x1	= x;
		y1	= y;
	}

	if (dev->_font_fill) lcdDrawFillRect(dev, x0, y0, x1, y1, dev->_font_fill_color);

	int bits;
	if(_DEBUG_)printf("xss=%d yss=%d\n",xss,yss);
	ofs = 0;
	yy = yss;
	xx = xss;
	for(h=0;h<ph;h++) {
		if(xsd) xx = xss;
		if(ysd) yy = yss;
		//for(w=0;w<(pw/8);w++) {
		bits = pw;
		for(w=0;w<((pw+4)/8);w++) {
			mask = 0x80;
			for(bit=0;bit<8;bit++) {
				bits--;
				if (bits < 0) continue;
				//if(_DEBUG_)printf("xx=%d yy=%d mask=%02x fonts[%d]=%02x\n",xx,yy,mask,ofs,fonts[ofs]);
				if (fonts[ofs] & mask) {
					lcdDrawPixel(dev, xx, yy, color);
				} else {
					//if (dev->_font_fill) lcdDrawPixel(dev, xx, yy, dev->_font_fill_color);
				}
				if (h == (ph-2) && dev->_font_underline)
					lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
				if (h == (ph-1) && dev->_font_underline)
					lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
				xx = xx + xd1;
				yy = yy + yd2;
				mask = mask >> 1;
			}
			ofs++;
		}
		yy = yy + yd1;
		xx = xx + xd2;
	}

	if (next < 0) next = 0;
	return next;
}

int lcdDrawString(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, char *ascii, uint16_t color) {
	int length = strlen(ascii);
	if(_DEBUG_)printf("lcdDrawString length=%d\n",length);
	for(int i=0;i<length;i++) {
		if(_DEBUG_)printf("ascii[%d]=%x x=%d y=%d\n",i,ascii[i],x,y);
		if (dev->_font_direction == 0)
			x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 1)
			y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 2)
			x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 3)
			y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
	}
	if (dev->_font_direction == 0) return x;
	if (dev->_font_direction == 2) return x;
	if (dev->_font_direction == 1) return y;
	if (dev->_font_direction == 3) return y;
	return 0;
}

// Draw Non-Alphanumeric character
// x:X coordinate
// y:Y coordinate
// code: charcter code
// color:color
int lcdDrawCode(TFT_t * dev, FontxFile *fx, uint16_t x,uint16_t y,uint8_t code,uint16_t color) {
	if(_DEBUG_)printf("code=%x x=%d y=%d\n",code,x,y);
	if (dev->_font_direction == 0)
		x = lcdDrawChar(dev, fx, x, y, code, color);
	if (dev->_font_direction == 1)
		y = lcdDrawChar(dev, fx, x, y, code, color);
	if (dev->_font_direction == 2)
		x = lcdDrawChar(dev, fx, x, y, code, color);
	if (dev->_font_direction == 3)
		y = lcdDrawChar(dev, fx, x, y, code, color);
	if (dev->_font_direction == 0) return x;
	if (dev->_font_direction == 2) return x;
	if (dev->_font_direction == 1) return y;
	if (dev->_font_direction == 3) return y;
	return 0;
}

#if 0
// Draw UTF8 character
// x:X coordinate
// y:Y coordinate
// utf8: UTF8 code
// color:color
int lcdDrawUTF8Char(TFT_t * dev, FontxFile *fx, uint16_t x,uint16_t y,uint8_t *utf8,uint16_t color) {
	uint16_t sjis[1];

	sjis[0] = UTF2SJIS(utf8);
	if(_DEBUG_)printf("sjis=%04x\n",sjis[0]);
	return lcdDrawSJISChar(dev, fx, x, y, sjis[0], color);
}

// Draw UTF8 string
// x:X coordinate
// y:Y coordinate
// utfs: UTF8 string
// color:color
int lcdDrawUTF8String(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, unsigned char *utfs, uint16_t color) {

	int i;
	int spos;
	uint16_t sjis[64];
	spos = String2SJIS(utfs, strlen((char *)utfs), sjis, 64);
	if(_DEBUG_)printf("spos=%d\n",spos);
	for(i=0;i<spos;i++) {
		if(_DEBUG_)printf("sjis[%d]=%x y=%d\n",i,sjis[i],y);
		if (dev->_font_direction == 0)
			x = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
		if (dev->_font_direction == 1)
			y = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
		if (dev->_font_direction == 2)
			x = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
		if (dev->_font_direction == 3)
			y = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
	}
	if (dev->_font_direction == 0) return x;
	if (dev->_font_direction == 2) return x;
	if (dev->_font_direction == 1) return y;
	if (dev->_font_direction == 3) return y;
	return 0;
}
#endif

// Set font direction
// dir:Direction
void lcdSetFontDirection(TFT_t * dev, uint16_t dir) {
	dev->_font_direction = dir;
}

// Set font filling
// color:fill color
void lcdSetFontFill(TFT_t * dev, uint16_t color) {
	dev->_font_fill = true;
	dev->_font_fill_color = color;
}

// UnSet font filling
void lcdUnsetFontFill(TFT_t * dev) {
	dev->_font_fill = false;
}

// Set font underline
// color:frame color
void lcdSetFontUnderLine(TFT_t * dev, uint16_t color) {
	dev->_font_underline = true;
	dev->_font_underline_color = color;
}

// UnSet font underline
void lcdUnsetFontUnderLine(TFT_t * dev) {
	dev->_font_underline = false;
}

// Backlight OFF
void lcdBacklightOff(TFT_t * dev) {
	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 0 );
	}
}

// Backlight ON
void lcdBacklightOn(TFT_t * dev) {
	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 1 );
	}
}

// Display Inversion Off
void lcdInversionOff(TFT_t * dev) {
	spi_master_write_command(dev, 0x20);	//Display Inversion Off
}

// Display Inversion On
void lcdInversionOn(TFT_t * dev) {
	spi_master_write_command(dev, 0x21);	//Display Inversion On
}
