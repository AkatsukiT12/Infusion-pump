// ===== UNIFIED IV INFUSION PUMP (L298N, DUAL-MODE, V4 - FIXED) =====
// - Supports both Remote and Manual operation.
// - REMOTE: Controlled by serial commands (START:patientID:speed:volume).
// - MANUAL: Controlled by physical button/potentiometer for Pump 1.
// - Fixed Motor 2 speed adjustment handling in REMOTE mode.

// ===== REQUIRED LIBRARIES =====
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== PIN DEFINITIONS FOR L298N H-BRIDGE =====
const int flowPin       = 2;
const int startBtn      = 4;
const int potPin        = A0;
const int L298N_ENA_PIN = 5;
const int L298N_IN1_PIN = 6;
const int L298N_IN2_PIN = 7;
const int L298N_ENB_PIN = 9;
const int L298N_IN3_PIN = 10;
const int L298N_IN4_PIN = 11;
const int greenLED      = 8;
const int redLED        = 12;
const int buzzer        = 13;

// ===== SYSTEM STATE & MODE =====
enum SystemMode { IDLE, MANUAL, REMOTE };
SystemMode currentMode = IDLE;
int activeMotor = 0;
bool isAlertActive = false;

// ===== CONSTANTS & CALIBRATION =====
#define BAUD_RATE 9600
float FLOW_CALIBRATION_FACTOR = 5.4;  // Calibrated based on actual flow measurements
#define DATA_SEND_INTERVAL 2000
#define LCD_UPDATE_INTERVAL 500
#define COMMAND_BUFFER_SIZE 60
float kp = 2.5;
const int PWM_MIN = 80;
const int PWM_MAX = 255;

// Motor speed presets for different flow rates (mL/hr)
// These should be calibrated to your specific pump/syringe setup
const int SPEED_10ML_HR = 85;   // ~10 mL/hr
const int SPEED_30ML_HR = 95;   // ~30 mL/hr  
const int SPEED_60ML_HR = 110;  // ~60 mL/hr
const int SPEED_120ML_HR = 130; // ~120 mL/hr

// ===== GLOBAL VARIABLES =====
volatile unsigned int flowPulseCount = 0;
float flowRate = 0.0;
float setRate = 0.0;
unsigned long lastCalcTime = 0;
float initialVolume = 0.0;
float remainingVolume = 0.0;
float liquidLevel = 250.0;
float totalDispensed = 0.0;
int motorSpeed = 0;
unsigned long lastDataSendTime = 0;
unsigned long lastLCDUpdateTime = 0;
char commandBuffer[COMMAND_BUFFER_SIZE];
int commandIndex = 0;

// ===== INTERRUPT SERVICE ROUTINES =====
void flowPulseCounter() { flowPulseCount++; }

// ===== SETUP =====
void setup() {
  Serial.begin(BAUD_RATE);
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Infusion System");
  lcd.setCursor(0, 1); lcd.print("Initializing...");
  delay(2000);

  pinMode(flowPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowPin), flowPulseCounter, FALLING);  // Try FALLING instead of RISING

  pinMode(potPin, INPUT);
  pinMode(startBtn, INPUT_PULLUP);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzer, OUTPUT);

  pinMode(L298N_ENA_PIN, OUTPUT);
  pinMode(L298N_IN1_PIN, OUTPUT);
  pinMode(L298N_IN2_PIN, OUTPUT);
  pinMode(L298N_ENB_PIN, OUTPUT);
  pinMode(L298N_IN3_PIN, OUTPUT);
  pinMode(L298N_IN4_PIN, OUTPUT);

  stopAllMotors();
  lastCalcTime = millis();

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("System Ready");
  lcd.setCursor(0, 1); lcd.print("Mode: IDLE");
}

