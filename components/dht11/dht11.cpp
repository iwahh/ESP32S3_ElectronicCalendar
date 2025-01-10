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

// 定义日志输出开关，0表示关闭，1表示打开
#define DHT11_LOG 0
// 日志标签，用于标识DHT11相关的日志信息
const char *const dht11Tag = "dht11";
// DHT11传感器默认连接的GPIO引脚
const gpio_num_t dht11DefaultPin = GPIO_NUM_1;
// DHT11任务默认的栈大小，单位为字节
const uint16_t dht11DefaultTaskStack = (1024 * 3);
// DHT11任务默认的优先级
const uint8_t dht11DefaultTaskPriority = (1);
// DHT11数据帧的默认长度，单位为位
const uint8_t dht11DefaultFrameLength = (40);
// RMT（Remote Memory Transfer）模块的默认分辨率，单位为Hz
const uint32_t dht11DefaultResolutionHz = (1 * 1000 * 1000);
// 用于判断DHT11数据信号的时间长度裕量，单位为ns
const uint8_t dht110CodeDurationMargin = (40);
// DHT11传感器的最大采样频率，单位为次/秒
const uint8_t dht11MaxSampleFrq = (2);
// 定义一个结构体来存储DHT11读取到的温湿度数据
typedef struct dht11DataStr
{
    float temp;       // 温度数据
    uint8_t humidity; // 湿度数据
} dht11DataStr_t;
// 定义一个DHT11操作类
class DHT11_C
{
public:
    // 带参数的构造函数，用于指定DHT11连接的GPIO引脚
    DHT11_C(gpio_num_t gpio)
    {
        this->gpio = gpio; // 保存传入的GPIO引脚
        this->initVal();   // 调用初始化函数
    }
    // 无参数的构造函数，使用默认的GPIO引脚
    DHT11_C(void)
    {
        this->gpio = dht11DefaultPin; // 使用默认的GPIO引脚
        this->initVal();              // 调用初始化函数
    }

    // 初始化成员变量的函数
    void initVal(void)
    {
        rmtChannelHandle = NULL;                                                                        // 初始化RMT通道句柄为NULL
        rmtReceiveCfg.signal_range_min_ns = (1000);                                                     // 设置RMT接收配置的最小信号时间范围
        rmtReceiveCfg.signal_range_max_ns = (200 * 1000);                                               // 设置RMT接收配置的最大信号时间范围
        rmtQueue = NULL;                                                                                // 初始化RMT队列句柄为NULL
        taskHandle = NULL;                                                                              // 初始化任务句柄为NULL
        memset(&this->data, 0x00, sizeof(dht11DataStr_t));                                              // 清空温湿度数据结构体
        memset(&this->rmtRxData, 0x00, sizeof(rmt_rx_done_event_data_t));                               // 清空RMT接收完成事件数据结构体
        memset(&this->rmtRxSymbols, 0x00, (sizeof(rmt_symbol_word_t) * (dht11DefaultFrameLength * 2))); // 清空RMT接收符号数组
    }

    // 初始化DHT11相关配置的函数，可选择是否创建任务
    esp_err_t init(bool isCreatTask = true);
    // 获取DHT11传感器温湿度数据的函数
    void getData(dht11DataStr_t *pData);

private:
    // RMT接收完成回调函数，用于处理RMT接收完成事件
    static bool rmtRxDoneCbs(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx);
    // DHT11任务处理函数，用于定期读取DHT11数据
    static void taskHandler(void *param);

    // 开始采样DHT11数据的函数
    esp_err_t startSample(void);
    // 解析RMT接收到的DHT11数据的函数
    esp_err_t parseItems(rmt_symbol_word_t *item, int itemNum, DHT11_C *pObj);

    dht11DataStr_t data;                   // 存储温湿度数据的结构体
    gpio_num_t gpio;                       // DHT11连接的GPIO引脚
    rmt_channel_handle_t rmtChannelHandle; // RMT通道句柄
    rmt_receive_config_t rmtReceiveCfg;    // RMT接收配置结构体
    QueueHandle_t rmtQueue;                // RMT队列句柄
    TaskHandle_t taskHandle;               // 任务句柄

