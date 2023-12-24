#pragma once
#include <Arduino.h>
#include <string>
#include <FastLED.h>
uint8_t tickslow;  
#define SWAPint(X, Y) \
    {                 \
        int temp = X; \
        X = Y;        \
        Y = temp;     \
    }
#define CANVAS_WIDTH 32
#define CANVAS_HEIGHT 8
#define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))
// FunctionInfo struct
struct FunctionInfo
{
    void (*functionPtr)();
    String functionName;
};

// uint16_t MyColor[26] = {0xF000, 0xF800, 0xFA00, 0xFC00, 0xFE00, 0xFFE0, 0xC7E0, 0x87E0, 0x47E0, 0x07E0, 0x07E8,
//                         0x07F0, 0x07F8, 0x07FF, 0x061F, 0x041F, 0x021F, 0x001F, 0x401F, 0x801F, 0xC01F,
//                         0xF81F, 0xF818, 0xF810, 0xF808, 0xF800};
CRGB MyColor[26] = {CRGB(0xF000), CRGB(0xF800), CRGB(0xFA00), CRGB(0xFC00), CRGB(0xFE00),
                    CRGB(0xFFE0), CRGB(0xC7E0), CRGB(0x87E0), CRGB(0x47E0), CRGB(0x07E0),
                    CRGB(0x07E8), CRGB(0x07F0), CRGB(0x07F8), CRGB(0x07FF), CRGB(0x061F),
                    CRGB(0x041F), CRGB(0x021F), CRGB(0x001F), CRGB(0x401F), CRGB(0x801F),
                    CRGB(0xC01F), CRGB(0xF81F), CRGB(0xF818), CRGB(0xF810), CRGB(0xF808),
                    CRGB(0xF800)};
void drawPixel(int16_t x, int16_t y, CRGB color);
void fillRect(int x, int y, int w, int h, CRGB color)
{
    for (int i = x; i < x + w; i++)
    {
        for (int j = y; j < y + h; j++)
        {
            drawPixel(i, j, color);
        }
    }
}

void drawLine(int x0, int y0, int x1, int y1, CRGB color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;)
    {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 > dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}
void drawFastHLine(int16_t x, int16_t y, int16_t w, CRGB color)
{
    drawLine(x, y, x + w - 1, y, color);
}
void drawFastVLine(int16_t x, int16_t y, int16_t h, CRGB color)
{
    drawLine(x, y, x, y + h - 1, color);
}
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, CRGB color)
{

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1)
    {
        SWAPint(y0, y1);
        SWAPint(x0, x1);
    }
    if (y1 > y2)
    {
        SWAPint(y2, y1);
        SWAPint(x2, x1);
    }
    if (y0 > y1)
    {
        SWAPint(y0, y1);
        SWAPint(x0, x1);
    }

    if (y0 == y2)
    { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)
            a = x1;
        else if (x1 > b)
            b = x1;
        if (x2 < a)
            a = x2;
        else if (x2 > b)
            b = x2;
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
            dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1; // Include y1 scanline
    else
        last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++)
    {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
            SWAPint(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++)
    {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
            SWAPint(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
}

void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, CRGB color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}
void xLine(int x0, int x1, int y, CRGB color)
{
    if (y < 0 || y >= CANVAS_HEIGHT)
        return;
    if (x0 > x1)
    {
        int xb = x0;
        x0 = x1;
        x1 = xb;
    }
    if (x0 < 0)
        x0 = 0;
    if (x1 > CANVAS_WIDTH)
        x1 = CANVAS_WIDTH;
    for (int x = x0; x < x1; x++)
        drawPixel(x, y, color);
}
void fillEllipse(int x, int y, int rx, int ry, CRGB color)
{
    if (ry == 0)
        return;
    float f = float(rx) / ry;
    f *= f;
    for (int i = 0; i < ry + 1; i++)
    {
        float s = rx * rx - i * i * f;
        int xr = (int)sqrt(s <= 0 ? 0 : s);
        xLine(x - xr, x + xr + 1, y + i, color);
        if (i)
            xLine(x - xr, x + xr + 1, y - i, color);
    }
}

void ellipse(int x, int y, int rx, int ry, CRGB color)
{
    if (ry == 0)
        return;
    int oxr = rx;
    float f = float(rx) / ry;
    f *= f;
    for (int i = 0; i < ry + 1; i++)
    {
        float s = rx * rx - i * i * f;
        int xr = (int)sqrt(s <= 0 ? 0 : s);
        xLine(x - oxr, x - xr + 1, y + i, color);
        xLine(x + xr, x + oxr + 1, y + i, color);
        if (i)
        {
            xLine(x - oxr, x - xr + 1, y - i, color);
            xLine(x + xr, x + oxr + 1, y - i, color);
        }
        oxr = xr;
    }
}