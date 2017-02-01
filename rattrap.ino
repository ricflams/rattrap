// GPIO
const int PIN_IN_TRAPSENSOR = D1;
const int PIN_OUT_LIGHT     = D2;
const int PIN_IN_IR_RIGHT   = D3;
const int PIN_IN_IR_LEFT    = D4;
const int PIN_OUT_RELAY     = D5;
const int PIN_IN_BUTTON     = D6;
const int PIN_OUT_ACTLED    = D7;
bool isButtonPressed() { return digitalRead(PIN_IN_BUTTON) == HIGH; }
bool isTrapOpen() { return digitalRead(PIN_IN_TRAPSENSOR) == LOW; }
bool isIrRight() { return digitalRead(PIN_IN_IR_RIGHT) == LOW; }
bool isIrLeft() { return digitalRead(PIN_IN_IR_LEFT) == LOW; }
void setRelay(bool on) { digitalWrite(PIN_OUT_RELAY, on ? HIGH : LOW); }
void setLight(bool on) { digitalWrite(PIN_OUT_LIGHT, on ? HIGH : LOW); }
void setActLed(bool on) { digitalWrite(PIN_OUT_ACTLED, on ? HIGH : LOW); }

// Trap door
int isOpen = false;
volatile bool indicateOpen = false;
int onCommandOpen(String command)
{
  indicateOpen = true;
  return 0;
}

// Motion IR detectors
int motionCount = 0;
bool motionDetect = false;
long motionDetectEndTimeInMsec = 0;
int motionCountEndTimeInSec = 0;

// This routine runs only once upon reset
void setup()
{
  Serial.begin(9600);

  pinMode(PIN_IN_TRAPSENSOR, INPUT_PULLDOWN);
  pinMode(PIN_IN_IR_LEFT, INPUT_PULLUP);
  pinMode(PIN_IN_IR_RIGHT, INPUT_PULLUP);
  pinMode(PIN_IN_BUTTON, INPUT_PULLDOWN);
  pinMode(PIN_OUT_LIGHT, OUTPUT);
  pinMode(PIN_OUT_RELAY, OUTPUT);
  pinMode(PIN_OUT_ACTLED, OUTPUT);

  setLight(false);
  setRelay(false);

  Particle.variable("is_open", isOpen);
  Particle.function("open", onCommandOpen);
}

// This routine gets called repeatedly, like once every 5-15 milliseconds.
void loop()
{
  // Update open-state and set light on if trap is open
  if (isOpen != isTrapOpen())
  {
    isOpen = isTrapOpen();
    setLight(isOpen);
  }

  // If not already open then open trap if so requested
  if (!isOpen && (indicateOpen || isButtonPressed()))
  {
    setRelay(true);
    auto timeout = Time.now() + 10; // Wait max 10 seconds for trap to open
    while (!isTrapOpen() && Time.now() < timeout)
    {
      delay(100);
    }
    delay(500); // keep magnet turned on a bit more
    setRelay(false);
  }
  indicateOpen = false;

  // Update motion detected. Don't detect changes during the debounce
  // period and only detect actual changes and report on motion detected.
  if (millis() > motionDetectEndTimeInMsec)
  {
    auto motion = isIrLeft() || isIrRight();
    if (motion != motionDetect)
    {
      motionDetect = motion;
      motionDetectEndTimeInMsec = millis() + 200; // debounce readings
      if (motionDetect) // only report motion-detect, not motion-undetect
      {
        motionCount++;
      }
    }
  }

  // Submit motions at regular intervals, if there are any
  if (Time.now() >= motionCountEndTimeInSec)
  {
    if (motionCount > 0)
    {
      Particle.publish("motion", String::format("%d", motionCount));
      motionCount = 0;
    }
    motionCountEndTimeInSec = Time.now() + 5*60; // repeat in 5 minutes
  }
}
