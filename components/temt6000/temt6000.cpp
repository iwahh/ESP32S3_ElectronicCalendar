#include "temt6000.h"

// ADC 连续转换完成的回调函数
bool TEMT6000_C::adcContinuousCbs(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    // 用于标记是否需要进行上下文切换
    BaseType_t mustYield = pdFALSE;
    // 将用户数据转换为 TEMT6000_C 类的指针
    TEMT6000_C *pObj = (TEMT6000_C *)user_data;
    // 如果任务句柄不为空
    if (pObj->taskHandle != NULL)
    {
        // 从 ISR（中断服务程序）中通知任务 ADC 转换已完成
        vTaskNotifyGiveFromISR(pObj->taskHandle, &mustYield);
    }
    // 标记 ADC 转换已完成
    pObj->adcConverDone = true;
    // 返回是否需要进行上下文切换
    return (mustYield == pdTRUE);
}

// 任务处理函数
void TEMT6000_C::taskHandler(void *param)
{
    // 将传入的参数转换为 TEMT6000_C 类的指针
    TEMT6000_C *pObj = (TEMT6000_C *)param;
    // 无限循环，任务持续运行
    while (true)
    {
        // 等待通知，直到接收到 ADC 转换完成的通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // 处理 ADC 数据
        pObj->handleAdcData(pObj);
        // 如果开启了日志输出
#if (TEMT_LOG == 1)
        // 输出光照强度信息
        ESP_LOGI(temtTag, "lightIntensity = %" PRIu8 "", pObj->lightIntensity);
        // 输出当前任务的剩余堆栈大小信息
        ESP_LOGI(temtTag, "free stack %d bytes", uxTaskGetStackHighWaterMark(NULL));
#endif
    }
}

// 获取光照强度的函数
uint8_t TEMT6000_C::getLightIntensity(void)
{
    // 如果任务句柄不为空
    if (this->taskHandle != NULL)
    {
        // 直接返回当前的光照强度
        return (this->lightIntensity);
    }
    // 如果 ADC 转换完成
    if (this->adcConverDone == true)
    {
        // 标记 ADC 转换未完成
        this->adcConverDone = false;
        // 处理 ADC 数据
        this->handleAdcData(this);
    }
    // 返回当前的光照强度
    return (this->lightIntensity);
}

// 低通滤波函数
int TEMT6000_C::lowPassFilter(int newData, int lastData)
{
    // 定义平滑因子
    const float alpha = 0.1f;
    // 计算低通滤波后的结果
    return ((alpha * newData) + (1 - alpha) * lastData);
}

// 处理 ADC 数据的函数
void TEMT6000_C::handleAdcData(TEMT6000_C *pObj)
{
    // 循环变量
    uint16_t i = 0;
    // 指向 ADC 数字输出数据的指针
    adc_digi_output_data_t *pBuf = NULL;
    // 存储 ESP - IDF 函数返回的错误码
    esp_err_t ret = ESP_OK;
    // 从 ADC 连续转换句柄中读取数据
    ret = adc_continuous_read(pObj->adcContinuousHandle, pObj->result, temtDefaultAdcReadBytesLen, &pObj->retNum, 0);
    // 检查读取数据操作是否成功，如果失败不进行中断处理
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    // 如果读取数据成功
    if (ret == ESP_OK)
    {
        // 遍历读取到的每个采样数据
        for (i = 0; i < temtDefaultAdcSingleSampleTimes; i++)
        {
            // 计算数据在数组中的位置，并将指针指向该位置
            pBuf = (adc_digi_output_data_t *)&pObj->result[i * SOC_ADC_DIGI_DATA_BYTES_PER_CONV];
            // 将当前采样数据的数值累加到结果总和中
            pObj->resultSum += pBuf->type2.data;
        }
        // 计算平均结果
        pObj->resultSum /= temtDefaultAdcSingleSampleTimes;
        // 对结果进行进一步处理，计算与光照强度相关的值
        pObj->resultSum = (10 + ((pObj->resultSum * 254) / (1 << temtDefaultAdcBitWidth)));
        // 使用低通滤波函数更新光照强度
        pObj->lightIntensity = pObj->lowPassFilter(pObj->resultSum, pObj->lightIntensity);
    }
}

