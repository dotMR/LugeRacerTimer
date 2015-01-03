#include <SoftwareSerial.h>

// ----------------------------------------
// Constants
// ----------------------------------------

// ---------- PINS ---------- //
const byte PIN_SOFTWARE_TX = 3;  // used to drive serial display
const byte PIN_SOFTWARE_RX = 2;  // used to drive serial display

const byte PIN_RED_ARCADE_BUTTON = 4;  // main arcade button
const byte PIN_GREEN_ARCADE_BUTTON = 5;  // main arcade button

const byte PIN_UI_POT_ANALOG = A4;  // LCD user interface selector potentiometer

const byte PIN_BUZZER = 7;  // the beeper
const byte PIN_GREEN_LED = 6;  // blinding green LED; a PWM pin
const byte PIN_RED_LED1 = 9;  // blinding red LED; a PWM pin
const byte PIN_RED_LED2 = 10;  // blinding red LED; a PWM pin
const byte PIN_RED_LED3 = 11;  // blinding red LED; a PWM pin

const byte PIN_SENSOR_ANALOG1 = A0;
const byte PIN_SENSOR_ANALOG2 = A1;

// ---------- SENSOR CONSTANTS ---------- //
const int BUFFER_LENGTH = 200;
const int SENSITIVITY_VAL = 60;
const int SECONDS_PER_HOUR = 3600;

// ---------- STATES ---------- //
const byte STATE_DIAGNOSTIC = 1;
const byte STATE_READY = 3;
const byte STATE_COUNTDOWN = 4;
const byte STATE_TIMER_RUNNING = 5;
const byte STATE_TIMER_STOPPED = 6;
const byte STATE_WAITING_FOR_RESET = 7;


// ----------------------------------------
// Variables
// ----------------------------------------

byte currentState = 0; // maintains system state
SoftwareSerial display(PIN_SOFTWARE_RX, PIN_SOFTWARE_TX); // RX, TX

// For timing
boolean timerRunning = false;
unsigned long startTime;  // the time in millis at the time the system tells a racer to go
unsigned long endTime;  // the time in millis that a "finish line" trigger event is received

unsigned long totalSamples = 0;

// For sensor 1, the finish sensor
int voltageValueBuffer[BUFFER_LENGTH];
int avgVoltageValue = 0;
int bufferIndex = 0;
int currentVoltageValue = 0;
unsigned long totalVoltageValue = 0;

char tempString[10];  // Will be used with sprintf to create strings

void setup()
{
  Serial.begin(9600);

  pinMode(PIN_RED_ARCADE_BUTTON, INPUT);
  digitalWrite(PIN_RED_ARCADE_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_GREEN_ARCADE_BUTTON, INPUT);
  digitalWrite(PIN_GREEN_ARCADE_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_UI_POT_ANALOG, INPUT);
  pinMode(PIN_GREEN_LED, OUTPUT);
  pinMode(PIN_RED_LED1, OUTPUT);
  pinMode(PIN_RED_LED2, OUTPUT);
  pinMode(PIN_RED_LED3, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_SENSOR_ANALOG1, INPUT);
  pinMode(PIN_SENSOR_ANALOG2, INPUT);

  blinkPin(PIN_BUZZER, 1, 200, false); // announce startup

  configureDisplay();

  runDiagnostics();
  
  displayTimerReady();

  currentState = STATE_READY;
}

void loop()
{
  switch(currentState)
  {
    case(STATE_READY):
    {
      if(digitalRead(PIN_RED_ARCADE_BUTTON) == LOW)
      {
        blinkPin(PIN_BUZZER, 1, 100, true);
        currentState = STATE_COUNTDOWN;
      }
      break;
    }

    case(STATE_COUNTDOWN):
    {
      resetVoltageBuffers();
      displayCountdown();
      currentState = STATE_TIMER_RUNNING;
      performCountdownSequence();
      break;
    }

    case(STATE_TIMER_RUNNING):
    {
      endTime = monitorEndSensor();
      blinkPin(PIN_BUZZER, 2, 100, true);
      currentState = STATE_TIMER_STOPPED;
      break;
    }

    case(STATE_TIMER_STOPPED):
    {
      sprintf(tempString, "%4d", getElapsedTime());

      clearDisplay();
      delay(250);
      display.print(tempString); // digit 2 decimal already set

      currentState = STATE_WAITING_FOR_RESET;
      break;
    }

    case(STATE_WAITING_FOR_RESET):
    {
      if(digitalRead(PIN_GREEN_ARCADE_BUTTON) == LOW)
      {
        blinkPin(PIN_BUZZER, 1, 100, false);
        displayTimerReady();
        currentState = STATE_READY;
      }
      break; 
    }
  }

  delay(10);
}


// ----------------------------------------
// Polling Methods
// ----------------------------------------

