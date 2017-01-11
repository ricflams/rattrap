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
const int MOTIONPUBLISH_INTERVAL_MSEC = 5 * 60 * 1000; // Every 5 min
volatile int motionCount = 0;
void onPublishMotion()
{
  Particle.publish("motion", String::format("%d", motionCount / 2));
  motionCount = 0;
}
Timer motionPublishTimer(MOTIONPUBLISH_INTERVAL_MSEC, onPublishMotion);
void onMotionDetected()
{
  motionCount++;
}

// This routine runs only once upon reset
void setup()
{
  Serial.begin(9600);

  pinMode(PIN_IN_TRAPSENSOR, INPUT_PULLDOWN);
  pinMode(PIN_IN_IR_LEFT, INPUT_PULLDOWN);
  pinMode(PIN_IN_IR_RIGHT, INPUT_PULLDOWN);
  pinMode(PIN_IN_BUTTON, INPUT_PULLDOWN);
  pinMode(PIN_OUT_LIGHT, OUTPUT);
  pinMode(PIN_OUT_RELAY, OUTPUT);
  pinMode(PIN_OUT_ACTLED, OUTPUT);

  attachInterrupt(PIN_IN_IR_LEFT, onMotionDetected, FALLING);
  attachInterrupt(PIN_IN_IR_RIGHT, onMotionDetected, FALLING);

  setLight(false);
  setRelay(false);

  Particle.variable("is_open", isOpen);
  Particle.function("open", onCommandOpen);
  motionPublishTimer.start();
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
    int timeout = Time.now() + 10; // Wait max n seconds for trap to open
    while (!isTrapOpen() && Time.now() < timeout)
    {
      delay(100);
    }
    delay(500); // keep magnet turned on a bit more
    setRelay(false);
  }
  indicateOpen = false;
}
