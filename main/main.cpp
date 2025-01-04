#include <stdio.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "myFont.h"
#include "ledDisplay.h"
#include "w25q64.h"
#include "WouoUI.h"
#include "dht11.h"
#include "temt6000.h"

extern "C" void app_main(void)
{

    ESP_LOGI("app_main", "free memory : %lu bytes", esp_get_free_heap_size());


    ledDisplay.init();
    w25q64.init();
    dht11.init(&dht11);
    temt6000.init(&temt6000);
    while (true)
    {
        ESP_LOGI("app_main", "Hello");
        vTaskDelay(pdMS_TO_TICKS(100000));
    }
}