long monitorEndSensor() // returns trap time in millis
{
  boolean trigger = false;
  totalSamples = 0;

  clearDisplay();
  setDecimals(0b00000010);  // Sets digit 2 decimal on

  while(!trigger)
  {
    if((totalSamples % 100) == 0)
    {
      endTime = millis();
      sprintf(tempString, "%4d", getElapsedTime());

      display.print(tempString);
    }

    currentVoltageValue = analogRead(PIN_SENSOR_ANALOG1);
    totalSamples++;

    if(abs(currentVoltageValue-avgVoltageValue) > SENSITIVITY_VAL)
    {
      currentVoltageValue = analogRead(PIN_SENSOR_ANALOG1);
      totalSamples++;

      if(abs(currentVoltageValue-avgVoltageValue) > SENSITIVITY_VAL)
      {
        currentVoltageValue = analogRead(PIN_SENSOR_ANALOG1);
        totalSamples++;

        if(abs(currentVoltageValue-avgVoltageValue) > SENSITIVITY_VAL)
        {
          currentVoltageValue = analogRead(PIN_SENSOR_ANALOG1);
          totalSamples++;

          if(abs(currentVoltageValue-avgVoltageValue) > SENSITIVITY_VAL)
          {
            currentVoltageValue = analogRead(PIN_SENSOR_ANALOG1);
            totalSamples++;

            if(abs(currentVoltageValue-avgVoltageValue) > SENSITIVITY_VAL)
            {
              trigger = true;
            }
          }
        }
      }
    }
    else
    {
      voltageValueBuffer[bufferIndex] = currentVoltageValue;
      totalVoltageValue = 0;

      for(int i=0;i<BUFFER_LENGTH;i++)
      {
        totalVoltageValue+=voltageValueBuffer[i];
      }

      avgVoltageValue = totalVoltageValue / BUFFER_LENGTH;

      bufferIndex++;
      if(bufferIndex >= BUFFER_LENGTH) bufferIndex = 0;
    }
  }
  return millis();
}


// ----------------------------------------
// Misc Methods
// ----------------------------------------
void performCountdownSequence()
{ 
  digitalWrite(PIN_RED_LED3, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(500);

  digitalWrite(PIN_RED_LED3, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  delay(500);

  digitalWrite(PIN_RED_LED2, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(500);

  digitalWrite(PIN_RED_LED2, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  delay(500);

  digitalWrite(PIN_RED_LED1, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(500);

  digitalWrite(PIN_RED_LED1, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  delay(500);

  startTime = millis();

  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(1500);

  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  delay(250);

  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_RED_LED1, HIGH);
  digitalWrite(PIN_RED_LED2, HIGH);
  digitalWrite(PIN_RED_LED3, HIGH);
  delay(250);

  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_RED_LED1, LOW);
  digitalWrite(PIN_RED_LED2, LOW);
  digitalWrite(PIN_RED_LED3, LOW);
  delay(250);

  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_RED_LED1, HIGH);
  digitalWrite(PIN_RED_LED2, HIGH);
  digitalWrite(PIN_RED_LED3, HIGH);
  delay(250);

  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_RED_LED1, LOW);
  digitalWrite(PIN_RED_LED2, LOW);
  digitalWrite(PIN_RED_LED3, LOW);
  delay(250);

  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_RED_LED1, HIGH);
  digitalWrite(PIN_RED_LED2, HIGH);
  digitalWrite(PIN_RED_LED3, HIGH);
  delay(250);

  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_RED_LED1, LOW);
  digitalWrite(PIN_RED_LED2, LOW);
  digitalWrite(PIN_RED_LED3, LOW);
  delay(250);

  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_RED_LED1, HIGH);
  digitalWrite(PIN_RED_LED2, HIGH);
  digitalWrite(PIN_RED_LED3, HIGH);
  delay(250);

  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_RED_LED1, LOW);
  digitalWrite(PIN_RED_LED2, LOW);
  digitalWrite(PIN_RED_LED3, LOW);
}

void resetVoltageBuffers()
{
  for(int i=0;i<BUFFER_LENGTH;i++)
  {
    voltageValueBuffer[i] = analogRead(PIN_SENSOR_ANALOG1);
    delay(5);
  }

  totalVoltageValue = 0;

  for(int i=0;i<BUFFER_LENGTH;i++)
  {
    totalVoltageValue+=voltageValueBuffer[i];
  }

  avgVoltageValue = totalVoltageValue / BUFFER_LENGTH;

}

void runDiagnostics()
{
  displayDiagnosticsMode();

  blinkPin(PIN_BUZZER, 5, 50, true);

  // Flash bright LEDs
  for(int i = 3; i > 0; i--)
  {
    digitalWrite(PIN_GREEN_LED, HIGH);
    digitalWrite(PIN_RED_LED1, HIGH);
    digitalWrite(PIN_RED_LED2, HIGH);
    digitalWrite(PIN_RED_LED3, HIGH);
    delay(250);
    digitalWrite(PIN_GREEN_LED, LOW);
    digitalWrite(PIN_RED_LED1, LOW);
    digitalWrite(PIN_RED_LED2, LOW);
    digitalWrite(PIN_RED_LED3, LOW);
    delay(250);
  }

  blinkPin(PIN_BUZZER, 5, 50, true);
}

int getElapsedTime()
{
  return (int)((endTime-startTime)/10);
}

void blinkPin(byte pin, int numFlashes, long blinkMillis, boolean trailingDelay)
{
  for(int i = numFlashes; i > 0; i--)
  {
    digitalWrite(pin, HIGH);
    delay(blinkMillis);
    digitalWrite(PIN_BUZZER, LOW);
    if(i > 1) // do the off delay if we have more than one blink left
    {
      delay(blinkMillis);
    }
    else if((i == 1) && trailingDelay)  // if we're on the last blink, only do an off delay if trailingDelay is true
    {
      delay(blinkMillis);
    }
  }
}








