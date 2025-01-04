// author : abnbnb
// date   : 2024-12-18
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <hal/gpio_types.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "gb2312.h"
#include "ledDisplay.h"
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include "myFont.h"
#include "w25q64.h"
#include "utf8_gb2312_switch.h"

/*
      25 <- R1    G1   -> 26
      27 <- B1    GND
      14 <- R2    G2   -> 12
      13 <- B2    E    -> 32
      23 <-  A    B    -> 22
      05 <-  C    D    -> 17
      16 <-CLK    LAT  -> 04
      15 <- OE    GND
*/
#define HUB75_R1 GPIO_NUM_41
#define HUB75_G1 GPIO_NUM_40
#define HUB75_B1 GPIO_NUM_39
#define HUB75_R2 GPIO_NUM_38
#define HUB75_G2 GPIO_NUM_37
#define HUB75_B2 GPIO_NUM_36
#define HUB75_A GPIO_NUM_0
#define HUB75_B GPIO_NUM_45
#define HUB75_C GPIO_NUM_48
#define HUB75_D GPIO_NUM_47
#define HUB75_E GPIO_NUM_35
#define HUB75_CLK GPIO_NUM_21
#define HUB75_OE GPIO_NUM_13
#define HUB75_LAT GPIO_NUM_14
#define LED_DISPLAY_TAG "ledDisplay"
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a = b;              \
        b = t;              \
    }
#endif
ledDisplay_C ledDisplay;

#ifdef LED_MATRIX_DEBUG

void ledDisplay_debug(void *param)
{
    while (true)
    {
        for (int8_t i = -10; i < LED_MATRIX_HEIGHT + 10; i++)
        {
            ledDisplay.drawFastHLine(0, i, 64, 0xFFFFFF);
            ledDisplay.drawFastHLine(0, i, 64, 0xFF, 0xFF, 0xFF);
            ledDisplay.drawFastVLine(i, 0, 64, 0xFFFFFF);
            ledDisplay.drawFastVLine(i, 0, 64, 0xFF, 0xFF, 0xFF);
            ledDisplay.drawLine(i, i + 10, i, i + 20, 0xFFFFFF);
            ledDisplay.setFont(SONT_TI);
            ledDisplay.drawChar(i, 0, 'A', 0x123456);
            ledDisplay.drawChar(0, i, 'a', 0x654321);
            ledDisplay.drawChar(i, i, '1', 0xFF, 0xFF, 0xFF);
            ledDisplay.setFont(FIXEDSYS);
            ledDisplay.drawString((LED_MATRIX_HEIGHT + 10) - i, (LED_MATRIX_HEIGHT + 10) - i, (char *)"Hello", 0xAB45F1);
            ledDisplay.drawString(i, i, (char *)"1234567890", 0xBC4F8D);
            ledDisplay.show();
            vTaskDelay(pdMS_TO_TICKS(10));
            ledDisplay.clear();
        }

        for (int i = LED_MATRIX_HEIGHT + 10; i >= -10; i--)
        {
            ledDisplay.drawFastHLine(0, i, 64, 0xFFFFFF);
            ledDisplay.drawFastHLine(0, i, 64, 0xFF, 0xFF, 0xFF);
            ledDisplay.drawFastVLine(i, 0, 64, 0xFFFFFF);
            ledDisplay.drawFastVLine(i, 0, 64, 0xFF, 0xFF, 0xFF);
            ledDisplay.drawLine(i, i + 10, i - 10, i, 0xFFFFFF);
            ledDisplay.setFont(SONT_TI);
            ledDisplay.drawChar(i, 0, 'A', 0x123456);
            ledDisplay.drawChar(0, i, 'a', 0x654321);
            ledDisplay.drawChar(i, i, '1', 0xFF, 0xFF, 0xFF);
            ledDisplay.setFont(FIXEDSYS);
            ledDisplay.drawString((LED_MATRIX_HEIGHT + 10) - i, (LED_MATRIX_HEIGHT + 10) - i, (char *)"Hello", 0xAB45F1);
            ledDisplay.drawString(i, i, (char *)"1234567890", 0xBC4F8D);
            ledDisplay.show();
            vTaskDelay(pdMS_TO_TICKS(10));
            ledDisplay.clear();
        }

        // ledDisplay.drawChineseChr(0, 0, "ゆ", 0xFFFFFF);
        ledDisplay.setFont(SONT_TI);
        // ledDisplay.drawString(0, 0, "1我【[]A", 0xFFFFFF);
        ledDisplay.show();
        vTaskDelay(pdMS_TO_TICKS(10));
        ledDisplay.clear();
        vTaskDelete(NULL);
    }
}
#endif

