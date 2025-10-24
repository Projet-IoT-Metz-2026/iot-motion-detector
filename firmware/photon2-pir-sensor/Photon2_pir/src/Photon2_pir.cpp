#include "Particle.h"
#include "ArduinoJson.h"
#include "EEPROM.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// --- Pins ---
constexpr int PIR_PIN = D2;
constexpr int LED_PIN = D7;

// -------- Persistance via EEPROM --------
const uint16_t CFG_MAGIC   = 0x42A5;
const uint16_t CFG_VERSION = 1;
const int      EEPROM_ADDR = 0;

struct Config {
  bool     detectionEnabled = true;
  uint32_t cooldownMs       = 3000;
  uint32_t detectionCount   = 0;
  uint16_t magic            = CFG_MAGIC;
  uint16_t version          = CFG_VERSION;
};
Config cfg;

void loadCfg() {
  Config tmp;
  EEPROM.get(EEPROM_ADDR, tmp);
  if (tmp.magic != CFG_MAGIC || tmp.version != CFG_VERSION) {
    cfg = Config();
    EEPROM.put(EEPROM_ADDR, cfg);
  } else {
    cfg = tmp;
  }
}
void saveCfg() { EEPROM.put(EEPROM_ADDR, cfg); }

// --- LED non-bloquante ---
bool ledBlinking=false, ledState=false;
uint32_t ledStart=0, ledInterval=0; int ledTarget=0, ledCount=0;
void startLedBlink(int n, uint32_t itv){
  ledTarget=n; ledInterval=itv; ledCount=0; ledBlinking=true; ledState=false; ledStart=millis();
  digitalWrite(LED_PIN, LOW);
}
void updateLedBlink(){
  if(!ledBlinking) return;
  if(millis()-ledStart>=ledInterval){
    ledStart=millis(); ledState=!ledState; digitalWrite(LED_PIN, ledState?HIGH:LOW);
    if(!ledState){ if(++ledCount>=ledTarget){ ledBlinking=false; digitalWrite(LED_PIN, LOW);} }
  }
}

// --- PIR ---
bool lastPir=LOW, inMotion=false;
uint32_t lastChange=0, lastValid=0;
constexpr uint32_t DEBOUNCE_MS=500;

// --- Buffer offline ---
struct Pending{ String name; String data; uint32_t ts; };
Vector<Pending> buffer;
constexpr size_t   MAX_BUFFER      = 50;
constexpr uint32_t BUFFER_RETRY_MS = 10000;
uint32_t lastBufferTry=0;

// --- Cloud helpers ---
int fnEnable(String){ cfg.detectionEnabled=true; saveCfg(); startLedBlink(1,200); return 1; }
int fnDisable(String){ cfg.detectionEnabled=false; saveCfg(); startLedBlink(3,100); return 1; }
int fnCooldown(String a){ int v=a.toInt(); if(v<1000||v>60000) return -1; cfg.cooldownMs=v; saveCfg(); return v; }
int fnGetStatus(String){
  StaticJsonDocument<256> d;
  d["firmware"]=System.version().c_str();
  d["uptime"]=millis()/1000;
  d["detectionEnabled"]=cfg.detectionEnabled;
  d["cooldownMs"]=cfg.cooldownMs;
  d["detectionCount"]=cfg.detectionCount;
  String out; serializeJson(d,out);
  Particle.publish("status",out,PRIVATE);
  return 1;
}
String varCooldown(){ return String(cfg.cooldownMs); }
String varEnabled(){ return cfg.detectionEnabled?"true":"false"; }
String varCount(){ return String(cfg.detectionCount); }

// --- Publication ---
void publishMotion(uint32_t count){
  StaticJsonDocument<256> d; d["event"]="motion"; d["count"]=count; d["ts"]=(int)Time.now();
  String payload; serializeJson(d,payload);
  if(!Particle.connected()){
    if(buffer.size()>=MAX_BUFFER) buffer.erase(0);
    buffer.append({"motion",payload,millis()}); return;
  }
  if(!Particle.publish("motion",payload,PRIVATE)){
    if(buffer.size()>=MAX_BUFFER) buffer.erase(0);
    buffer.append({"motion",payload,millis()});
  }
}
void tryFlushBuffer(){
  if(!Particle.connected()||buffer.isEmpty()) return;
  if(millis()-lastBufferTry<BUFFER_RETRY_MS) return;
  lastBufferTry=millis();
  size_t i=0;
  while(i<buffer.size()){
    auto &m = buffer[i];
    if(Particle.publish(m.name,m.data,PRIVATE)) buffer.erase(i);
    else i++;
  }
}

// --- Setup / Loop ---
void setup(){
  pinMode(LED_PIN,OUTPUT); digitalWrite(LED_PIN,LOW);
  pinMode(PIR_PIN,INPUT_PULLDOWN);
  loadCfg();

  Particle.variable("cooldownMs", varCooldown);
  Particle.variable("detEnabled", varEnabled);
  Particle.variable("detCount",   varCount);
  Particle.function("enable",  fnEnable);
  Particle.function("disable", fnDisable);
  Particle.function("cooldown",fnCooldown);
  Particle.function("getStatus",fnGetStatus);

  waitUntil(Particle.connected);
  Particle.syncTime();

  startLedBlink(3,200);
}

void loop(){
  updateLedBlink();

  bool cur = digitalRead(PIR_PIN);
  uint32_t now = millis();

  if(cur!=lastPir){
    if(now-lastChange>DEBOUNCE_MS){
      if(cur==HIGH && !inMotion){
        if((now-lastValid)>cfg.cooldownMs && cfg.detectionEnabled){
          cfg.detectionCount++; saveCfg();
          lastValid=now; inMotion=true; digitalWrite(LED_PIN,HIGH);
          publishMotion(cfg.detectionCount);
        }
      }
      if(cur==LOW && inMotion){
        inMotion=false; digitalWrite(LED_PIN,LOW);
      }
      lastChange=now;
    }
  }
  lastPir=cur;

  tryFlushBuffer();
  delayMicroseconds(100);
}
