#!/bin/bash

# ============================================
# SCRIPT ALL-IN-ONE - Déploiement DPS Complet
# ============================================
# Ce script fait TOUT en une seule commande :
# 1. Déploie l'infrastructure (RG, IoT Hub, DPS)
# 2. Crée l'enrollment group (clé symétrique)
# 3. Génère les clés devices
# 4. Sauvegarde tout dans un fichier
# 5. Met à jour automatiquement secrets.h
#
# Usage:
#   ./deploy-dps.sh

set -e

# ============================================
# CONFIGURATION
# ============================================

PROJECT_NAME="iotdetector"
ENVIRONMENT="dev"
LOCATION="germanywestcentral"
ENROLLMENT_GROUP_NAME="auto-provisioning-group"

RG_NAME="rg-${PROJECT_NAME}-${ENVIRONMENT}"
IOT_HUB_NAME="iot-${PROJECT_NAME}-${ENVIRONMENT}"
DPS_NAME="dps-${PROJECT_NAME}-${ENVIRONMENT}"

# ============================================
# FONCTION : Dérivation de clé device
# ============================================

derive_device_key() {
    local group_key=$1
    local registration_id=$2
    
    # HMAC-SHA256(groupKey, registrationId) en base64
    echo -n "$registration_id" | openssl dgst -sha256 -hmac "$(echo "$group_key" | base64 -d)" -binary | base64
}

# ============================================
# BANNIÈRE
# ============================================

echo ""
echo "============================================"
echo "🚀 DÉPLOIEMENT DPS COMPLET - ALL-IN-ONE"
echo "============================================"
echo ""
echo "Configuration :"
echo "  - Projet      : $PROJECT_NAME"
echo "  - Environment : $ENVIRONMENT"
echo "  - Région      : $LOCATION"
echo "  - RG          : $RG_NAME"
echo "  - IoT Hub     : $IOT_HUB_NAME"
echo "  - DPS         : $DPS_NAME"
echo ""

# ============================================
# ÉTAPE 1 : Vérification Azure CLI
# ============================================

echo "📋 Étape 1/8 : Vérification Azure CLI..."

if ! command -v az &> /dev/null; then
    echo "❌ Azure CLI n'est pas installé"
    echo "👉 Installer depuis : https://aka.ms/installazurecli"
    exit 1
fi

echo "✅ Azure CLI détecté"
if ! az account show &> /dev/null; then
    echo "❌ Non connecté à Azure"
    echo "👉 Exécuter : az login"
    exit 1
fi

SUBSCRIPTION_NAME=$(az account show --query name -o tsv)
echo "✅ Connecté à : $SUBSCRIPTION_NAME"
echo ""

# ============================================
# ÉTAPE 2 : Resource Group
# ============================================

echo "📋 Étape 2/8 : Vérification Resource Group..."

RG_EXISTS=$(az group exists --name "$RG_NAME" --output tsv)

if [ "$RG_EXISTS" = "true" ]; then
    echo "✅ Resource Group existe : $RG_NAME"
else
    echo "🔧 Création du Resource Group..."
    az group create \
      --name "$RG_NAME" \
      --location "$LOCATION" \
      --output none
    echo "✅ Resource Group créé"
fi
echo ""

# ============================================
# ÉTAPE 3 : IoT Hub
# ============================================

echo "📋 Étape 3/8 : Vérification IoT Hub..."

IOT_HUB_CHECK=$(az iot hub show \
  --name "$IOT_HUB_NAME" \
  --resource-group "$RG_NAME" \
  --query "name" -o tsv 2>/dev/null || echo "NOT_FOUND")

if [ "$IOT_HUB_CHECK" != "NOT_FOUND" ]; then
    echo "✅ IoT Hub existe : $IOT_HUB_NAME"
    CREATE_IOT_HUB=false
else
    echo "⚠️  IoT Hub n'existe pas, il sera créé"
    CREATE_IOT_HUB=true
fi
echo ""

# ============================================
# ÉTAPE 4 : Annulation déploiements actifs
# ============================================

echo "📋 Étape 4/8 : Vérification déploiements actifs..."

RUNNING_DEPLOYMENTS=$(az deployment group list \
  --resource-group "$RG_NAME" \
  --query "[?properties.provisioningState=='Running'].name" -o tsv 2>/dev/null || echo "")

