// This #include statement was automatically added by the Particle IDE.
#include <HC-SR04.h>

// GPIO
const int PIN_TRAPSENSOR = D0;
const int PIN_OUTER_ECHO = D1;
const int PIN_OUTER_TRIG = D2;
const int PIN_INNER_ECHO = D3;
const int PIN_INNER_TRIG = D4;
const int PIN_RELAY = D5;
const int PIN_MANUAL_BUTTON = D6;

bool isButtonPressed() { return digitalRead(PIN_MANUAL_BUTTON) == HIGH; }
bool isTrapOpen() { return digitalRead(PIN_TRAPSENSOR) == LOW; }
void setRelay(bool on) { digitalWrite(PIN_RELAY, on ? HIGH : LOW); }

// Motion-detection
const int DETECTION_PERIOD_MSEC = 2000;
const int COOLOFF_PERIOD_MSEC = 3000;
const int DETECT_INTERVAL_MSEC = 250;
// Monthly detect-start-hour           jan feb mar apr may jun jul aug sep oct nov dec
const int MonthlyDetectHourStart[] = { 20, 20, 20, 21, 21, 22, 22, 21, 21, 20, 20, 20 };
const int DetectHourEnd = 5;
typedef enum
{
  StateTrapOpen,
  StateTrapClosed,
  StateWaitForMotion,
  StateDetectMotion,
  StateCoolOff
} State;
State state = StateWaitForMotion;
unsigned long stateEndTimeInMsec = 0;
// Motion detectors
HC_SR04 outerSensor = HC_SR04(PIN_OUTER_TRIG, PIN_OUTER_ECHO);
HC_SR04 innerSensor = HC_SR04(PIN_INNER_TRIG, PIN_INNER_ECHO);
bool outerMotionDetected = false;
bool innerMotionDetected = false;
char const* motionDirection;
bool hasOuterMotion() { return outerSensor.distCM() < 20; }
bool hasInnerMotion() { return innerSensor.distCM() < 20; }

// Trap door
const int ENGAGEMENT_WAIT_MSEC = 10000;
int isOpen = false;
volatile bool indicateOpen = false;
int onCommandOpen(String command)
{
  indicateOpen = true;
  return 0;
}
void onReleaseTrap(const char *event, const char *data)
{
  indicateOpen = true;
}

// This routine runs only once upon reset
void setup()
{
  Serial.begin(9600);
  pinMode(PIN_TRAPSENSOR, INPUT_PULLDOWN);
  pinMode(PIN_MANUAL_BUTTON, INPUT_PULLDOWN);
  pinMode(PIN_RELAY, OUTPUT);
  outerSensor.init();
  innerSensor.init();
  
  setRelay(false);

  Particle.variable("is_open", isOpen);
  Particle.function("open", onCommandOpen);
  Particle.subscribe("rat_release_trap", onReleaseTrap, MY_DEVICES);
}

// This routine gets called repeatedly about once every millisecond
void loop()
{
  // Don't do anything if trap is open
  if (isTrapOpen())
  {
    state = StateTrapOpen;
    return;
  }
  
  // Always honor requests to open the trap
  if (indicateOpen || isButtonPressed())
  {
    setRelay(true);
    auto timeout = millis() + 10000; // Wait max 10 seconds for trap to open
    while (!isTrapOpen() && millis() < timeout)
    {
      delay(100);
    }
    delay(500); // keep magnet turned on a bit more
    setRelay(false);
    indicateOpen = false;
  }

  // Detect motion
  switch (state)
  {
    case StateTrapOpen:
      //Particle.publish("TrapOpen -> Closed");
      stateEndTimeInMsec = millis() + ENGAGEMENT_WAIT_MSEC;
      state = StateTrapClosed;
      break;
      
    case StateTrapClosed:
      if (millis() > stateEndTimeInMsec)
      {
        state = StateWaitForMotion;
        //Particle.publish("TrapClosed -> Wait");
      }
      break;
      
    case StateWaitForMotion:
      outerMotionDetected = hasOuterMotion();
      delay(3); // Sleep 3ms to avoid inner/outer sound interferences
      innerMotionDetected = hasInnerMotion();
      if (outerMotionDetected || innerMotionDetected)
      {
        //char buf[100];
        //auto innerDist = (int)innerSensor.distCM(); delay(3);
        //auto outerDist = (int)outerSensor.distCM();
        //sprintf(buf, "%d %d", innerDist, outerDist);
        //Particle.publish("distances", buf);
        motionDirection = outerMotionDetected? "in" : "out";
        stateEndTimeInMsec = millis() + DETECTION_PERIOD_MSEC;
        state = StateDetectMotion;
        //Particle.publish("detecting", motionDirection);
      }
      else
      {
        delay(DETECT_INTERVAL_MSEC);
      }
      break;

    case StateDetectMotion:
      outerMotionDetected |= hasOuterMotion();
      delay(3); // Sleep 3ms to avoid inner/outer sound interferences
      innerMotionDetected |= hasInnerMotion();
      if (outerMotionDetected && innerMotionDetected)
      {
        // We detected something so fire off an event if it's time to detect at all
        auto month = Time.month(); // month is 1-12
        auto hour = Time.hour() + 1 + (month>3 && month<11 ? 1 : 0); // +1 for UTC->CET and +1 if DST, simplified
        if (hour< DetectHourEnd || hour >= MonthlyDetectHourStart[month - 1])
        {
          Particle.publish("rat_motion_detected", motionDirection);
        }
        stateEndTimeInMsec = millis() + COOLOFF_PERIOD_MSEC;
        state = StateCoolOff;
        //Particle.publish("cooloff", motionDirection);
      }
      else if (millis() > stateEndTimeInMsec)
      {
        state = StateWaitForMotion;
        //Particle.publish("nope, waiting");
      }
      else
      {
        delay(DETECT_INTERVAL_MSEC);
      }
      break;
      
    case StateCoolOff:
      if (millis() > stateEndTimeInMsec)
      {
        state = StateWaitForMotion;
        //Particle.publish("cooled, waiting");
      }
      break;
  }
}