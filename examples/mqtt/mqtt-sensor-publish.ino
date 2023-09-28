/*
    The code here will include the publishing of sensor data to the MQTT broker.

    At first, a fully-managed instance using Mosquitto was tried (and successfully deployed)
    but connection to the broker is not easy as we don't have a static IP.

    The easiest option (for now at least) is to use EMQX since it's an open-source
    public MQTT broker. Thi also means that what we publish there is public.
        - Publish to "broker.emqx.io" as the hostname; SSL can be done on
          port 8883 as well

    If we ever want to change, we can scale either with that or create a managed cloud
    borker in GCP/Azure or whatever platform we choose. -> the docker-compose.yml file
    attached can help with this, but it's not needed yet

    Install the libraries:
        - EspMQTTClient
        - PubSubClient
*/

#include <EspMQTTClient.h>

const str wifi_name="";
const str wifi_pass="";

EspMQTTClient client(
    wifi_name,
    wifi_pass,
    "broker.emqx.io", // MQTT Broker server ip
    // "user",        // Can be omitted if not needed
    // "pass",        // Can be omitted if not needed
    "ESPClient", // Client name that uniquely identify your device
    1883         // MQTT broker port
);

// 388 is the no-noise reading, up to 1023
const int adc = A0;
int noise_read = 0;

void setup()
{
    Serial.begin(115200);

    // Optional functionalities of EspMQTTClient
    client.enableDebuggingMessages();                                         // Enable debugging messages sent to serial output
    client.enableLastWillMessage("ESPClient/lastwill", "I am going offline"); // You can activate the retain flag by setting the third parameter to true
}

void onConnectionEstablished()
{
    // Subscribe to "mytopic/test" and display received message to Serial
    client.subscribe("mytopic/test", [](const String &payload)
                     { Serial.println(payload); });

    // Subscribe to "mytopic/wildcardtest/#" and display received message to Serial
    // client.subscribe("mytopic/wildcardtest/#", [](const String &topic, const String &payload)
    //                 { Serial.println("(From wildcard) topic: " + topic + ", payload: " + payload); });
}

void loop()
{
    if (client.isConnected())
    {

        noise_read = analogRead(adc);

        // Publish an incremental value
        client.publish("mytopic/test", String(noise_read));

        delay(500);
    }
    delay(500);
    client.loop();
}
