#ifndef __LED_DISPLAY_H
#define __LED_DISPLAY_H

#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

#define LED_MATRIX_DEBUG 
#define LED_MATRIX_HEIGHT 64
#define LED_MATRIX_WIDTH 64
#define LED_MATRIX_CHAIN_LEN 1

typedef enum {
    SONT_TI = 0,
    FIXEDSYS,
}fontTypeEnum_t;

typedef struct showZoneStr {
    uint8_t xStart;
    uint8_t xEnd;
    uint8_t yStart;
    uint8_t yEnd;
} showZoneStr_t;

class ledDisplay_C
{
public:
    ledDisplay_C() {
        this->brightness = 10;
        this->showZone.xStart = 0;
        this->showZone.xEnd = LED_MATRIX_WIDTH;
        this->showZone.yStart = 0;
        this->showZone.yEnd = LED_MATRIX_HEIGHT;
        fontType = FIXEDSYS;
    }

    ~ledDisplay_C() {}

    void init(void);
    void show(void);
    void clear(void); 
    void setFont(fontTypeEnum_t fontType);
    void setBrightness(uint8_t brightness);
    void drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
    void drawPixel(int16_t x, int16_t y, uint32_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint32_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t r, uint8_t g, uint8_t b);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint32_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t r, uint8_t g, uint8_t b);
    void drawChar(int16_t x, int16_t y, char chr, uint32_t color);
    void drawChar(int16_t x, int16_t y, char chr, uint8_t r, uint8_t g, uint8_t b);
    void drawString(int16_t x, int16_t y, char *str, uint32_t color);
    void drawString(int16_t x, int16_t y, char *str, uint8_t r, uint8_t g, uint8_t b);
    void drawChineseChr(int16_t x, int16_t y, char *str, uint32_t color);
    void drawChineseChr(int16_t x, int16_t y, char *str, uint8_t r, uint8_t g, uint8_t b);
    void drawPicture(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint32_t *pBuf);
    void setShowZone(uint8_t xStart, uint8_t yStart, uint8_t xEnd, uint8_t yEnd);
    void drawNum(int16_t x, int16_t y, int num, uint32_t color);
    fontTypeEnum_t getFontTYpe(void)
    {
        return this->fontType;
    }
    MatrixPanel_I2S_DMA *displayPtr = nullptr;
private:
    uint8_t brightness;
    // 此二维数组的每一个bit对应每个像素，1为对应像素有颜色数据，0为对应像素无颜色数据
    uint8_t bitMap[64][8];
    
    showZoneStr_t showZone;
    fontTypeEnum_t fontType;
};

extern ledDisplay_C ledDisplay;

#endif