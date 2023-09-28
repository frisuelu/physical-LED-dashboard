/*
    This code is a quick adaptation to work in the physical dashboard directly,
    sending the raw microphone signal to an MQTT server, and also calculating the
    FFT directly and representing it in the MAX7219 LED display.

    The "actual" FFT representation is obtained inside TouchDesigner using the
    raw signal from this and the other two D1 Minis.

    The physical dashboard is 44x16.

    To plot, follow these tutorials:
    https://docs.derivative.ca/Arduino
    https://www.youtube.com/watch?v=V_Q_fDukTI0

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
#include <ESP8266WiFiMulti.h>
#include <EspMQTTClient.h>

#define CHANNEL A0               // ADC channel for the D1 Mini
#define SAMPLES 128              // Must be a power of 2
#define SAMPLING_FREQUENCY 10000 // Hz, must be less than 10000 due to ADC

#define CLK_PIN D4    // or SCK
#define DATA_PIN D7   // or MOSI
#define CS_PIN D5     // or SS
#define MAX_DEVICES 1 // Number of LED displays in paralell
#define COLUMNS 8     // Number of LED columns
#define HEIGHT 8      // Height of LED display

// Avaliable WIFI SSIDs
const char *wifi_ssid[] = {"network1", "network2"};

// WIFI passwords for the SSIDs, in the same order
const char *wifi_pass[] = {"network_pass1", "network_pass2"};

// MQTT broker IP
const char *broker_ip = "broker.emqx.io";

// For the entrance D1 mini, it should be this
const char *client_name = "ESPClient3";

// Unsecured port when no ID is needed
const short broker_port = 1883;

// Topic name, for the entrance one it's "ESPClient/micro3"
const String topic = "ESPClient/micro3";

// WIFI client, handles multiple SSID connections automatically
ESP8266WiFiMulti multi_wifi_client;

// Tiemout for the WIFI connection (milliseconds)
const uint32_t connectTimeoutms = 10000;

/*
    MQTT connection ONLY; WIFI is handled by the library ESP8266WifiMulti
    since it allows multiple SSID settings at once.

    Constructor used is:

    EspMQTTClient(
        const char* mqttServerIp,
        const short mqttServerPort,
        const char* mqttUsername, // Optional, only when using SSL connection
        const char* mqttPassword, // Same
        const char* mqttClientName
    );
*/
EspMQTTClient client(
    broker_ip,   // MQTT Broker server ip
    broker_port, // MQTT broker port
    client_name  // Client name that uniquely identifies your device
);

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

// Patterns for the column display (44x16)
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

// FFT value (for plotting) and raw noise value
int value, noise;

void setup()
{
    Serial.begin(115200);

    // Sampling period in microseconds
    sampling_period_micros = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

    // Start MAX7219 display
    display.begin();
    display.clear();

    // Set internal LED as output for blinking
    pinMode(LED_BUILTIN, OUTPUT);

    // Optional functionalities of EspMQTTClient
    client.enableDebuggingMessages();                                         // Enable debugging messages sent to serial output
    client.enableLastWillMessage("ESPClient/lastwill", "I am going offline"); // You can activate the retain flag by setting the third parameter to true
    client.setMqttReconnectionAttemptDelay(5000);                             // Time to retry MQTT connection, default is 15s
    client.setWifiReconnectionAttemptDelay(5000);                             // Time to retry WIFI connection, default is 60s
    client.setKeepAlive(20);                                                  // Keep alive interval in seconds

    // Set D1 Mini as station
    WiFi.mode(WIFI_STA);

    // Register Wifi networks here
    multi_wifi_client.addAP(wifi_ssid[0], wifi_pass[0]);
    multi_wifi_client.addAP(wifi_ssid[1], wifi_pass[1]);
}

void onConnectionEstablished()
{
    // Needed for the MQTT library to run
    // Serial.println("Connection established");
}

// Blink the LED based on the time input (milliseconds!)
void blink_led(int delay_time)
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delay_time);
    digitalWrite(LED_BUILTIN, LOW);
}

// Print FFT calculation to MAX7219
void fft()
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

    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */

    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD); /* Compute FFT */

    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES); /* Compute magnitudes */

    /* MAX7219 PLOT */

    for (int i = 0; i <= COLUMNS; i++)
    {
        // Constrain the values for greater resolution
        translatedReal[i] = constrain(
            vReal[i],
            constrain_low,
            constrain_high);

        // Map the values into the LED range for the display
        translatedReal[i] = map(
            translatedReal[i],
            constrain_low,
            constrain_high,
            0,
            HEIGHT);

        // Get the display value
        value = spectralHeight[translatedReal[i]];

        // Display the value in the LED
        display.setColumn(COLUMNS - i, value);
    }
}

void loop()
{
    /* MQTT CONNECTION */

    fft();

    // Keep alive the WIFI connection
    if (multi_wifi_client.run(connectTimeoutms) == WL_CONNECTED)
    {
        if (client.isConnected())
        {
            // Blink LED to signify correct sending of data
            // ~16 ms corresponds to 60 readings per second
            blink_led(16);

            // Read the microphone value
            noise = analogRead(CHANNEL); //- int(1.25 * 1024 / 3.3);

            // Publish the value in the MQTT topic
            client.publish(topic, String(noise));

            // Plot the "local" FFT
            fft();
        }

        // Loop the client while it connects and sends data
        client.loop();
    }
}
