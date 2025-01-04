// author : abnbnb
// date   : 2024-12-19
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "hal/gpio_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "w25q64.h"
#include "gb2312.h"

#define W25Q64_TAG "W25Q64"
#define W25Q64_DEBUG
#define W25Q64_WRITE_ENABLE 0x06     // 芯片写使能
#define W25Q64_WRITE_DISABLE 0x04    // 芯片写失能
#define W25Q64_JEDEC_ID 0x9F         // 读取芯片JEDECID
#define W25Q64_READ_DATA 0x03        // 读数据
#define W25Q64_PAGE_PROGRAM 0x02     // 页编程，每页为256字节
#define W25Q64_SECTOR_ERASE 0x20     // 擦除扇区
#define W25Q64_BLOCK_ERASE_64KB 0xD8 // 擦除块
#define W25Q64_CHIP_ERASE 0xC7       // 擦除整个芯片
#define W25Q64_READ_STATUS_REG1 0x05 // 读取状态寄存器1
#define W25Q64_BUSY_FLAG 0x01        // 忙标志
#define W25Q64_DUMMY_DATA 0xFF
#define W25Q64_CLK_SPEED SPI_MASTER_FREQ_8M // SPI速度
#define W25Q64_ADDRESS_BITS (3 * 8)         // 地址长度
#define W25Q64_COMMAND_BITS (1 * 8)         // 命令长度
#define W25Q64_DUMMY_BITS (0 * 8)           // 补偿输入延迟位，8MSPI速度下不需要考虑
#define W25Q64_PAGE_SIZE (256)              // 页大小
#define W25Q64_SPI_HOST SPI2_HOST           // 指定SPI外设
#define W25Q64_SPI_MISO_PIN GPIO_NUM_6      // 引脚配置
#define W25Q64_SPI_MOSI_PIN GPIO_NUM_4
#define W25Q64_SPI_CLK_PIN GPIO_NUM_5
#define W25Q64_SPI_CS_PIN GPIO_NUM_7

W25Q64_C w25q64;

esp_err_t W25Q64_C::init(void)
{
    esp_err_t ret = ESP_OK;
    spi_host_device_t host_id = W25Q64_SPI_HOST;
    spi_dma_chan_t dma_chan = SPI_DMA_CH_AUTO;
    spi_bus_config_t bus_config;
    spi_device_interface_config_t dev_config;
    memset(&bus_config, 0x00, sizeof(bus_config));
    memset(&dev_config, 0x00, sizeof(spi_device_interface_config_t));

    bus_config.mosi_io_num = W25Q64_SPI_MOSI_PIN;
    bus_config.miso_io_num = W25Q64_SPI_MISO_PIN;
    bus_config.sclk_io_num = W25Q64_SPI_CLK_PIN;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.data4_io_num = -1;
    bus_config.data5_io_num = -1;
    bus_config.data6_io_num = -1;
    bus_config.data7_io_num = -1;
    bus_config.max_transfer_sz = 1024;
    bus_config.flags = SPICOMMON_BUSFLAG_MASTER;
    bus_config.isr_cpu_id = INTR_CPU_ID_AUTO;
    bus_config.intr_flags = 0;

    dev_config.command_bits = W25Q64_COMMAND_BITS;
    dev_config.address_bits = W25Q64_ADDRESS_BITS;
    dev_config.dummy_bits = W25Q64_DUMMY_BITS;
    dev_config.mode = 0;
    dev_config.clock_source = SPI_CLK_SRC_DEFAULT;
    dev_config.duty_cycle_pos = 128;
    dev_config.cs_ena_pretrans = 0;
    dev_config.cs_ena_posttrans = 0;
    dev_config.clock_speed_hz = W25Q64_CLK_SPEED;
    dev_config.input_delay_ns = 0;
    dev_config.spics_io_num = W25Q64_SPI_CS_PIN;
    dev_config.flags = 0;
    dev_config.queue_size = 8;
    dev_config.pre_cb = NULL;  // 发送数据前中断回调
    dev_config.post_cb = NULL; // 发送数据后中断回调

    ret = spi_bus_initialize(host_id, &bus_config, dma_chan);

    ret = spi_bus_add_device(host_id, &dev_config, &this->spiDevHandle);

    ret = this->getJedecId();

#if (W25Q64_CHINESE_FONT_DATA_WRITE_DONE == 0)

#define GB2312_FONT_BUF_START_WRITE_ADDR 0

    int bufLen = sizeof(gb2312FontBuf);
    int pageNum = bufLen / W25Q64_PAGE_SIZE;
    int addr = GB2312_FONT_BUF_START_WRITE_ADDR;
    int pageCnt = 0;
    uint8_t *pBuf = (uint8_t *)&gb2312FontBuf[0];
    uint8_t noEnoughOnePgae = bufLen % W25Q64_PAGE_SIZE;

    ESP_LOGI(W25Q64_TAG, "bufLen = %d pageNum = %d", bufLen, pageNum);

    while (pageNum-- != 0)
    {
        ret = this->pageWrite(pBuf, W25Q64_PAGE_SIZE, addr);
        addr += W25Q64_PAGE_SIZE;
        pBuf += W25Q64_PAGE_SIZE;
        ESP_LOGI(W25Q64_TAG, "page %d write success", pageCnt);
        pageCnt++;
    }

    if (noEnoughOnePgae != 0)
    {
        ret = this->pageWrite(pBuf, noEnoughOnePgae, addr);
    }
#else
    uint8_t testBuf[GB2312_SINGLE_FONT_BUF_LEN];
    memset(testBuf, 0xAA, sizeof(testBuf));

    this->readData(testBuf, GB2312_SINGLE_FONT_BUF_LEN, 0);

    for(int i = 0; i < GB2312_SINGLE_FONT_BUF_LEN; i++)
    {
        if(testBuf[i] != 0x00)
        {
            ret = ESP_FAIL;
        }
    }
#endif

    if (ret == ESP_OK)
    {
        ESP_LOGI(W25Q64_TAG, "init successed");
    }
    else
    {
        ESP_LOGI(W25Q64_TAG, "init failed");
    }

    return ret;
}

