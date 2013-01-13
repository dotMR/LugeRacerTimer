#include <SoftwareSerial.h>

// ----------------------------------------
// Constants
// ----------------------------------------

// ---------- PINS ---------- //
const byte PIN_LCD1 = 2;  // used to drive LCD serial backpack
const byte PIN_LCD2 = 3;  // // used to drive LCD serial backpack

const byte PIN_RED_ARCADE_BUTTON = 4;  // main arcade button
const byte PIN_GREEN_ARCADE_BUTTON = 5;  // main arcade button

const byte PIN_MOMENTARY_BUTTON = 12;  // LCD user interface button
const byte PIN_UI_POT_ANALOG = A4;  // LCD user interface selector potentiometer

const byte PIN_BUZZER = 7;  // the beeper
const byte PIN_GREEN_LED = 6;  // blinding green LED; a PWM pin
const byte PIN_RED_LED1 = 9;  // blinding red LED; a PWM pin
const byte PIN_RED_LED2 = 10;  // blinding red LED; a PWM pin
const byte PIN_RED_LED3 = 11;  // blinding red LED; a PWM pin

const byte PIN_LED =  13;  // the built-in debugging LED

const byte PIN_END_SENSOR = A0;
const byte PIN_TRAP_SENSOR = A1;

// ---------- SENSOR CONSTANTS ---------- //
const int BUFFER_LENGTH = 200;
const int SENSITIVITY_VAL = 60;
const int NUM_READS_REQUIRED = 5;

const int SENSOR_DISTANCE = 36; // the distance between the trap and end sensor in inches.  Used for calculating trap speed.
const long INCHES_PER_MILE = 63360;
const int SECONDS_PER_HOUR = 3600;

// ---------- STATES ---------- //
const byte STATE_DIAGNOSTIC = 1;
const byte STATE_RACER_SELECTED = 3;
const byte STATE_COUNTDOWN = 4;
const byte STATE_TIMER_RUNNING = 5;
const byte STATE_TIMER_STOPPED = 6;
const byte STATE_WAITING_FOR_RESET = 7;

const boolean RUN_EXPRESS = false;

// ----------------------------------------
// Variables
// ----------------------------------------

byte currentState = 0; // maintains system state
SoftwareSerial display(PIN_LCD1, PIN_LCD2); // RX, TX

// For timing
boolean timerRunning = false;
unsigned long startTime;  // the time in millis when the system tells a racer to go (green light)
unsigned long trapTime; // the time in millis when the trap event is received
unsigned long endTime;  // the time in millis that a "finish line" trigger event is received
float elapsedRaceTime;  // the elapsed time between the racer go signal and the "finish line" trigger event
float elapsedTrapTime;  // the elapsed time between the racer go signal and the "finish line" trigger event
float trapSpeed;  // calculated racer speed between trap and end sensor

// used for finish line
int voltageBuffer_finishLine[BUFFER_LENGTH];
int avgVoltage_finishLine = 0;
int bufferIndex_finishLine = 0;
int currentVoltage_finishLine = 0;
unsigned long bufferSum_finishLine = 0;
unsigned long totalEndSamples = 0;

// the trap sensor (uphill)
int voltageBuffer_trap[BUFFER_LENGTH];
int avgVoltage_trap = 0;
int bufferIndex_trap = 0;
int currentVoltage_trap = 0;
unsigned long bufferSum_trap = 0;
unsigned long totalTrapSamples = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(PIN_RED_ARCADE_BUTTON, INPUT);
  digitalWrite(PIN_RED_ARCADE_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_GREEN_ARCADE_BUTTON, INPUT);
  digitalWrite(PIN_GREEN_ARCADE_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_MOMENTARY_BUTTON, INPUT);
  digitalWrite(PIN_MOMENTARY_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_UI_POT_ANALOG, INPUT);
  pinMode(PIN_GREEN_LED, OUTPUT);
  pinMode(PIN_RED_LED1, OUTPUT);
  pinMode(PIN_RED_LED2, OUTPUT);
  pinMode(PIN_RED_LED3, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_END_SENSOR, INPUT);
  pinMode(PIN_TRAP_SENSOR, INPUT);

  // Beep to announce ready
  blinkPin(PIN_BUZZER, 1, 200, false);

  configureLCD();

  refreshLCD();
  displayEnterDiagnostic();
  delay(1500);

  // read button for diagnostic state
  if(digitalRead(PIN_RED_ARCADE_BUTTON) == LOW)
  {
    currentState = STATE_DIAGNOSTIC;
  } 
  else {
    displayTimerReady();
    currentState = STATE_RACER_SELECTED;
  }
}

