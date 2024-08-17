#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <LiquidCrystal_I2C.h>

const uint16_t PIN_IR_TRANSMITTER = 5;
const uint16_t PIN_SDA = 21;
const uint16_t PIN_SCL = 22;
const uint16_t PIN_THERMOSTAT[] = { 16 /* G1 */, 17 /* G2 */, 18 /* G3 */, 19 /* Y1 */, 23 /* Y2 */, 25 /* W1 */, 26 /* W2 */ };
long long PIN_LAST_ACTIVE[] = { -1 /* G1 */, -1 /* G2 */, -1 /* G3 */, -1 /* Y1 */, -1 /* Y2 */, -1 /* W1 */, -1 /* W2 */ };

const int G1 = 0;
const int G2 = 1;
const int G3 = 2;
const int Y1 = 3;
const int Y2 = 4;
const int W1 = 5;
const int W2 = 6;

const int CALL_UNKNOWN = 0;
const int CALL_OFF = 1;
const int CALL_FAN = 2;
const int CALL_COOL_STAGE_1 = 3;
const int CALL_COOL_STAGE_2 = 4;

const int LCD_COLUMNS = 20;
const int LCD_ROWS = 4;
const int LCD_ADDRESS = 0x27;

// the previous call from the thermostat
int previousCall = CALL_UNKNOWN;
// the timestamp when the thermostat started the previous call
int previousCallMillis = -1;

// the timestamp when an IR signal was sent the last time
int irCommandSentMillis = -1;

// the current call from the thermostat
int currentCall = CALL_UNKNOWN;
// the timestamp when the thermostat started the current call
int currentCallMillis = -1;

IRsend irsend(PIN_IR_TRANSMITTER);
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);

const uint16_t cmdMideaOff[199] = { 4410, 4372, 558, 1594, 552, 522, 556, 1596, 534, 1616, 554, 520, 554, 518, 556, 1598, 532, 542, 554, 518, 556, 1596,
                                    554, 520, 556, 520, 556, 1598, 530, 1618, 554, 520, 556, 1596, 534, 538, 558, 1596, 554, 1596, 556, 1596, 556, 1594, 554, 520, 554, 1594, 556,
                                    1596, 554, 1596, 556, 520, 554, 518, 556, 520, 556, 520, 532, 1620, 532, 540, 536, 542, 552, 1596, 556, 1594, 556, 1596, 556, 516, 558, 518,
                                    534, 540, 556, 520, 558, 516, 556, 518, 556, 518, 560, 516, 556, 1596, 558, 1592, 556, 1596, 554, 1596, 554, 1598, 552, 5190, 4418, 4372, 558,
                                    1592, 558, 518, 556, 1592, 560, 1594, 556, 516, 558, 516, 558, 1592, 560, 516, 558, 518, 558, 1592, 560, 514, 560, 516, 562, 1590, 556, 1594,
                                    560, 516, 560, 1590, 560, 516, 562, 1590, 558, 1592, 556, 1594, 558, 1592, 560, 516, 560, 1592, 556, 1592, 560, 1590, 560, 516, 584, 492, 558,
                                    516, 584, 490, 576, 1576, 582, 492, 582, 494, 582, 1568, 584, 1568, 584, 1568, 582, 494, 582, 492, 582, 492, 558, 518, 578, 498, 558, 516, 554,
                                    518, 580, 496, 556, 1592, 556, 1596, 580, 1570, 556, 1596, 552, 1598, 554 };

const uint16_t cmdMideaOn[299] = { 4392, 4396, 534, 1620, 532, 542, 534, 1616, 530, 1622, 532, 542, 530, 544, 532, 1618, 532, 542, 532, 542, 534, 1618,
                                   532, 540, 536, 540, 532, 1618, 536, 1620, 530, 540, 554, 1598, 532, 1618, 530, 542, 558, 1594, 532, 1618, 534, 1620, 530, 1622, 550, 1598, 534,
                                   1618, 530, 542, 556, 1594, 558, 518, 556, 518, 558, 518, 558, 516, 556, 516, 560, 518, 556, 516, 558, 1596, 558, 1592, 558, 1596, 556, 514, 562,
                                   514, 560, 516, 558, 516, 560, 1592, 558, 516, 560, 516, 560, 514, 560, 1592, 560, 1590, 558, 1592, 558, 1594, 560, 5186, 4442, 4346, 560, 1592,
                                   584, 490, 566, 1586, 582, 1566, 586, 490, 582, 492, 584, 1566, 584, 490, 584, 490, 582, 1570, 582, 492, 584, 490, 582, 1568, 584, 1570, 580, 494,
                                   580, 1570, 558, 1594, 556, 518, 554, 1596, 556, 1596, 554, 1596, 556, 1596, 554, 1596, 554, 1596, 554, 522, 554, 1600, 552, 522, 552, 524, 552, 524,
                                   552, 524, 550, 546, 530, 544, 530, 546, 528, 1622, 528, 1624, 530, 1620, 526, 548, 528, 548, 526, 550, 526, 550, 524, 1626, 526, 550, 524, 552, 524,
                                   550, 524, 1628, 522, 1632, 518, 1630, 520, 1630, 520, 5226, 4376, 4414, 518, 1634, 516, 1634, 518, 556, 518, 1656, 492, 580, 496, 1656, 494, 582,
                                   494, 1658, 494, 578, 494, 1658, 494, 1656, 494, 580, 496, 578, 494, 1658, 496, 1654, 494, 582, 494, 580, 494, 582, 492, 582, 492, 582, 494, 580, 494,
                                   582, 492, 580, 494, 582, 496, 584, 488, 580, 494, 580, 494, 580, 494, 580, 496, 582, 492, 580, 494, 1658, 492, 582, 496, 580, 494, 582, 492, 582, 492,
                                   582, 494, 582, 494, 582, 492, 584, 492, 582, 468, 606, 470, 1684, 466, 1684, 468, 1682, 466, 1708, 444, 630, 444, 632, 444 };