// 初始化函数实现
esp_err_t TEMT6000_C::init(bool isCreatTask)
{
    // 存储 ESP - IDF 函数返回的错误码
    esp_err_t ret = ESP_OK;
    // ADC 连续转换句柄配置结构体
    adc_continuous_handle_cfg_t adcContinuousHandleCfg;
    // ADC 连续转换配置结构体
    adc_continuous_config_t adcContinuousCfg;
    // ADC 数字模式配置结构体
    adc_digi_pattern_config_t adcDigiPatternCfg;
    // ADC 连续转换事件回调结构体
    adc_continuous_evt_cbs_t adcContinuousEvtCbs;

    // 将 ADC 连续转换句柄配置结构体清零
    memset(&adcContinuousHandleCfg, 0x00, sizeof(adc_continuous_handle_cfg_t));
    // 将 ADC 连续转换配置结构体清零
    memset(&adcContinuousCfg, 0x00, sizeof(adc_continuous_config_t));
    // 将 ADC 数字模式配置结构体清零
    memset(&adcDigiPatternCfg, 0x00, sizeof(adc_digi_pattern_config_t));
    // 将 ADC 连续转换事件回调结构体清零
    memset(&adcContinuousEvtCbs, 0x00, sizeof(adc_continuous_evt_cbs_t));

    // 设置 ADC 连续转换句柄配置的最大存储缓冲区大小
    adcContinuousHandleCfg.max_store_buf_size = temtDefaultAdcReadBytesLen;
    // 设置 ADC 连续转换句柄配置的单次转换采集数据长度
    adcContinuousHandleCfg.conv_frame_size = temtDefaultAdcReadBytesLen;
    // 创建 ADC 连续转换句柄
    ret = adc_continuous_new_handle(&adcContinuousHandleCfg, &this->adcContinuousHandle);
    // 检查创建句柄操作是否成功，如果失败不进行中断处理
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    // 设置 ADC 数字模式配置的衰减
    adcDigiPatternCfg.atten = temtDefaultAdcAttent;
    // 设置 ADC 数字模式配置的位宽
    adcDigiPatternCfg.bit_width = temtDefaultAdcBitWidth;
    // 设置 ADC 数字模式配置的通道
    adcDigiPatternCfg.channel = this->adcBaseCfg.channel;
    // 设置 ADC 数字模式配置的单元
    adcDigiPatternCfg.unit = this->adcBaseCfg.unit;

    // 设置 ADC 连续转换配置的 ADC 模式
    adcContinuousCfg.adc_pattern = &adcDigiPatternCfg;
    // 设置 ADC 连续转换配置的转换模式
    adcContinuousCfg.conv_mode = temtDefaultAdcConverMode;
    // 设置 ADC 连续转换配置的数据输出格式
    adcContinuousCfg.format = temtDefaultAdcOutputFormat;
    // 设置 ADC 连续转换配置的模式数量
    adcContinuousCfg.pattern_num = 1;
    // 设置 ADC 连续转换配置的采样频率
    adcContinuousCfg.sample_freq_hz = temtDefaultAdcSampleFrq;

    // 配置 ADC 连续转换
    ret = adc_continuous_config(this->adcContinuousHandle, &adcContinuousCfg);
    // 检查配置操作是否成功，如果失败不进行中断处理
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    // 设置 ADC 连续转换事件回调的转换完成回调函数
    adcContinuousEvtCbs.on_conv_done = TEMT6000_C::adcContinuousCbs;
    // 注册 ADC 连续转换事件回调
    ret = adc_continuous_register_event_callbacks(this->adcContinuousHandle, &adcContinuousEvtCbs, this);
    // 检查注册操作是否成功，如果失败不进行中断处理
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    // 启动 ADC 连续转换
    ret = adc_continuous_start(this->adcContinuousHandle);
    // 检查启动操作是否成功，如果失败不进行中断处理
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    // 如果需要创建任务
    if (isCreatTask == true)
    {
        // 创建任务，任务处理函数为 taskHandler
        if (xTaskCreate(TEMT6000_C::taskHandler, "temt6000Task", temtDefaultTaskStack, this, temtDefaultTaskPriority, &this->taskHandle) != pdPASS)
        {
            // 如果任务创建失败，输出警告日志
            ESP_LOGW(temtTag, "errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
            // 设置错误码为 ESP_FAIL
            ret = ESP_FAIL;
        }
    }
    // 检查整个初始化过程是否成功，如果失败不进行中断处理
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    // 返回错误码
    return ret;
}
