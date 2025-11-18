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

// set this to the hardware serial port you wish to use
#define HWSERIAL Serial1
const int led_pin = 13;
unsigned long baud = 19200;
byte buffer[80];
void setup()
{
  Serial.begin(baud); // USB is always 12 Mbit/sec
  HWSERIAL.begin(baud);	// communication to hardware serial
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // let USB enumerate and then announce readiness
  delay(200);
  Serial.println("READY"); // host should wait for this
}

void loop()
{
  //Serial.println("Hello World...");
  int rd = Serial.available();
  if (rd > 0) {
    int wr = HWSERIAL.availableForWrite();
    if (wr > 0){
      // compute how much data to move, the smallest
      // of rd, wr and the buffer size
      if (rd > wr) rd = wr;
      if (rd > 80) rd = 80;
      int n = Serial.readBytes((char *)buffer, rd);
      Serial.println(n);
      // write it to the hardware serial port
      HWSERIAL.write(buffer, n);
      digitalWrite(led_pin, HIGH);

    }
    
  } else {
    digitalWrite(led_pin, LOW);
    Serial.println("Did not recieve data");
  }
  delay(1);
    // do not print too fast!
}

