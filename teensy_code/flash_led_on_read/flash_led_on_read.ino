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
const int fan_pin = 33;
unsigned long baud = 19200;
byte buffer[80];
void setup()
{
  analogWriteResolution(12);  // analogWrite value 0 to 4095, or 4096 for high
  Serial.begin(baud); // USB is always 12 Mbit/sec
  HWSERIAL.begin(baud);	// communication to hardware serial
  pinMode(led_pin, OUTPUT);
  pinMode(fan_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  analogWrite(fan_pin, 4096);
  // let USB enumerate and then announce readiness
  delay(200);
  Serial.println("READY"); // host should wait for this
}

double extract_speed_from_ASCII(byte* buffer) {
    // 1. Create a temporary buffer for the 8 characters + 1 for null terminator
    char temp[9];

    // 2. Copy 8 bytes starting from index 14
    // We use &buf[14] to get the memory address of the 14th character
    memcpy(temp, &buffer[14], 8);

    // 3. IMPORTANT: Add the null terminator '\0'
    // String conversion functions like atof() stop reading when they hit \0
    temp[8] = '\0';

    // 4. Convert the string to a double and return it
    return atof(temp);
}

int convertSpeedToInt(double speed) {
    // 1. Constrain the input so speeds below 10 stay 10 
    //    and speeds above 300 stay 300.
    if (speed <= 10.0) return 2000;
    if (speed >= 300.0) return 4095;

    // 2. Linear Mapping Formula:
    // (input - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
    double mappedValue = (speed - 10.0) * (4095.0 - 2000) / (150 - 10.0) + 0.0;

    // 3. Return as a rounded 8-bit integer (0-255)
    return (int)round(mappedValue);
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
      (void) Serial.readBytes((char *)buffer, rd);
      double speed = extract_speed_from_ASCII(buffer);
      analogWrite(fan_pin, convertSpeedToInt(speed));
      //Serial.println(n);
      // write it to the hardware serial port
      //HWSERIAL.write(buffer, n);
      digitalWrite(led_pin, HIGH);

    }
    
  } else {
    digitalWrite(led_pin, LOW);
    Serial.println("Did not recieve data");
  }
  delay(1);
    // do not print too fast!
}

