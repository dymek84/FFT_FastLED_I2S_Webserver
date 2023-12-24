#include "i2sfft.h"
// Colors and palettes
DEFINE_GRADIENT_PALETTE(purple_gp){
    0, 0, 212, 255,    // blue
    255, 179, 0, 255}; // purple
DEFINE_GRADIENT_PALETTE(outrun_gp){
    0, 141, 0, 100,   // purple
    127, 255, 192, 0, // yellow
    255, 0, 5, 255};  // blue
DEFINE_GRADIENT_PALETTE(greenblue_gp){
    0, 0, 255, 60,    // green
    64, 0, 236, 255,  // cyan
    128, 0, 5, 255,   // blue
    192, 0, 236, 255, // cyan
    255, 0, 255, 60}; // green
DEFINE_GRADIENT_PALETTE(redyellow_gp){
    0, 200, 200, 200,    // white
    64, 255, 218, 0,     // yellow
    128, 231, 0, 0,      // red
    192, 255, 218, 0,    // yellow
    255, 200, 200, 200}; // white
CRGBPalette16 purplePal = purple_gp;
CRGBPalette16 outrunPal = outrun_gp;
CRGBPalette16 greenbluePal = greenblue_gp;
CRGBPalette16 heatPal = redyellow_gp;
const int CENTRE_X = CANVAS_WIDTH / 2;
const int CENTRE_Y = CANVAS_HEIGHT / 2;
uint16_t mXY(uint8_t x, uint8_t y)
{
    uint16_t idx;
    if (x & 0x01)
    {
        idx = (x * CANVAS_HEIGHT) + y;
    }
    else
    {
        uint8_t reverseX = (CANVAS_HEIGHT - 1 - y);
        idx = (x * CANVAS_HEIGHT) + reverseX;
    }
    return idx;
}
void drawPixel(int16_t x, int16_t y, CRGB color)
{
    if ((x < 0) || (y < 0) || (x >= CANVAS_WIDTH) || (y >= CANVAS_HEIGHT))
        return;

    int16_t t;
    switch (rotation)
    {
    case 1:
        t = x;
        x = CANVAS_WIDTH - 1 - y;
        y = t;
        break;
    case 2:
        x = CANVAS_WIDTH - 1 - x;
        y = CANVAS_HEIGHT - 1 - y;
        break;
    case 3:
        t = x;
        x = y;
        y = CANVAS_HEIGHT - 1 - t;
        break;
    }
    int idx = 0;

    if (x & 0x01)
    {
        idx = (x * CANVAS_HEIGHT) + y;
    }
    else
    {
        uint8_t reverseX = (CANVAS_HEIGHT - 1 - y);
        idx = (x * CANVAS_HEIGHT) + reverseX;
    }
    matrix[idx] = color;
}
int barWidth = 2;
void rainbowBars()
{
    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int band = 0; band < BANDS_NUMBER; band++)
    {
        int xStart = barWidth * band;
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            for (int y = 0; y <= barHeights[band]; y++)
            {
                drawPixel(x, y, CHSV((x / barWidth) * (255 / BANDS_NUMBER), 255, 255));
            }
        }
    }
}

void purpleBars()
{
    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int band = 0; band < BANDS_NUMBER; band++)
    {
        int xStart = barWidth * band;
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            for (int y = 0; y < barHeights[band]; y++)
            {
                drawPixel(x, y, ColorFromPalette(purplePal, y * (255 / barHeights[band])));
            }
        }
    }
}

void changingBars()
{
    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int band = 0; band < BANDS_NUMBER; band++)
    {
        int xStart = barWidth * band;
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            for (int y = 0; y < barHeights[band]; y++)
            {
                drawPixel(x, y, CHSV(y * (255 / CANVAS_HEIGHT) + tick, 255, 255));
            }
        }
    }
}

void centerBars()
{
    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int band = 0; band < BANDS_NUMBER; band++)
    {
        int xStart = barWidth * band;
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            if (barHeights[band] % 2 == 0)
                barHeights[band]--;
            int yStart = ((CANVAS_HEIGHT - barHeights[band]) / 2);
            for (int y = yStart; y <= (yStart + barHeights[band]); y++)
            {
                int colorIndex = constrain((y - yStart) * (255 / barHeights[band]), 0, 255);
                drawPixel(x, y, ColorFromPalette(heatPal, colorIndex));
            }
        }
    }
}

void whitePeak()
{
    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int band = 0; band < BANDS_NUMBER; band++)
    {
        int xStart = barWidth * band;
        int peakHeight = peak[band];
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            drawPixel(x, peakHeight, CRGB::White);
        }
    }
}

void outrunPeak()
{
    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int band = 0; band < BANDS_NUMBER; band++)
    {
        int xStart = barWidth * band;
        int peakHeight = peak[band];
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            drawPixel(x, peakHeight, ColorFromPalette(outrunPal, peakHeight * (255 / CANVAS_HEIGHT)));
        }
    }
}

