// include/message_queue.h
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <Arduino.h>

/**
 * Taille maximale de la file d'attente
 */
static const int MESSAGE_QUEUE_MAX_SIZE = 50;

/**
 * Taille maximale d'un message
 */
static const int MESSAGE_MAX_LENGTH = 512;

/**
 * Initialise la file d'attente des messages
 * Charge les messages persistés depuis LittleFS si disponibles
 */
void messageQueueInit();

/**
 * Ajoute un message à la file d'attente
 * Si la file est pleine, le message le plus ancien est supprimé
 *
 * @param message Message JSON à ajouter
 * @return true si le message a été ajouté, false en cas d'erreur
 */
bool messageQueuePush(const char* message);

/**
 * Récupère le prochain message de la file sans le retirer
 *
 * @param buffer Buffer pour stocker le message
 * @param bufferSize Taille du buffer
 * @return true si un message est disponible, false sinon
 */
bool messageQueuePeek(char* buffer, size_t bufferSize);

/**
 * Retire le prochain message de la file
 *
 * @return true si un message a été retiré, false sinon
 */
bool messageQueuePop();

/**
 * Retourne le nombre de messages en attente
 */
int messageQueueSize();

/**
 * Vérifie si la file est vide
 */
bool messageQueueIsEmpty();

/**
 * Vérifie si la file est pleine
 */
bool messageQueueIsFull();

/**
 * Vide complètement la file d'attente
 */
void messageQueueClear();

/**
 * Sauvegarde la file d'attente sur LittleFS
 */
bool messageQueueSave();

/**
 * Charge la file d'attente depuis LittleFS
 */
bool messageQueueLoad();

#endif // MESSAGE_QUEUE_H
