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

const byte PIN_END_SENSOR = A0;
const byte PIN_TRAP_SENSOR = A2;

// Shift Register pins for controlling xmas tree
// http://arduino.cc/en/Tutorial/ShiftOut
const int PIN_LATCH = 8; //Pin connected to ST_CP
const int PIN_CLK = 12; //Pin connected to SH_CP
const int PIN_DATA = 11; //Pin connected to DS

const boolean STARTUP_DELAY = false; // delay before countdown activates (for testing alone or remotely)

// ---------- SENSOR CONSTANTS ---------- //
const int BUFFER_LENGTH = 200;
const int SENSITIVITY_VAL = 60;
const int NUM_READS_REQUIRED = 5;

const int SENSOR_DISTANCE = 38; // the distance between the trap and end sensor in inches.  Used for calculating trap speed.
const long INCHES_PER_MILE = 63360;
const long SECONDS_PER_HOUR = 3600;

// ---------- STATES ---------- //
const byte STATE_DIAGNOSTIC = 1;
const byte STATE_READY = 3;
const byte STATE_COUNTDOWN = 4;
const byte STATE_TIMER_RUNNING = 5;
const byte STATE_TIMER_STOPPED = 6;
const byte STATE_WAITING_FOR_RESET = 7;

// ---------- DISPLAY ---------- //
const int DISPLAY_RESULT_TIME = 0;
const int DISPLAY_RESULT_SPEED = 1;
const long RESULTS_DISPLAY_TIME = 1500; // ms between time and speed display in results

// ----------------------------------------
// Variables
// ----------------------------------------
byte currentState = 0; // maintains system state
SoftwareSerial display(PIN_SOFTWARE_RX, PIN_SOFTWARE_TX); // RX, TX

// For timing
boolean timerRunning = false;

unsigned long startTime;  // the time in millis when the system tells a racer to go (green light)
unsigned long trapTime; // the time in millis when the trap event is received
unsigned long endTime;  // the time in millis the finish event is received

unsigned long lastDisplayUpdate = 0;  // the time in millis the finish event is received
int lastResultDisplayed = DISPLAY_RESULT_SPEED;

float calcTrapTime;  // the elapsed time between the trap sensor and finish line
float calcRaceTime;  // the elapsed time between the racer go signal and the "finish line" trigger event

float calcInchesPerSecond; // calculated speed racer traveled through the trap
float calcMilesPerSecond; // calculated speed racer traveled through the trap
float calcMPH; // calculated speed racer traveled through the trap

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

char tempTimeString[10];  // Will be used with sprintf to create strings
char tempSpeedString[10];  // Will be used with sprintf to create strings

void setup()
{
  Serial.begin(9600);

  pinMode(PIN_RED_ARCADE_BUTTON, INPUT);
  digitalWrite(PIN_RED_ARCADE_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_GREEN_ARCADE_BUTTON, INPUT);
  digitalWrite(PIN_GREEN_ARCADE_BUTTON, HIGH);  //sets pullup resistor

  pinMode(PIN_UI_POT_ANALOG, INPUT);

  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_END_SENSOR, INPUT);
  pinMode(PIN_TRAP_SENSOR, INPUT);

  // Control the shift register (xmas tree)
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  buzz(PIN_BUZZER, 1, 200, false); // announce startup

  resetTree();

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
        buzz(PIN_BUZZER, 1, 100, true);
        currentState = STATE_COUNTDOWN;
      }
      break;
    }

    case(STATE_COUNTDOWN):
    {
      if(STARTUP_DELAY)
      {
        performCountdownDelay(); 
      }
      
      resetTimer();

      displayCountdown();
      currentState = STATE_TIMER_RUNNING;
      performCountdownSequence();
      break;
    }

    case(STATE_TIMER_RUNNING):
    {
      monitorSensors(); // loop watching for sensors to be triggered
      buzz(PIN_BUZZER, 2, 100, true);
      currentState = STATE_TIMER_STOPPED;
      break;
    }

    case(STATE_TIMER_STOPPED):
    {
      sprintf(tempTimeString, "%4d", getElapsedTime());
      sprintf(tempSpeedString, "%4d", getSpeed());

      currentState = STATE_WAITING_FOR_RESET;
      break;
    }

    case(STATE_WAITING_FOR_RESET):
    {
      if(digitalRead(PIN_GREEN_ARCADE_BUTTON) == LOW)
      {
        buzz(PIN_BUZZER, 1, 100, false);
        displayTimerReady();
        currentState = STATE_READY;
        break;
      }

      unsigned long now = millis();

      if((now - lastDisplayUpdate) > RESULTS_DISPLAY_TIME)
      {
        lastDisplayUpdate = now;
        
        if(lastResultDisplayed == DISPLAY_RESULT_SPEED)
        {
          //display time
          clearDisplay();
          displayWait();
          setDecimals(0b00010000);  // Sets colon on
          displayWait();
          display.print(tempTimeString);
          lastResultDisplayed = DISPLAY_RESULT_TIME;
        }
        else
        {
          //display speed
          clearDisplay();
          displayWait();
          setDecimals(0b00000010);  // Sets digit 2 decimal on
          displayWait();
          display.print(tempSpeedString);
          lastResultDisplayed = DISPLAY_RESULT_SPEED;
        }
      }

      break;
    }
  }

  delay(10);
}

