#include "temt6000.h"

static esp_err_t init(temt6000Str_t *obj);
static bool adcReadLenDoneCbs(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);
static void task(void *param);

temt6000Str_t temt6000 = {
    .init = init,
};

static esp_err_t init(temt6000Str_t *obj)
{
    esp_err_t ret = ESP_OK;
    adc_continuous_handle_cfg_t adcContinuousHandleCfg;
    adc_continuous_config_t adcContinuousCfg;
    adc_digi_pattern_config_t adcPatternCfg;
    adc_continuous_evt_cbs_t adcCbsCfg;

    memset(&adcContinuousHandleCfg, 0x00, sizeof(adc_continuous_handle_cfg_t));
    memset(&adcContinuousCfg, 0x00, sizeof(adc_continuous_config_t));
    memset(&adcPatternCfg, 0x00, sizeof(adc_digi_pattern_config_t));
    memset(&adcCbsCfg, 0x00, sizeof(adc_continuous_evt_cbs_t));

    adcContinuousHandleCfg.max_store_buf_size = TEMT6000_USE_ADC_READ_LEN;
    adcContinuousHandleCfg.conv_frame_size = TEMT6000_USE_ADC_READ_LEN;

    ret = adc_continuous_new_handle(&adcContinuousHandleCfg, &obj->adcContinuousHandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    adcPatternCfg.atten = TEMT6000_USE_ADC_ATTEN;
    adcPatternCfg.bit_width = TEMT6000_USE_ADC_BIT_WIDTH;
    adcPatternCfg.channel = TEMT6000_USE_ADC_CHANNEL;
    adcPatternCfg.unit = TEMT6000_USE_ADC_UNIT;

    adcContinuousCfg.adc_pattern = &adcPatternCfg;
    adcContinuousCfg.conv_mode = TEMT6000_USE_ADC_CONV_MODE;
    adcContinuousCfg.format = TEMT6000_USE_ADC_OUTPUT_TYPE;
    adcContinuousCfg.pattern_num = 1;
    adcContinuousCfg.sample_freq_hz = TEMT6000_USE_ADC_SAMPLE_FRQ;

    ret = adc_continuous_config(obj->adcContinuousHandle, &adcContinuousCfg);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    adcCbsCfg.on_conv_done = TEMT6000_USE_ADC_CONV_DONE_CBS;
    ret = adc_continuous_register_event_callbacks(obj->adcContinuousHandle, &adcCbsCfg, (void *)obj);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    ret = adc_continuous_start(obj->adcContinuousHandle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    if (xTaskCreate(TEMT6000_TASK_FUN, "temt6000Task", TEMT6000_TASK_STACK, NULL, TEMT6000_TASK_PRIORITY, &obj->taskHandle) != pdPASS)
    {
        ESP_LOGE(TEMT6000_TAG, "errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
        return ESP_FAIL;
    }

    if (ret == ESP_OK)
    {
        ESP_LOGI(TEMT6000_TAG, "init");
    }

    return ret;
}

static bool adcReadLenDoneCbs(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    temt6000Str_t *p = (temt6000Str_t *)user_data;
    // Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(p->taskHandle, &mustYield);
    return (mustYield == pdTRUE);
}

static void task(void *param)
{
#define ALPHA 0.1f // 平滑因子(0~1)
    esp_err_t ret = ESP_OK;
    uint32_t retNum = 0;
    uint8_t result[TEMT6000_USE_ADC_READ_LEN];
    uint32_t resultSum = 0;
    uint16_t i = 0;
    adc_digi_output_data_t *p = NULL;
    TEMT6000_OBJ.lightIntensity = 10;
    while (true)
    {
        // 1 / TEMT6000_USE_ADC_SAMPLE_FRQ * TEMT6000_USE_ADC_READ_TIMES = 25ms
        // 1 / 2560 * 64 = 25ms
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ret = adc_continuous_read(TEMT6000_OBJ.adcContinuousHandle, result, TEMT6000_USE_ADC_READ_LEN, &retNum, 0);
        if (ret == ESP_OK)
        {
            // ESP_LOGI(TEMT6000_TAG, "ret is %x, retNum is %"PRIu32" bytes", ret, retNum);
            for (i = 0; i < TEMT6000_USE_ADC_READ_TIMES; i++)
            {
                p = (adc_digi_output_data_t *)&result[i * SOC_ADC_DIGI_DATA_BYTES_PER_CONV];
                resultSum += p->type2.data;
                // ESP_LOGI(TEMT6000_TAG, "conv result = %"PRIu16"", p->type2.data);
            }
            resultSum /= TEMT6000_USE_ADC_READ_TIMES;
            resultSum = (10 + ((resultSum * 254) / (1 << TEMT6000_USE_ADC_BIT_WIDTH)));
            // 低通滤波
            TEMT6000_OBJ.lightIntensity = ((ALPHA * resultSum) + (1 - ALPHA) * TEMT6000_OBJ.lightIntensity);
            // ESP_LOGI(TEMT6000_TAG, "lightIntensity = %"PRIu8"", TEMT6000_OBJ.lightIntensity);
        }
        // ESP_LOGI(TEMT6000_TAG, "free stack %d bytes", uxTaskGetStackHighWaterMark(NULL));
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void temt6000Test(void)
{
    init(&temt6000);
}