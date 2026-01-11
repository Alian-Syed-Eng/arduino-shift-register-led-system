#include <LiquidCrystal.h>


// --------------------
// Shift register (SN74HC595)
const int SR_DATA  = 11;  // SER / DS  (pin 14)
const int SR_LATCH = 10;  // RCLK / ST_CP (pin 12)
const int SR_CLOCK = 13;  // SRCLK / SH_CP (pin 11)

// Ultrasonic sensor (HC-SR04)
const int TRIG_PIN = 8;
const int ECHO_PIN = 7;

// Buzzer + Mute button
const int BUZZER_PIN = A0;   // buzzer to A0, buzzer to GND
const int MUTE_BTN   = 6;    // button to GND, 

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 9, 5, 4, 3, 2);

const int STOP_CM     = 10;   // 
const int CAUTION_CM  = 25;   // 
const int FAR_SILENT  = 60;   // 
const int NO_SIGNAL   = 999;  // 

// Button debounce
bool muted = false;
bool lastStableBtn = HIGH;
bool lastReadingBtn = HIGH;
unsigned long lastDebounceT0 = 0;
const unsigned long DEBOUNCE_MS = 35;

// Buzzer timing
unsigned long lastBeepTime = 0;
bool beepOn = false;



int readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long d = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms
  if (d == 0) return NO_SIGNAL;
  return (int)(d * 0.034 / 2.0); // cm
}

byte ledBarFromDistance(int cm) {

  if (cm == NO_SIGNAL) return B00000000;

  if (cm < 5)       return B11111111;
  else if (cm < 10) return B01111111;
  else if (cm < 15) return B00111111;
  else if (cm < 20) return B00011111;
  else if (cm < 25) return B00001111;
  else if (cm < 30) return B00000111;
  else if (cm < 35) return B00000011;
  else              return B00000001;
}

const char* statusFromDistance(int cm) {
  if (cm == NO_SIGNAL) return "NO SIGNAL";
  if (cm <= STOP_CM)   return "STOP";
  if (cm <= CAUTION_CM) return "CAUTION";
  return "SAFE";
}

void writeShiftRegister(byte value) {
  digitalWrite(SR_LATCH, LOW);
  shiftOut(SR_DATA, SR_CLOCK, MSBFIRST, value);
  digitalWrite(SR_LATCH, HIGH);
}

void updateMuteToggle() {
  bool reading = digitalRead(MUTE_BTN);

  if (reading != lastReadingBtn) {
    lastDebounceT0 = millis();
    lastReadingBtn = reading;
  }

  if (millis() - lastDebounceT0 > DEBOUNCE_MS) {
    if (reading != lastStableBtn) {
      lastStableBtn = reading;

     
      if (lastStableBtn == LOW) {
        muted = !muted;
      }
    }
  }
}

void buzzerFromDistance(int cm) {
  
  if (muted) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    beepOn = false;
    return;
  }

  // No signal or far away so silent
  if (cm == NO_SIGNAL || cm > FAR_SILENT) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    beepOn = false;
    return;
  }

  // STOP
  if (cm <= STOP_CM) {
    tone(BUZZER_PIN, 2000);
    return;
  }

  // Beep Interval
  int interval = map(cm, FAR_SILENT, STOP_CM, 450, 80);
  interval = constrain(interval, 80, 450);

  unsigned long now = millis();
  if (now - lastBeepTime >= (unsigned long)interval) {
    lastBeepTime = now;
    beepOn = !beepOn;

    if (beepOn) {
      tone(BUZZER_PIN, 1500);    
      digitalWrite(BUZZER_PIN, HIGH);   
    } else {
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

void updateLCD(int cm) {
  
  lcd.setCursor(0, 0);
  lcd.print("Dist:");
  lcd.setCursor(5, 0);
  lcd.print("     "); // clear area
  lcd.setCursor(5, 0);

  if (cm == NO_SIGNAL) {
    lcd.print("--");
  } else {
    lcd.print(cm);
    lcd.print("cm");
  }

  lcd.setCursor(12, 0);
  if (muted) lcd.print("MUTE");
  else       lcd.print("    ");

  
  lcd.setCursor(0, 1);
  lcd.print("Status:");
  lcd.setCursor(7, 1);
  lcd.print("        "); // clear area
  lcd.setCursor(7, 1);
  lcd.print(statusFromDistance(cm));
}


void setup() {
  // Shift register pins
  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Buzzer and button
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MUTE_BTN, INPUT_PULLUP);

  // LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Parking Assist");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(700);

  // Start with outputs off
  writeShiftRegister(B00000000);
  noTone(BUZZER_PIN);
}

void loop() {
  updateMuteToggle();

  int cm = readDistanceCm();

  // LED bar
  writeShiftRegister(ledBarFromDistance(cm));

  // LCD
  updateLCD(cm);

  // Buzzer
  buzzerFromDistance(cm);

  delay(50);
}
