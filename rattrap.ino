// GPIO
const int PIN_IN_TRAPSENSOR = D1;
const int PIN_OUT_LIGHT = D2;
const int PIN_IN_IR_RIGHT = D3;
const int PIN_IN_IR_LEFT = D4;
const int PIN_OUT_RELAY = D5;
const int PIN_IN_BUTTON = D6;
const int PIN_OUT_ACTLED = D7;
bool isButtonPressed() { return digitalRead(PIN_IN_BUTTON) == HIGH; }
bool isTrapOpen() { return digitalRead(PIN_IN_TRAPSENSOR) == LOW; }
bool isIrRight() { return digitalRead(PIN_IN_IR_RIGHT) == LOW; }
bool isIrLeft() { return digitalRead(PIN_IN_IR_LEFT) == LOW; }
void setRelay(bool on) { digitalWrite(PIN_OUT_RELAY, on ? HIGH : LOW); }
void setLight(bool on) { digitalWrite(PIN_OUT_LIGHT, on ? HIGH : LOW); }
void setActLed(bool on) { digitalWrite(PIN_OUT_ACTLED, on ? HIGH : LOW); }

// Motion-detection
const int DETECTION_PERIOD_MSEC = 1500;
const int COOLOFF_PERIOD_MSEC = 3000;
// Monthly detect-start-hour           jan feb mar apr may jun jul aug sep oct nov dec
const int MonthlyDetectHourStart[] = { 20, 20, 20, 21, 21, 21, 22, 22, 21, 21, 20, 20 };
const int DetectHourEnd = 5;
typedef enum {
  StateWaiting,
  StateDetecting,
  StateCoolOff
} State;
State state = StateWaiting;
unsigned long stateEndTimeInMsec = 0;
// Motion IR detectors
bool motionDetectRight = false;
bool motionDetectLeft = false;
char const* motionDirection;

// Trap door
int isOpen = false;
volatile bool indicateOpen = false;
int onCommandOpen(String command)
{
  indicateOpen = true;
  return 0;
}

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
    auto timeout = millis() + 10000; // Wait max 10 seconds for trap to open
    while (!isTrapOpen() && millis() < timeout)
    {
      delay(100);
    }
    delay(500); // keep magnet turned on a bit more
    setRelay(false);
  }
  indicateOpen = false;

  // Detect motion
  switch (state)
  {
    case StateWaiting:
      if (isIrRight() || isIrLeft())
      {
        motionDirection = isIrRight() ? "in" : "out";
        motionDetectRight = false;
        motionDetectLeft = false;
        stateEndTimeInMsec = millis() + DETECTION_PERIOD_MSEC;
        state = StateDetecting;
        //Particle.publish("detecting", motionDirection);
      }
      break;
    case StateDetecting:
      if (isIrRight())
      {
        motionDetectRight = true;
      }
      if (isIrLeft())
      {
        motionDetectLeft = true;
      }
      if (motionDetectRight && motionDetectLeft)
      {
        // We detected something so fire off an event if it's time to detect at all
        auto month = Time.month(); // month is 1-12
        auto hour = Time.hour() + 1 + (month>3 && month<11 ? 1 : 0); // +1 for UTC->CET and +1 if DST, simplified
        if (hour< DetectHourEnd || hour >= MonthlyDetectHourStart[month - 1])
        {
          Particle.publish("motion_detected", motionDirection);
        }
        stateEndTimeInMsec = millis() + COOLOFF_PERIOD_MSEC;
        state = StateCoolOff;
        //Particle.publish("cooloff", motionDirection);
      }
      else if (millis() > stateEndTimeInMsec)
      {
        state = StateWaiting;
        //Particle.publish("nope, waiting");
      }
      break;
    case StateCoolOff:
      if (millis() > stateEndTimeInMsec)
      {
        state = StateWaiting;
        //Particle.publish("cooled, waiting");
      }
      break;
  }
}
