/*
  Copyright 2024 the original author or authors.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

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

const int DEBOUNCE_INTERVAL_MS = 2000;

long long PIN_LAST_ACTIVE[] = { 
  -DEBOUNCE_INTERVAL_MS /* G1 */, 
  -DEBOUNCE_INTERVAL_MS /* G2 */, 
  -DEBOUNCE_INTERVAL_MS /* G3 */, 
  -DEBOUNCE_INTERVAL_MS /* Y1 */, 
  -DEBOUNCE_INTERVAL_MS /* Y2 */, 
  -DEBOUNCE_INTERVAL_MS /* W1 */, 
  -DEBOUNCE_INTERVAL_MS /* W2 */ 
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
const int CALL_HEAT_STAGE_1 = 5;
const int CALL_HEAT_STAGE_2 = 6;

const int LCD_COLUMNS = 20;
const int LCD_ROWS = 4;
const int LCD_ADDRESS = 0x27;

/*
  The number of calls to store. Must be at least 2 to store
  - the candidate call and
  - the active call
*/
const int CALL_HISTORY = 5;

/*
  History of the last ${CALL_HISTORY} calls:

  call[0]: the candidate call (that might be promoted to active or replaced with the next candidate call)
  call[1]: the active call
  call[2]: the previous call
  call[3]: the pre-previous call
  ...
*/
int call[CALL_HISTORY];

/*
  The timestamps when the calls started. 
*/
int callStart[CALL_HISTORY];

const int CALL_IDX_CAND = 0;
const int CALL_IDX_ACTIVE = 1;
const int CALL_IDX_PREV = 2;
const int CALL_IDX_PPRV = 3;
const int CALL_IDX_P3RV = 4;

IRsend irsend(PIN_IR_TRANSMITTER);
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);

//
// The following codes have been recorded using https://github.com/crankyoldgit/IRremoteESP8266
//
const uint64_t MR_COOL_GEN4_OFF =                 0xA10C7EFFFFF3;
const uint64_t MR_COOL_GEN4_FAN_LOW =             0xA18C7EFFFF73;
const uint64_t MR_COOL_GEN4_COOL_75F_FAN_LOW =    0xA1886DFFFF6D;
const uint64_t MR_COOL_GEN4_COOL_75F_FAN_MEDIUM = 0xA1906DFFFF7D;
const uint64_t MR_COOL_GEN4_HEAT_75F_FAN_LOW =    0xA18B6DFFFF6F;
const uint64_t MR_COOL_GEN4_HEAT_75F_FAN_HIGH =   0xA19B6DFFFF77;


void setup() {
  irsend.begin();
  Serial.begin(115200, SERIAL_8N1);
  for (int i = 0; i < sizeof call / sizeof call[0]; i++) {
    call[i] = CALL_UNKNOWN;
    callStart[i] = -1;
  }
  for (int i = 0; i < sizeof PIN_THERMOSTAT / sizeof PIN_THERMOSTAT[0]; i++) {
    pinMode(PIN_THERMOSTAT[i], INPUT_PULLUP);
  }
  lcd.init();
  lcd.backlight();
  lcd.clear();  
}

void loop() {
  readThermostat();
  displayCallHistory();
  delay(100);
}

