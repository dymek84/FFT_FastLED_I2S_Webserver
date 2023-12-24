#pragma once
#define FASTLED_INTERNAL
#include "utils.h"
#include <driver/i2s.h>
#include <arduinoFFT.h>

// bck_pin = 26, ws_pin = 25, data_pin = 22, channel_pin = 21, channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT
class AudioInI2S
{
public:
    AudioInI2S(int bck_pin, int ws_pin, int data_pin, int channel_pin = -1, i2s_channel_fmt_t channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT);
    void read(int32_t _samples[]);
    void begin(int sample_size, int sample_rate = 44100, i2s_port_t i2s_port_number = I2S_NUM_0);

private:
    int _bck_pin;
    int _ws_pin;
    int _data_pin;
    int _channel_pin;
    i2s_channel_fmt_t _channel_format;
    int _sample_size;
    int _sample_rate;
    i2s_port_t _i2s_port_number;

    i2s_config_t _i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 0, // set in begin()
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 0, // set in begin()
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};

    i2s_pin_config_t _i2s_mic_pins = {
        .bck_io_num = I2S_PIN_NO_CHANGE, // set in begin()
        .ws_io_num = I2S_PIN_NO_CHANGE,  // set in begin()
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_PIN_NO_CHANGE // set in begin()
    };
};

AudioInI2S::AudioInI2S(int bck_pin, int ws_pin, int data_pin, int channel_pin, i2s_channel_fmt_t channel_format)
{
    _bck_pin = bck_pin;
    _ws_pin = ws_pin;
    _data_pin = data_pin;
    _channel_pin = channel_pin;
    _channel_format = channel_format;
}

void AudioInI2S::begin(int sample_size, int sample_rate, i2s_port_t i2s_port_number)
{
    if (_channel_pin >= 0)
    {
        pinMode(_channel_pin, OUTPUT);
        digitalWrite(_channel_pin, _channel_format == I2S_CHANNEL_FMT_ONLY_RIGHT ? LOW : HIGH);
    }

    _sample_rate = sample_rate;
    _sample_size = sample_size;
    _i2s_port_number = i2s_port_number;

    _i2s_mic_pins.bck_io_num = _bck_pin;
    _i2s_mic_pins.ws_io_num = _ws_pin;
    _i2s_mic_pins.data_in_num = _data_pin;

    _i2s_config.sample_rate = _sample_rate;
    _i2s_config.dma_buf_len = _sample_size;
    _i2s_config.channel_format = _channel_format;

    // start up the I2S peripheral
    i2s_driver_install(_i2s_port_number, &_i2s_config, 0, NULL);
    i2s_set_pin(_i2s_port_number, &_i2s_mic_pins);
}

void AudioInI2S::read(int32_t _samples[])
{
    // read I2S stream data into the samples buffer
    // Serial.println("read");
    size_t bytes_read = 0;
    i2s_read(_i2s_port_number, _samples, sizeof(int32_t) * _sample_size, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);
    // Serial.println("samples_read");
}
AudioInI2S audioIn(5, 17, 16, 21, I2S_CHANNEL_FMT_ONLY_RIGHT);
#define SAMPLE_SIZE 1024
#define SAMPLE_RATE 44100
float real[SAMPLE_SIZE];
float imag[SAMPLE_SIZE];
ArduinoFFT<float> FFT = ArduinoFFT<float>(real, imag, SAMPLE_SIZE, SAMPLE_RATE);

void dofft(int32_t input[], int sample_size)
{
    for (int i = 0; i < sample_size; i++)
    {
        real[i] = input[i];
        imag[i] = 0;
    }
    // FFT = new ArduinoFFT<float>(input, imagine, SAMPLE_SIZE, SAMPLE_RATE);
    FFT.dcRemoval();
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward, false); /* Weigh data (compensated) */
    FFT.compute(FFTDirection::Forward);                              /* Compute FFT */
    FFT.complexToMagnitude();                                        /* Compute magnitudes */
    for (int i = 0; i < sample_size / 2; i++)
    {
        input[i] = real[i];
        input[i] = input[i] >> 17;
    }
}

#define BANDS_NUMBER 16
#define MATRIX_PIN 14

#define MATRIX_SIZE (CANVAS_WIDTH * CANVAS_HEIGHT)
CRGB matrix[MATRIX_SIZE];
int rotation = 0;

