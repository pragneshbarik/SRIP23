// Import the necessary libraries
#include <Arduino.h>

// Define the TX pin
const int txPin = 1;

// Setup the serial connection
void setup()
{
    // Set the baud rate to 9600
    Serial.begin(115200);

    // Set the TX pin as an output
    pinMode(txPin, OUTPUT);
}

// Loop forever
void loop()
{
    // Read the data from the TX pin
    int data = digitalRead(txPin);

    // Print the data to the serial monitor
    Serial.println(data);
}