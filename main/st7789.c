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

#define TAG "ST7789"

#define SPI_CMD_MODE  0x00
#define SPI_DATA_MODE 0x01

#define WRITE_BUFF_LEN 1024
#define FONT_GLYPH_BUFF_LEN 256
#define DISPLAY_GLYPH_BUFF_LEN 512

static uint16_t packet[WRITE_BUFF_LEN];	       // Write colors buffer
static uint8_t  dots[FONT_GLYPH_BUFF_LEN];	   // Font file glyph buffet
static uint16_t glyph[DISPLAY_GLYPH_BUFF_LEN]; // Glyph buffer for display write

void delayMS(int ms) {
    int _ms = ms + (portTICK_PERIOD_MS - 1);
    TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
    vTaskDelay(xTicksToDelay);
}

void spi_master_init(TFT_t *dev, display_config_t *display_config, spi_device_interface_config_t *spiInterfaceConfig)
{
    if (display_config->pinCS >= 0) {
        gpio_reset_pin( display_config->pinCS );
        gpio_set_direction( display_config->pinCS, GPIO_MODE_OUTPUT );
        gpio_set_level( display_config->pinCS, 0 );
    }

    gpio_reset_pin(display_config->pinDC);
    gpio_set_direction( display_config->pinDC, GPIO_MODE_OUTPUT );
    gpio_set_level(display_config->pinDC, 0);

    if (display_config->pinRESET >= 0) {
        gpio_reset_pin( display_config->pinRESET );
        gpio_set_direction( display_config->pinRESET, GPIO_MODE_OUTPUT );
        gpio_set_level( display_config->pinRESET, 1 );
        delayMS(50);
        gpio_set_level( display_config->pinRESET, 0 );
        delayMS(50);
        gpio_set_level( display_config->pinRESET, 1 );
        delayMS(50);
    }

    if (display_config->pinBL >= 0) {
        gpio_reset_pin(display_config->pinBL);
        gpio_set_direction(display_config->pinBL, GPIO_MODE_OUTPUT);
        gpio_set_level(display_config->pinBL, 0);
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = display_config->pinMOSI,
        .miso_io_num = -1,
        .sclk_io_num = display_config->pinSCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0
    };

    esp_err_t ret = spi_bus_initialize( display_config->spiHost, &buscfg, SPI_DMA_CH_AUTO );
    ESP_LOGD(TAG, "spi_bus_initialize=%d",ret);
    assert(ret==ESP_OK);

    spi_device_handle_t handle;
    ret = spi_bus_add_device(display_config->spiHost, spiInterfaceConfig, &handle);
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
    gpio_set_level(dev->_dc, SPI_DATA_MODE);
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

uint16_t spi_master_write_packet(TFT_t * dev, uint16_t color, uint16_t size)
{
    gpio_set_level(dev->_dc, SPI_DATA_MODE);

    // Swapping bytes in a word
    uint16_t color_word = color >> 8 | (color & 0xFF) << 8;

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

/**
 * @brief Under development
 * 
 * @param dev 
 * @param colors 
 * @param size 
 * @return uint16_t 
 */
uint16_t spi_master_read_packet(TFT_t *dev, uint16_t *colors, uint16_t size)
{
    gpio_set_level(dev->_dc, SPI_DATA_MODE);

    //ESP_LOG_BUFFER_HEX("colors input", colors, size);
    ESP_LOGD("size", "size: %d", size);

    spi_transaction_t SPITransaction;

    memset( &SPITransaction, 0, sizeof(spi_transaction_t) );
    SPITransaction.flags = SPI_TRANS_VARIABLE_DUMMY;
    // SPITransaction.
    SPITransaction.length = size * 16;
    //SPITransaction.rxlength = size * 16; // bits in a packet
    SPITransaction.tx_buffer = NULL;
    SPITransaction.rx_buffer = colors;

    esp_err_t ret = spi_device_transmit( dev->_SPIHandle, &SPITransaction );
    assert(ret==ESP_OK);

    ESP_LOG_BUFFER_HEX("colors", colors, size);
    ESP_LOGD("rxlength", "%d", SPITransaction.rxlength);

    // TODO: Swapping bytes in a color
    return SPITransaction.rxlength / 16;
}

uint16_t spi_master_write_colors(TFT_t * dev, uint16_t *colors, uint16_t size)
{
    gpio_set_level(dev->_dc, SPI_DATA_MODE);

    uint16_t len;
    uint16_t total = 0;
    uint16_t colorIndex = 0;
    for (uint16_t p = 0; p <= size; p += WRITE_BUFF_LEN) {
        len = (size - p) > WRITE_BUFF_LEN ? WRITE_BUFF_LEN : (size - p);

        for(int i = 0; i < len; i++) {
            // Swapping bytes in a word
            packet[i] = colors[colorIndex] >> 8 | (colors[colorIndex] & 0xFF) << 8;
            colorIndex++;
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

/**
 * @brief Initialize a lcd device with a config
 *
 * @param dev
 * @param display_config
 */
void lcdInit(TFT_t *dev, display_config_t *display_config, spi_device_interface_config_t *spiInterfaceConfig)
{
    dev->_width = display_config->width;
    dev->_height = display_config->height;
    dev->maxX = display_config->width - 1;
    dev->maxY = display_config->height - 1;
    dev->_offsetx = 0;
    dev->_offsety = 0;
    dev->_font_direction = DIRECTION0;
    dev->_font_fill = false;
    dev->_font_underline = false;
    dev->diplayBufferLen = WRITE_BUFF_LEN;

    spi_master_init(dev, display_config, spiInterfaceConfig);

    spi_master_write_command(dev, LCD_CMD_SWRESET);	//Software Reset
    delayMS(150);

    spi_master_write_command(dev, LCD_CMD_SLPOUT);	//Sleep Out
    delayMS(255);

    spi_master_write_command(dev,   LCD_CMD_COLMOD);	//Interface Pixel Format
    spi_master_write_data_byte(dev, 0x55); // 0 101 0 101 - 16 bit/pixel, 65K RGB interface
    // spi_master_write_data_byte(dev, 0x66); // 18 bit color format
    delayMS(10);

    spi_master_write_command(dev, LCD_CMD_MADCTL);	//Memory Data Access Control
    spi_master_write_data_byte(dev, 0x00);

    spi_master_write_command(dev, LCD_CMD_CASET);	//Column Address Set
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0xF0);

    spi_master_write_command(dev, LCD_CMD_RASET);	//Row Address Set
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0xF0);

    spi_master_write_command(dev, LCD_CMD_INVON);	//Display Inversion On
    delayMS(10);

    spi_master_write_command(dev, LCD_CMD_NORON);	//Normal Display Mode On
    delayMS(10);

    spi_master_write_command(dev, LCD_CMD_DISPON);	//Display ON
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


/**
 * @brief Draw multiple pixels to a region
 *
 * @param dev
 * @param x
 * @param y
 * @param width
 * @param height
 * @param pixels
 * @param size
 */
void lcdDrawPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *pixels, uint16_t size)
{
    if (width == 0) return;
    if (height == 0) return;
    if (x > dev->_width - 1) return;
    if (y > dev->_height - 1) return;

    uint16_t _x1 = x;
    uint16_t _x2 = x + width - 1;
    uint16_t _y1 = y;
    uint16_t _y2 = y + height - 1;

    if (_x2 > dev->_width - 1) {
        _x2 = dev->_width - 1;
    }
    if (_y2 > dev->_height - 1) {
        _y2 = dev->_height - 1;
    };

    uint16_t _size = size <= width * height ? size : width * height;

    spi_master_write_command(dev, LCD_CMD_CASET);	// set column(x) address
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, LCD_CMD_RASET);	// set Page(y) address
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, LCD_CMD_RAMWR);	//	Memory Write

    spi_master_write_colors(dev, pixels, _size);
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
    if (width == 0) return;
    if (height == 0) return;
    if (x1 > dev->_width - 1) return;
    if (y1 > dev->_height - 1) return;

    uint16_t _x1 = x1;
    uint16_t _x2 = x1 + width - 1;
    uint16_t _y1 = y1;
    uint16_t _y2 = y1 + height - 1;

    if (_x2 > dev->_width - 1) {
        _x2 = dev->_width - 1;
    }
    if (_y2 > dev->_height - 1) {
        _y2 = dev->_height - 1;
    };

    uint16_t size = width * height;

    spi_master_write_command(dev, LCD_CMD_CASET);	// set column(x) address
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, LCD_CMD_RASET);	// set Page(y) address
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, LCD_CMD_RAMWR);	//	Memory Write

    spi_master_write_packet(dev, color, size);
}

