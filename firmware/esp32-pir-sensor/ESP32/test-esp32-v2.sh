#!/bin/bash

# Script de test ESP32 v2.0 pour Git Bash (Windows)
# Usage: ./test-esp32-v2.sh

# Configuration
HUB_NAME="iot-detector-am2025"
DEVICE_ID="esp32-pir-01"

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════╗"
echo "║   🧪 TEST ESP32 FIRMWARE v2.0        ║"
echo "╚═══════════════════════════════════════╝"
echo -e "${NC}"

# Fonction pour envoyer une commande
send_command() {
    local cmd=$1
    local desc=$2
    
    echo -e "${YELLOW}📤 $desc${NC}"
    az iot device c2d-message send \
      --hub-name $HUB_NAME \
      --device-id $DEVICE_ID \
      --data "$cmd" \
      --output none 2>/dev/null
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Commande envoyée${NC}\n"
    else
        echo -e "${RED}❌ Erreur lors de l'envoi${NC}\n"
    fi
}

# Fonction d'attente
wait_seconds() {
    local seconds=$1
    local message=$2
    
    if [ -n "$message" ]; then
        echo -e "${CYAN}$message${NC}"
    fi
    
    echo -ne "${BLUE}⏳ Attente de $seconds secondes...${NC}"
    for ((i=$seconds; i>0; i--)); do
        echo -ne "\r${BLUE}⏳ $i secondes restantes...   ${NC}"
        sleep 1
    done
    echo -e "\r${GREEN}✓ Terminé!                    ${NC}\n"
}

# TEST 1: Connexion
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}TEST 1 : Vérification connexion${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}\n"

send_command '{"command":"getStatus"}' "Demande statut système"
wait_seconds 3

# TEST 2: ArduinoJson - Changement cooldown
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}TEST 2 : Changement cooldown (ArduinoJson)${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}\n"

send_command '{"command":"setCooldown","value":10000}' "Cooldown à 10 secondes"
wait_seconds 3

send_command '{"command":"getStatus"}' "Vérification nouveau cooldown"
wait_seconds 3

echo -e "${YELLOW}🔍 Vérifie les logs ESP32:${NC}"
echo -e "${YELLOW}   Le JSON doit contenir 'cooldown':10000${NC}"
wait_seconds 3

# TEST 3: Structures - Enable/Disable
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}TEST 3 : Structures (Enable/Disable)${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}\n"

send_command '{"command":"disable"}' "Désactivation détection"
wait_seconds 5

echo -e "${YELLOW}👋 TEST MANUEL : Bouge devant le PIR...${NC}"
echo -e "${YELLOW}   ➜ Il ne devrait PAS détecter${NC}"
wait_seconds 10

send_command '{"command":"enable"}' "Réactivation détection"
wait_seconds 3

echo -e "${YELLOW}👋 TEST MANUEL : Bouge devant le PIR...${NC}"
echo -e "${YELLOW}   ➜ Il DOIT détecter maintenant${NC}"
wait_seconds 10

# TEST 4: EEPROM (CRITIQUE!)
echo -e "${MAGENTA}═══════════════════════════════════════${NC}"
echo -e "${MAGENTA}TEST 4 : EEPROM ⭐ CRITIQUE${NC}"
echo -e "${MAGENTA}═══════════════════════════════════════${NC}\n"

send_command '{"command":"setCooldown","value":15000}' "Cooldown à 15 secondes"
wait_seconds 3

echo -e "${CYAN}💾 La config doit être sauvegardée en EEPROM${NC}"
wait_seconds 2

echo -e "${YELLOW}🔄 REDÉMARRAGE de l'ESP32...${NC}"
send_command '{"command":"reboot"}' "Commande reboot"

echo -e "${RED}⚠️  L'ESP32 va redémarrer...${NC}"
wait_seconds 25 "Attente redémarrage complet"

echo -e "${CYAN}📊 Vérification persistance...${NC}"
send_command '{"command":"getStatus"}' "Statut après reboot"
wait_seconds 3

echo -e "${MAGENTA}"
echo "╔════════════════════════════════════════╗"
echo "║  🔍 VÉRIFICATION CRITIQUE TEST 4 :    ║"
echo "║                                        ║"
echo "║  Dans les logs ESP32, tu DOIS voir :  ║"
echo "║  [CONFIG] ✅ Chargé: cooldown=15000   ║"
echo "║                                        ║"
echo "║  Si 15000 → ✅ EEPROM fonctionne !    ║"
echo "║  Si 5000  → ❌ EEPROM ne marche pas   ║"
echo "╚════════════════════════════════════════╝"
echo -e "${NC}\n"
wait_seconds 5