esp_err_t W25Q64_C::getJedecId(void)
{
    // 因为发送命令时并不需要发送地址，所以需要将地址长度改为0，所以改为使用扩展结构体
    spi_transaction_ext_t t;
    esp_err_t ret = ESP_OK;
    uint8_t dummyData[3] = {W25Q64_DUMMY_DATA, W25Q64_DUMMY_DATA, W25Q64_DUMMY_DATA};
    memset(&t, 0x00, sizeof(spi_transaction_ext_t));

    t.command_bits = 8;
    t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_DUMMY;
    t.base.cmd = W25Q64_JEDEC_ID;
    t.base.length = 3 * 8;
    t.base.tx_buffer = dummyData;
    t.base.rx_buffer = NULL;

    ret = spi_device_transmit(this->spiDevHandle, &t.base);

    ESP_LOGI(W25Q64_TAG, "jedec id = 0x%02x%02x%02x", t.base.rx_data[0], t.base.rx_data[1], t.base.rx_data[2]);

    return ret;
}

esp_err_t W25Q64_C::writeEnable(void)
{
    spi_transaction_ext_t t;
    esp_err_t ret = ESP_OK;
    memset(&t, 0x00, sizeof(spi_transaction_ext_t));

    t.command_bits = 8;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_DUMMY;
    t.base.cmd = W25Q64_WRITE_ENABLE;
    t.base.length = 0;

    ret = spi_device_transmit(this->spiDevHandle, &t.base);

    return ret;
}

esp_err_t W25Q64_C::waitBusy(void)
{
    spi_transaction_ext_t t;
    esp_err_t ret = ESP_OK;
    uint8_t dummyData = W25Q64_DUMMY_DATA;
    uint8_t timeOutCnt = 100;
    memset(&t, 0x00, sizeof(spi_transaction_ext_t));

    t.command_bits = 8;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_DUMMY | SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    t.base.cmd = W25Q64_READ_STATUS_REG1;
    t.base.length = 1 * 8;
    t.base.tx_data[0] = dummyData;

    while (timeOutCnt-- != 0)
    {
        ret = spi_device_transmit(this->spiDevHandle, &t.base);

        if ((t.base.rx_data[0] & W25Q64_BUSY_FLAG) == 0)
        {
            break;
        }
    }

    if (timeOutCnt == 0)
    {
        ESP_LOGE(W25Q64_TAG, "W25Q64_C::waitBusy time out");
    }

    return ret;
}

esp_err_t W25Q64_C::sectorErase(uint32_t addr)
{
    spi_transaction_t t;
    esp_err_t ret = ESP_OK;
    memset(&t, 0x00, sizeof(spi_transaction_t));

    ret = this->writeEnable();
    ret = this->waitBusy();

    t.cmd = W25Q64_SECTOR_ERASE;
    t.addr = addr;
    t.length = 0 * 8; // 无需发送任何额外数据，只需发送命令和地址即可
    t.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_DUMMY;

    ret = spi_device_transmit(this->spiDevHandle, &t);

    ret = this->waitBusy();

    return ret;
}

