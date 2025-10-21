// src/led_handler.cpp
#include "led_handler.h"

static int s_ledPin = -1;
static bool ledBlinking = false;
static bool ledState = false;
static unsigned long blinkStart = 0;
static unsigned long blinkInterval = 0;
static int blinkTarget = 0;
static int blinkCount  = 0;

void ledSetup(int ledPin) {
  if (ledPin < 0) {
    Serial.println("[LED] ⚠️ Invalid LED pin");
    return;
  }
  s_ledPin = ledPin;
  pinMode(s_ledPin, OUTPUT);
  digitalWrite(s_ledPin, LOW);
  Serial.printf("[LED] Initialized on GPIO %d\n", ledPin);
}

void startLedBlink(int count, unsigned long intervalMs) {
  // ✅ FIX: Validation des paramètres pour éviter race condition
  if (count <= 0 || intervalMs == 0) {
    Serial.println("[LED] ⚠️ Invalid blink parameters");
    return;
  }
  if (s_ledPin < 0) {
    Serial.println("[LED] ⚠️ LED not initialized");
    return;
  }
  
  blinkTarget = count;
  blinkCount  = 0;
  blinkInterval = intervalMs;
  ledBlinking = true;
  ledState = false;
  blinkStart = millis();
  digitalWrite(s_ledPin, LOW);
}

void updateLedBlink() {
  if (!ledBlinking || s_ledPin < 0) return;
  
  unsigned long now = millis();
  // ✅ FIX: Gestion correcte de l'overflow millis()
  if ((now - blinkStart) >= blinkInterval) {
    blinkStart = now;
    ledState = !ledState;
    digitalWrite(s_ledPin, ledState ? HIGH : LOW);
    
    if (!ledState) { // cycle complet (ON->OFF)
      blinkCount++;
      if (blinkCount >= blinkTarget) {
        ledBlinking = false;
        digitalWrite(s_ledPin, LOW);
      }
    }
  }
}