if [ -n "$RUNNING_DEPLOYMENTS" ]; then
    echo "⚠️  Annulation des déploiements actifs..."
    for deployment in $RUNNING_DEPLOYMENTS; do
        echo "   - Annulation : $deployment"
        az deployment group cancel \
          --resource-group "$RG_NAME" \
          --name "$deployment" 2>/dev/null || true
    done
    echo "⏳ Attente 10 secondes..."
    sleep 10
fi
echo ""

# ============================================
# ÉTAPE 5 : Déploiement Bicep
# ============================================

echo "📋 Étape 5/8 : Déploiement infrastructure..."
echo "⏳ 3-5 minutes..."
echo ""

DEPLOYMENT_NAME="deploy-dps-$(date +%Y%m%d-%H%M%S)"

az deployment sub create \
  --name "$DEPLOYMENT_NAME" \
  --location "$LOCATION" \
  --template-file main.bicep \
  --parameters projectName="$PROJECT_NAME" \
               environment="$ENVIRONMENT" \
               location="$LOCATION" \
               createNewIotHub=$CREATE_IOT_HUB \
  --output none

if [ $? -eq 0 ]; then
    echo "✅ Déploiement réussi !"
else
    echo "❌ Échec du déploiement Bicep"
    exit 1
fi
echo ""

# ============================================
# ÉTAPE 5B : Création de l'enrollment group
# ============================================

echo "📋 Étape 5B/8 : Vérification / création de l'enrollment group avec clé symétrique..."

EXISTING_GROUP=$(az iot dps enrollment-group show \
  --dps-name "$DPS_NAME" \
  --resource-group "$RG_NAME" \
  --enrollment-id "$ENROLLMENT_GROUP_NAME" \
  --query "enrollmentGroupId" -o tsv 2>/dev/null || echo "")

if [ -n "$EXISTING_GROUP" ]; then
  echo "✅ Enrollment group déjà existant : $ENROLLMENT_GROUP_NAME"
else
  echo "🔧 Création de l'enrollment group (attestation = symmetricKey)..."
  az iot dps enrollment-group create \
    --dps-name "$DPS_NAME" \
    --resource-group "$RG_NAME" \
    --enrollment-id "$ENROLLMENT_GROUP_NAME" \
    --provisioning-status enabled \
    --iot-hubs "$IOT_HUB_NAME.azure-devices.net" \
    --allocation-policy static \
    --attestation-type symmetricKey \
    --output none
  echo "✅ Enrollment group créé avec attestation par clé symétrique"
fi
echo ""

# ============================================
# ÉTAPE 6 : Extraction Outputs
# ============================================

echo "📋 Étape 6/8 : Récupération des informations..."

DPS_ID_SCOPE=$(az deployment sub show \
  --name "$DEPLOYMENT_NAME" \
  --query "properties.outputs.dpsIdScope.value" -o tsv)

DPS_ENDPOINT=$(az deployment sub show \
  --name "$DEPLOYMENT_NAME" \
  --query "properties.outputs.dpsEndpoint.value" -o tsv)

IOT_HUB_HOSTNAME=$(az deployment sub show \
  --name "$DEPLOYMENT_NAME" \
  --query "properties.outputs.iotHubHostName.value" -o tsv)

PRIMARY_KEY=$(az iot dps enrollment-group show \
  --dps-name "$DPS_NAME" \
  --resource-group "$RG_NAME" \
  --enrollment-id "$ENROLLMENT_GROUP_NAME" \
  --show-keys \
  --query 'attestation.symmetricKey.primaryKey' -o tsv)

echo "✅ Outputs récupérés"
echo ""

# ============================================
# ÉTAPE 7 : Génération des clés devices
# ============================================

echo "🔐 Génération des clés devices..."
echo ""

ESP32_REG_ID="esp32-pir-01"
ESP32_DEVICE_KEY=$(derive_device_key "$PRIMARY_KEY" "$ESP32_REG_ID")

PHOTON2_REG_ID="photon2-pir-01"
PHOTON2_DEVICE_KEY=$(derive_device_key "$PRIMARY_KEY" "$PHOTON2_REG_ID")

