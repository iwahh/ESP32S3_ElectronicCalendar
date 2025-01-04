#ifndef __DHT11_H
#define __DHT11_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_rx.h"
#include "driver/rmt_tx.h"
#include "soc/rmt_reg.h"
#include "esp_log.h"
#include "esp_system.h"
#include "rom/ets_sys.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"

#define DHT11_LOG 0 // 置1打开log
#define DHT11_TAG "dht11"
#define DHT11_GPIO_NUM GPIO_NUM_1
#define DHT11_0_CODE_DURATION_MARGIN 40       // dht11的0码高电平持续时间为26~28us，这里认为当高电平持续时间小于40则为bit0
#define DHT11_RESOLUTION_HZ (1 * 1000 * 1000) // 1MHz分辨率，1 tick = 1us
#define DHT11_STANDARD_FRAME_LENGTH 40        // dht11标准数据帧长度为40bit
#define DHT11_SAMP_FRE 2                      // s
#define DHT11_TASK_STACK (1024 * 3)           // 任务堆栈大小
#define DHT11_TASK_PRIORITY 1                 // 任务优先级
#define DHT11_OBJ dht11                       // 创建的对象名称

    typedef struct dht11Str dht11Str_t;

    struct dht11Str
    {
        gpio_num_t gpio;                    // 使用到的引脚
        rmt_rx_channel_config_t rxChanCfg;  // 接收通道配置
        rmt_channel_handle_t rxChanHandle;  // 接收通道句柄
        QueueHandle_t rxQueue;              // 接收数据队列
        rmt_rx_event_callbacks_t rxDoneCbs; // 接收完成中断回调
        rmt_receive_config_t rxCfg;         // 接收配置
        float temp;                         // 温度数据
        uint8_t humidity;                   // 湿度数据

        esp_err_t (*init)(dht11Str_t *obj);                            // 初始化
        esp_err_t (*startSamp)(const dht11Str_t *obj);                 // 触发采样
        esp_err_t (*parseItems)(rmt_symbol_word_t *item, int itemNum); // 数据转换
        float (*getTemp)(const dht11Str_t *obj);                       // 获取温度
        uint8_t (*getHumidity)(const dht11Str_t *obj);                 // 获取湿度
        void (*task)(void *param);                                     // 触发采样及数据转换任务
    };

    extern dht11Str_t DHT11_OBJ;

#ifdef __cplusplus
}
#endif

#endif