#define STRIPE_SIZE 55
#define STRIPE_PIN 12
CRGB stripe[STRIPE_SIZE];
int squelch = 4; // Squelch, cuts out low level sounds
int gain = 19;
float fftBin[1024];
float fftCalc[BANDS_NUMBER];
int fftResult[BANDS_NUMBER];
// Table of linearNoise results to be multiplied by squelch in order to reduce squelch across fftResult bins.
int linearNoise[BANDS_NUMBER] = {44, 38, 26, 25, 20, 12, 9, 6, 4, 4, 3, 2, 2, 2, 2, 2};
uint8_t volumeUnit;
uint8_t volumeUnitPeak;
// Table of multiplication factors so that we can even out the frequency response.
double fftResultPink[BANDS_NUMBER] = {1.20, 1.21, 1.33, 1.48, 1.68, 1.56, 1.55, 1.63, 1.79, 1.62, 1.80, 2.06, 2.47, 3.35, 6.83, 9.55};
uint8_t peak[BANDS_NUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t prevFFTValue[BANDS_NUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t barHeights[BANDS_NUMBER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int8_t _peakMaxIndex;
uint8_t tick = 0;

int bandValues[BANDS_NUMBER];

float fftAdd(int from, int to)
{
    int i = from;
    float result = 0;
    while (i <= to)
    {
        result += fftBin[i++];
    }
    return result;
}
void setupi2sfft()
{
    // put your setup code here, to run once:

    audioIn.begin(1024);
    FastLED.addLeds<WS2812B, MATRIX_PIN, GRB>(matrix, MATRIX_SIZE);
    FastLED.addLeds<WS2812B, STRIPE_PIN, GRB>(stripe, STRIPE_SIZE);
    FastLED.setBrightness(255);
    FastLED.show();

    Serial.println("setup done");
}
void loopis2fft()
{
    // put your main code here, to run repeatedly:
    int32_t samples[1024];

    audioIn.read(samples);

    dofft(samples, 1024);

    for (int i = 0; i < 1024; i++)
    { // Values for bins 0 and 1 are WAY too large. Might as well start at 3.
        double t = 0.0;
        t = abs(samples[i]);
        t = t / 16.0; // Reduce magnitude. Want end result to be linear and ~4096 max.
        fftBin[i] = t;
    }                                      // for()
    fftCalc[0] = (fftAdd(3, 4)) / 2;       // 60 - 100
    fftCalc[1] = (fftAdd(4, 5)) / 2;       // 80 - 120
    fftCalc[2] = (fftAdd(5, 7)) / 3;       // 100 - 160
    fftCalc[3] = (fftAdd(7, 9)) / 3;       // 140 - 200
    fftCalc[4] = (fftAdd(9, 12)) / 4;      // 180 - 260
    fftCalc[5] = (fftAdd(12, 16)) / 5;     // 240 - 340
    fftCalc[6] = (fftAdd(16, 21)) / 6;     // 320 - 440
    fftCalc[7] = (fftAdd(21, 28)) / 7;     // 420 - 600
    fftCalc[8] = (fftAdd(29, 37)) / 8;     // 580 - 760
    fftCalc[9] = (fftAdd(37, 48)) / 10;    // 740 - 980
    fftCalc[10] = (fftAdd(48, 64)) / 15;   // 960 - 1300
    fftCalc[11] = (fftAdd(64, 84)) / 20;   // 1280 - 1700
    fftCalc[12] = (fftAdd(84, 111)) / 26;  // 1680 - 2240
    fftCalc[13] = (fftAdd(111, 147)) / 35; // 2220 - 2960
    fftCalc[14] = (fftAdd(147, 194)) / 40; // 2940 - 3900
    fftCalc[15] = (fftAdd(194, 255)) / 50; // 3880 - 5120

    // Noise supression of fftCalc bins using squelch adjustment for different input types.
    for (int i = 0; i < BANDS_NUMBER; i++)
    {
        fftCalc[i] = fftCalc[i] - (float)squelch * (float)linearNoise[i] / 4.0 <= 0 ? 0 : fftCalc[i];
    }

    // Adjustment for frequency curves.
    for (int i = 0; i < BANDS_NUMBER; i++)
    {
        fftCalc[i] = fftCalc[i] * fftResultPink[i];
    }

    // Manual linear adjustment of gain using gain adjustment for different input types.
    for (int i = 0; i < BANDS_NUMBER; i++)
    {
        fftCalc[i] = fftCalc[i] * gain / 40 + fftCalc[i] / 16.0;
    }

    // Now, let's dump it all into fftResult. Need to do this, otherwise other routines might grab fftResult values prematurely.
    for (int i = 0; i < BANDS_NUMBER; i++)
    {
        fftResult[i] = constrain((int)fftCalc[i], 0, 254);
    }
    uint8_t divisor = 1;
    int volume = 0;

    for (int i = 0; i < BANDS_NUMBER; i += divisor)
    {
        uint8_t fftValue;

        fftValue = fftResult[i];
        volume += fftValue;

        fftValue = ((prevFFTValue[i / divisor] * 3) + fftValue) / 4; // Dirty rolling average between frames to reduce flicker
        barHeights[i / divisor] = fftValue / (255 / 8);              // Scale bar height

        if (barHeights[i / divisor] > peak[i / divisor]) // Move peak up
            peak[i / divisor] = min(8, (int)barHeights[i / divisor]);
        if (peak[i] > _peakMaxIndex)
            _peakMaxIndex = i;

        prevFFTValue[i / divisor] = fftValue; // Save prevFFTValue for averaging later
    }

    volumeUnit = volume / BANDS_NUMBER;
    if (volumeUnit > volumeUnitPeak)
        volumeUnitPeak = volumeUnit;
    // Decay peak
    EVERY_N_MILLISECONDS(160)
    {
        for (uint8_t band = 0; band < BANDS_NUMBER; band++)
            if (peak[band] > 0)
                peak[band] -= 1;
    }
    tick++;
}