/**
 * @brief Read colors from region in display memory to buffer (!under development!)
 * 
 * @param dev 
 * @param x 
 * @param y 
 * @param width 
 * @param height 
 * @param colors 
 * @param size 
 */
uint16_t lcdReadRegion(TFT_t * dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *colors)
{
    if (width == 0) return 0;
    if (height == 0) return 0;
    if (x > dev->maxX) return 0;
    if (y > dev->maxY) return 0;

    uint16_t _x1 = x;
    uint16_t _x2 = x + width - 1;
    uint16_t _y1 = y;
    uint16_t _y2 = y + height - 1;

    if (_x2 > dev->maxX) {
        _x2 = dev->maxX;
    }
    if (_y2 > dev->maxY) {
        _y2 = dev->maxY;
    };

    uint16_t colorsLen = width * height;

    ESP_LOGD("xy", "_x1:%d _x2:%d _y1:%d _y2:%d", _x1, _x2, _y1, _y2);

    spi_master_write_command(dev, LCD_CMD_COLMOD);  // Interface Pixel Format
    spi_master_write_data_byte(dev, 0x66);          // Needs by memory write (18bits color coding)

    spi_master_write_command(dev, LCD_CMD_CASET);	// set column(x) address
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, LCD_CMD_RASET);	// set Page(y) address
    spi_master_write_addr(dev, _y1, _y2);

    spi_master_write_command(dev, LCD_CMD_RAMRD);	//	Memory read
    uint16_t size = spi_master_read_packet(dev, colors, colorsLen);      // Read colors to buffer

    spi_master_write_command(dev, LCD_CMD_COLMOD);	// Interface Pixel Format
    spi_master_write_data_byte(dev, 0x55);          // Change back

    return size;
}

