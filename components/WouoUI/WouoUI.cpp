#include "WouoUI.h"
#include "ledDisplay.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void test(void)
{
    while(true)
    {
        ledDisplay.drawString(0, 0, "龘鐽龖", 0xFFFFFF);
        ledDisplay.show();
        vTaskDelay(pdMS_TO_TICKS(10));
        ledDisplay.clear();
    }
}