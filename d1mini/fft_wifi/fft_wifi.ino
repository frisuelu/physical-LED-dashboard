/*
    This code publishes the raw sensor signals to a MQTT topic using the public broker provided by
    EMQX; for the time being, this unsecured (i.e, no SSL) & public connection should suffice for
    testing.

    Schematics:

        MAX9814 GND  -> WEMOS G
        MAX9814 Vdd  -> WEMOS 3.3V
        MAX9814 Gain -> WEMOS 5V
        MAX9814 Out  -> WEMOS A0

*/

#include <ESP8266WiFiMulti.h>
#include <EspMQTTClient.h>

// ADC channel for the D1 Mini
#define CHANNEL A0

// Avaliable WIFI SSIDs
const char *wifi_ssid[] = {"network1", "network2"};

// WIFI passwords for the SSIDs, in the same order
const char *wifi_pass[] = {"network_pass1", "network_pass2"};

// MQTT broker IP
const char *broker_ip = "broker.emqx.io";

/*
    MQTT broker port; for the EMQX one:
        - Unsecured port when no ID is needed: 1883
        - Secured port: 8883
*/
const short broker_port = 1883;

// ------------------ CHANGE THESE ------------------ //

/*
    The MQTT Client name MUST be changed between the D1 Minis,
    otherwise they WILL overlap and disconnect each other from
    the broker.

    Any name is valid, as it's not used in any way later
*/
const char *client_name = "ESPClient1";

/*
    Topic name, MUST be changed based on which D1 Mini we're uploading to.

    The EXPECTED topics are: "ESPClient/micro1", "ESPClient/micro2", "ESPClient/micro3"
*/
const String topic = "ESPClient/micro1";

// -------------------------------------------------- //

// Microphone sensor value
int value = 0;

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

void setup()
{
    Serial.begin(115200);

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
    // The MQTT library requires this method to be set
}

// Blink the LED based on the time input (milliseconds!)
void blink_led(int delay_time)
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delay_time);
    digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
    // Keep alive the WIFI connection
    if (multi_wifi_client.run(connectTimeoutms) == WL_CONNECTED)
    {
        // When the MQTT Client is connected
        if (client.isConnected())
        {
            // Blink LED to signify correct sending of data
            // ~16 ms corresponds to 60 readings per second
            blink_led(16);

            // Read the microphone value
            value = analogRead(CHANNEL); //- int(1.25 * 1024 / 3.3);

            // Publish the value in the MQTT topic
            client.publish(topic, String(value));
        }

        // Loop the client while it connects and sends data
        client.loop();
    }
}