// Display OFF
void lcdDisplayOff(TFT_t * dev) {
    spi_master_write_command(dev, 0x28);	//Display off
}

// Display ON
void lcdDisplayOn(TFT_t * dev) {
    spi_master_write_command(dev, 0x29);	//Display on
}

/**
 * @brief Fill screen with color
 *
 * @param dev
 * @param color
 */
void lcdFillScreen(TFT_t *dev, uint16_t color) {
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
    lcdDrawHLineT(dev, x1, y1, length, 1, color);
}

/**
 * @brief Fast display a horizontal line with a thickness
 *
 * @param dev
 * @param x1
 * @param y1
 * @param length
 * @param b
 * @param color
 */
void lcdDrawHLineT(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t length, uint16_t b, uint16_t color)
{
    if (length == 0) return;
    if (b < 1) return;

    lcdDrawFillRect(dev, x1, y1, length, b, color);
}

/**
 * @brief Fast display a vertical line
 *
 * @param dev
 * @param x1
 * @param y1
 * @param height
 * @param color
 */
void lcdDrawVLine(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t height, uint16_t color)
{
    lcdDrawVLineT(dev, x1, y1, height, 1, color);
}

/**
 * @brief Fast display a vertical line with a thickness
 *
 * @param dev
 * @param x1
 * @param y1
 * @param height
 * @param p
 * @param color
 */
void lcdDrawVLineT(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t height, uint16_t b, uint16_t color)
{
    if (height == 0) return;
    if (b < 1) return;

    lcdDrawFillRect(dev, x1, y1, b, height, color);
}

/**
 * @brief Draw line by coordinates
 *
 * @param dev
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 * @param color Line color
 */
