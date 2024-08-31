#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <LiquidCrystal_I2C.h>

const uint16_t PIN_IR_TRANSMITTER = 5;
const uint16_t PIN_SDA = 21;
const uint16_t PIN_SCL = 22;
const uint16_t PIN_THERMOSTAT[] = { 
  16 /* G1 */, 
  17 /* G2 */, 
  18 /* G3 */, 
  19 /* Y1 */, 
  23 /* Y2 */, 
  25 /* W1 */, 
  26 /* W2 */ 
};

const int DEBOUNCE_INTEVAL_MS = 2000;

long long PIN_LAST_ACTIVE[] = { 
  -DEBOUNCE_INTEVAL_MS /* G1 */, 
  -DEBOUNCE_INTEVAL_MS /* G2 */, 
  -DEBOUNCE_INTEVAL_MS /* G3 */, 
  -DEBOUNCE_INTEVAL_MS /* Y1 */, 
  -DEBOUNCE_INTEVAL_MS /* Y2 */, 
  -DEBOUNCE_INTEVAL_MS /* W1 */, 
  -DEBOUNCE_INTEVAL_MS /* W2 */ 
};

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





const uint16_t MR_COOL_GEN4_OFF[199] = { 4388, 4392,  556, 1596,  530, 542,  532, 1618,  532, 542,  532, 540,  534, 542,  552, 518,  534, 1618,  532, 540,  
  534, 540,  532, 542,  534, 542,  532, 1618,  532, 540,  556, 518,  534, 538,  534, 540,  556, 1594,  530, 1618,  532, 540,  536, 1616,  556, 1592,  554, 
  520,  532, 1616,  532, 1616,  556, 1594,  554, 1592,  558, 1596,  550, 1592,  556, 1592,  556, 1592,  556, 1594,  556, 1590,  556, 1590,  558, 1590,  
  558, 1590,  558, 1590,  558, 1590,  560, 1590,  560, 1590,  556, 1590,  560, 1590,  558, 1590,  560, 514,  560, 1590,  558, 1590,  560, 514,  560, 1590,  
  560, 5180,  4414, 4368,  558, 516,  558, 1590,  584, 490,  584, 1566,  558, 1590,  580, 1566,  582, 1568,  580, 492,  580, 1568,  580, 1568,  580, 1566,  
  582, 1566,  582, 492,  580, 1566,  582, 1568,  558, 1590,  582, 1568,  558, 516,  556, 516,  556, 1592,  558, 518,  554, 518,  556, 1594,  554, 520,  554, 
  522,  554, 520,  554, 522,  552, 522,  552, 520,  552, 522,  552, 522,  550, 524,  550, 522,  552, 522,  550, 544,  528, 544,  530, 546,  528, 544,  530, 
  546,  528, 546,  528, 544,  528, 546,  526, 546,  528, 1622,  526, 550,  524, 548,  526, 1624,  524, 550,  522 }; 

const uint16_t MR_COOL_GEN4_FAN_LOW[199] = { 4410, 4370,  556, 1594,  556, 516,  558, 1592,  556, 516,  558, 518,  556, 518,  558, 516,  556, 1592,  554, 
  1596,  554, 516,  560, 514,  556, 518,  556, 1600,  550, 1596,  552, 518,  556, 518,  558, 516,  556, 1594,  554, 1596,  552, 1596,  552, 1594,  554, 
  1592,  556, 1592,  556, 518,  556, 1598,  552, 1596,  552, 1594,  554, 1596,  552, 1596,  552, 1596,  554, 1594,  552, 1596,  552, 1596,  552, 1598,  552, 
  1596,  552, 1596,  554, 1594,  552, 1594,  554, 1596,  552, 1596,  552, 520,  556, 1594,  554, 1594,  554, 1594,  554, 518,  556, 516,  558, 1592,  552, 
  1596,  556, 5182,  4412, 4368,  558, 516,  558, 1592,  558, 518,  556, 1592,  560, 1588,  558, 1592,  560, 1588,  558, 518,  558, 514,  582, 1566,  582, 
  1566,  582, 1566,  582, 490,  582, 490,  582, 1566,  582, 1566,  582, 1566,  580, 494,  580, 492,  556, 518,  558, 516,  556, 518,  554, 520,  556, 1592,  
  554, 520,  554, 520,  554, 520,  556, 518,  554, 520,  552, 522,  550, 522,  554, 520,  552, 524,  550, 524,  550, 542,  530, 544,  532, 544,  530, 546,  
  528, 546,  526, 548,  526, 1622,  524, 550,  526, 548,  526, 550,  522, 1626,  524, 1626,  522, 552,  520, 556,  520 }; 

const uint16_t MR_COOL_GEN4_COOL_75F_FAN_LOW[199] = { 4434, 4344,  560, 1590,  556, 518,  558, 1592,  556, 518,  556, 516,  560, 516,  562, 510,  562, 1586,  
556, 1592,  558, 516,  560, 512,  560, 514,  558, 1592,  558, 514,  562, 514,  560, 512,  560, 512,  560, 1590,  560, 1588,  560, 514,  560, 1590,  558, 1590,  
556, 518,  558, 1588,  560, 1590,  576, 1574,  582, 1564,  582, 1566,  582, 1566,  582, 1568,  580, 1568,  582, 1564,  560, 1590,  556, 1592,  558, 1592,  558, 
1592,  558, 1590,  556, 1592,  556, 1592,  556, 1592,  556, 516,  582, 1566,  558, 1592,  554, 518,  558, 1592,  558, 1592,  556, 516,  558, 1592,  558, 5182,  
4436, 4344,  558, 516,  556, 1592,  558, 518,  556, 1594,  554, 1592,  582, 1566,  556, 1594,  556, 516,  556, 518,  554, 1594,  554, 1594,  556, 1592,  554, 520,  
554, 1594,  554, 1596,  552, 1594,  554, 1596,  554, 520,  552, 524,  552, 1596,  552, 522,  550, 524,  552, 1598,  550, 522,  550, 544,  530, 544,  530, 544,  
528, 544,  528, 548,  526, 546,  526, 548,  524, 548,  526, 546,  526, 550,  522, 550,  524, 550,  524, 550,  522, 552,  520, 552,  520, 556,  518, 1628,  520, 
554,  520, 554,  518, 1630,  520, 554,  518, 556,  518, 1630,  518, 554,  520 }; 

