#ifndef __W25Q64_H
#define __W25Q64_H

#include "driver/spi_master.h"

// 是否已将中文字库数据写入W25Q64芯片
#define W25Q64_CHINESE_FONT_DATA_WRITE_DONE 1

class W25Q64_C
{
public:
    W25Q64_C(void) {};
    ~W25Q64_C(void) {};
    esp_err_t init(void);
    esp_err_t getJedecId(void);
    esp_err_t writeEnable(void);
    esp_err_t waitBusy(void);
    esp_err_t sectorErase(uint32_t addr);
    esp_err_t chipErase(void);
    esp_err_t pageWrite(uint8_t *pBuf, uint16_t len, uint32_t addr);
    esp_err_t arbitrailyWrite(uint8_t *pBuf, uint32_t len, uint32_t addr);
    esp_err_t readData(uint8_t *pBuf, uint16_t len, uint32_t addr);
private:
    spi_device_handle_t spiDevHandle;
};

extern W25Q64_C w25q64;

#endif