esp_err_t W25Q64_C::chipErase(void)
{
    spi_transaction_ext_t t;
    esp_err_t ret = ESP_OK;
    memset(&t, 0x00, sizeof(spi_transaction_ext_t));

    ret = this->writeEnable();
    ret = this->waitBusy();

    t.command_bits = 8;
    t.base.cmd = W25Q64_CHIP_ERASE;
    t.base.length = 0 * 8;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_DUMMY;

    ret = spi_device_transmit(this->spiDevHandle, &t.base);

    ret = this->waitBusy();

    return ret;
}

esp_err_t W25Q64_C::pageWrite(uint8_t *pBuf, uint16_t len, uint32_t addr)
{
    esp_err_t ret = ESP_OK;
    spi_transaction_t t;
    memset(&t, 0x00, sizeof(spi_transaction_t));

    ret = this->writeEnable();
    ret = this->waitBusy();

    t.cmd = W25Q64_PAGE_PROGRAM;
    t.addr = addr;
    t.length = 8 * len;
    t.tx_buffer = pBuf;
    t.rx_buffer = NULL;

    if (((addr % W25Q64_PAGE_SIZE) != 0) || (len > W25Q64_PAGE_SIZE))
    {
        ESP_LOGE(W25Q64_TAG, "no page alignment or len > W25Q64_PAGE_SIZE");
        return ESP_FAIL;
    }

    ret = spi_device_transmit(this->spiDevHandle, &t);

    ret = this->waitBusy();

    return ret;
}

esp_err_t W25Q64_C::arbitrailyWrite(uint8_t *pBuf, uint32_t len, uint32_t addr)
{
    uint8_t numOfPageToWrite = len / W25Q64_PAGE_SIZE;       // 需要写多少页
    uint8_t noEnoughOnePgae = len % W25Q64_PAGE_SIZE;        // 未满一页的数据个数
    uint8_t isAlignment = addr % W25Q64_PAGE_SIZE;           // 是否进行页对齐
    uint8_t cntToAlignment = W25Q64_PAGE_SIZE - isAlignment; // 差多少个数据可以进行页对齐
    esp_err_t ret = ESP_OK;

    // 页对齐
    if (isAlignment == 0)
    {
        // 是否满一页
        if (numOfPageToWrite == 0)
        {
            ret = this->pageWrite(pBuf, len, addr);
        }
        else
        {
            while (numOfPageToWrite-- != 0)
            {
                ret = this->pageWrite(pBuf, W25Q64_PAGE_SIZE, addr);
                addr += W25Q64_PAGE_SIZE;
                pBuf += W25Q64_PAGE_SIZE;
            }
            // 剩下不满一页的继续写入
            if (noEnoughOnePgae != 0)
            {
                ret = this->pageWrite(pBuf, noEnoughOnePgae, addr);
            }
        }
    }
    // 未进行页对齐
    else
    {
        if (numOfPageToWrite == 0)
        {
            // 如果当前页剩下的地方不够写
            if (cntToAlignment < noEnoughOnePgae)
            {
                // 先把当前页剩下的地方写完
                ret = this->pageWrite(pBuf, cntToAlignment, addr);

                // 到下一页把剩下的写完
                addr += cntToAlignment;
                pBuf += cntToAlignment;
                ret = this->pageWrite(pBuf, (noEnoughOnePgae - cntToAlignment), addr);
            }
            else
            {
                // 如果剩下的空间足够，那直接写入
                ret = this->pageWrite(pBuf, len, addr);
            }
        }
        else
        {
            // 先写入能够使得页对齐的数据长度
            ret = this->pageWrite(pBuf, cntToAlignment, addr);
            len -= cntToAlignment;
            // 对齐之后重新计算需要写多少页，以及剩下多少个不满一页的数据
            numOfPageToWrite = len / W25Q64_PAGE_SIZE;
            noEnoughOnePgae = len % W25Q64_PAGE_SIZE;
            addr += cntToAlignment;
            pBuf += cntToAlignment;

            while (numOfPageToWrite-- != 0)
            {
                ret = this->pageWrite(pBuf, W25Q64_PAGE_SIZE, addr);
                addr += W25Q64_PAGE_SIZE;
                pBuf += W25Q64_PAGE_SIZE;
            }

            // 剩下不满一页的继续写入
            if (noEnoughOnePgae != 0)
            {
                ret = this->pageWrite(pBuf, noEnoughOnePgae, addr);
            }
        }
    }

    return ret;
}

esp_err_t W25Q64_C::readData(uint8_t *pBuf, uint16_t len, uint32_t addr)
{
    esp_err_t ret = ESP_OK;
    spi_transaction_t t;
    memset(&t, 0x00, sizeof(spi_transaction_t));

    ret = this->writeEnable();

    t.cmd = W25Q64_READ_DATA;
    t.addr = addr;
    t.length = 8 * len;
    t.tx_buffer = NULL;
    t.rx_buffer = pBuf;

    ret = spi_device_transmit(this->spiDevHandle, &t);

    return ret;
}