// ===== MAIN LOOP =====
void loop() {
  handlePhysicalControls();

  if (isAlertActive) {
    triggerAlertVisuals();
    delay(500);
    return;
  }

  readSerialCommands();

  if (millis() - lastCalcTime >= 1000) {
    calculateFlowRate();
    if(currentMode == REMOTE) calculateLiquidLevel();
    runModeLogic();
    lastCalcTime = millis();
  }

  performSafetyChecks();

  if (millis() - lastLCDUpdateTime >= LCD_UPDATE_INTERVAL) {
    updateLCDDisplay();
    lastLCDUpdateTime = millis();
  }

  if (millis() - lastDataSendTime >= DATA_SEND_INTERVAL) {
    sendDataToPC();
    lastDataSendTime = millis();
  }
}

// ===== MODE & CONTROL LOGIC =====
// ===== MODE & CONTROL LOGIC =====
void handlePhysicalControls() {
 if (digitalRead(startBtn) == LOW) {
  delay(300); // Debounce button press

  // First, check for alert acknowledgement
  if (isAlertActive) {
   isAlertActive = false;
   noTone(buzzer);
   digitalWrite(redLED, LOW);
   currentMode = IDLE;
   Serial.println("Alert acknowledged and cleared by user.");
   return;
  }

    // *** THIS IS THE FIX ***
    // If the system is being controlled by the app (REMOTE mode),
    // the physical button should be ignored.
    if (currentMode == REMOTE) {
      Serial.println("Button press ignored: System is in REMOTE mode.");
      return; 
    }
  
    // At this point, mode is either IDLE or MANUAL
  if (currentMode == IDLE) {
   // We are IDLE, so switch to MANUAL
   currentMode = MANUAL;
   activeMotor = 1;
   totalDispensed = 0.0;
   Serial.println("Switched to MANUAL mode (Controlling Pump 1).");
  } else {
      // We are MANUAL, so switch to IDLE
   currentMode = IDLE;
   stopAllMotors();
   Serial.println("Switched to IDLE mode.");
  }
 }
}

void runModeLogic() {
  if (currentMode == MANUAL) {
    // MANUAL MODE: Potentiometer controls desired flow rate (0-100 mL/hr)
    setRate = map(analogRead(potPin), 0, 1023, 0, 100);
    
    // PID-like control to adjust motor speed to match setRate
    float error = setRate - flowRate;
    motorSpeed += (int)(kp * error);
    motorSpeed = constrain(motorSpeed, 0, PWM_MAX);

    // Apply minimum PWM threshold to overcome motor friction
    if (setRate > 0.5 && motorSpeed > 0 && motorSpeed < PWM_MIN) motorSpeed = PWM_MIN;
    if (setRate < 0.5) motorSpeed = 0;

    adjustActiveMotorSpeed(motorSpeed);
    digitalWrite(greenLED, HIGH);
    
  } else if (currentMode == REMOTE) {
    // REMOTE MODE: App specifies exact PWM speed, no potentiometer
    // motorSpeed is already set by START command, just maintain it
    adjustActiveMotorSpeed(motorSpeed);
    digitalWrite(greenLED, HIGH);
  }
}

// ===== MOTOR CONTROL FUNCTIONS =====
void startMotor1(int speed, float volume) {
  stopAllMotors();
  activeMotor = 1;
  motorSpeed = constrain(speed, 0, 255);
  initialVolume = volume;
  totalDispensed = 0.0;
  remainingVolume = volume;
  liquidLevel = 100.0;

  digitalWrite(L298N_IN1_PIN, HIGH);
  digitalWrite(L298N_IN2_PIN, LOW);
  analogWrite(L298N_ENA_PIN, motorSpeed);
  
  Serial.print("Motor 1 started - Speed: "); Serial.print(motorSpeed);
  Serial.print(", Volume: "); Serial.println(volume);
}

void startMotor2(int speed, float volume) {
  stopAllMotors();
  activeMotor = 2;
  motorSpeed = constrain(speed, 0, 255);
  initialVolume = volume;
  totalDispensed = 0.0;
  remainingVolume = volume;
  liquidLevel = 100.0;

  digitalWrite(L298N_IN3_PIN, HIGH);
  digitalWrite(L298N_IN4_PIN, LOW);
  analogWrite(L298N_ENB_PIN, motorSpeed);
  
  Serial.print("Motor 2 started - Speed: "); Serial.print(motorSpeed);
  Serial.print(", Volume: "); Serial.println(volume);
  
  // Debug: Read back pin states
  Serial.print("DEBUG Motor 2 - IN3: "); Serial.print(digitalRead(L298N_IN3_PIN));
  Serial.print(", IN4: "); Serial.print(digitalRead(L298N_IN4_PIN));
  Serial.print(", ENB PWM: "); Serial.println(motorSpeed);
}

