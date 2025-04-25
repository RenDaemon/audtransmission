#include "arduinoFFT.h"

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

const uint16_t samples = 64; //This value MUST ALWAYS be a power of 2
const double signalFrequency = 1000;
const double samplingFrequency = 5000;
const uint8_t amplitude = 100;

double vReal[samples];
double vImag[samples];

// Initialize FFT with vReal, vImag, and samples
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, samples, samplingFrequency);

void setup()
{
    Serial.begin(115200);
    Serial.println("Ready");
}

void loop()
{
    /* Build raw data */
    double cycles = (((samples-1) * signalFrequency) / samplingFrequency); //Number of signal cycles that the sampling will read
    for (uint16_t i = 0; i < samples; i++)
    {
        vReal[i] = analogRead(0);
        vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
    }

    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Updated method name
    FFT.compute(FFT_FORWARD);                        // Updated method name
    FFT.complexToMagnitude();                        // Updated method name
    double x = FFT.majorPeak();                      // Updated method name
    Serial.print("Freq:");
    Serial.println(x, 6);
    delay(1000);
}