void moveWaterfall()
{
    // Move screen up starting at 2nd row from top
    for (int y = CANVAS_HEIGHT - 2; y >= 0; y--)
    {
        for (int x = 0; x < CANVAS_WIDTH; x++)
        {
            matrix[mXY(x, y + 1)] = matrix[mXY(x, y)];
        }
    }
}

void createWaterfall()
{
    EVERY_N_MILLISECONDS(30) { moveWaterfall(); }
    for (int band = 0; band < BANDS_NUMBER; band++)
    {

        int xStart = barWidth * band;
        // Draw bottom line
        for (int x = xStart; x < xStart + barWidth; x++)
        {
            drawPixel(x, 0, CHSV(constrain(map(fftResult[band], 0, 254, 160, 0), 0, 160), 255, 255));
        }
    }
}
void displayBands()
{
    int bandWidth = CANVAS_WIDTH / BANDS_NUMBER;
    uint8_t *bands = barHeights;
    uint8_t *peaks = peak;
    int peakBandIndex = _peakMaxIndex;
    int peakBandValue = peaks[peakBandIndex];

    fill_solid(matrix, MATRIX_SIZE, CRGB(0, 0, 0));
    for (int i = 0; i < 32; i++)
    {

        for (int y = 0; y < 8; y++)
        {
            if (y < barHeights[i / 2])
            {
                drawPixel(i, y, CHSV(y * (255 / CANVAS_HEIGHT) + tick, 255, 255));
            }
            else
            {
                drawPixel(i, y, CRGB(0, 0, 0));
            }

            if (y == peak[i / 2])
            {
                drawPixel(i, y, CRGB(122, 122, 122));
            }
        }
    }
}
int Hy = CANVAS_HEIGHT;
int dx = CANVAS_WIDTH / BANDS_NUMBER;
int dy = CANVAS_HEIGHT / Hy;

void analyzerLinesByIntensity()
{
    fadeToBlackBy(matrix, MATRIX_SIZE, 110);

    for (int i = 0; i < BANDS_NUMBER; i++)
    {
        int level = barHeights[i];
        int nextLevel;
        if (i < 15)
            nextLevel = peak[i + 1];
        else if (i < 13 && i > 5)
            nextLevel = peak[i + 1] * i / 2;
        else
            nextLevel = peak[i];
        if (nextLevel >= 8)
            nextLevel = 7;
        if (level >= 8)
            level = 7;
        CRGB color1 = CHSV(peak[i] + barHeights[i] + tickslow, 255, 255);
        CRGB color2 = CHSV(peak[i] + tickslow + barHeights[i], 255, 128 + barHeights[i] * 4);

        uint8_t y = Hy - 1 - (level * dy);
        uint8_t nextY = Hy - 1 - (nextLevel * dy);

        // y = y >= Hy ? Hy - 1 : y;
        // nextY = nextY >= Hy ? Hy - 1 : nextY;
        drawLine(i * dx, 7 - nextY, i * dx + dx, 7 - y, color2);
        drawLine(i * dx, 7 - y, i * dx + dx, 7 - nextY, color1);
    }
}

void drawBarsInternal()
{
    CRGB temp[256];

      fill_solid(temp, 256, CRGB::Black);
    nblend(matrix, temp, 256, 125);

 
        for (int i = 0; i < BANDS_NUMBER; i++)
        {

            // drawPixel(i, y, CHSV((i * BANDS_NUMBER), 255, 255));

            fillRect(i * 2, 0, 2, barHeights[i], CHSV((i * 255 / BANDS_NUMBER), 255, 255));
            // fillRect( i * 2 - 1, 7, 1, 7- peak[i], CHSV((i * 2 + tickslow), 255, 255));
              drawLine(i * dx,  peak[i], i * dx + dx,  barHeights[i], CRGB::White);
        }
    
}

FunctionInfo functionArray[] = {
    {drawBarsInternal, "drawBarsInternal"},
    {analyzerLinesByIntensity, "analyzerLinesByIntensity"},
    
    {displayBands, "displayBands"},
    {createWaterfall, "createWaterfall"},
    {whitePeak, "whitePeak"},
    {outrunPeak, "outrunPeak"},
    {rainbowBars, "rainbowBars"},
    {purpleBars, "purpleBars"},
    {changingBars, "changingBars"},
    {centerBars, "centerBars"},

};
int patternStripeIndex = 0;

void chagepattern()
{

    patternStripeIndex++;
    if (patternStripeIndex >= sizeof(functionArray) / sizeof(functionArray[0]))
    {
        patternStripeIndex = 0;
    }
    Serial.print("Pattern changed to: ");
    Serial.println(functionArray[patternStripeIndex].functionName);
}
void loopPatternMatrix()
{
    loopis2fft();
    // fftResult is now ready to be used by other routines.
    delay(10);
    EVERY_N_SECONDS(5)
    {
        patternStripeIndex = 0;
        // chagepattern();
    }
    functionArray[patternStripeIndex].functionPtr(); // Call the function using the function pointer

    // displayBands();
}

void setupPatternMatrix()
{
    setupi2sfft();
}