void stopAllMotors() {
  analogWrite(L298N_ENA_PIN, 0);
  digitalWrite(L298N_IN1_PIN, LOW);
  digitalWrite(L298N_IN2_PIN, LOW);
  analogWrite(L298N_ENB_PIN, 0);
  digitalWrite(L298N_IN3_PIN, LOW);
  digitalWrite(L298N_IN4_PIN, LOW);
  motorSpeed = 0;
  activeMotor = 0;
  digitalWrite(greenLED, LOW);
  
  Serial.println("All motors stopped");
}

void adjustActiveMotorSpeed(int speed) {
  motorSpeed = constrain(speed, 0, 255);
  if (activeMotor == 1) {
    analogWrite(L298N_ENA_PIN, motorSpeed);
    Serial.print("Motor 1 speed adjusted to: "); Serial.println(motorSpeed);
  } else if (activeMotor == 2) {
    analogWrite(L298N_ENB_PIN, motorSpeed);
    Serial.print("Motor 2 speed adjusted to: "); Serial.println(motorSpeed);
  }
}

// ===== SENSOR & CALCULATION FUNCTIONS =====
void calculateFlowRate() {
  noInterrupts();
  unsigned int pulses = flowPulseCount;
  flowPulseCount = 0;
  interrupts();
  
  // Debug output
  Serial.print("DEBUG: Pulses in 1 sec = "); Serial.print(pulses);
  Serial.print(", Cal Factor = "); Serial.print(FLOW_CALIBRATION_FACTOR, 1);
  
  flowRate = (pulses / FLOW_CALIBRATION_FACTOR);
  totalDispensed += (pulses / FLOW_CALIBRATION_FACTOR);
  
  Serial.print(", Flow Rate = "); Serial.print(flowRate, 1); 
  Serial.println(" mL/hr");
}

void calculateLiquidLevel() {
  if (initialVolume > 0) {
    remainingVolume = initialVolume - totalDispensed;
    if (remainingVolume < 0) remainingVolume = 0;
    liquidLevel = (remainingVolume / initialVolume) * 100.0;
  } else {
    liquidLevel = 0;
  }
}

// ===== SAFETY & ALERTS =====
void performSafetyChecks() {
  if (activeMotor == 0) return;

  String alertMessage = "";
  if (currentMode == MANUAL && setRate > 10.0 && flowRate < (setRate * 0.2)) {
    alertMessage = "PUMP STALLED!";
  }

  if (alertMessage != "") {
    Serial.print("EMERGENCY: "); Serial.println(alertMessage);
    stopAllMotors();
    currentMode = IDLE;
    sendDataToPC();
    triggerAlert(alertMessage);
  }
}

void triggerAlert(String msg) {
  isAlertActive = true;
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("!!! ERROR !!!");
  lcd.setCursor(0, 1); lcd.print(msg);
}

void triggerAlertVisuals() {
  digitalWrite(redLED, !digitalRead(redLED));
  tone(buzzer, 1000, 250);
}

// ===== DISPLAY & COMMUNICATION =====
void updateLCDDisplay() {
  if (isAlertActive) return;

  lcd.clear();
  lcd.setCursor(0, 0);

  switch(currentMode) {
    case MANUAL:
      lcd.print("P1 Set: "); lcd.print(setRate, 0);
      lcd.setCursor(0, 1);
      lcd.print("Flow: "); lcd.print(flowRate, 1);
      break;
    case REMOTE:
      lcd.print("P"); lcd.print(activeMotor);
      lcd.print(" Flow:"); lcd.print(flowRate, 1);
      lcd.setCursor(0, 1);
      lcd.print("Lvl: "); lcd.print((int)liquidLevel); lcd.print("%");
      break;
    case IDLE:
      lcd.print("System Idle");
      lcd.setCursor(0, 1);
      lcd.print("Press Start Btn");
      break;
  }
}

