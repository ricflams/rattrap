// GPIO
const int PIN_IN_BUTTON = D1;
const int PIN_OUT_GREENLED = D4;
const int PIN_OUT_REDLED = D5;
bool isButtonPressed() { return digitalRead(PIN_IN_BUTTON) == HIGH; }
void setGreenLed(bool on) { digitalWrite(PIN_OUT_GREENLED, on ? HIGH : LOW); }
void setRedLed(bool on) { digitalWrite(PIN_OUT_REDLED, on ? HIGH : LOW); }

typedef enum {
    Motionless,
    MotionIn,
    MotionOut,
} Motion;
Motion motion = Motionless;

void onMotionDetected(const char *event, const char *data)
{
  motion = strcmp(data, "in") == 0 ? MotionIn : MotionOut;
}


// This routine runs only once upon reset
void setup()
{
  Serial.begin(9600);

  pinMode(PIN_IN_BUTTON, INPUT_PULLDOWN);
  pinMode(PIN_OUT_GREENLED, OUTPUT);
  pinMode(PIN_OUT_REDLED, OUTPUT);
  
  Particle.subscribe("rat_motion_detected", onMotionDetected, MY_DEVICES);
}

// This routine gets called repeatedly, like once every 5-15 milliseconds.
void loop()
{
  // Flash the light when motion is detected
  if (motion != Motionless)
  {
      for (auto i = 0; i < 7; i++)
      {
        setGreenLed(motion == MotionIn);
        setRedLed(motion == MotionOut);
        delay(200);
        setRedLed(false);
        setGreenLed(false);
        delay(50);
      }
      motion = Motionless;
  }

  // Publish the release-event when button is pressed and
  // wait until it's no longer pressed, accounting for jitter
  if (isButtonPressed())
  {
      Particle.publish("rat_release_trap");
      while (isButtonPressed())
      {
          delay(200);
      }
      delay(1000);
  }
}
