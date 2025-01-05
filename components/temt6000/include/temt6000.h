#ifndef __TEMT6000_H
#define __TEMT6000_H

#include <string.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

// 定义一个宏，用于控制是否开启日志输出
// 当 TEMT_LOG 为 1 时，开启日志输出；为 0 时，关闭日志输出
#define TEMT_LOG 0
// 定义一个常量指针，用于作为日志输出的标签
// 第一个 const 表示指针指向的字符串内容不可变，第二个 const 表示指针本身不可变
const char *const temtTag = "temt6000";
// 定义默认的 ADC 单元
const adc_unit_t temtDefaultAdcUnit = ADC_UNIT_1;
// 定义默认的 ADC 通道
const adc_channel_t temtDefaultAdcChannel = ADC_CHANNEL_1;
// 定义默认的 ADC 转换模式
const adc_digi_convert_mode_t temtDefaultAdcConverMode = ADC_CONV_SINGLE_UNIT_1;
// 定义默认的 ADC 输出衰减
const adc_atten_t temtDefaultAdcAttent = ADC_ATTEN_DB_11;
// 定义默认的 ADC 位宽
const uint8_t temtDefaultAdcBitWidth = SOC_ADC_DIGI_MAX_BITWIDTH;
// 定义默认的 ADC 采样频率
const uint16_t temtDefaultAdcSampleFrq = (2560);
// 定义默认的 ADC 采样数据保存格式
const adc_digi_output_format_t temtDefaultAdcOutputFormat = ADC_DIGI_OUTPUT_FORMAT_TYPE2;
// 定义默认的 ADC 单次循环采样数据长度（字节）
const uint16_t temtDefaultAdcReadBytesLen = (256);
// 定义默认的 ADC 单次循环采样次数
// 通过默认的采样数据长度除以每次转换的字节数计算得出
const uint16_t temtDefaultAdcSingleSampleTimes = (temtDefaultAdcReadBytesLen / SOC_ADC_DIGI_DATA_BYTES_PER_CONV);
// 定义默认的任务堆栈大小
const uint16_t temtDefaultTaskStack = (1024 * 3);
// 定义默认的任务优先级
const uint8_t temtDefaultTaskPriority = (1);
// 定义一个结构体，用于存储 ADC 配置信息
struct temtAdcCfgStr
{
    adc_unit_t unit;       // ADC 单元
    adc_channel_t channel; // ADC 通道
};

// 定义一个名为 TEMT6000_C 的类，用于处理与 TEMT6000 相关的功能
class TEMT6000_C
{
public:
    ~TEMT6000_C(void)
    {
        if(this->taskHandle)
        {
            vTaskDelete(this->taskHandle);
        }
    }

    // 无参数构造函数
    TEMT6000_C(void)
    {
        // 设置默认的 ADC 单元和通道
        adcBaseCfg.unit = temtDefaultAdcUnit;
        adcBaseCfg.channel = temtDefaultAdcChannel;
        // 调用初始化函数，初始化其他成员变量
        this->initVal();
    }

    // 带参数的构造函数，用于指定 ADC 单元和通道
    TEMT6000_C(adc_unit_t unit, adc_channel_t channel)
    {
        // 设置传入的 ADC 单元和通道
        adcBaseCfg.unit = unit;
        adcBaseCfg.channel = channel;
        // 调用初始化函数，初始化其他成员变量
        this->initVal();
    }

    // 初始化成员变量的函数
    void initVal(void)
    {
        // 标记 ADC 转换是否完成，初始为 false
        adcConverDone = false;
        // ADC 连续转换句柄，初始为 NULL
        adcContinuousHandle = NULL;
        // 任务句柄，初始为 NULL
        taskHandle = NULL;
        // 光照强度，初始为 10
        lightIntensity = 10;
        // 读取数据的数量，初始为 0
        retNum = 0;
        // 结果总和，初始为 0
        resultSum = 0;
        // 将结果数组初始化为全 0
        memset(result, 0x00, sizeof(result));
    }

    // 初始化函数，用于配置 ADC 并可选择创建任务
    esp_err_t init(bool isCreatTask = true);

    // 低通滤波函数，用于处理 ADC 数据
    int lowPassFilter(int newData, int lastData);

    // 获取光照强度的函数
    uint8_t getLightIntensity(void);

private:
    // ADC 连续转换完成的回调函数
    static bool adcContinuousCbs(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);

    // 任务处理函数，用于处理 ADC 数据
    static void taskHandler(void *param);

    // 处理 ADC 数据的函数
    static void handleAdcData(TEMT6000_C *pObj);

    // 光照强度
    uint8_t lightIntensity;
    // ADC 转换完成标志
    bool adcConverDone;
    // ADC 配置结构体
    temtAdcCfgStr adcBaseCfg;
    // ADC 连续转换句柄
    adc_continuous_handle_t adcContinuousHandle;
    // 任务句柄
    TaskHandle_t taskHandle;
    // 读取数据的数量
    uint32_t retNum;
    // 结果总和
    uint32_t resultSum;
    // 存储 ADC 采样数据的数组
    uint8_t result[temtDefaultAdcReadBytesLen];
};


#endif