void sendDataToPC() {
  bool isActive = (activeMotor != 0);
  Serial.print(flowRate, 2); Serial.print(",");
  Serial.print(liquidLevel, 1); Serial.print(",");
  Serial.print(isActive ? 1 : 0); Serial.print(",");
  Serial.println(0);
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (commandIndex > 0) {
        commandBuffer[commandIndex] = '\0';
        processCommand(commandBuffer);
        commandIndex = 0;
      }
    } else if (commandIndex < COMMAND_BUFFER_SIZE - 1) {
      commandBuffer[commandIndex++] = inChar;
    }
  }
}

void processCommand(char* command) {
  if (isAlertActive) {
    Serial.println("Command ignored: System alert is active.");
    return;
  }
  
  Serial.print("Received command: "); Serial.println(command);
  char* cmd = strtok(command, ":");

  if (strcmp(cmd, "TEST2") == 0) {
    // Manual test for Motor 2 - runs at full speed
    Serial.println("=== TESTING MOTOR 2 ===");
    digitalWrite(L298N_IN3_PIN, HIGH);
    digitalWrite(L298N_IN4_PIN, LOW);
    analogWrite(L298N_ENB_PIN, 255); // Full speed
    Serial.println("Motor 2 should be running at FULL SPEED");
    Serial.print("IN3: "); Serial.println(digitalRead(L298N_IN3_PIN));
    Serial.print("IN4: "); Serial.println(digitalRead(L298N_IN4_PIN));
    return;
  }

  if (strcmp(cmd, "START") == 0) {
    char* patientID = strtok(NULL, ":");
    char* speedStr = strtok(NULL, ":");
    char* volumeStr = strtok(NULL, ":");
    
    if (patientID != NULL && speedStr != NULL && volumeStr != NULL) {
      currentMode = REMOTE;
      int speed = atoi(speedStr);
      float volume = atof(volumeStr);
      
      if (strcmp(patientID, "PT12345") == 0) {
        startMotor1(speed, volume);
        Serial.println("Started Pump 1 for PT12345.");
      } else if (strcmp(patientID, "PT6789") == 0 || strcmp(patientID, "PT67890") == 0) {
        startMotor2(speed, volume);
        Serial.println("Started Pump 2 for PT6789/PT67890.");
      } else {
        Serial.print("Unknown Patient ID: "); Serial.println(patientID);
        currentMode = IDLE;
      }
    } else {
      Serial.println("ERROR: Invalid START command format.");
    }
  } else if (strcmp(cmd, "STOP") == 0) {
    currentMode = IDLE;
    stopAllMotors();
    sendDataToPC(); // Send immediate status update
    Serial.println("REMOTE mode stopped by command.");
  } else if (strcmp(cmd, "SPEED") == 0) {
    if (currentMode == REMOTE) {
      char* speedStr = strtok(NULL, ":");
      if (speedStr != NULL) {
        int newSpeed = atoi(speedStr);
        adjustActiveMotorSpeed(newSpeed);
        Serial.print("Speed adjusted to: "); Serial.println(motorSpeed);
      } else {
        Serial.println("ERROR: Invalid SPEED command format.");
      }
    } else {
      Serial.println("ERROR: SPEED command only works in REMOTE mode.");
    }
  } else if (strcmp(cmd, "CAL") == 0) {
    // Calibration command: CAL:450.0
    char* calStr = strtok(NULL, ":");
    if (calStr != NULL) {
      float newCal = atof(calStr);
      if (newCal > 0) {
        FLOW_CALIBRATION_FACTOR = newCal;
        Serial.print("Calibration factor updated to: "); Serial.println(FLOW_CALIBRATION_FACTOR, 1);
      } else {
        Serial.println("ERROR: Calibration must be > 0");
      }
    } else {
      Serial.print("Current calibration factor: "); Serial.println(FLOW_CALIBRATION_FACTOR, 1);
    }
  } else {
    Serial.println("Unknown command.");
  }
}