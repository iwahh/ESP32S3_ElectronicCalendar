#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "hal/gpio_types.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include <string.h>

#define DHT11_LOG 1
const char *const dht11Tag = "dht11";
const gpio_num_t dht11DefaultPin = GPIO_NUM_1;
const uint16_t dht11DefaultTaskStack = (1024 * 3);
const uint8_t dht11DefaultTaskPriority = (1);
const uint8_t dht11DefaultFrameLength = (40);
const uint16_t dht11DefaultResolutionHz = (1 * 1000 * 1000);

class DHT11_C
{
public:
    DHT11_C(gpio_num_t gpio)
    {
        this->gpio = gpio;
        this->initVal();
    }
    DHT11_C(void)
    {
        this->gpio = dht11DefaultPin;
        this->initVal();
    }

    void initVal(void)
    {
        rmtChannelHandle = NULL;
        rmtReceiveCfg.signal_range_min_ns = (1000);
        rmtReceiveCfg.signal_range_max_ns = (200 * 1000);
        rmtQueue = NULL;
        taskHandle = NULL;
        temp = 0.0f;
        humidity = 0;
    }

    esp_err_t init(bool isCreatTask = true);
    float getTemp(void);
    uint8_t getHumidity(void);

private:
    static bool rmtRxDoneCbs(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx);
    static void taskHandler(void *param);

    esp_err_t startSample(void);
    esp_err_t parseItems(rmt_symbol_word_t *item, int itemNum);

    float temp;
    uint8_t humidity;
    gpio_num_t gpio;
    rmt_channel_handle_t rmtChannelHandle;
    rmt_receive_config_t rmtReceiveCfg;
    QueueHandle_t rmtQueue;
    TaskHandle_t taskHandle;
};

esp_err_t DHT11_C::init(bool isCreatTask)
{
    esp_err_t ret = ESP_OK;
    rmt_rx_channel_config_t rmtRxChannelCfg;
    rmt_rx_event_callbacks_t rmtRxEvtCbs;
    memset(&rmtRxChannelCfg, 0x00, sizeof(rmt_rx_channel_config_t));
    memset(&rmtRxEvtCbs, 0x00, sizeof(rmt_rx_event_callbacks_t));
    rmtRxChannelCfg.clk_src = RMT_CLK_SRC_DEFAULT;
    rmtRxChannelCfg.flags.invert_in = false;
    rmtRxChannelCfg.flags.io_loop_back = false;
    rmtRxChannelCfg.flags.with_dma = false;
    rmtRxChannelCfg.gpio_num = this->gpio;
    rmtRxChannelCfg.intr_priority = 0;
    rmtRxChannelCfg.mem_block_symbols = (dht11DefaultFrameLength * 2); // 2倍冗余
    rmtRxChannelCfg.resolution_hz = dht11DefaultResolutionHz;
    ret = rmt_new_rx_channel(&rmtRxChannelCfg, &this->rmtChannelHandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    rmtRxEvtCbs.on_recv_done = DHT11_C::rmtRxDoneCbs;
    ret = rmt_rx_register_event_callbacks(this->rmtChannelHandle, &rmtRxEvtCbs, this);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    ret = rmt_enable(this->rmtChannelHandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    if (isCreatTask == true)
    {
        if (xTaskCreate(DHT11_C::taskHandler, "dht11Task", dht11DefaultTaskStack, this, dht11DefaultTaskPriority, &this->taskHandle) != pdPASS)
        {
            ESP_LOGW(dht11Tag, "errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
            return ESP_FAIL;
        }
        this->rmtQueue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
        if (this->rmtQueue == NULL)
        {
            ESP_LOGW(dht11Tag, "creat queue fail");
            vTaskDelete(this->taskHandle);
            return ESP_FAIL;
        }
    }
    return ret;
}

void DHT11_C::taskHandler(void *param)
{
    while (true)
    {
    }
}

bool DHT11_C::rmtRxDoneCbs(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx)
{
    BaseType_t mustYield = pdFALSE;
    DHT11_C *pObj = (DHT11_C *)user_ctx;
    xQueueSendFromISR(pObj->rmtQueue, edata, &mustYield);
    return (mustYield == pdTRUE);
}

esp_err_t DHT11_C::startSample(void)
{
    if (gpio_set_direction(this->gpio, GPIO_MODE_OUTPUT) != ESP_OK)
    {
        goto fail;
    }
    if (gpio_set_level(this->gpio, 0) != ESP_OK)
    {
        goto fail;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    if (gpio_set_level(this->gpio, 1) != ESP_OK)
    {
        goto fail;
    }
    ets_delay_us(20);
    if (gpio_set_direction(this->gpio, GPIO_MODE_INPUT) != ESP_OK)
    {
        goto fail;
    }
    return ESP_OK;
fail:
    return ESP_FAIL;
}

float DHT11_C::getTemp(void)
{
    if(this->taskHandler != NULL)
    {
        return (this->temp);
    }
}

uint8_t DHT11_C::getHumidity(void)
{
    if(this->taskHandle != NULL)
    {
        return (this->humidity);
    }
}

esp_err_t DHT11_C::parseItems(rmt_symbol_word_t *item, int itemNum)
{
    rmt_symbol_word_t *pItem = item;
    uint8_t i = 0;
    uint16_t temp = 0;
    uint16_t humidity = 0;
    uint8_t checkSum = 0;

    if(itemNum < 42)
    {
        return ESP_FAIL;
    }
}