/*
  Use this as the Pico drivers to program with Arduino

  https://arduino-pico.readthedocs.io/en/latest/install.html
*/

// the setup function runs once when you press reset or power the board
void setup()
{
  // Start serial port reading
  Serial.begin(9600);
}

// the loop function runs over and over again forever
void loop()
{
  // Read internal temperatue sensor
  float temp = analogReadTemp();

  // Set serial messages
  String a = "The temperature is: ";
  String b = " ºC";
  String message = a + temp + b;

  // Print a message to the serial port
  Serial.println(message);

  // Wait for one second
  delay(1000);
}
