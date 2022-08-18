#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/spi_master.h"
#include "math.h"

#include "st7789.h"
#include "colors.h"
#include "fontx.h"

#define	INTERVAL 2000/portTICK_PERIOD_MS
#define WAIT vTaskDelay(INTERVAL)

#define CONFIG_WIDTH        240
#define CONFIG_HEIGHT       240
#define CONFIG_MOSI_GPIO    GPIO_NUM_23
#define CONFIG_SCLK_GPIO    GPIO_NUM_18
#define CONFIG_CS_GPIO      GPIO_NUM_NC   /* Not connected */
#define CONFIG_DC_GPIO      GPIO_NUM_4
#define CONFIG_RESET_GPIO   GPIO_NUM_5
#define CONFIG_BL_GPIO      GPIO_NUM_NC   /* Not connected */
#define CONFIG_SPI_HOST     SPI3_HOST

static const char *TAG = "ST7789";

static FontxFile fx16G[2];
static FontxFile fx24G[2];
static FontxFile fx32G[2];
static FontxFile fx32L[2];
static FontxFile fx16M[2];
static FontxFile fx24M[2];
static FontxFile fx32M[2];

static void SPIFFS_Directory(char * path)
{
    DIR* dir = opendir(path);
    assert(dir != NULL);
    while (true) {
        struct dirent*pe = readdir(dir);
        if (!pe) break;
        ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
    }
    closedir(dir);
}

double getTimeSec( void )
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    double time = spec.tv_sec + spec.tv_nsec / 1e9;
    return time;
}

void TimerTextTest(TFT_t *dev)
{
    uint8_t xpos = 30;
    uint8_t ypos = 20;
    char str[50];
    uint16_t bgColor = rgb24to16(WEB_BLACK);
    uint16_t textColor = rgb24to16(WEB_ORANGE_RED);
    uint8_t displayPeriod = 5;
    double startTime = getTimeSec();

    lcdFillScreen(dev, bgColor);

    while (getTimeSec() - startTime <= displayPeriod) {
        sprintf(str, "Elapsed %.2fs", getTimeSec());

        uint16_t strWidth = lcdDrawString(dev, fx24G, xpos, ypos, str, textColor, bgColor);
        uint16_t strEnd = xpos + strWidth;
        // Filling a background after the string to the display edge
        lcdDrawFillRect(dev, strEnd, ypos, dev->_width - strEnd, ypos + 24, bgColor);
    }

    ESP_LOGI(__FUNCTION__, "Completed.");
}

void TextComplexBackgroundTest(TFT_t *dev)
{
    uint16_t step = ceil(dev->_width / (float)BLUE);
    uint16_t color = BLACK + 1;
    uint16_t colorStep = 1;

    double startTime = getTimeSec();

    // Drawing blue gradient
    for (uint16_t x = 0; x < dev->_width; x += step)
    {
        lcdDrawFillRect(dev, x, 0, step, dev->_height, color);
        color += colorStep;
    }

    // Drawing strings
    char *str1 = "Once upon a time...";
    char *str2 = "Once upon a time...";

    lcdDrawString(dev, fx24G, 0, 50, str1, rgb24to16(WEB_GOLDENROD), BLUE);
    double endTime = getTimeSec() - startTime;
    ESP_LOGI(__FUNCTION__, "lcdDrawString time: %f s", endTime);

    startTime = getTimeSec();
    lcdDrawStringS(dev, fx24G, 0, 100, str2, rgb24to16(WEB_GOLDENROD));

    endTime = getTimeSec() - startTime;
    ESP_LOGI(__FUNCTION__, "lcdDrawStringS time: %f s", endTime);

    
}

void ReadMadCtlTest(TFT_t *dev)
{
    mad_ctl_t madCtl;
    lcdReadMemoryDataAccessControl(dev, &madCtl);
    ESP_LOGI("MADCTL flags", "MX=%d RGB=%d ML=%d MV=%d MX=%d MY=%d", madCtl.MH, madCtl.RGB, madCtl.ML, madCtl.MV, madCtl.MX, madCtl.MY);
}

