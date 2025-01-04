#ifndef __TEMT6000_H
#define __TEMT6000_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_hal.h"
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

#define TEMT6000_TAG "temt6000"
#define TEMT6000_USE_ADC_UNIT ADC_UNIT_1                                                           // SAR ADC1
#define TEMT6000_USE_ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1                                          // 只用ADC1进行转换
#define TEMT6000_USE_ADC_ATTEN ADC_ATTEN_DB_11                                                     // 输入电压衰减，扩大量程
#define TEMT6000_USE_ADC_CHANNEL ADC_CHANNEL_1                                                     // 使用ADC通道1，即GPIO2
#define TEMT6000_USE_ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH                                       // 12bitADC
#define TEMT6000_USE_ADC_SAMPLE_FRQ (2560)                                                         // 采样频率Hz
#define TEMT6000_USE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2                                  // ADC数据输出格式
#define TEMT6000_USE_ADC_READ_LEN 256                                                              // ADC数据读取长度，单位字节
#define TEMT6000_USE_ADC_READ_TIMES (TEMT6000_USE_ADC_READ_LEN / SOC_ADC_DIGI_DATA_BYTES_PER_CONV) // 单次ADC采集次数
#define TEMT6000_USE_ADC_CONV_DONE_CBS adcReadLenDoneCbs                                           // 单次ADC采集指定次数完成回调
#define TEMT6000_TASK_FUN task
#define TEMT6000_TASK_STACK (1024 * 4)
#define TEMT6000_TASK_PRIORITY 1
#define TEMT6000_OBJ temt6000

    typedef struct temt6000Str temt6000Str_t;

    struct temt6000Str
    {
        adc_continuous_handle_t adcContinuousHandle;
        TaskHandle_t taskHandle;

        // 已将采集到的电压线性映射到0~255，即光照强度为0~255，前提增益为ADC_ATTEN_DB_11
        uint8_t lightIntensity;

        esp_err_t (*init)(temt6000Str_t *obj);
    };

    extern temt6000Str_t temt6000;

#ifdef __cplusplus
}
#endif

#endif