echo "─────────────────────────────────────────"
echo "📱 ESP32 PIR Sensor"
echo "─────────────────────────────────────────"
echo "Registration ID : $ESP32_REG_ID"
echo "Device Key      : $ESP32_DEVICE_KEY"
echo ""
echo "📱 Photon2 PIR Sensor"
echo "─────────────────────────────────────────"
echo "Registration ID : $PHOTON2_REG_ID"
echo "Device Key      : $PHOTON2_DEVICE_KEY"
echo ""

# ============================================
# RÉSUMÉ FINAL
# ============================================

echo "============================================"
echo "✅ DÉPLOIEMENT COMPLET TERMINÉ !"
echo "============================================"
echo ""
echo "📝 CREDENTIALS POUR LE FIRMWARE :"
echo "-------------------------------------------"
echo "DPS_ID_SCOPE       : $DPS_ID_SCOPE"
echo "DPS_ENDPOINT       : $DPS_ENDPOINT"
echo "GROUP_PRIMARY_KEY  : $PRIMARY_KEY"
echo "IOT_HUB_HOSTNAME   : $IOT_HUB_HOSTNAME"
echo ""

# ============================================
# SAUVEGARDE
# ============================================

cat > dps-credentials.env << EOF
# ============================================
# DPS Credentials - Generated: $(date)
# ============================================

# DPS Configuration
DPS_ID_SCOPE=$DPS_ID_SCOPE
DPS_ENDPOINT=$DPS_ENDPOINT
DPS_GLOBAL_ENDPOINT=global.azure-devices-provisioning.net

# Enrollment Group
ENROLLMENT_GROUP_NAME=$ENROLLMENT_GROUP_NAME
GROUP_PRIMARY_KEY=$PRIMARY_KEY

# IoT Hub
IOT_HUB_HOSTNAME=$IOT_HUB_HOSTNAME

# Azure Resources
RESOURCE_GROUP=$RG_NAME
IOT_HUB_NAME=$IOT_HUB_NAME
DPS_NAME=$DPS_NAME

# ============================================
# DEVICE CREDENTIALS
# ============================================

ESP32_PIR_01_REGISTRATION_ID=$ESP32_REG_ID
ESP32_PIR_01_DEVICE_KEY=$ESP32_DEVICE_KEY

PHOTON2_PIR_01_REGISTRATION_ID=$PHOTON2_REG_ID
PHOTON2_PIR_01_DEVICE_KEY=$PHOTON2_DEVICE_KEY
EOF

echo "💾 Credentials sauvegardés : dps-credentials.env"
echo ""

# ============================================
# MISE À JOUR AUTOMATIQUE DE secrets.h
# ============================================

SECRETS_H_PATH="../firmware/esp32-pir-sensor/ESP32/include/secrets.h"

if [ -f "$SECRETS_H_PATH" ]; then
    echo "🔧 Mise à jour automatique de secrets.h..."
    cp "$SECRETS_H_PATH" "${SECRETS_H_PATH}.backup"

    # Utilisation du délimiteur | pour éviter les erreurs sur les clés contenant /
    sed -i "s|#define DPS_ID_SCOPE \".*\"|#define DPS_ID_SCOPE \"$DPS_ID_SCOPE\"|" "$SECRETS_H_PATH"
    sed -i "s|#define REGISTRATION_ID \".*\"|#define REGISTRATION_ID \"$ESP32_REG_ID\"|" "$SECRETS_H_PATH"
    sed -i "s|#define DEVICE_KEY \".*\"|#define DEVICE_KEY \"$ESP32_DEVICE_KEY\"|" "$SECRETS_H_PATH"

    echo "✅ secrets.h mis à jour automatiquement"
    echo "   Backup sauvegardé : ${SECRETS_H_PATH}.backup"
else
    echo "⚠️  secrets.h non trouvé à : $SECRETS_H_PATH"
    echo "   Veuillez copier manuellement les credentials"
fi

echo ""
echo "🎉 Infrastructure prête pour le firmware !"
echo ""
echo "📋 Prochaine étape : Compilation et téléversement du firmware"
echo "   cd ../firmware/esp32-pir-sensor/ESP32"
echo "   pio run --target upload"
echo ""
