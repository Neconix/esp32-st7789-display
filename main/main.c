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

#include "st7789.h"
#include "fontx.h"

#define	INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

static const char *TAG = "ST7789";

static void SPIFFS_Directory(char * path) {
    DIR* dir = opendir(path);
    assert(dir != NULL);
    while (true) {
        struct dirent*pe = readdir(dir);
        if (!pe) break;
        ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
    }
    closedir(dir);
}

#define CONFIG_WIDTH        240
#define CONFIG_HEIGHT       240
#define CONFIG_MOSI_GPIO    23
#define CONFIG_SCLK_GPIO    18
#define CONFIG_CS_GPIO     -1   /* Not connected */
#define CONFIG_DC_GPIO      4
#define CONFIG_RESET_GPIO   5
#define CONFIG_BL_GPIO     -1   /* Not connected */


static FontxFile fx16G[2];
static FontxFile fx24G[2];
static FontxFile fx32G[2];
static FontxFile fx32L[2];
static FontxFile fx16M[2];
static FontxFile fx24M[2];
static FontxFile fx32M[2];

TickType_t TextTest(TFT_t *dev, uint8_t view_iteration, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdFillScreen(dev, BLACK);
    lcdSetFontDirection(dev, DIRECTION0);

    uint8_t xpos = 5;
    uint8_t ypos = 15;

    char *s;
    
    s = "Small text line 1";
    lcdDrawString(dev, fx16G, xpos, ypos, s, WHITE);

    s = "Medium text line 2";
    lcdDrawString(dev, fx16G, xpos, ypos*2, s, RED);

    s = "Medium text line 3";
    lcdDrawString(dev, fx24G, xpos, ypos*3 + 5, s, BLUE);

    s = "Big text line 4";
    lcdDrawString(dev, fx32G, xpos, ypos*4 + 18, s, GREEN);

    char s2[30];
    sprintf(s2, "Iteration: %d", view_iteration);
    lcdDrawString(dev, fx24G, xpos, ypos*5 + 22, &s2, YELLOW);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d",diffTick*portTICK_PERIOD_MS);
    return diffTick;
}

void ST7789(void *pvParameters)
{	
    TFT_t dev;
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 0);

    uint8_t view_iteration = 0;

    while(1) {
        view_iteration++;

        TextTest(&dev, view_iteration, CONFIG_WIDTH, CONFIG_HEIGHT);
        WAIT;
    }
}


void app_main(void)
{
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

    InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
    InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
    InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
    InitFontx(fx32L,"/spiffs/LATIN32B.FNT",""); // 16x32Dot Latin
    InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
    InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
    InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo

    xTaskCreate(ST7789, "ST7789", 1024*6, NULL, 2, NULL);
}