const uint16_t MR_COOL_GEN4_COOL_75F_FAN_MEDIUM[199] = { 4416, 4366,  558, 1592,  580, 494,  560, 1588,  556, 516,  560, 516,  560, 512,  558, 514,  560, 1590,  
560, 1588,  558, 518,  562, 512,  558, 1590,  556, 514,  562, 512,  560, 514,  560, 514,  560, 512,  562, 1588,  560, 1586,  562, 512,  562, 1588,  560, 1588,  
562, 512,  558, 1590,  564, 1586,  556, 1592,  560, 1590,  572, 1576,  560, 1588,  582, 1566,  560, 1590,  558, 1590,  562, 1588,  560, 1588,  560, 1588,  558, 
1592,  560, 1586,  584, 1564,  560, 1590,  582, 1564,  584, 490,  582, 1566,  584, 1566,  582, 1566,  582, 1566,  578, 1568,  582, 492,  584, 1564,  556, 5186, 
4408, 4372,  556, 518,  580, 1568,  558, 514,  582, 1568,  556, 1592,  556, 1592,  556, 1590,  556, 520,  580, 490,  560, 1590,  554, 1594,  556, 518,  558, 1590,  
556, 1594,  556, 1592,  560, 1590,  556, 1590,  582, 492,  558, 516,  558, 1592,  558, 514,  582, 492,  580, 1566,  562, 512,  562, 512,  580, 494,  578, 494,  578, 
498,  554, 518,  556, 516,  556, 520,  554, 520,  554, 518,  554, 518,  554, 520,  554, 520,  554, 518,  556, 520,  552, 522,  552, 520,  554, 1594,  552, 522,  554, 
520,  552, 524,  548, 526,  550, 522,  552, 1598,  550, 524,  550 }; 



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
    irCommandSentMillis = millis();
    if (currentCall == CALL_OFF) {
      Serial.println("Sending message for CALL_OFF.");
      irsend.sendRaw(MR_COOL_GEN4_OFF, sizeof(MR_COOL_GEN4_OFF) / sizeof(MR_COOL_GEN4_OFF[0]), 38);
      return;
    }
    if (currentCall == CALL_FAN) {
      Serial.println("Sending message for FAN.");
      irsend.sendRaw(MR_COOL_GEN4_FAN_LOW, sizeof(MR_COOL_GEN4_FAN_LOW) / sizeof(MR_COOL_GEN4_FAN_LOW[0]), 38);
      return;
    }
    if (currentCall == CALL_COOL_STAGE_1) {
      Serial.println("Sending message for COOL_STAGE_1.");
      irsend.sendRaw(MR_COOL_GEN4_COOL_75F_FAN_LOW, sizeof(MR_COOL_GEN4_COOL_75F_FAN_LOW) / sizeof(MR_COOL_GEN4_COOL_75F_FAN_LOW[0]), 38);
      return;
    }
    if (currentCall == CALL_COOL_STAGE_2) {
      Serial.println("Sending message for COOL_STAGE_2.");
      irsend.sendRaw(MR_COOL_GEN4_COOL_75F_FAN_MEDIUM, sizeof(MR_COOL_GEN4_COOL_75F_FAN_MEDIUM) / sizeof(MR_COOL_GEN4_COOL_75F_FAN_MEDIUM[0]), 38);
      return;
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
    sendMessage();
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

bool checkThermostatPin(int pinIndex) {
  bool active = 0 == digitalRead(PIN_THERMOSTAT[pinIndex]);
  if (active) {
    PIN_LAST_ACTIVE[pinIndex] = millis();
  }
  // If there was at least one call within the last ${DEBOUNCE_INTEVAL_MS} milliseconds, consider the pin to be called.
  //
  // The AC waveform that is fed into the optoinsulator results in intermittent false negatives when treating
  // the output as digital signal. The time based heuristic eliminates the need for a capacitor to smoothen the
  // output signal.
  return (millis() - PIN_LAST_ACTIVE[pinIndex]) < DEBOUNCE_INTEVAL_MS;
}

void readThermostat() {
  bool callG1 = checkThermostatPin(G1);
  bool callG2 = checkThermostatPin(G2);
  bool callG3 = checkThermostatPin(G3);
  bool callY1 = checkThermostatPin(Y1);
  bool callY2 = checkThermostatPin(Y2);
  bool callW1 = checkThermostatPin(W1);
  bool callW2 = checkThermostatPin(W2);

  if (!callG1 && !callY1 && !callY2) {
    processCallFromThermostat(CALL_OFF);
    return;
  }
  if (callG1 && !callY1 && !callY2) {
    processCallFromThermostat(CALL_FAN);
    return;
  }
  if (callY1 && !callY2) {
    processCallFromThermostat(CALL_COOL_STAGE_1);
    return;
  }
  if (callY2) {
    processCallFromThermostat(CALL_COOL_STAGE_2);
    return;
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
  delay(100);
}
