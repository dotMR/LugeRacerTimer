/**
 * Methods to drive Sparkfun 7-Segment Serial Display
 * https://learn.sparkfun.com/tutorials/using-the-serial-7-segment-display/example-1-serial-uart
 * https://www.sparkfun.com/products/11442
 *
 * @author mrusso
 */
const byte CMD_CLEAR = 0x76;
const byte CMD_CURSOR_CONTROL = 0x79;
const byte CMD_BRIGHTNESS = 0x7A;

const byte CURSOR_POSITION_0 = 0x0;
const byte CURSOR_POSITION_1 = 0x1;
const byte CURSOR_POSITION_2 = 0x2;
const byte CURSOR_POSITION_3 = 0x3;
 
// ----------------------------------------
// Display Methods
// ----------------------------------------
void configureDisplay()
{
  display.begin(9600);
  clearDisplay();
  setBrightness(175);  // Medium brightness
  display.print("SBLA");
  delay(500);
  clearDisplay();
}

//  This will clear the display and reset the cursor
void clearDisplay()
{
  display.write(CMD_CLEAR);
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  display.write(CMD_BRIGHTNESS);  // Set brightness command byte
  display.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals)
{
  display.write(0x77);
  display.write(decimals);
}

void displayTimerReady()
{
  clearDisplay();
  delay(250);
  display.print("RDY");
}

void displayCountdown()
{
  clearDisplay();
  display.print(3210); 
}

void displayResults(int timeTenths)
{
  clearDisplay();
  delay(100);
  setDecimals(0b00000100);  // Sets digit 3 decimal on
  display.print(timeTenths);
}

