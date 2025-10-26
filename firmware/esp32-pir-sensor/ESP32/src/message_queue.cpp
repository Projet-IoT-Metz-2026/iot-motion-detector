// src/message_queue.cpp
#include "message_queue.h"
#include <LittleFS.h>

// ============================================
// VARIABLES GLOBALES
// ============================================

struct QueueMessage {
  char data[MESSAGE_MAX_LENGTH];
  bool valid;
};

static QueueMessage messageQueue[MESSAGE_QUEUE_MAX_SIZE];
static int queueHead = 0;  // Index du prochain message à lire
static int queueTail = 0;  // Index où insérer le prochain message
static int queueCount = 0; // Nombre de messages dans la file

static const char* QUEUE_FILE = "/littlefs/message_queue.dat";

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

void messageQueueInit() {
  Serial.println("[Queue] Initialisation de la file d'attente...");

  // Initialiser la file
  for (int i = 0; i < MESSAGE_QUEUE_MAX_SIZE; i++) {
    messageQueue[i].valid = false;
    messageQueue[i].data[0] = '\0';
  }

  queueHead = 0;
  queueTail = 0;
  queueCount = 0;

  // Charger les messages persistés
  if (messageQueueLoad()) {
    Serial.printf("[Queue] %d messages chargés depuis le stockage\n", queueCount);
  } else {
    Serial.println("[Queue] Aucun message persisté trouvé");
  }
}

bool messageQueuePush(const char* message) {
  if (message == nullptr || strlen(message) == 0) {
    Serial.println("[Queue] ❌ Message invalide");
    return false;
  }

  if (strlen(message) >= MESSAGE_MAX_LENGTH) {
    Serial.println("[Queue] ❌ Message trop long");
    return false;
  }

  // Si la file est pleine, supprimer le message le plus ancien
  if (queueCount >= MESSAGE_QUEUE_MAX_SIZE) {
    Serial.println("[Queue] ⚠️ File pleine, suppression du message le plus ancien");
    messageQueuePop();
  }

  // Ajouter le message
  strncpy(messageQueue[queueTail].data, message, MESSAGE_MAX_LENGTH - 1);
  messageQueue[queueTail].data[MESSAGE_MAX_LENGTH - 1] = '\0';
  messageQueue[queueTail].valid = true;

  queueTail = (queueTail + 1) % MESSAGE_QUEUE_MAX_SIZE;
  queueCount++;

  Serial.printf("[Queue] ✅ Message ajouté (%d/%d)\n", queueCount, MESSAGE_QUEUE_MAX_SIZE);

  // Sauvegarder sur LittleFS
  messageQueueSave();

  return true;
}

bool messageQueuePeek(char* buffer, size_t bufferSize) {
  if (queueCount == 0) {
    return false;
  }

  if (!messageQueue[queueHead].valid) {
    Serial.println("[Queue] ❌ Message invalide en tête de file");
    return false;
  }

  strncpy(buffer, messageQueue[queueHead].data, bufferSize - 1);
  buffer[bufferSize - 1] = '\0';

  return true;
}

bool messageQueuePop() {
  if (queueCount == 0) {
    return false;
  }

  messageQueue[queueHead].valid = false;
  messageQueue[queueHead].data[0] = '\0';

  queueHead = (queueHead + 1) % MESSAGE_QUEUE_MAX_SIZE;
  queueCount--;

  Serial.printf("[Queue] Message retiré (%d/%d restants)\n", queueCount, MESSAGE_QUEUE_MAX_SIZE);

  // Sauvegarder sur LittleFS
  messageQueueSave();

  return true;
}

int messageQueueSize() {
  return queueCount;
}

bool messageQueueIsEmpty() {
  return queueCount == 0;
}

bool messageQueueIsFull() {
  return queueCount >= MESSAGE_QUEUE_MAX_SIZE;
}

void messageQueueClear() {
  for (int i = 0; i < MESSAGE_QUEUE_MAX_SIZE; i++) {
    messageQueue[i].valid = false;
    messageQueue[i].data[0] = '\0';
  }

  queueHead = 0;
  queueTail = 0;
  queueCount = 0;

  Serial.println("[Queue] File d'attente vidée");

  // Supprimer le fichier persisté
  if (LittleFS.exists(QUEUE_FILE)) {
    LittleFS.remove(QUEUE_FILE);
  }
}

bool messageQueueSave() {
  if (!LittleFS.begin(true)) {
    Serial.println("[Queue] ❌ Échec montage LittleFS");
    return false;
  }

  File file = LittleFS.open(QUEUE_FILE, "w");
  if (!file) {
    Serial.println("[Queue] ❌ Échec ouverture fichier pour écriture");
    return false;
  }

  // Écrire l'en-tête
  file.write((uint8_t*)&queueHead, sizeof(queueHead));
  file.write((uint8_t*)&queueTail, sizeof(queueTail));
  file.write((uint8_t*)&queueCount, sizeof(queueCount));

  // Écrire les messages
  for (int i = 0; i < MESSAGE_QUEUE_MAX_SIZE; i++) {
    file.write((uint8_t*)&messageQueue[i].valid, sizeof(bool));
    if (messageQueue[i].valid) {
      file.write((uint8_t*)messageQueue[i].data, MESSAGE_MAX_LENGTH);
    }
  }

  file.close();
  return true;
}

bool messageQueueLoad() {
  if (!LittleFS.begin(true)) {
    Serial.println("[Queue] ❌ Échec montage LittleFS");
    return false;
  }

  if (!LittleFS.exists(QUEUE_FILE)) {
    return false;
  }

  File file = LittleFS.open(QUEUE_FILE, "r");
  if (!file) {
    Serial.println("[Queue] ❌ Échec ouverture fichier pour lecture");
    return false;
  }

  // Lire l'en-tête
  file.read((uint8_t*)&queueHead, sizeof(queueHead));
  file.read((uint8_t*)&queueTail, sizeof(queueTail));
  file.read((uint8_t*)&queueCount, sizeof(queueCount));

  // Vérifier la cohérence
  if (queueCount < 0 || queueCount > MESSAGE_QUEUE_MAX_SIZE) {
    Serial.printf("[Queue] ❌ Données corrompues (count=%d)\n", queueCount);
    file.close();
    return false;
  }

  // Lire les messages
  for (int i = 0; i < MESSAGE_QUEUE_MAX_SIZE; i++) {
    file.read((uint8_t*)&messageQueue[i].valid, sizeof(bool));
    if (messageQueue[i].valid) {
      file.read((uint8_t*)messageQueue[i].data, MESSAGE_MAX_LENGTH);
    }
  }

  file.close();
  return true;
}