void loop()
{
  switch(currentState)
  {    
    case(STATE_DIAGNOSTIC):
    {
      runDiagnosticLoop();
      displayTimerReady();
      currentState = STATE_RACER_SELECTED;      
      break;
    }

    case(STATE_RACER_SELECTED):
    {
      if(RUN_EXPRESS || digitalRead(PIN_RED_ARCADE_BUTTON) == LOW)
      {
        blinkPin(PIN_BUZZER, 1, 100, true);
        currentState = STATE_COUNTDOWN;
      }
      break;
    }

    case(STATE_COUNTDOWN):
    {
      resetVoltageBuffers();
      displayTimerRunning();
      currentState = STATE_TIMER_RUNNING;
      performCountdownSequence();
      break;
    }

    case(STATE_TIMER_RUNNING):
    {
      endTime = monitorSensors();
      blinkPin(PIN_BUZZER, 2, 100, true);
      currentState = STATE_TIMER_STOPPED;
      break;
    }

    case(STATE_TIMER_STOPPED):
    {
      elapsedRaceTime = (((float)endTime) - ((float)startTime)) / ((float)1000.0);
      elapsedTrapTime = (float)endTime - (float)trapTime;

      Serial.print("(float)startTime: ");
      Serial.print((float) startTime);
      Serial.print("(float)trapTime: ");
      Serial.print((float) trapTime);
      Serial.print("(float)endTime: ");
      Serial.println((float) endTime);
      Serial.print("elapsed trap time: ");
      Serial.println(elapsedTrapTime);
      Serial.print("elapsed time: ");
      Serial.println(elapsedRaceTime);

      // calculate miles per hour from inches per second
      float inchesPerSecond = SENSOR_DISTANCE / elapsedTrapTime;
      float milesPerSecond = inchesPerSecond / INCHES_PER_MILE;
      trapSpeed = milesPerSecond * SECONDS_PER_HOUR;

      displayResults();

      if(elapsedRaceTime <= 2) {
        Serial.print(" Time found: ");
        Serial.println(elapsedRaceTime);
        Serial.print(" At buffer value: ");
        Serial.println(bufferIndex_finishLine);
        Serial.print(" # of samples @ end: ");
        Serial.println(totalEndSamples);

        Serial.println("Buffer values: ");
        for(int i=0;i<BUFFER_LENGTH;i++)
        {
          Serial.print("[");
          Serial.print(i);
          Serial.print("]: ");
          Serial.println(voltageBuffer_finishLine[i]);
        }
      }

      currentState = STATE_WAITING_FOR_RESET;
      break;
    }

    case(STATE_WAITING_FOR_RESET):
    {
      if(RUN_EXPRESS || digitalRead(PIN_GREEN_ARCADE_BUTTON) == LOW)
      {
        blinkPin(PIN_BUZZER, 1, 100, false);
        displayTimerReady();
        currentState = STATE_RACER_SELECTED; 
      }
      break; 
    }
  }

  delay(10);
}


