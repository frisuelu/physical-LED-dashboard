/*
    This code enables Fast Fourier Transforms of the microphone's audio
    input to display a wave for the audio at different peak Hz values.

    Adapted from:
    https://github.com/kosme/arduinoFFT/blob/master/Examples/FFT_03/FFT_03.ino

    Follow this guide to plot correctly:
    https://www.instructables.com/Portable-Sound-Analyzer-on-ESP32/
    https://circuitdigest.com/microcontroller-projects/diy-music-audio-visualizer-using-dot-matrix-display-and-arduino-nano

    Connect MAX7219 to D1 Mini:
    https://i0.wp.com/www.esp8266learning.com/wp-content/uploads/2016/07/wemos-and-max7219-_bb.png?resize=768%2C341

    Use this library to control the MAX7219 module:
    https://github.com/MajicDesigns/MD_MAX72XX/tree/main/examples

    Schematics:

        MAX7219 VCC -> WEMOS 3.3V
        MAX7219 GND -> WEMOS G
        MAX7219 DIN -> WEMOS D7
        MAX7219 CS  -> WEMOS D5
        MAX7219 CLK -> WEMOS D4

        MAX9814 GND  -> WEMOS G
        MAX9814 Vdd  -> WEMOS 3.3V
        MAX9814 Gain -> WEMOS 5V
        MAX9814 Out  -> WEMOS A0

*/

#include <arduinoFFT.h>
#include <MD_MAX72xx.h>

#define CHANNEL A0               // ADC channel for the D1 Mini
#define SAMPLES 256              // Must be a power of 2
#define SAMPLING_FREQUENCY 10000 // Hz, must be less than 10000 due to ADC

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

#define CLK_PIN D4    // or SCK
#define DATA_PIN D7   // or MOSI
#define CS_PIN D5     // or SS
#define MAX_DEVICES 1 // Number of LED displays in paralell
#define COLUMNS 8     // Number of LED columns
#define HEIGHT 8      // Height of LED display

// Fast Fourier Transform object implementation
arduinoFFT FFT = arduinoFFT();

// MAX7219 LED display (8x8)
MD_MAX72XX display = MD_MAX72XX(
    MD_MAX72XX::GENERIC_HW,
    DATA_PIN,
    CLK_PIN,
    CS_PIN,
    MAX_DEVICES);

unsigned int sampling_period_micros;
unsigned long microseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

int translatedReal[SAMPLES];

// Lower value for the range constraints for the signal plotting
int constrain_low = 0;
// Higher value for the range constraints for the signal plotting
int constrain_high = 512;

// Patterns for the column display (8x8)
int spectralHeight[] = {
    0b00000000,
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111110,
    0b11111111,
};

int value;

void setup()
{
    Serial.begin(115200);
    sampling_period_micros = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

    display.begin();
    display.clear();
}

void loop()
{
    /* SAMPLING */
    microseconds = micros();
    for (int i = 0; i < SAMPLES; i++)
    {
        // Analog read, we need to substract offset of 1.25V!
        vReal[i] = analogRead(CHANNEL) - int(1.25 * 1024 / 3.3);
        vImag[i] = 0;
        while (micros() - microseconds < sampling_period_micros)
        {
            // Empty loop
        }

        microseconds += sampling_period_micros;
    }

    /* FFT */

    // Serial.println("Data:");
    // PrintVector(vReal, SAMPLES, SCL_TIME);

    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */

    // Serial.println("Weighed data:");
    // PrintVector(vReal, SAMPLES, SCL_TIME);

    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD); /* Compute FFT */

    // Serial.println("Computed Real values:");
    // PrintVector(vReal, SAMPLES, SCL_INDEX);

    // Serial.println("Computed Imaginary values:");
    // PrintVector(vImag, SAMPLES, SCL_INDEX);

    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES); /* Compute magnitudes */

    // Serial.println("Computed magnitudes:");
    // PrintVector(vReal, (SAMPLES >> 1), SCL_FREQUENCY);

    // double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

    // Serial.println(peak, 6); // Print out what frequency is the most dominant.

    /*
        TODO:

        Check the bins and aggregate them more, so low band sounds are more grouped
        and we can see higher pitched sounds more on the higher bands
    */
    for (int i = 0; i <= COLUMNS; i++)
    {
        // Constrain the values to the range [0, 1023] for greater resolution
        translatedReal[i] = constrain(
            vReal[i],
            constrain_low,
            constrain_high);

        // Map the values into the range [0, 8] for the display
        translatedReal[i] = map(
            translatedReal[i],
            constrain_low,
            constrain_high,
            0,
            HEIGHT);

        // Get the display value
        value = spectralHeight[translatedReal[i]];

        // Display the value in the LED
        display.setRow(i, value);

        // Print raw values to serial to save in txt file
        Serial.print(i);
        Serial.print("\t");
        Serial.println(value);
    }

    // This means 20 "refreshes" per second
    // delay(50);
}

void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
    for (uint16_t i = 0; i < bufferSize; i++)
    {
        double abscissa;
        /* Print abscissa value */
        switch (scaleType)
        {
        case SCL_INDEX:
            abscissa = (i * 1.0);
            break;
        case SCL_TIME:
            abscissa = ((i * 1.0) / SAMPLING_FREQUENCY);
            break;
        case SCL_FREQUENCY:
            abscissa = ((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES);
            break;
        }
        Serial.print(abscissa, 6);
        if (scaleType == SCL_FREQUENCY)
            Serial.print("Hz");
        Serial.print(" ");
        Serial.println(vData[i], 4);
    }
    Serial.println();
}