void readThermostat() {
  bool callG1 = checkThermostatPin(G1);
  bool callG2 = checkThermostatPin(G2);
  bool callG3 = checkThermostatPin(G3);
  bool callY1 = checkThermostatPin(Y1);
  bool callY2 = checkThermostatPin(Y2);
  bool callW1 = checkThermostatPin(W1);
  bool callW2 = checkThermostatPin(W2);

  if (!callG1 && !callY1 && !callY2 && !callW1 && !callW2) {
    processCallFromThermostat(CALL_OFF);
    return;
  }
  if (callG1 && !callY1 && !callY2 && !callW1 && !callW2) {
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
  if (callW1 && !callW2) {
    processCallFromThermostat(CALL_HEAT_STAGE_1);
    return;
  }
  if (callW2) {
    processCallFromThermostat(CALL_HEAT_STAGE_2);
    return;
  }
}

void displayCallHistory() {
  displayCall(0, CALL_IDX_ACTIVE, "CURR");
  displayCall(1, CALL_IDX_PREV, "PREV");
  displayCall(2, CALL_IDX_PPRV, "PPRV");
  displayCall(3, CALL_IDX_P3RV, "P3RV");
}

void displayCall(int line, int callIndex, String prefix) {
  lcd.setCursor(0, line);
  switch (call[callIndex]) {
    case CALL_OFF:
      lcd.print(prefix + ": OFF  ");
      break;
    case CALL_FAN:
      lcd.print(prefix + ": FAN  ");
      break;
    case CALL_COOL_STAGE_1:
      lcd.print(prefix + ": COOL1");
      break;
    case CALL_COOL_STAGE_2:
      lcd.print(prefix + ": COOL2");
      break;
    case CALL_HEAT_STAGE_1:
      lcd.print(prefix + ": HEAT1");
      break;
    case CALL_HEAT_STAGE_2:
      lcd.print(prefix + ": HEAT2");
      break;      
    default:
      lcd.print(prefix + ": -    ");
      break;
  }
  lcd.setCursor(12, line);
  lcd.print(formatTime(callStart[callIndex], callIndex == CALL_IDX_ACTIVE ? millis() : callStart[callIndex - 1]));
}

bool checkThermostatPin(int pinIndex) {
  bool active = 0 == digitalRead(PIN_THERMOSTAT[pinIndex]); 
  if (active) {
    PIN_LAST_ACTIVE[pinIndex] = millis();
  }
  /*
    If there was at least one call within the last ${DEBOUNCE_INTERVAL_MS} milliseconds, consider the pin to be called.
  
    The AC waveform that is fed into the optoinsulator results in intermittent false negatives when treating
    the output as digital signal. The time based heuristic eliminates the need for a capacitor to smoothen the
    output signal.
  */
  return (millis() - PIN_LAST_ACTIVE[pinIndex]) < DEBOUNCE_INTERVAL_MS;
}

void processCallFromThermostat(int callFromThermostat) {
  /*
    A newly detected call from the thermostat is first treated as a "candidate call".

    Let's consider an example to understand the purpose of candidate calls:

    t=  0: thermostat calls for off
    t=100: thermostat calls for COOL Stage 2 (Y1 + Y2)

    The AC waveform that is fed into the optoinsulators and the timing of code executed can result in the following
    calls from the thermostat to be observed:

    t=  1: wirestat detects call OFF          (all pins inactive)
    t=101: wirestat detects call COOL Stage 1 (Y1 active / Y2 inactive)
    t=102: wirestat detects call COOL Stage 2 (Y1 active / Y2 active)

    The thermostat never calls for COOL Stage 1. However, wirestat detects this call because there is a moment in time 
    where it reads Y1 as active and Y2 as inactive. Shortly afterwards, Y2 reads as active and the intended call 
    (COOL Stage 2) is detected.

    Treating newly detected calls as candidates allows wirestat to filter out "phantom calls" like COOL Stage 1 in the 
    above example.
  */
  if (call[CALL_IDX_CAND] != callFromThermostat) {
      
    call[CALL_IDX_CAND] = callFromThermostat;
    callStart[CALL_IDX_CAND] = millis();
    
  } else {    

    /*
      If the candidate call is detected for a time period longer than ${DEBOUNCE_INTERVAL_MS}, it is promoted to be the
      next active call.
    */
    bool promoteCandidateToActive = call[CALL_IDX_CAND] != call[CALL_IDX_ACTIVE] &&
      (millis() - callStart[CALL_IDX_CAND] > DEBOUNCE_INTERVAL_MS);
    if (promoteCandidateToActive) {
      
      /*
        Shift the history to make room for the new active call.
      */
      for (int i = CALL_HISTORY - 1; i >= 2; i--) {
        call[i] = call[i - 1];
        callStart[i] = callStart[i - 1];
      }
      
      /*
        Promote the candidate to active.
      */
      call[CALL_IDX_ACTIVE] = call[CALL_IDX_CAND];
      callStart[CALL_IDX_ACTIVE] = millis();
      
      /*
        Send new active call to the indoor unit.
      */
      sendMessage();
    }    
  }
}

void sendMessage() {
    switch (call[CALL_IDX_ACTIVE]) {
      case CALL_OFF:
        Serial.println("Sending message for CALL_OFF.");
        irsend.sendMidea(MR_COOL_GEN4_OFF);
        break;
      case CALL_FAN:
        Serial.println("Sending message for FAN.");
        irsend.sendMidea(MR_COOL_GEN4_FAN_LOW);
        break;
      case CALL_COOL_STAGE_1:
        Serial.println("Sending message for COOL_STAGE_1.");
        irsend.sendMidea(MR_COOL_GEN4_COOL_75F_FAN_LOW);
        break;
      case CALL_COOL_STAGE_2:
        Serial.println("Sending message for COOL_STAGE_2.");
        irsend.sendMidea(MR_COOL_GEN4_COOL_75F_FAN_MEDIUM);
        break;
      case CALL_HEAT_STAGE_1:
        Serial.println("Sending message for HEAT_STAGE_1.");
        irsend.sendMidea(MR_COOL_GEN4_HEAT_75F_FAN_LOW);
        break;
      case CALL_HEAT_STAGE_2:
        Serial.println("Sending message for HEAT_STAGE_2.");
        irsend.sendMidea(MR_COOL_GEN4_HEAT_75F_FAN_HIGH);
        break;        
  }
}

String formatTime(unsigned long start, unsigned long end) {
  if (start == -1) {
    return "        ";
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