/**************************************************************************/
/*!
   @brief     初始化LED矩阵
    @param    void
*/
/**************************************************************************/
void ledDisplay_C::init(void)
{
    HUB75_I2S_CFG::i2s_pins _pins = {
        HUB75_R1,
        HUB75_G1,
        HUB75_B1,
        HUB75_R2,
        HUB75_G2,
        HUB75_B2,
        HUB75_A,
        HUB75_B,
        HUB75_C,
        HUB75_D,
        HUB75_E,
        HUB75_LAT,
        HUB75_OE,
        HUB75_CLK,
    };
    HUB75_I2S_CFG mxconfig(LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT, LED_MATRIX_CHAIN_LEN,
                           _pins);
    mxconfig.double_buff = true;
    mxconfig.clkphase = false;
    this->displayPtr = new MatrixPanel_I2S_DMA(mxconfig);
    this->displayPtr->setBrightness(this->brightness);
    this->displayPtr->begin();
    this->displayPtr->setBrightness(this->brightness);
    ESP_LOGI(LED_DISPLAY_TAG, "init success !");
#ifdef LED_MATRIX_DEBUG
    BaseType_t ret = xTaskCreate(ledDisplay_debug, "ledDisplay_debug", 1024 * 4, NULL, 1, NULL);
    if (ret == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
    {
        ESP_LOGE(LED_DISPLAY_TAG, "errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
    }
#endif
}
/**************************************************************************/
/*!
   @brief     发送显存数据至HUB75
    @param    void
*/
/**************************************************************************/
void ledDisplay_C::show(void)
{
    this->displayPtr->flipDMABuffer();
}
/**************************************************************************/
/*!
   @brief     清除显存数据并清除bitMap
    @param    void
*/
/**************************************************************************/
void ledDisplay_C::clear(void)
{
    memset(this->bitMap, 0x00, sizeof(this->bitMap));
    this->displayPtr->clearScreen();
}
/**************************************************************************/
/*!
   @brief     设置字体
    @param    void
*/
/**************************************************************************/
void ledDisplay_C::setFont(fontTypeEnum_t fontType)
{
    this->fontType = fontType;
}
/**************************************************************************/
/*!
   @brief     发送显存数据至HUB75
    @param    void
*/
/**************************************************************************/
void ledDisplay_C::setBrightness(uint8_t brightness)
{
    this->brightness = brightness;
    this->displayPtr->setBrightness(brightness);
}
/**************************************************************************/
/*!
   @brief     画点至显存并标记bitMap对应bit
    @param    x  x坐标
    @param    y  y坐标
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if ((x < 0) || (x >= LED_MATRIX_WIDTH) || (y < 0) || (y >= LED_MATRIX_HEIGHT))
    {
        return;
    }
    this->bitMap[x][y / 8] |= 1 << (y % 8);
    this->displayPtr->drawPixelRGB888(x, y, r, g, b);
}
/**************************************************************************/
/*!
   @brief     画点至显存并标记bitMap对应bit
    @param    x  x坐标
    @param    y  y坐标
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawPixel(int16_t x, int16_t y, uint32_t color)
{
    if ((x < 0) || (x >= LED_MATRIX_WIDTH) || (y < 0) || (y >= LED_MATRIX_HEIGHT))
    {
        return;
    }
    this->bitMap[x][y / 8] |= 1 << (y % 8);
    this->displayPtr->drawPixelRGB888(x, y, ((color & 0xFF0000) >> 16), ((color & 0x00FF00) >> 8), ((color & 0x0000FF) >> 0));
}
/**************************************************************************/
/*!
   @brief     绘制两点线段至显存
    @param    x0  起x坐标
    @param    y0  起y坐标
    @param    x1  止x坐标
    @param    y1  止y坐标
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)
    {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1)
    {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            this->drawPixel(y0, x0, color);
        }
        else
        {
            this->drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}
/**************************************************************************/
/*!
   @brief     绘制两点线段至显存
    @param    x0  起x坐标
    @param    y0  起y坐标
    @param    x1  止x坐标
    @param    y1  止y坐标
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)
    {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1)
    {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            this->drawPixel(y0, x0, r, g, b);
        }
        else
        {
            this->drawPixel(x0, y0, r, g, b);
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}
/**************************************************************************/
/*!
   @brief     绘制垂直线段至显存
    @param    x   起x坐标
    @param    y   起y坐标
    @param    h   高度
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawFastVLine(int16_t x, int16_t y, int16_t h, uint32_t color)
{
    this->drawLine(x, y, x, y + h - 1, color);
}
/**************************************************************************/
/*!
   @brief     绘制垂直线段至显存
    @param    x   起x坐标
    @param    y   起y坐标
    @param    h   高度
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t r, uint8_t g, uint8_t b)
{
    this->drawLine(x, y, x, y + h - 1, r, g, b);
}
/**************************************************************************/
/*!
   @brief    绘制水平线至显存
    @param    x   起x坐标
    @param    y   起y坐标
    @param    w   长度
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawFastHLine(int16_t x, int16_t y, int16_t w, uint32_t color)
{
    this->drawLine(x, y, x + w - 1, y, color);
}
/**************************************************************************/
/*!
   @brief    绘制水平线至显存
    @param    x   起x坐标
    @param    y   起y坐标
    @param    w   长度
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t r, uint8_t g, uint8_t b)
{
    this->drawLine(x, y, x + w - 1, y, r, g, b);
}
/**************************************************************************/
/*!
   @brief    绘制字符至显存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    chr 绘制的字符
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawChar(int16_t x, int16_t y, char chr, uint32_t color)
{
#if 1
    fontInfoStr_t info;
    // 获取字体数据
    getFontBuf(this->fontType, chr, &info);
    // 计算有效的横纵坐标
    int16_t xStart = fmax(x, this->showZone.xStart);
    int16_t xEnd = fmin(x + info.width, this->showZone.xEnd);
    int16_t yStart = fmax(y, this->showZone.yStart);
    int16_t yEnd = fmin(y + info.height, this->showZone.yEnd);
    // 计算每列之间偏移的字节数
    uint8_t skipBytesPerCol = (info.height % 8 != 0) ? (info.height / 8 + 1) : (info.height / 8);
    // 当前绘制列需要跳过的字节数
    uint8_t skipBytesCurCol = (y > this->showZone.yStart) ? 0 : ((this->showZone.yStart - y) / 8);
    // 当前绘制列需要偏移的bit数
    uint8_t skipBitsCurCol = (y > this->showZone.yStart) ? 0 : ((this->showZone.yStart - y) % 8);
    // 计算跳过的列数
    uint8_t skipColNum = (x > this->showZone.xStart) ? 0 : (this->showZone.xStart - x);
    uint8_t pBit = skipBitsCurCol;
    uint8_t *pBuf = info.pBuf;
    // 需配合字符取模方式理解
    for (int16_t i = xStart; i < xEnd; i++)
    {
        // 定位到当前位置的字符数据
        pBuf = info.pBuf + (skipColNum++) * skipBytesPerCol + skipBytesCurCol;
        pBit = skipBitsCurCol;
        for (int16_t j = yStart; j < yEnd; j++)
        {
            if ((*pBuf) & (1 << pBit))
            {
                this->drawPixel(i, j, color);
            }
            pBit++;
            if (pBit > 7)
            {
                pBit = 0;
                pBuf++;
            }
        }
    }
#else
    fontInfoStr_t info;
    uint8_t pBit = 0;
    int16_t i = 0, j = 0;

    getFontBuf(this->fontType, chr, &info);

    for (i = x; i < (x + info.width); i++)
    {
        if ((i < this->showZone.xStart) || (i > this->showZone.xEnd))
        {
            info.pBuf += info.height / 8;
            if ((info.height % 8) != 0)
            {
                // 不满8bit的自动补零换列，所以多加一个字节偏移
                info.pBuf++;
            }
            continue;
        }
        for (j = y; j < (y + info.height); j++)
        {
            if ((j < this->showZone.yStart) || (j > this->showZone.yEnd))
            {
                pBit++;
                if (pBit > 7)
                {
                    pBit = 0;
                    info.pBuf++;
                }
                continue;
            }
            if ((*info.pBuf) & ((0x01) << pBit))
            {
                this->drawPixel(i, j, color);
            }
            pBit++;
            if (pBit > 7)
            {
                pBit = 0;
                info.pBuf++;
            }
        }
        // 如果绘制完这一列，并且字高不是8的整数倍，则需跳到下一字节
        if ((info.height % 8) != 0)
        {
            info.pBuf++;
        }
        pBit = 0;
    }
#endif
}
/**************************************************************************/
/*!
   @brief    绘制字符至显存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    chr 绘制的字符
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawChar(int16_t x, int16_t y, char chr, uint8_t r, uint8_t g, uint8_t b)
{
#if 1
    fontInfoStr_t info;
    // 获取字体数据
    getFontBuf(this->fontType, chr, &info);
    // 计算有效的横纵坐标
    int16_t xStart = fmax(x, this->showZone.xStart);
    int16_t xEnd = fmin(x + info.width, this->showZone.xEnd);
    int16_t yStart = fmax(y, this->showZone.yStart);
    int16_t yEnd = fmin(y + info.height, this->showZone.yEnd);
    // 计算每列之间偏移的字节数
    uint8_t skipBytesPerCol = (info.height % 8 != 0) ? (info.height / 8 + 1) : (info.height / 8);
    // 当前绘制列需要跳过的字节数
    uint8_t skipBytesCurCol = (y > this->showZone.yStart) ? 0 : ((this->showZone.yStart - y) / 8);
    // 当前绘制列需要偏移的bit数
    uint8_t skipBitsCurCol = (y > this->showZone.yStart) ? 0 : ((this->showZone.yStart - y) % 8);
    // 计算跳过的列数
    uint8_t skipColNum = (x > this->showZone.xStart) ? 0 : (this->showZone.xStart - x);
    uint8_t pBit = skipBitsCurCol;
    uint8_t *pBuf = info.pBuf;
    // 需配合字符取模方式理解
    for (int16_t i = xStart; i < xEnd; i++)
    {
        // 定位到当前位置的字符数据
        pBuf = info.pBuf + (skipColNum++) * skipBytesPerCol + skipBytesCurCol;
        pBit = skipBitsCurCol;
        for (int16_t j = yStart; j < yEnd; j++)
        {
            if ((*pBuf) & (1 << pBit))
            {
                this->drawPixel(i, j, r, g, b);
            }
            pBit++;
            if (pBit > 7)
            {
                pBit = 0;
                pBuf++;
            }
        }
    }
#else
    fontInfoStr_t info;
    uint8_t pBit = 0;
    int16_t i = 0, j = 0;

    getFontBuf(this->fontType, chr, &info);

    for (i = x; i < (x + info.width); i++)
    {
        if ((i < this->showZone.xStart) || (i > this->showZone.xEnd))
        {
            info.pBuf += info.height / 8;
            if ((info.height % 8) != 0)
            {
                // 不满8bit的自动补零换列，所以多加一个字节偏移
                info.pBuf++;
            }
            continue;
        }
        for (j = y; j < (y + info.height); j++)
        {
            if ((j < this->showZone.yStart) || (j > this->showZone.yEnd))
            {
                pBit++;
                if (pBit > 7)
                {
                    pBit = 0;
                    info.pBuf++;
                }
                continue;
            }
            if ((*info.pBuf) & ((0x01) << pBit))
            {
                this->drawPixel(i, j, r, g, b);
            }
            pBit++;
            if (pBit > 7)
            {
                pBit = 0;
                info.pBuf++;
            }
        }
        // 如果绘制完这一列，并且字高不是8的整数倍，则需跳到下一字节
        if ((info.height % 8) != 0)
        {
            info.pBuf++;
        }
        pBit = 0;
    }
#endif
}
/**************************************************************************/
/*!
   @brief    绘制字符串至显存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    chr 绘制的字符
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawString(int16_t x, int16_t y, char *str, uint32_t color)
{
    size_t len = strlen(str);
    size_t i = 0;
    int16_t xCur = x;
    fontInfoStr_t info;
    getFontBuf(this->fontType, *str, &info);
    while (i < len)
    {
        // 判断是否为ASCII字符(范围0~127)
        if ((uint8_t)str[i] <= 127)
        {
            this->drawChar(xCur, y, str[i], color);
            i++;
            xCur += info.width;
        }
        // 判断是否为多字节UTF - 8字符（这里简单处理，实际更复杂）
        else if ((uint8_t)str[i] >= 0xC0)
        {
            // 对于UTF - 8中中文一般是3字节编码
            if (((uint8_t)str[i] >= 0xE0) && ((uint8_t)str[i] <= 0xEF) && (i + 2 < len))
            {
                this->drawChineseChr(xCur, y, &str[i], color);
                i += 3;
                xCur += GB2312_FONT_WIDTH;
            }
            else
            {
                i++;
            }
        }
    }
}
/**************************************************************************/
/*!
   @brief    绘制字符串至显存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    chr 绘制的字符
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawString(int16_t x, int16_t y, char *str, uint8_t r, uint8_t g, uint8_t b)
{
    size_t len = strlen(str);
    size_t i = 0;
    int16_t xCur = x;
    fontInfoStr_t info;
    getFontBuf(this->fontType, *str, &info);
    while (i < len)
    {
        // 判断是否为ASCII字符(范围0~127)
        if ((uint8_t)str[i] <= 127)
        {
            this->drawChar(xCur, y, str[i], r, g, b);
            i++;
            xCur += info.width;
        }
        // 判断是否为多字节UTF - 8字符（这里简单处理，实际更复杂）
        else if ((uint8_t)str[i] >= 0xC0)
        {
            // 对于UTF - 8中中文一般是3字节编码
            if (((uint8_t)str[i] >= 0xE0) && ((uint8_t)str[i] <= 0xEF) && (i + 2 < len))
            {
                this->drawChineseChr(xCur, y, &str[i], r, g, b);
                i += 3;
                xCur += GB2312_FONT_WIDTH;
            }
            else
            {
                i++;
            }
        }
    }
}
/**************************************************************************/
/*!
   @brief    绘制图片至缓存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    w  宽度
    @param    h  高度
    @param    pBuf  数据
*/
/**************************************************************************/
void ledDisplay_C::drawPicture(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint32_t *pBuf)
{
    for (int16_t i = x; i < x + w; i++)
    {
        for (int16_t j = y; j < y + h; j++)
        {
            this->drawPixel(i, j, *(pBuf++));
        }
    }
}
/**************************************************************************/
/*!
   @brief    绘制中文字符串至显存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    chr 绘制的字符
    @param    color 24-bit 8-8-8 rgb颜色数据
*/
/**************************************************************************/
void ledDisplay_C::drawChineseChr(int16_t x, int16_t y, char *str, uint32_t color)
{
    uint8_t chineseFontBuf[GB2312_SINGLE_FONT_BUF_LEN];
    uint8_t *pBuf = NULL;
    uint8_t gb2312[2];
    uint32_t offset;
    // utf8用三个字节表示汉字，gb2312用两个字节表示汉字
    if (str == NULL)
    {
        return;
    }
    utf8_to_gb2312((uint8_t *)str, 3, gb2312, 2);
    // 详情见GB2312编码范围
    if ((0xB0 <= gb2312[0]) && (gb2312[0] <= 0xF7) && (0xA1 <= gb2312[1]) && (gb2312[1] <= 0xFE))
    {
        offset = ((gb2312[0] - 0xB0) * 94 + (gb2312[1] - 0xA1) + 846) * GB2312_SINGLE_FONT_BUF_LEN;
    }
    else if ((0xA1 <= gb2312[0]) && (gb2312[0] <= 0xA9) && (0xA1 <= gb2312[1]) && (gb2312[1] <= 0xFE))
    {
        offset = ((gb2312[0] - 0xA1) * 94 + (gb2312[1] - 0xA1)) * GB2312_SINGLE_FONT_BUF_LEN;
    }
    else
    {
        return;
    }
    // 根据偏移获取字模数据
    if (w25q64.readData(chineseFontBuf, GB2312_SINGLE_FONT_BUF_LEN, offset) != ESP_OK)
    {
        return;
    }

    pBuf = &chineseFontBuf[0];

    for (int i = 0; i < GB2312_FONT_HEIGHT; i++)
    {
        // 左半部分
        for (int j = 0; j < 8; j++)
        {
            if (((*pBuf) & (0x80 >> j)) != 0)
            {
                this->drawPixel(x + j, y + i, color);
            }
        }
        pBuf++;
        // 右半部分
        for (int j = 0; j < 8; j++)
        {
            if (((*pBuf) & (0x80 >> j)) != 0)
            {
                this->drawPixel(x + j + 8, y + i, color);
            }
        }
        pBuf++;
    }
}
/**************************************************************************/
/*!
   @brief    绘制中文字符串至显存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    chr 绘制的字符
    @param    r  红色数据
    @param    g  绿色数据
    @param    b  蓝色数据
*/
/**************************************************************************/
void ledDisplay_C::drawChineseChr(int16_t x, int16_t y, char *str, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t chineseFontBuf[GB2312_SINGLE_FONT_BUF_LEN];
    uint8_t *pBuf = NULL;
    uint8_t gb2312[2];
    uint32_t offset;
    // utf8用三个字节表示汉字，gb2312用两个字节表示汉字
    if (str == NULL)
    {
        return;
    }
    utf8_to_gb2312((uint8_t *)str, 3, gb2312, 2);
    // 详情见GB2312编码范围
    if ((0xB0 <= gb2312[0]) && (gb2312[0] <= 0xF7) && (0xA1 <= gb2312[1]) && (gb2312[1] <= 0xFE))
    {
        offset = ((gb2312[0] - 0xB0) * 94 + (gb2312[1] - 0xA1) + 846) * GB2312_SINGLE_FONT_BUF_LEN;
    }
    else if ((0xA1 <= gb2312[0]) && (gb2312[0] <= 0xA9) && (0xA1 <= gb2312[1]) && (gb2312[1] <= 0xFE))
    {
        offset = ((gb2312[0] - 0xA1) * 94 + (gb2312[1] - 0xA1)) * GB2312_SINGLE_FONT_BUF_LEN;
    }
    else
    {
        return;
    }
    // 根据偏移获取字模数据
    if (w25q64.readData(chineseFontBuf, GB2312_SINGLE_FONT_BUF_LEN, offset) != ESP_OK)
    {
        return;
    }

    pBuf = &chineseFontBuf[0];

    for (int i = 0; i < GB2312_FONT_HEIGHT; i++)
    {
        // 左半部分
        for (int j = 0; j < 8; j++)
        {
            if (((*pBuf) & (0x80 >> j)) != 0)
            {
                this->drawPixel(x + j, y + i, r, g, b);
            }
        }
        pBuf++;
        // 右半部分
        for (int j = 0; j < 8; j++)
        {
            if (((*pBuf) & (0x80 >> j)) != 0)
            {
                this->drawPixel(x + j + 8, y + i, r, g, b);
            }
        }
        pBuf++;
    }
}
/**************************************************************************/
/*!
   @brief    设置字符的显示区域
   // 注：左上角为起始位置
    @param    xStart   起x坐标
    @param    yStart   起y坐标
    @param    xEnd     止x坐标
    @param    yEnd     止y坐标
*/
/**************************************************************************/
void ledDisplay_C::setShowZone(uint8_t xStart, uint8_t yStart, uint8_t xEnd, uint8_t yEnd)
{
    this->showZone.xStart = xStart;
    this->showZone.yStart = yStart;
    this->showZone.xEnd = xEnd;
    this->showZone.yEnd = yEnd;
}
/**************************************************************************/
/*!
   @brief    绘制数字至缓存
   // 注：左上角为起始位置
    @param    x   起x坐标
    @param    y   起y坐标
    @param    num     要绘制的数字
    @param    color   颜色
*/
/**************************************************************************/
void ledDisplay_C::drawNum(int16_t x, int16_t y, int num, uint32_t color)
{
    char str[100];
    snprintf(str, sizeof(str), "%d", num);
    this->drawString(x, y, str, color);
}