// ----------------------------------------
// Polling Methods
// ----------------------------------------
long monitorSensors() // returns end time in millis
{
  boolean endTriggered, trapTriggered, exit = false;
  totalEndSamples, totalTrapSamples = 0;

  // monitor trap & end sensor in succession
  // watch for 'n' consecutive analog reads above the set sensitivity value to indicate a trigger

  while(!exit)
  {
    if(!trapTriggered)
    {
      // watch trap sensor for consecutive reads
      for(int i=1;i=NUM_READS_REQUIRED;i++)
      {
        // read voltage and update buffer
        currentVoltage_trap = analogRead(PIN_TRAP_SENSOR);
        voltageBuffer_trap[bufferIndex_trap] = currentVoltage_trap;
        totalTrapSamples++;

        // update rolling buffer index
        bufferIndex_trap++;
        if(bufferIndex_trap >= BUFFER_LENGTH) bufferIndex_trap = 0;

        if(abs(currentVoltage_trap-avgVoltage_trap) > SENSITIVITY_VAL)
        {
          if(i==NUM_READS_REQUIRED)
          {
            trapTime = millis();
            trapTriggered = true;
          }
        }
        else
          break;
      }

      // recalculate avg voltage if still monitoring sensor
      if(!trapTriggered)
      {
        bufferSum_trap = 0;
        for(int i=0;i<BUFFER_LENGTH;i++)
        {
          bufferSum_trap+=voltageBuffer_trap[i];
        }
        avgVoltage_trap = bufferSum_trap / BUFFER_LENGTH;
      }
    }

    // now check for consecutive analog reads @ finish line
    for(int i=1;i=NUM_READS_REQUIRED;i++)
    {
      currentVoltage_finishLine = analogRead(PIN_END_SENSOR);
      voltageBuffer_finishLine[bufferIndex_finishLine] = currentVoltage_finishLine;
      totalEndSamples++;

      // handle rolling buffer index
      bufferIndex_finishLine++;
      if(bufferIndex_finishLine >= BUFFER_LENGTH) bufferIndex_finishLine = 0;

      if(abs(currentVoltage_finishLine-avgVoltage_finishLine) > SENSITIVITY_VAL)
      {
        if(i==NUM_READS_REQUIRED)
        {
          exit = true;
          endTriggered = true;
        }
      }
      else
        break;
    }

    if(!endTriggered)
    {
      // recalculate avg voltage if still monitoring sensor
      bufferSum_finishLine = 0;
      for(int i=0;i<BUFFER_LENGTH;i++)
      {
        bufferSum_finishLine+=voltageBuffer_finishLine[i];
      }
      avgVoltage_finishLine = bufferSum_finishLine / BUFFER_LENGTH;
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
  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);

  startTime = millis();

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
    voltageBuffer_trap[i] = analogRead(PIN_TRAP_SENSOR);
    voltageBuffer_finishLine[i] = analogRead(PIN_END_SENSOR);
    delay(5);
  }

  bufferSum_trap = 0;
  bufferSum_finishLine = 0;

  for(int i=0;i<BUFFER_LENGTH;i++)
  {
    bufferSum_trap+=voltageBuffer_trap[i];
    bufferSum_finishLine+=voltageBuffer_finishLine[i];
  }

  avgVoltage_trap = bufferSum_trap / BUFFER_LENGTH;
  avgVoltage_finishLine = bufferSum_finishLine / BUFFER_LENGTH;
}

void runDiagnosticLoop()
{
  displayDiagnosticMode();
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

  long monitorLoopCount = 0;
  resetVoltageBuffers();

  // Listen for arcade button to escape the diagnostic state
  while(digitalRead(PIN_GREEN_ARCADE_BUTTON) != LOW)
  {
    currentVoltage_finishLine = analogRead(PIN_END_SENSOR);

    if(abs(currentVoltage_finishLine-avgVoltage_finishLine) > SENSITIVITY_VAL)
    {
      blinkPin(PIN_BUZZER, 3, 100, false);
      lcdPrintLine2(currentVoltage_finishLine);
      lcdPrintLine3(avgVoltage_finishLine);
      resetVoltageBuffers();
      delay(3000);
      blinkPin(PIN_BUZZER, 1, 100, false);      
    }
    else
    {
      voltageBuffer_finishLine[bufferIndex_finishLine] = currentVoltage_finishLine;

      bufferSum_finishLine = 0;

      for(int i=0;i<BUFFER_LENGTH;i++)
      {
        bufferSum_finishLine+=voltageBuffer_finishLine[i];
      }

      avgVoltage_finishLine = bufferSum_finishLine / BUFFER_LENGTH;

      bufferIndex_finishLine++;
      if(bufferIndex_finishLine >= BUFFER_LENGTH) bufferIndex_finishLine = 0;  
    }

    if(monitorLoopCount++ >= 750)
    {
      // display analog sensor values
      lcdPrintLine2(currentVoltage_finishLine);
      lcdPrintLine3(avgVoltage_finishLine);
      monitorLoopCount = 0;
    }
  }

  blinkPin(PIN_BUZZER, 5, 50, true);
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


























