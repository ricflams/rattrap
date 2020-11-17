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

HC_SR04 outerSensor = HC_SR04(PIN_OUTER_TRIG, PIN_OUTER_ECHO);
HC_SR04 innerSensor = HC_SR04(PIN_INNER_TRIG, PIN_INNER_ECHO);
bool hasOuterMotion() { return outerSensor.distCM() < 20; }
bool hasInnerMotion() { return innerSensor.distCM() < 20; }

// Trap door
int isOpen = false;
volatile bool shouldOpenTrap = false;
int onCommandOpen(String command)
{
	shouldOpenTrap = true;
	return 0;
}
void onReleaseTrap(const char *event, const char *data)
{
	shouldOpenTrap = true;
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
	isOpen = isTrapOpen();
	if (isOpen)
	{
		delay(10000);
		return;
	}
	
	// Always honor requests to open the trap
	if (shouldOpenTrap || isButtonPressed())
	{
		setRelay(true);
		auto timeout = millis() + 10000; // Wait max 10 seconds for trap to open
		while (!isTrapOpen() && millis() < timeout)
		{
			delay(100);
		}
		delay(500); // keep magnet turned on a bit more
		setRelay(false);
		shouldOpenTrap = false;
	}

	// Detect motion
	auto outer = hasOuterMotion();
	auto inner = hasInnerMotion();
	if (!outer && !inner)
	{
	    delay(200);
	    return;
	}
	
	auto direction = outer ? "in" : "out";
	auto timeout = millis() + 2000; // Detect motion for max n msec
	while (millis() < timeout)
	{
		outer |= hasOuterMotion();
		inner |= hasInnerMotion();
		if (outer && inner)
		{
		    // We have detected motion on both sensors
			//char buf[100];
			//sprintf(buf, "%s: %dcm %dcm (%dcm %dcm)", motionDirection, outerDist, innerDist, outerDist, innerDist);
			//Particle.publish("motion2", buf);
			Particle.publish("rat_motion_detected", direction);
			if (Time.hour() >= 22 || Time.hour() < 5)
			{
			    // Emit special event during night for triggering email-notice
				Particle.publish("rat_night_motion", direction);
			}
			delay(4000); // Cool off
			break;
		}
		delay(5);
	}
}