void MenuTest(TFT_t *dev)
{
    double startTick, diffTick;

    lcdFillScreen(dev, BLACK);

    uint16_t xpos = 5;
    uint16_t ypos = 5;
    uint16_t step = 30;
    uint16_t items = 8;

    uint16_t rect_x1 = 0;
    uint16_t width = dev->_width - 2;
    uint16_t height = 24;
    uint16_t rect_y1 = 4;
    uint16_t rect_step = step;

    char s[30];
    for (int i = 0; i < items; i++)
    {
        sprintf(s, "Menu item %d", i+1);
        lcdDrawString(dev, fx24G, xpos, ypos, s, GREEN, BLACK);
        ypos += step;
    }

    for (int i = 0; i < items; i++)
    {
        startTick = getTimeSec();
        // Clear previous menu rectangle
        if (i > 0 && i < items) {
            lcdDrawRect(dev, rect_x1, rect_y1 - step, width, height, BLACK);
        }
        // Display next rectangle
        lcdDrawRect(dev, rect_x1, rect_y1, width, height, YELLOW);

        rect_y1 += rect_step;

        diffTick = getTimeSec() - startTick;
        ESP_LOGI(__FUNCTION__, "Menu rect '%d' drawing time: %f s", i, diffTick);
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}

void SaturationBlue(TFT_t *dev)
{
    double startTick, diffTick;
    startTick = getTimeSec();

    for (uint16_t color = BLACK; color <= BLUE; color++)
    {
        lcdDrawFillRect(dev, 0, 0, dev->_width, dev->_height, color);
    }

    diffTick = getTimeSec() - startTick;
    ESP_LOGI(__FUNCTION__, "drawing time: %f s", diffTick);
}

void SaturationRed(TFT_t *dev)
{
    double startTick, diffTick;
    startTick = getTimeSec();

    uint16_t color;
    for (uint16_t i = 0; i < 0x1F; i++)
    {
        // Increment only red part of color
        color = i << 11;
        lcdDrawFillRect(dev, 0, 0, dev->_width, dev->_height, color);
    }

    diffTick = getTimeSec() - startTick;
    ESP_LOGI(__FUNCTION__, "drawing time: %f s", diffTick);
}

void SaturationGreen(TFT_t *dev)
{
    double startTick, diffTick;
    startTick = getTimeSec();

    uint16_t color;
    // Green color have one more bit wide
    for (uint16_t i = 0; i < 0x3F; i++)
    {
        // Increment only green part of color in RGB565
        color = i << 5;
        lcdDrawFillRect(dev, 0, 0, dev->_width, dev->_height, color);
    }

    diffTick = getTimeSec() - startTick;
    ESP_LOGI(__FUNCTION__, "drawing time: %f s", diffTick);
}

void Lines(TFT_t *dev)
{
    double startTick, diffTick;

    uint16_t left = 0;
    uint16_t top = 0;
    uint16_t length = dev->_width;
    uint16_t height = dev->_height;
    uint16_t thickness = 4;
    uint16_t step = 40;

    lcdFillScreen(dev, BLACK);

    startTick = getTimeSec();

    lcdDrawHLineT(dev, left, top+step*1, length, thickness, GREEN);
    lcdDrawHLineT(dev, left, top+step*2, length, thickness, CYAN);
    lcdDrawHLineT(dev, left, top+step*3, length, thickness, YELLOW);
    lcdDrawHLineT(dev, left, top+step*4, length, thickness, PURPLE);
    lcdDrawHLineT(dev, left, top+step*5, length, thickness, RED);

    lcdDrawVLineT(dev, left+step*1, top, height, thickness, GREEN);
    lcdDrawVLineT(dev, left+step*2, top, height, thickness, CYAN);
    lcdDrawVLineT(dev, left+step*3, top, height, thickness, YELLOW);
    lcdDrawVLineT(dev, left+step*4, top, height, thickness, PURPLE);
    lcdDrawVLineT(dev, left+step*5, top, height, thickness, RED);

    diffTick = getTimeSec() - startTick;
    ESP_LOGI(__FUNCTION__, "drawing time: %f s", diffTick);
}

void Squares(TFT_t *dev)
{
    double startTick, diffTick;

    uint16_t left = 0;
    uint16_t top = 0;
    uint16_t width = dev->_width;
    uint16_t height = dev->_height;
    uint16_t step = 16;
    uint16_t thickness = 3;

    lcdFillScreen(dev, BLACK);

    startTick = getTimeSec();

    uint16_t color = RED;
    for (uint8_t i = 0; i < 8*step; i+=step)
    {
        lcdDrawRectT(dev, left+i, top+i, width-2*i, height-2*i, thickness, color);
        color = color - (4 << 11); // shift to get red component
    }

    diffTick = getTimeSec() - startTick;
    ESP_LOGI(__FUNCTION__, "drawing time: %f s", diffTick);
}

void RandomRects(TFT_t *dev)
{
    double startTick, diffTick;

    lcdFillScreen(dev, BLACK);

    startTick = getTimeSec();

    uint16_t colors[] = { CYAN, RED, YELLOW, PURPLE, GREEN, BLUE };
    uint16_t color;
    for (uint8_t i = 0; i < 8; i++)
    {
        for (uint8_t i = 0; i < 6; i++)
        {
            uint16_t left = rand() / 1e7;
            uint16_t top  = rand() / 1e7;
            uint16_t width  = dev->_width / 4;
            uint16_t height = dev->_height / 3;

            color = colors[i];
            lcdDrawFillRect(dev, left, top, width, height, color);
        }
    }

    diffTick = getTimeSec() - startTick;
    ESP_LOGI(__FUNCTION__, "drawing time: %f s", diffTick);
}

void ST7789_Tests(void *pvParameters)
{
    TFT_t *pDisplay = (TFT_t *)pvParameters;
    lcdFillScreen(pDisplay, BLACK);

    for (;;)
    {
        TimerTextTest(pDisplay);
        MenuTest(pDisplay);
        WAIT;
        TextComplexBackgroundTest(pDisplay);
        WAIT;
        Lines(pDisplay);
        WAIT;
        SaturationBlue(pDisplay);
        WAIT;
        SaturationRed(pDisplay);
        WAIT;
        SaturationGreen(pDisplay);
        WAIT;
        Squares(pDisplay);
        WAIT;
        RandomRects(pDisplay);
        WAIT;
        // Need a slower interface for correct working, see InitDisplay()
        // ReadMadCtlTest(pDisplay);
        // WAIT;
    }
}

void InitSpiffs() {
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed =true
    };

    // Use settings defined above toinitialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is anall-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total,&used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
    }

    SPIFFS_Directory("/spiffs/");
}

