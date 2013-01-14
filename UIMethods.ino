// ----------------------------------------
// UI Methods
// ----------------------------------------
void displayEnterDiagnostic()
{
  lcdPrintLine3("Diagnostic Mode");
}

void displayDiagnosticMode()
{
  clearLCD();
  lcdWait();
  lcdPrintLine1(" * DIAGNOSTIC MODE *");
  lcdWait();
  lcdPrintLine4("               Exit"); 
}

void refreshLCD()
{
  clearLCD();
  lcdWait();
  lcdPrintLine1("*  Race Timer v1.1 *");
}

void displayTimerReady()
{
  refreshLCD();
  lcdWait();
  lcdPrintLine4("Start");
}

void displayTimerRunning()
{
  refreshLCD();
  lcdWait();
  lcdPrintLine3("-- Timer Running --");
}

void displayResults()
{
  refreshLCD();
  lcdWait();

  if(calcMPH != 0)
  {
    lcdPrintLine2(calcMPH);
    display.print(" mph");
    lcdWait();
  }

  lcdPrintLine3(calcRaceTime);
  display.print(" sec");
  lcdWait();
  lcdPrintLine4("               Reset");
}


