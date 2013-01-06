/**
 * Methods to drive LCD via Sparkfun Serial LCD Backpack
 *
 * https://www.sparkfun.com/products/258
 *
 * @author mrusso
 */
const byte FLAG_CMD_BACKLIGHT = 0x7C;  //command flag for backlight stuff
const byte FLAG_CMD_OPERATION = 0xFE;  //command flag std operations

const byte LIGHT_LEVEL_ON = 147;
const byte LIGHT_LEVEL_OFF = 128;

const unsigned long LCD_DELAY = 125;

const byte POSITION_LINE_1 = 128;
const byte POSITION_LINE_2 = 192;
const byte POSITION_LINE_3 = 148;
const byte POSITION_LINE_4 = 212;

const byte CMD_CLEAR = 0x01;
const byte CMD_LINE_CURSOR_ON = 0x0E;
const byte CMD_BLINK_CURSOR_ON = 0x0D;
const byte CMD_CURSORS_OFF = 0x0C;
 
// ----------------------------------------
// LCD Methods
// ----------------------------------------
void configureLCD()
{
  display.begin(9600);
  clearLCD();
  delay(750);
  backlightOff();
  delay(750);
  backlightOn();
}

void lcdWait()
{
  delay(LCD_DELAY); 
}

void backlightOn()
{
  display.write(FLAG_CMD_BACKLIGHT);   
  display.write(LIGHT_LEVEL_ON);  // light level.
}

void backlightOff()
{
  display.write(FLAG_CMD_BACKLIGHT); // command flag for backlight stuff
  display.write(LIGHT_LEVEL_OFF);  // light level for off.
}

void sendCommand(byte command)
{
  display.write(FLAG_CMD_OPERATION); // command flag
  display.write(command);
}

void clearLCD()
{
  sendCommand(CMD_CLEAR);
}

void selectLineOne()
{  
  sendCommand(POSITION_LINE_1);
}

void selectLineTwo()
{ 
  sendCommand(POSITION_LINE_2);
}

void selectLineThree()
{ 
  sendCommand(POSITION_LINE_3);
}

void selectLineFour()
{
  sendCommand(POSITION_LINE_4);
}

void turnUnderlineCursorOn()
{
  sendCommand(CMD_LINE_CURSOR_ON); 
}

void turnBlinkingCursorOn()
{
  sendCommand(CMD_BLINK_CURSOR_ON); 
}

void turnCursorsOff()
{
  sendCommand(CMD_CURSORS_OFF); 
}

// ----------------------------------------
// LCD Print Methods
// ----------------------------------------

void lcdPrintLine1(String text)
{
  selectLineOne();
  lcdWait();
  display.print(text);
}

void lcdPrintLine1(float floatToPrint)
{
  selectLineOne();
  lcdWait();
  display.print(floatToPrint);
}

void lcdPrintLine2(String text)
{
  selectLineTwo();
  lcdWait();
  display.print(text);
}

void lcdPrintLine2(float floatToPrint)
{
  selectLineTwo();
  lcdWait();
  display.print(floatToPrint);
}

void lcdPrintLine3(String text)
{
  selectLineThree();
  lcdWait();
  display.print(text);
}

void lcdPrintLine3(float floatToPrint)
{
  selectLineThree();
  lcdWait();
  display.print(floatToPrint);
}

void lcdPrintLineThree(int intToPrint)
{
  selectLineThree();
  lcdWait();
  display.print(intToPrint);
}

void lcdPrintLine4(String text)
{
  selectLineFour();
  lcdWait();
  display.print(text);
}

void lcdPrintLine4(float floatToPrint)
{
  selectLineFour();
  lcdWait();
  display.print(floatToPrint);
}

