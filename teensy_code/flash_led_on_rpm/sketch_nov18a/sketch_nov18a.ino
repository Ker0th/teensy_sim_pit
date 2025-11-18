
// set this to the hardware serial port you wish to use
#define HWSERIAL Serial1
const int led_pin = 13;
// Match PC side baud (use same baud your host opens)
unsigned long baud = 115200;

String incoming = "";
byte buffer[80];

// blinking state
unsigned long lastToggle = 0;
bool ledState = false;
unsigned long blinkHalfPeriodMs = 500; // default: 1 flash/sec => period 1000ms => half 500ms
int lastHz = 0;

void setup()
{
  Serial.begin(baud); // USB serial
  HWSERIAL.begin(baud);	// communication to hardware serial (if used)
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // let USB enumerate and then announce readiness
  delay(200);
  Serial.println("READY"); // host should wait for this
}

// parse RPM from packet content like "carData|SPEED:...,RPM:1234.5,THROTTLE:...,BRAKE:...|"
// returns negative on parse failure
float parseRPMFromPacket(const String &pkt) {
  int idx = pkt.indexOf("RPM:");
  if (idx < 0) return -1.0;
  int start = idx + 4;
  int end = start;
  while (end < pkt.length()) {
    char c = pkt.charAt(end);
    if (!( (c >= '0' && c <= '9') || c == '.' || c == '-' )) break;
    end++;
  }
  if (end == start) return -1.0;
  String num = pkt.substring(start, end);
  return num.toFloat();
}

void updateBlinkForRPM(float rpm) {
  if (rpm < 0) return; // ignore
  // Map rpm to flashes/sec (Hz). Adjust mapping as desired.
  // Here: 0..999 -> 1Hz, 1000..1999 -> 2Hz, 2000..2999 -> 3Hz, ... cap at 6Hz
  int hz = 1 + (int)(rpm / 1000.0);
  if (hz < 1) hz = 1;
  if (hz > 10) hz = 10;

  if (hz != lastHz) {
    lastHz = hz;
    unsigned long periodMs = 1000UL / (unsigned long)hz; // ms per flash
    // use 50% duty cycle (on half, off half)
    blinkHalfPeriodMs = periodMs / 2;
    // ensure non-zero
    if (blinkHalfPeriodMs == 0) blinkHalfPeriodMs = 1;
    // optional debug
    Serial.print("RPM=");
    Serial.print(rpm);
    Serial.print(" -> ");
    Serial.print(hz);
    Serial.println(" Hz");
  }
}

void loop()
{
  // Read all incoming USB bytes and accumulate into `incoming`.
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    incoming += c;
    // forward to hardware serial as before (if needed)
    // (use buffer for efficiency; here direct write is fine)
  }

  // If we have one or more '|' delimited packets, extract and handle them.
  // Packet format expected: ... |payload| ...
  int idxStart = incoming.indexOf('|');
  while (idxStart >= 0) {
    int idxEnd = incoming.indexOf('|', idxStart + 1);
    if (idxEnd < 0) break; // wait for full packet
    String packet = incoming.substring(idxStart + 1, idxEnd); // between pipes
    // attempt to parse RPM and update blink rate
    float rpm = parseRPMFromPacket(packet);
    if (rpm >= 0) {
      updateBlinkForRPM(rpm);
    }
    // forward the raw packet (including delimiters) to hardware serial if desired
    // e.g. HWSERIAL.write((const uint8_t*)packet.c_str(), packet.length());
    // remove processed chunk (everything up to idxEnd)
    incoming = incoming.substring(idxEnd + 1);
    idxStart = incoming.indexOf('|');
  }

  // If nothing was received recently, you might still want to keep LED behavior.
  // Non-blocking blink: toggle LED when half period elapses.
  unsigned long now = millis();
  if (now - lastToggle >= blinkHalfPeriodMs) {
    ledState = !ledState;
    digitalWrite(led_pin, ledState ? HIGH : LOW);
    lastToggle = now;
  }

  // small yield to avoid busy spin
  delay(1);
}