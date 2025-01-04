#include "dht11.h"
/**
 * @brief 初始化dht11
 *
 * @param[in] obj 创建的对象
 * @return
 *      - ESP_OK: 初始化成功
 *      - ESP_FAIL: 初始化失败
 */
static esp_err_t init(dht11Str_t *obj);
/**
 * @brief 触发采样
 *
 * @param[in] obj 要触发的对象
 * @return
 *      - ESP_OK: 触发成功
 *      - ESP_FAIL: 触发失败
 */
static esp_err_t startSamp(const dht11Str_t *obj);
/**
 * @brief 采样数据转换
 *
 * @param[in] item 采样数据指针
 * @param[in] itemNum 采样数据长度
 * @return
 *      - ESP_OK: 转换成功
 *      - ESP_FAIL: 转换失败
 */
static esp_err_t parseItems(rmt_symbol_word_t *item, int itemNum);
/**
 * @brief 触发采样及数据转换任务
 *
 * @param[in] param 用户参数
 * @return
 *      - NULL
 */
static void task(void *param);
/**
 * @brief 数据接收完成中断回调
 *
 * @param[in] channel 通道句柄
 * @param[in] edata  采集到的数据
 * @param[in] user_data 用户参数
 * @return
 *      - true: 需要进行任务调度
 *      - false: 无需进行任务调度
 */
static bool dht11RxDoneCbs(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data);
/**
 * @brief 获取温度数据
 *
 * @param[in] obj 获取的对象
 * @return
 *      - temp: 温度
 */
static float getTemp(const dht11Str_t *obj);
/**
 * @brief 获取湿度数据
 *
 * @param[in] obj 获取的对象
 * @return
 *      - humidi: 湿度
 */
static uint8_t getHumidity(const dht11Str_t *obj);

// 创建对象并设置好对应函数指针
dht11Str_t DHT11_OBJ = {
    .init = init,
    .startSamp = startSamp,
    .parseItems = parseItems,
    .task = task,
    .rxDoneCbs = {
        .on_recv_done = dht11RxDoneCbs,
    },
    .getTemp = getTemp,
    .getHumidity = getHumidity,
};

static bool dht11RxDoneCbs(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t highTaskWakeup = pdFALSE;
    QueueHandle_t rxQueue = (QueueHandle_t)user_data;
    // 将接收到的数据发送到数据处理任务
    xQueueSendFromISR(rxQueue, edata, &highTaskWakeup);
    return (highTaskWakeup == pdTRUE);
}