// ----------------------------------------
// Polling Methods
// ----------------------------------------
void monitorSensors() // returns end time in millis
{
  clearDisplay();
  displayWait();
  setDecimals(0b00010000);  // Sets colon on

  boolean endTriggered = false;
  boolean trapTriggered = false;
  boolean exit = false;
  totalEndSamples = 0;
  totalTrapSamples = 0;
  trapTime = 0;
  endTime = 0;

  // monitor trap & end sensor in succession
  // watch for 'n' consecutive analog reads above the set sensitivity value to indicate a trigger

  while(!exit)
  {
    if((totalEndSamples % 100) == 0) // keep running stopwatch time
    {
      endTime = millis();
      sprintf(tempTimeString, "%4d", getElapsedTime());
      display.print(tempTimeString);
    }
    
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
            buzz(PIN_BUZZER, 1, 250, false);
            trapTriggered = true;
            break;
          }
        }
        else
        {
          break;
        }
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
          endTime = millis();
          endTriggered = true;
          exit = true;
          break;
        }
      }
      else
      {
        break;
      }
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
}

// TODO: test this again
void performCountdownDelay()
{
  buzz(PIN_BUZZER, 1, 500, false);
  delay(30000);

  buzz(PIN_BUZZER, 1, 500, false);
  delay(500);
  buzz(PIN_BUZZER, 1, 500, false);

  delay(30000);

  buzz(PIN_BUZZER, 1, 500, false);
  delay(500);
  buzz(PIN_BUZZER, 1, 500, false);
  delay(500);
  buzz(PIN_BUZZER, 1, 500, false);

  delay(30000);
}


// ----------------------------------------
// Misc Methods
// ----------------------------------------
void performCountdownSequence()
{ 
  fireR1();
  buzz(PIN_BUZZER, 1, 500, false);
  resetTree();
  delay(500);

  fireR2();
  buzz(PIN_BUZZER, 1, 500, false);
  resetTree();
  delay(500);

  fireR3();
  buzz(PIN_BUZZER, 1, 500, false);
  resetTree();
  delay(500);

  startTime = millis();

  fireG1();
  buzz(PIN_BUZZER, 1, 1500, false);
  resetTree();
  delay(250);

  lightTree();
  delay(250);

  resetTree();
  delay(250);

  lightTree();
  delay(250);

  resetTree();
  delay(250);

  lightTree();
  delay(250);

  resetTree();
  delay(250);

  lightTree();
  delay(250);

  resetTree();
}

void resetTimer()
{
  lastResultDisplayed = DISPLAY_RESULT_SPEED; // always show time first
  resetVoltageBuffers();
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

void runDiagnostics()
{
  displayDiagnosticsMode();

  buzz(PIN_BUZZER, 5, 50, true);

  // Flash bright LEDs
  for(int i = 3; i > 0; i--)
  {
    lightTree();
    delay(250);
    resetTree();
    delay(250);
  }

  buzz(PIN_BUZZER, 5, 50, true);
}

int getElapsedTime()
{
  return (int)((endTime-startTime)/10);
}

int getSpeed()
{
  calcTrapTime = 0;
  calcMPH = 0;
  calcInchesPerSecond = 0;
  calcMilesPerSecond = 0;

  if(trapTime != 0)
  {
    long elapsedTrap = (endTime - trapTime);
    calcTrapTime = (float) elapsedTrap / 1000.0;
  }
  else
  {
    return 0;
  }

  // calculate miles per hour from inches per second
  // assuming trap sensor was triggered
  if(calcTrapTime != 0)
  {
    calcInchesPerSecond = (float) SENSOR_DISTANCE / calcTrapTime;
    calcMilesPerSecond = calcInchesPerSecond / (float) INCHES_PER_MILE;
    calcMPH = calcMilesPerSecond * (float) SECONDS_PER_HOUR;
    
    return (int) (calcMPH * 100.0);
  }
 
  return 0;
}

// inspired by: http://www.faludi.com/itp/arduino/buzzer_example.pde
void buzz(byte targetPin, int numFlashes, long buzzMillis, boolean trailingDelay) {
  const long FREQUENCY = 2048;
  const long DELAY_VALUE = 1000000 / FREQUENCY / 2; // delay between transitions (2 phases, 2048 Hz, in microseconds)
  long numCycles = FREQUENCY * buzzMillis/ 1000; // number of cycles to produce (for proper timing)

  for(int i = numFlashes; i > 0; i--)
  {
    for (long j=0; j < numCycles; j++)
    {
      digitalWrite(PIN_BUZZER, HIGH);
      delayMicroseconds(DELAY_VALUE);
      digitalWrite(PIN_BUZZER, LOW);
      delayMicroseconds(DELAY_VALUE);
    }
    if(i > 1) // do the off delay if we have more than one blink left
    {
      delay(buzzMillis);
    }
    else if((i == 1) && trailingDelay)  // if we're on the last blink, only do an off delay if trailingDelay is true
    {
      delay(buzzMillis);
    }
  }
}

























