/**
 * Methods to drive Xmas Tree Start timer utilizing shift register
 * 
 * http://arduino.cc/en/uploads/Tutorial/595datasheet.pdf
 * (See Fig 1, Page3 for pin mapping)
 * 
 * Assumes Connections:
 *   LED_R1: Q3
 *   LED_R2: Q4
 *   LED_R3: Q5
 *   LED_G1: Q6
 *   Buzzer: Q7
 *
 * @author mrusso
 */
const byte BYTE_CLEAR = B0;
const byte BYTE_ALL = B00011111;

const byte BYTE_LED_R1 = B00010000;
const byte BYTE_LED_R2 = B00001000;
const byte BYTE_LED_R3 = B00000100;
const byte BYTE_LED_G1 = B00000010;

const byte BYTE_BUZZER = B00000001;


void resetTree() {
  send_to_tree(BYTE_CLEAR);
}

void lightTree() {
 send_to_tree(BYTE_ALL);
}

void fireR1() {
 send_to_tree(BYTE_LED_R1);
}

void fireR2() {
 send_to_tree(BYTE_LED_R2);
}

void fireR3() {
 send_to_tree(BYTE_LED_R3);
}

void fireG1() {
 send_to_tree(BYTE_LED_G1);
}

void send_to_tree(byte data) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLK, LSBFIRST, data);
  digitalWrite(PIN_LATCH, HIGH);
  // for now no delay, just try to be smart about how it is called
}