static esp_err_t init(dht11Str_t *obj)
{
    esp_err_t ret = ESP_OK;
    obj->gpio = DHT11_GPIO_NUM;
    // 接收通道使用默认时钟提供时基
    obj->rxChanCfg.clk_src = RMT_CLK_SRC_DEFAULT;
    // 接收通道分辨率
    obj->rxChanCfg.resolution_hz = DHT11_RESOLUTION_HZ;
    // 通道每次可以存储的符号数量
    obj->rxChanCfg.mem_block_symbols = DHT11_STANDARD_FRAME_LENGTH * 2;
    // gpio num
    obj->rxChanCfg.gpio_num = DHT11_GPIO_NUM;
    // deinit the rxChanHandle
    obj->rxChanHandle = NULL;
    ret = rmt_new_rx_channel(&obj->rxChanCfg, &obj->rxChanHandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    // 注册中断回调函数并创建队列
    obj->rxQueue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    // assert(obj->rxQueue);
    if (obj->rxQueue == NULL)
    {
        return ESP_FAIL;
    }
    ret = rmt_rx_register_event_callbacks(obj->rxChanHandle, &obj->rxDoneCbs, obj->rxQueue);
    // 以下参数只与dht11的时序有关
    obj->rxCfg.signal_range_min_ns = 1000;       // 最短信号持续时间为1us，低于此时间认为是干扰
    obj->rxCfg.signal_range_max_ns = 200 * 1000; // dht11最长信号持续时间为80us，超过200us则认为是结束信号
    // 使能rmt
    ret = rmt_enable(obj->rxChanHandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    // 创建采样计算任务
    if (xTaskCreate(obj->task, "dht11Task", DHT11_TASK_STACK, NULL, DHT11_TASK_PRIORITY, NULL) != pdPASS)
    {
        ret = ESP_FAIL;
        ESP_LOGE(DHT11_TAG, "task creat fail : errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
    }
    return ret;
}

static esp_err_t startSamp(const dht11Str_t *obj)
{
    if (gpio_set_direction(obj->gpio, GPIO_MODE_OUTPUT) != ESP_OK)
    {
        return ESP_FAIL;
    }
    // 发送20ms低电平触发DHT11开始采集
    if (gpio_set_level(obj->gpio, 0) != ESP_OK)
    {
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    // 拉高20us表示可以开始接收数据
    if (gpio_set_level(obj->gpio, 1) != ESP_OK)
    {
        return ESP_FAIL;
    }
    ets_delay_us(20);
    // 设置输入模式开始接收数据
    if (gpio_set_direction(obj->gpio, GPIO_MODE_INPUT) != ESP_OK)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t parseItems(rmt_symbol_word_t *item, int itemNum)
{
    rmt_symbol_word_t *pItem = item;
    uint8_t i = 0;
    uint16_t temp = 0;
    uint16_t humidity = 0;
    uint8_t checkSum = 0;

    if (itemNum < 42)
    {
        // 响应+40bit数据+结束=42
        return ESP_FAIL;
    }
// 跳过应答信号
#if (DHT11_LOG == 1)
    ESP_LOGI(DHT11_TAG, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
             pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    pItem++;
    // 湿度数据
    for (i = 0; i < 16; i++, pItem++)
    {
        humidity <<= 1;
        // 正常情况第一阶段应该是低电平，但是如果第一阶段是高电平的话，可能dht响应的80us低电平没被rmt接收
        if (pItem->level0 == 1)
        {
            // 只看高电平持续时间
            if (pItem->duration0 > DHT11_0_CODE_DURATION_MARGIN)
            {
                // 高位先行
                humidity |= 0x01;
            }
        }
        else
        {
            // 只看高电平持续时间
            if (pItem->duration1 > DHT11_0_CODE_DURATION_MARGIN)
            {
                // 高位先行
                humidity |= 0x01;
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(DHT11_TAG, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
                 pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    }
    // 温度数据
    for (i = 0; i < 16; i++, pItem++)
    {
        temp <<= 1;
        // 正常情况第一阶段应该是低电平，但是如果第一阶段是高电平的话，可能dht响应的80us低电平没被rmt接收
        if (pItem->level0 == 1)
        {
            // 只看高电平持续时间
            if (pItem->duration0 > DHT11_0_CODE_DURATION_MARGIN)
            {
                // 高位先行
                temp |= 0x01;
            }
        }
        else
        {
            // 只看高电平持续时间
            if (pItem->duration1 > DHT11_0_CODE_DURATION_MARGIN)
            {
                // 高位先行
                temp |= 0x01;
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(DHT11_TAG, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
                 pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    }
    // 校验和
    for (i = 0; i < 8; i++, pItem++)
    {
        checkSum <<= 1;
        // 正常情况第一阶段应该是低电平，但是如果第一阶段是高电平的话，可能dht响应的80us低电平没被rmt接收
        if (pItem->level0 == 1)
        {
            // 只看高电平持续时间
            if (pItem->duration0 > DHT11_0_CODE_DURATION_MARGIN)
            {
                // 高位先行
                checkSum |= 0x01;
            }
        }
        else
        {
            // 只看高电平持续时间
            if (pItem->duration1 > DHT11_0_CODE_DURATION_MARGIN)
            {
                // 高位先行
                checkSum |= 0x01;
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(DHT11_TAG, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
                 pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    }

    if (((humidity & 0x00FF) + ((humidity & 0xFF00) >> 8) +
         (temp & 0x00FF) + ((temp & 0xFF00) >> 8)) != checkSum)
    {
#if (DHT11_LOG == 1)
        ESP_LOGI(DHT11_TAG, "check fail");
#endif
        // 校验失败，丢掉此帧
        return ESP_FAIL;
    }

    DHT11_OBJ.humidity = ((humidity & 0xFF00) >> 8);
    temp = (((temp & 0xFF00) >> 8) * 10) + (temp & 0x00FF);
    DHT11_OBJ.temp = (float)temp / 10;

    return ESP_OK;
}

static void task(void *param)
{
    // 保存接收到的数据，长度为dht11标准帧的两倍
    rmt_symbol_word_t rxSymbols[DHT11_STANDARD_FRAME_LENGTH * 2];
    rmt_rx_done_event_data_t rxData;
    memset(&rxSymbols[0], 0x00, sizeof(rxSymbols));
    memset(&rxData, 0x00, sizeof(rxData));
    while (true)
    {
        if (DHT11_OBJ.startSamp(&DHT11_OBJ) == ESP_OK)
        {
            if (rmt_receive(DHT11_OBJ.rxChanHandle, rxSymbols, sizeof(rxSymbols), &DHT11_OBJ.rxCfg) == ESP_OK)
            {
                if (xQueueReceive(DHT11_OBJ.rxQueue, &rxData, pdMS_TO_TICKS(1000)) == pdPASS)
                {
                    DHT11_OBJ.parseItems(rxData.received_symbols, rxData.num_symbols);
                }
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(DHT11_TAG, "humidity = %d, temp = %.1f", DHT11_OBJ.humidity, DHT11_OBJ.temp);
        ESP_LOGI(DHT11_TAG, "free stack %d bytes", uxTaskGetStackHighWaterMark(NULL));
#endif
        vTaskDelay(pdMS_TO_TICKS(DHT11_SAMP_FRE * 1000));
    }
}

static float getTemp(const dht11Str_t *obj)
{
    return obj->temp;
}

static uint8_t getHumidity(const dht11Str_t *obj)
{
    return obj->humidity;
}