void lcdDrawLine(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
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

/**
 * @brief Draw a rectangle primitive
 *
 * @param dev a pointer to lcd device struct
 * @param x1 x coordinate of the upper left corner
 * @param y1 y coordinate of the upper left corner
 * @param width length of the rectangle
 * @param height width of the rectangle
 * @param color border color
 */
void lcdDrawRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t color) {
    lcdDrawHLineT(dev, x1, y1, width, 1, color);
    lcdDrawHLineT(dev, x1, y1 + height - 1, width, 1, color);
    lcdDrawVLineT(dev, x1, y1, height, 1, color);
    lcdDrawVLineT(dev, x1 + width - 1, y1, height, 1, color);
}

/**
 * @brief Draw a rectangle primitive with a border thickness
 *
 * @param dev a pointer to lcd device struct
 * @param x1 x coordinate of the upper left corner
 * @param y1 y coordinate of the upper left corner
 * @param width length of the rectangle
 * @param height width of the rectangle
 * @param b border thickness
 * @param color border color
 */
void lcdDrawRectT(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t b, uint16_t color) {
    uint16_t yh = y1 + height - b;
    uint16_t xv = x1 + width - b;
    lcdDrawHLineT(dev, x1, y1, width, b, color);
    lcdDrawHLineT(dev, x1, yh, width, b, color);
    lcdDrawVLineT(dev, x1, y1, height, b, color);
    lcdDrawVLineT(dev, xv, y1, height, b, color);
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
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Convert RGB 24-bit to 16-bit RGB565 color
 *
 * @param color
 * @return uint16_t
 */
uint16_t rgb24to16(uint32_t color)
{
    uint8_t b = color & 0xFF;
    uint8_t g = (color & 0xFF00) >> 8;
    uint8_t r = (color & 0xFF0000) >> 16;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Fast draw char by code with a color and background color. Returns a char width in pixels.
 *
 * @param dev
 * @param fxs
 * @param x
 * @param y
 * @param charCode
 * @param color
 * @param bgColor
 * @return uint8_t
 */
uint8_t lcdDrawChar(TFT_t *dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t charCode, uint16_t color, uint16_t bgColor)
{
    uint8_t pw, ph;

    GetFontx(fxs, charCode, &dots, &pw, &ph);

    if (x + pw > dev->_width - 1) {
        return 0;
    }
    if (y + ph > dev->_height - 1) {
        return 0;
    }

    uint16_t glyphIndex = 0;
    uint16_t glyphBytes = ceil(pw / 8.0);
    uint8_t lastBitsCount = pw % 8;
    uint8_t lastBitNumber = lastBitsCount == 0 ? 0 : 8 - lastBitsCount;
    uint8_t lastBitIndex = 0;
    uint8_t bytesCounter = 0;
    //ESP_LOGD("GetFontx", "rc=%d pw=%d ph=%d glyphBytes=%d lastBits=%d", rc, pw, ph, glyphBytes, lastBitsCount);

    for (uint16_t i = 0; i < ph * glyphBytes; i++) {
        // Find partial filled byte
        bytesCounter++;
        if (bytesCounter == glyphBytes) {
            lastBitIndex = lastBitNumber;
            bytesCounter = 0;
        } else {
            lastBitIndex = 0;
        }

        // Convert positive glyph bits to bytes and negative bits to background color
        for (int8_t bitIndex = 7; bitIndex >= lastBitIndex; bitIndex--) {
            glyph[glyphIndex] = ((dots[i] >> bitIndex) & 0x01) ? color : bgColor;
            // TODO buffer overflow control
            glyphIndex++;
        }
    }

    // Send to display memory
    lcdDrawPixels(dev, x, y, pw, ph, &glyph, glyphIndex);

    return pw;
}

/**
 * @brief Not implemented
 * 
 * @param dev 
 * @param fxs 
 * @param x 
 * @param y 
 * @param charCode 
 * @param color 
 * @param bgColor 
 * @return uint8_t 
 */
uint8_t lcdDrawCharS(TFT_t *dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t charCode, uint16_t color)
{
    uint8_t pw, ph;

    GetFontx(fxs, charCode, &dots, &pw, &ph);

    if (x + pw > dev->_width - 1) {
        return 0;
    }
    if (y + ph > dev->_height - 1) {
        return 0;
    }

    uint16_t dotX = x;
    uint16_t dotY = y;
    uint16_t glyphBytes = ceil(pw / 8.0);
    uint8_t lastBitsCount = pw % 8;
    uint8_t lastBitNumber = lastBitsCount == 0 ? 0 : 8 - lastBitsCount;
    uint8_t lastBitIndex = 0;
    uint8_t bytesCounter = 0;
    //ESP_LOGD("GetFontx", "rc=%d pw=%d ph=%d glyphBytes=%d lastBits=%d", rc, pw, ph, glyphBytes, lastBitsCount);

    for (uint16_t i = 0; i < ph * glyphBytes; i++) {
        // Find partial filled byte
        bytesCounter++;
        if (bytesCounter == glyphBytes) {
            lastBitIndex = lastBitNumber;
            bytesCounter = 0;
        } else {
            lastBitIndex = 0;
        }

        // Convert positive glyph bits to bytes and negative bits to background color
        for (int8_t bitIndex = 7; bitIndex >= lastBitIndex; bitIndex--) {
            if ((dots[i] >> bitIndex) & 0x01) {
                lcdDrawPixel(dev, dotX, dotY, color);
            }
        }
    }

    return pw;
}

/**
 * @brief Fast draw a string with a color and background color. Returns a string length in pixels.
 *
 * @param dev
 * @param fx
 * @param x
 * @param y
 * @param str
 * @param color
 * @param bgColor
 * @return uint16_t
 */
uint16_t lcdDrawString(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bgColor)
{
    size_t length = strlen(str);
    uint16_t strX = x;
    uint16_t strWidth = 0;
    uint16_t charWidth = 0;

    for(size_t i = 0; i < length; i++) {
        charWidth = lcdDrawChar(dev, fx, strX + strWidth, y, str[i], color, bgColor);
        if (charWidth == 0) {
            break;
        }
        strWidth += charWidth;
    }

    return strWidth;
}

/**
 * @brief Not implemented
 * 
 * @param dev 
 * @param fx 
 * @param x 
 * @param y 
 * @param str 
 * @param color 
 * @return uint16_t 
 */
uint16_t lcdDrawStringS(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, char *str, uint16_t color)
{
    size_t length = strlen(str);
    uint16_t strX = x;
    uint16_t strWidth = 0;
    uint16_t charWidth = 0;

    for(size_t i = 0; i < length; i++) {
        charWidth = lcdDrawCharS(dev, fx, strX + strWidth, y, str[i], color);
        if (charWidth == 0) {
            break;
        }
        strWidth += charWidth;
    }

    return strWidth;
}

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

/**
 * @brief Reading display data access control (MADCTL) bits information
 * 
 * @param dev 
 * @param mad_ctl 
 * @return uint16_t 
 */
esp_err_t lcdReadMemoryDataAccessControl(TFT_t *dev, mad_ctl_t *madCtl)
{
    spi_master_write_command(dev, LCD_CMD_RDD_MADCTL);

    gpio_set_level(dev->_dc, SPI_DATA_MODE);

    spi_transaction_t SPITransaction;
    memset(&SPITransaction, 0, sizeof(spi_transaction_t));
    SPITransaction.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_MODE_OCT;
    SPITransaction.length = 8 * 1; // in bits
    SPITransaction.rxlength = SPITransaction.length;

    esp_err_t ret = spi_device_transmit( dev->_SPIHandle, &SPITransaction );

    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t madByte = SPITransaction.rx_data[0];

    madCtl->MH  = madByte >> 2 & 0x01;
    madCtl->RGB = madByte >> 3 & 0x01;
    madCtl->ML  = madByte >> 4 & 0x01;
	madCtl->MV  = madByte >> 5 & 0x01;
	madCtl->MX  = madByte >> 6 & 0x01;
	madCtl->MY  = madByte >> 7 & 0x01;

    return ret;
}