const uint16_t cmdMrCoolOn[199] = { 4410, 4368, 558, 1592, 560, 512, 560, 1588, 562, 510, 562, 514, 560, 512, 586, 488, 586, 1562, 562, 1588, 558, 516, 582,
                                    490, 586, 490, 560, 1586, 562, 514, 584, 488, 580, 492, 584, 490, 582, 1566, 586, 1562, 584, 490, 586, 1564, 584, 490, 584, 1564, 584, 490, 582, 1566,
                                    582, 1566, 586, 1562, 584, 1566, 582, 1566, 582, 1564, 584, 1564, 584, 1566, 582, 1564, 584, 1566, 584, 1564, 584, 1564, 584, 1566, 582, 1568, 582, 1564,
                                    582, 1566, 582, 492, 582, 1564, 584, 1564, 584, 492, 580, 1566, 588, 488, 582, 490, 584, 492, 582, 5158, 4410, 4368, 558, 518, 560, 1586, 556, 518, 556,
                                    1592, 556, 1590, 556, 1594, 556, 1592, 558, 516, 558, 516, 582, 1568, 556, 1592, 556, 1592, 556, 518, 556, 1592, 556, 1592, 556, 1592, 558, 1592, 558, 514,
                                    584, 490, 558, 1592, 556, 516, 556, 1592, 556, 516, 582, 1566, 582, 490, 556, 520, 556, 516, 558, 518, 556, 518, 556, 518, 554, 520, 552, 520, 556, 518,
                                    556, 518, 554, 520, 554, 518, 554, 520, 556, 520, 554, 520, 554, 520, 552, 1596, 554, 522, 552, 522, 552, 1596, 552, 524, 548, 1598, 550, 1600, 550, 1598,
                                    550 };

const uint16_t cmdMrCoolOff[199] = { 4434, 4346, 556, 1618, 532, 518, 582, 1590, 532, 520, 582, 490, 582, 492, 580, 492, 582, 1590, 534, 518, 582, 494, 580, 492,
                                     582, 492, 582, 1568, 552, 520, 582, 490, 582, 492, 582, 494, 580, 1590, 534, 1616, 532, 518, 580, 1592, 550, 500, 580, 1590, 534, 516, 582, 1588, 538, 1610,
                                     538, 1612, 532, 1616, 532, 1616, 532, 1618, 530, 1616, 534, 1596, 552, 1616, 530, 1596, 552, 1596, 552, 1596, 552, 1594, 556, 1598, 552, 1594, 552, 1596, 554,
                                     1596, 552, 1594, 554, 1596, 552, 518, 556, 1594, 556, 516, 556, 516, 560, 516, 556, 5182, 4410, 4372, 556, 516, 558, 1592, 556, 516, 556, 1592, 556, 1592, 556,
                                     1594, 556, 1590, 558, 518, 556, 1590, 558, 1590, 558, 1596, 554, 1588, 560, 514, 584, 1562, 562, 1588, 582, 1568, 582, 1568, 582, 490, 582, 492, 580, 1568, 580,
                                     494, 558, 1592, 554, 518, 556, 1592, 578, 494, 556, 516, 558, 518, 556, 518, 556, 518, 554, 520, 554, 520, 554, 520, 554, 520, 552, 522, 552, 520, 552, 522, 552,
                                     522, 552, 524, 548, 524, 552, 542, 530, 546, 528, 546, 528, 546, 528, 1620, 530, 546, 526, 1622, 528, 1622, 526, 1624, 524 };