    rmt_symbol_word_t rmtRxSymbols[dht11DefaultFrameLength * 2]; // 存储RMT接收符号的数组
    rmt_rx_done_event_data_t rmtRxData;                          // 存储RMT接收完成事件数据的结构体
};
// 初始化DHT11相关配置的函数实现
esp_err_t DHT11_C::init(bool isCreatTask)
{
    esp_err_t ret = ESP_OK; // 初始化返回值为ESP_OK，表示成功
    // 定义RMT接收通道配置结构体
    rmt_rx_channel_config_t rmtRxChannelCfg;
    // 定义RMT接收事件回调结构体
    rmt_rx_event_callbacks_t rmtRxEvtCbs;
    // 清空RMT接收通道配置结构体
    memset(&rmtRxChannelCfg, 0x00, sizeof(rmt_rx_channel_config_t));
    // 清空RMT接收事件回调结构体
    memset(&rmtRxEvtCbs, 0x00, sizeof(rmt_rx_event_callbacks_t));

    // 设置RMT时钟源为默认值
    rmtRxChannelCfg.clk_src = RMT_CLK_SRC_DEFAULT;
    // 设置输入信号不反转
    rmtRxChannelCfg.flags.invert_in = false;
    // 设置不启用IO回环
    rmtRxChannelCfg.flags.io_loop_back = false;
    // 设置不使用DMA
    rmtRxChannelCfg.flags.with_dma = false;
    // 设置RMT接收通道使用的GPIO引脚
    rmtRxChannelCfg.gpio_num = this->gpio;
    // 设置中断优先级为0
    rmtRxChannelCfg.intr_priority = 0;
    // 设置存储RMT符号的内存块大小，2倍冗余
    rmtRxChannelCfg.mem_block_symbols = (dht11DefaultFrameLength * 2);
    // 设置RMT分辨率
    rmtRxChannelCfg.resolution_hz = dht11DefaultResolutionHz;

    // 创建新的RMT接收通道
    ret = rmt_new_rx_channel(&rmtRxChannelCfg, &this->rmtChannelHandle);
    // 检查创建通道的返回值，不中断程序
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    // 设置RMT接收完成事件的回调函数
    rmtRxEvtCbs.on_recv_done = DHT11_C::rmtRxDoneCbs;
    // 注册RMT接收事件回调函数
    ret = rmt_rx_register_event_callbacks(this->rmtChannelHandle, &rmtRxEvtCbs, this);
    // 检查注册回调的返回值，不中断程序
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    // 启用RMT通道
    ret = rmt_enable(this->rmtChannelHandle);
    // 检查启用通道的返回值，不中断程序
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    if (isCreatTask == true) // 如果需要创建任务
    {
        // 创建DHT11任务
        if (xTaskCreate(DHT11_C::taskHandler, "dht11Task", dht11DefaultTaskStack, this, dht11DefaultTaskPriority, &this->taskHandle) != pdPASS)
        {
            // 记录任务创建失败的警告日志
            ESP_LOGW(dht11Tag, "errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
            return ESP_FAIL; // 返回失败
        }
    }
    // 创建RMT队列
    this->rmtQueue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    if (this->rmtQueue == NULL) // 如果队列创建失败
    {
        // 记录队列创建失败的警告日志
        ESP_LOGW(dht11Tag, "creat queue fail");
        return ESP_FAIL; // 返回失败
    }
    return ret; // 返回初始化结果
}

// DHT11任务处理函数实现
void DHT11_C::taskHandler(void *param)
{
    DHT11_C *pObj = (DHT11_C *)param; // 将传入的参数转换为DHT11_C对象指针
    while (true)                      // 无限循环
    {
        if (pObj->startSample() == ESP_OK) // 如果开始采样成功
        {
            if (rmt_receive(pObj->rmtChannelHandle, pObj->rmtRxSymbols, sizeof(pObj->rmtRxSymbols), &pObj->rmtReceiveCfg) == ESP_OK) // 如果RMT接收数据成功
            {
                if (xQueueReceive(pObj->rmtQueue, &pObj->rmtRxData, pdMS_TO_TICKS(1000)) == pdPASS) // 如果从队列中接收到数据成功
                {
                    pObj->parseItems(pObj->rmtRxData.received_symbols, pObj->rmtRxData.num_symbols, pObj); // 解析接收到的数据
                }
            }
        }
#if (DHT11_LOG == 1) // 如果日志开关打开
        // 记录温湿度数据
        ESP_LOGI(dht11Tag, "humidity = %d, temp = %.1f", pObj->data.humidity, pObj->data.temp);
        // 记录任务栈剩余空间
        ESP_LOGI(dht11Tag, "free stack %d bytes", uxTaskGetStackHighWaterMark(NULL));
#endif
        // 延迟指定时间，控制采样频率
        vTaskDelay(pdMS_TO_TICKS(dht11MaxSampleFrq * 1000));
    }
}
// RMT接收完成回调函数实现
bool DHT11_C::rmtRxDoneCbs(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx)
{
    BaseType_t mustYield = pdFALSE;      // 初始化是否需要任务切换标志为否
    DHT11_C *pObj = (DHT11_C *)user_ctx; // 将传入的用户上下文转换为DHT11_C对象指针
    if (pObj->rmtQueue != NULL)          // 如果RMT队列存在
    {
        // 从ISR（中断服务程序）向队列发送数据
        xQueueSendFromISR(pObj->rmtQueue, edata, &mustYield);
    }
    return (mustYield == pdTRUE); // 返回是否需要任务切换的结果
}
// 开始采样DHT11数据的函数实现
esp_err_t DHT11_C::startSample(void)
{
    if (gpio_set_direction(this->gpio, GPIO_MODE_OUTPUT) != ESP_OK) // 设置GPIO为输出模式，如果失败
    {
        goto fail; // 跳转到失败处理
    }
    if (gpio_set_level(this->gpio, 0) != ESP_OK) // 设置GPIO电平为低，如果失败
    {
        goto fail; // 跳转到失败处理
    }
    // 延迟20毫秒
    vTaskDelay(pdMS_TO_TICKS(20));
    if (gpio_set_level(this->gpio, 1) != ESP_OK) // 设置GPIO电平为高，如果失败
    {
        goto fail; // 跳转到失败处理
    }
    // 延迟20微秒
    ets_delay_us(20);
    if (gpio_set_direction(this->gpio, GPIO_MODE_INPUT) != ESP_OK) // 设置GPIO为输入模式，如果失败
    {
        goto fail; // 跳转到失败处理
    }
    return ESP_OK; // 返回成功
fail:
    return ESP_FAIL; // 返回失败
}
// 获取DHT11传感器温湿度数据的函数实现
void DHT11_C::getData(dht11DataStr_t *pData)
{
    if (this->taskHandle != NULL) // 如果任务已经创建
    {
        pData->temp = this->data.temp;         // 将存储的温度数据赋值给输出参数
        pData->humidity = this->data.humidity; // 将存储的湿度数据赋值给输出参数
    }
    else // 如果任务未创建
    {
        if (this->startSample() == ESP_OK) // 如果开始采样成功
        {
            if (rmt_receive(this->rmtChannelHandle, this->rmtRxSymbols, sizeof(this->rmtRxSymbols), &this->rmtReceiveCfg) == ESP_OK) // 如果RMT接收数据成功
            {
                if (xQueueReceive(this->rmtQueue, &this->rmtRxData, pdMS_TO_TICKS(1000)) == pdPASS) // 如果从队列中接收到数据成功
                {
                    if (this->parseItems(this->rmtRxData.received_symbols, this->rmtRxData.num_symbols, this) == ESP_OK) // 如果解析数据成功
                    {
                        pData->temp = this->data.temp;         // 将存储的温度数据赋值给输出参数
                        pData->humidity = this->data.humidity; // 将存储的湿度数据赋值给输出参数
                    }
                }
            }
        }
    }
}
esp_err_t DHT11_C::parseItems(rmt_symbol_word_t *item, int itemNum, DHT11_C *pObj)
{
    rmt_symbol_word_t *pItem = item;
    uint8_t i = 0;
    uint16_t temp = 0;
    uint16_t humidity = 0;
    uint8_t checkSum = 0;
    if (itemNum < 42) // 响应+40bit数据+结束=42
    {
        return ESP_FAIL;
    }
#if (DHT11_LOG == 1)
    ESP_LOGI(dht11Tag, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
             pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    pItem++;                          // 跳过应答信号
    for (i = 0; i < 16; i++, pItem++) // 湿度数据
    {
        humidity <<= 1; // 正常情况第一阶段应该是低电平，但是如果第一阶段是高电平的话，可能dht响应的80us低电平没被rmt接收
        if (pItem->level0 == 1)
        {
            // 只看高电平持续时间
            if (pItem->duration0 > dht110CodeDurationMargin)
            {
                humidity |= 0x01;
            }
        }
        else
        {
            if (pItem->duration1 > dht110CodeDurationMargin)
            {
                humidity |= 0x01;
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(dht11Tag, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
                 pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    }
    for (i = 0; i < 16; i++, pItem++)
    {
        temp <<= 1;
        if (pItem->level0 == 1)
        {
            if (pItem->duration0 > dht110CodeDurationMargin)
            {
                temp |= 0x01;
            }
        }
        else
        {
            if (pItem->duration1 > dht110CodeDurationMargin)
            {
                temp |= 0x01;
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(dht11Tag, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
                 pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    }
    for (i = 0; i < 8; i++, pItem++)
    {
        checkSum <<= 1;
        if (pItem->level0 == 1)
        {
            if (pItem->duration0 > dht110CodeDurationMargin)
            {
                checkSum |= 0x01;
            }
        }
        else
        {
            if (pItem->duration1 > dht110CodeDurationMargin)
            {
                checkSum |= 0x01;
            }
        }
#if (DHT11_LOG == 1)
        ESP_LOGI(dht11Tag, "level0 = %d, level1 = %d, duration0 = %d, duration1 = %d",
                 pItem->level0, pItem->level1, pItem->duration0, pItem->duration1);
#endif
    }
    if (((humidity & 0x00FF) + ((humidity & 0xFF00) >> 8) +
         (temp & 0x00FF) + ((temp & 0xFF00) >> 8)) != checkSum)
    {
#if (DHT11_LOG == 1)
        ESP_LOGI(dht11Tag, "check fail");
#endif
        // 校验失败，丢掉此帧
        return ESP_FAIL;
    }
    pObj->data.humidity = ((humidity & 0xFF00) >> 8);
    temp = (((temp & 0xFF00) >> 8) * 10) + (temp & 0x00FF);
    pObj->data.temp = (float)temp / 10;
    return ESP_OK;
}