# TEST 5: Nouvelles métriques
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}TEST 5 : Nouvelles métriques${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}\n"

send_command '{"command":"getStatus"}' "Statut complet"
wait_seconds 3

echo -e "${YELLOW}🔍 Vérifie sur Azure les champs :${NC}"
echo -e "${CYAN}   • wifiReconnects${NC}"
echo -e "${CYAN}   • mqttReconnects${NC}"
echo -e "${CYAN}   • failedPublishes${NC}\n"
wait_seconds 3

# TEST 6: Device Twin
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}TEST 6 : Device Twin + EEPROM${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}\n"

send_command '{"command":"getTwin"}' "Demande Device Twin"
wait_seconds 3

echo -e "${CYAN}📝 Modification Device Twin depuis Azure...${NC}"
az iot hub device-twin update \
  --hub-name $HUB_NAME \
  --device-id $DEVICE_ID \
  --set properties.desired='{"detectionEnabled":true,"cooldown":8000}' \
  --output none 2>/dev/null

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Device Twin mis à jour${NC}\n"
else
    echo -e "${RED}❌ Erreur mise à jour Twin${NC}\n"
fi

wait_seconds 5 "Attente réception par l'ESP32"

echo -e "${YELLOW}🔍 Vérifie les logs ESP32 :${NC}"
echo -e "${CYAN}   [TWIN] 📨 TWIN DESIRED PATCH REÇU !${NC}"
echo -e "${CYAN}   [TWIN] cooldown: xxx → 8000 ms${NC}"
echo -e "${CYAN}   [CONFIG] ✅ Sauvegardé en EEPROM${NC}\n"
wait_seconds 3

# TEST 7: Réinitialisation
echo -e "${GREEN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}TEST 7 : Réinitialisation${NC}"
echo -e "${GREEN}═══════════════════════════════════════${NC}\n"

send_command '{"command":"setCooldown","value":5000}' "Retour cooldown 5s"
wait_seconds 2

send_command '{"command":"enable"}' "Activation détection"
wait_seconds 2

# RÉSUMÉ FINAL
echo -e "\n${BLUE}"
echo "╔═══════════════════════════════════════╗"
echo "║      ✅ TESTS TERMINÉS !              ║"
echo "╚═══════════════════════════════════════╝"
echo -e "${NC}\n"

echo -e "${YELLOW}📋 CHECKLIST DE VALIDATION :${NC}\n"

echo -e "${NC}┌────────────────────────────────────┐${NC}"
echo -e "${NC}│ ✓ TEST 1 : Connexion               │${NC}"
echo -e "${NC}│ ✓ TEST 2 : ArduinoJson             │${NC}"
echo -e "${NC}│ ✓ TEST 3 : Enable/Disable          │${NC}"
echo -e "${CYAN}│ ⭐ TEST 4 : EEPROM (critique)      │${NC}"
echo -e "${NC}│ ✓ TEST 5 : Métriques               │${NC}"
echo -e "${CYAN}│ ⭐ TEST 6 : Twin + EEPROM          │${NC}"
echo -e "${NC}│ ✓ TEST 7 : Réinitialisation        │${NC}"
echo -e "${NC}└────────────────────────────────────┘${NC}\n"

echo -e "${YELLOW}🎯 LES 3 TESTS CRITIQUES :${NC}\n"
echo -e "${CYAN}   1. TEST 4: cooldown=15000 persiste après reboot ?${NC}"
echo -e "${CYAN}   2. TEST 5: wifiReconnects/mqttReconnects présents ?${NC}"
echo -e "${CYAN}   3. TEST 6: Twin appliqué ET sauvegardé ?${NC}\n"

echo -e "${YELLOW}📚 Commandes utiles :${NC}\n"
echo -e "${NC}   • Logs ESP32  : pio device monitor${NC}"
echo -e "${NC}   • Azure events: az iot hub monitor-events --hub-name $HUB_NAME --device-id $DEVICE_ID${NC}\n"

echo -e "${GREEN}✨ Si les 3 tests critiques passent, v2.0 est PARFAIT ! ✨${NC}\n"

read -p "Ouvrir le moniteur série ? (o/n) " response
if [[ "$response" == "o" || "$response" == "O" ]]; then
    echo -e "\n${GREEN}📺 Ouverture moniteur série...${NC}\n"
    pio device monitor
fi