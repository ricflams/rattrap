const int PIN_IN_TRAPSENSOR = D1;
const int PIN_OUT_LIGHT     = D2;
const int PIN_IN_IR_RIGHT   = D3;
const int PIN_IN_IR_LEFT    = D4;
const int PIN_OUT_RELAY     = D5;
const int PIN_IN_BUTTON     = D6;
const int PIN_OUT_ACTLED    = D7;

bool IsButtonPressed() { return digitalRead(PIN_IN_BUTTON) == HIGH; }
bool IsTrapOpen() { return digitalRead(PIN_IN_TRAPSENSOR) == LOW; }
void SetRelay(bool on) { digitalWrite(PIN_OUT_RELAY, on ? HIGH : LOW); }
void SetLight(bool on) { digitalWrite(PIN_OUT_LIGHT, on ? HIGH : LOW); }
void SetActLed(bool on) { digitalWrite(PIN_OUT_ACTLED, on ? HIGH : LOW); }

volatile bool IndicateOpen = false;
volatile bool IndicateIrLeft = false;
volatile bool IndicateIrRight = false;
int IsOpen = false;

int OnCommandOpen(String command)
{
  IndicateOpen = true;
  return 1;
}

void OnIrLeftChange()
{
  IndicateIrLeft = true;
}

void OnIrRightChange()
{
  IndicateIrRight = true;
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

  attachInterrupt(PIN_IN_IR_LEFT, OnIrLeftChange, FALLING);
  attachInterrupt(PIN_IN_IR_RIGHT, OnIrRightChange, FALLING);

  SetLight(false);
  SetRelay(false);

  Particle.variable("is_open", IsOpen);
  Particle.function("open", OnCommandOpen);
}

// This routine gets called repeatedly, like once every 5-15 milliseconds.
void loop()
{
  // Update open-state and set light on if trap is open
  if (IsTrapOpen() != IsOpen)
  {
    IsOpen = IsTrapOpen();
    SetLight(IsOpen);
  }

  // Submit IR events, unless blocked by trap-door; if so then clear them
  if (!IsOpen)
  {
    if (IndicateIrLeft)
    {
      IndicateIrLeft = false;
      Particle.publish("ir_left");
    }
    if (IndicateIrRight)
    {
      IndicateIrRight = false;
      Particle.publish("ir_right");
    }
    if (IsButtonPressed())
    {
      IndicateOpen = true;
    }
  }
  else
  {
    IndicateIrLeft = false;
    IndicateIrRight = false;
  }

  // Open trap if so requested and it's not already open
  if (IndicateOpen && !IsOpen)
  {
    SetRelay(true);
    // Wait max n seconds for trap to open
    int timeout = Time.now() + 10;
    while (!IsOpen && Time.now() < timeout)
    {
      IsOpen = IsTrapOpen();
      delay(100);
    }
    delay(400); // keep magnet turned on a bit more
    SetRelay(false);
    IndicateOpen = false;
  }
}