void InitDisplay(TFT_t *display)
{
    ESP_LOGI(TAG, "Initializing display");
    spi_device_interface_config_t spiInterfaceConfig;
    memset(&spiInterfaceConfig, 0, sizeof(spiInterfaceConfig));
    spiInterfaceConfig.clock_speed_hz = SPI_MASTER_FREQ_40M; // Fast write
    // spiInterfaceConfig.clock_speed_hz = SPI_MASTER_FREQ_8M; // Reading worked correct only on this speed
    spiInterfaceConfig.queue_size = 10;
    spiInterfaceConfig.mode = 3;                       // SPI Mode for correct reading possibility (found from tests on a bare-metal)
    spiInterfaceConfig.flags = SPI_DEVICE_3WIRE;       // No MISO wire, write and read on a same line
    // spiInterfaceConfig.flags |= SPI_DEVICE_HALFDUPLEX; // Correct read on the same line

    if ( CONFIG_CS_GPIO >= 0 ) {
        spiInterfaceConfig.spics_io_num = CONFIG_CS_GPIO;
    } else {
        spiInterfaceConfig.spics_io_num = GPIO_NUM_NC;
    }

    display_config_t displayConfig = {
        .width = CONFIG_WIDTH,
        .height = CONFIG_HEIGHT,
        .pinMOSI = CONFIG_MOSI_GPIO,
        .pinSCLK = CONFIG_SCLK_GPIO,
        .pinCS = CONFIG_CS_GPIO,
        .pinDC = CONFIG_DC_GPIO,
        .pinRESET = CONFIG_RESET_GPIO,
        .pinBL = CONFIG_BL_GPIO,
        .spiHost = SPI3_HOST
    };

    lcdInit(display, &displayConfig, &spiInterfaceConfig);
}

void app_main(void)
{
    static TFT_t display;

    InitSpiffs();
    InitDisplay(&display);

    //InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
    InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
    // InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
    // InitFontx(fx32L,"/spiffs/LATIN32B.FNT",""); // 16x32Dot Latin
    // InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
    // InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
    // InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo

    ESP_LOGI(TAG, "Init complete");

    xTaskCreate(ST7789_Tests, "ST7789_Tests", 1024 * 6, &display, 2, NULL);
}
