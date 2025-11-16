/* Simple "Hello World" example.

   After uploading this to your board, use Serial Monitor
   to view the message.  When Serial is selected from the
   Tools > USB Type menu, the correct serial port must be
   selected from the Tools > Serial Port AFTER Teensy is
   running this code.  Teensy only becomes a serial device
   while this code is running!  For non-Serial types,
   the Serial port is emulated, so no port needs to be
   selected.

   This example code is in the public domain.
*/
const int led_pin = 13;
byte buffer[80];
void setup()
{
  Serial.begin(9600); // USB is always 12 Mbit/sec
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
}

void loop()
{
  //Serial.println("Hello World...");

  int rd = Serial.available();
  if (rd > 0) {
    int n = Serial.readBytes((char *)buffer, rd);
    digitalWrite(led_pin, HIGH);
    
  } else {
    digitalWrite(led_pin, LOW);
  }
  delay(1);
    // do not print too fast!
}