void setup() {
  irsend.begin();
  Serial.begin(115200, SERIAL_8N1);

  for (int i = 0; i < sizeof PIN_THERMOSTAT / sizeof PIN_THERMOSTAT[0]; i++) {
    pinMode(PIN_THERMOSTAT[i], INPUT_PULLUP);
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TSTAT CALL  HH:MM:SS");
  lcd.setCursor(0, 1);
  lcd.print("====================");
}

void sendMessage() {
  if ((millis() - irCommandSentMillis) >= 0) {
    // irsend.sendRaw(cmdMrCoolOff, sizeof(cmdMrCoolOff) / sizeof(cmdMrCoolOff[0]), 38);
    irCommandSentMillis = millis();
    if (currentCall == CALL_OFF) {
      //   irsend.sendRaw(cmdMrCoolOff, sizeof(cmdMrCoolOff) / sizeof(cmdMrCoolOff[0]), 38);
    }
    if (currentCall == CALL_COOL_STAGE_1 || currentCall == CALL_COOL_STAGE_2) {
      //   irsend.sendRaw(cmdMrCoolOn, sizeof(cmdMrCoolOn) / sizeof(cmdMrCoolOn[0]), 38);
    }
  }
}

void processCallFromThermostat(int call) {
  if (currentCall != call) {
    irCommandSentMillis = -1;
    previousCall = currentCall;
    previousCallMillis = currentCallMillis;
    currentCallMillis = millis();
    currentCall = call;
  }
}

String formatTime(unsigned long start, unsigned long end) {
  if (start == -1) {
    return "n/a";
  }
  unsigned long seconds = (end - start) / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;

  char timeString[9];  // HH:MM is 5 characters + null terminator
  sprintf(timeString, "%02lu:%02lu:%02lu", hours, minutes, seconds);

  return String(timeString);
}

bool checkCall(int pinIndex) {
  bool active = 0 == digitalRead(PIN_THERMOSTAT[pinIndex]);
  if (active) {
    PIN_LAST_ACTIVE[pinIndex] = millis();
  }
  // If there was at least one call within the last 5 seconds, consider the pin to be called.
  //
  // The AC waveform that is fed into the optoinsulator results in intermittent false negatives when treating
  // the output as digital signal. The time based heuristic eliminates the need for a capacitor to smoothen the
  // output signal.
  return (millis() - PIN_LAST_ACTIVE[pinIndex]) < 2000;
}

void readThermostat() {
  bool callG1 = checkCall(G1);
  bool callG2 = checkCall(G2);
  bool callG3 = checkCall(G3);
  bool callY1 = checkCall(Y1);
  bool callY2 = checkCall(Y2);
  bool callW1 = checkCall(W1);
  bool callW2 = checkCall(W2);

  if (callG1) {
    Serial.print("G1=1,");
  } else {
    Serial.print("G1=0,");
  }

  if (callG2) {
    Serial.print("G2=1,");
  } else {
    Serial.print("G2=0,");
  }

  if (callG3) {
    Serial.print("G3=1,");
  } else {
    Serial.print("G3=0,");
  }

  if (callY1) {
    Serial.print("Y1=1,");
  } else {
    Serial.print("Y1=0,");
  }

  if (callY2) {
    Serial.print("Y2=1,");
  } else {
    Serial.print("Y2=0,");
  }

  if (callW1) {
    Serial.print("W1=1,");
  } else {
    Serial.print("W1=0,");
  }

  if (callW2) {
    Serial.print("W2=1,");
  } else {
    Serial.print("W2=0,");
  }
  Serial.println("");

  if (!callG1 && !callY1 && !callY2) {
    processCallFromThermostat(CALL_OFF);
  }
  if (callG1 && !callY1 && !callY2) {
    processCallFromThermostat(CALL_FAN);
  }
  if (callY1 && !callY2) {
    processCallFromThermostat(CALL_COOL_STAGE_1);
  }
  if (callY2) {
    processCallFromThermostat(CALL_COOL_STAGE_2);
  }
}

void updateDisplay() {
  lcd.setCursor(0, 2);
  if (currentCall == CALL_UNKNOWN) {
    lcd.print("CURR: n/a  ");
  } else if (currentCall == CALL_OFF) {
    lcd.print("CURR: OFF  ");
  } else if (currentCall == CALL_FAN) {
    lcd.print("CURR: FAN  ");
  } else if (currentCall == CALL_COOL_STAGE_1) {
    lcd.print("CURR: COOL1");
  } else if (currentCall == CALL_COOL_STAGE_2) {
    lcd.print("CURR: COOL2");
  }
  lcd.setCursor(12, 2);
  lcd.print(formatTime(currentCallMillis, millis()));

  lcd.setCursor(0, 3);
  if (previousCall == CALL_UNKNOWN) {
    lcd.print("PREV: n/a  ");
  } else if (previousCall == CALL_OFF) {
    lcd.print("PREV: OFF  ");
  } else if (previousCall == CALL_FAN) {
    lcd.print("PREV: FAN  ");
  } else if (previousCall == CALL_COOL_STAGE_1) {
    lcd.print("PREV: COOL1");
  } else if (previousCall == CALL_COOL_STAGE_2) {
    lcd.print("PREV: COOL2");
  }
  lcd.setCursor(12, 3);
  lcd.print(formatTime(previousCallMillis, currentCallMillis));
}

void loop() {
  readThermostat();
  updateDisplay();
  // sendMessage();
  delay(100);
}
