#!/usr/bin/env node

/**
 * ============================================
 * Script de Dérivation des Clés Device
 * ============================================
 * Ce script génère les clés symétriques individuelles
 * pour chaque device à partir de la clé du groupe.
 * 
 * Algorithme : HMAC-SHA256(groupPrimaryKey, registrationId) → base64
 * 
 * Usage:
 *   node derive-device-keys.js <GROUP_PRIMARY_KEY>
 * 
 * Ou avec les credentials du fichier .env :
 *   node derive-device-keys.js $(grep GROUP_PRIMARY_KEY dps-credentials.env | cut -d= -f2)
 */

const crypto = require('crypto');

// ============================================
// CONFIGURATION
// ============================================

const DEVICES = [
  { name: 'ESP32 PIR Sensor', registrationId: 'esp32-pir-01' },
  { name: 'Photon2 PIR Sensor', registrationId: 'photon2-pir-01' }
];

// ============================================
// FONCTION : Dérivation de clé
// ============================================

/**
 * Dérive une clé device à partir de la clé du groupe
 * @param {string} groupKey - Clé primaire du groupe (base64)
 * @param {string} registrationId - ID d'enregistrement du device
 * @returns {string} - Clé device dérivée (base64)
 */
function deriveDeviceKey(groupKey, registrationId) {
  const groupKeyBuffer = Buffer.from(groupKey, 'base64');
  const hmac = crypto.createHmac('sha256', groupKeyBuffer);
  hmac.update(registrationId, 'utf8');
  return hmac.digest('base64');
}

// ============================================
// MAIN
// ============================================

function main() {
  console.log('============================================');
  console.log('🔐 DÉRIVATION DES CLÉS DEVICE');
  console.log('============================================\n');

  // Vérification de l'argument
  if (process.argv.length < 3) {
    console.error('❌ Erreur : clé du groupe manquante\n');
    console.log('Usage:');
    console.log('  node derive-device-keys.js <GROUP_PRIMARY_KEY>');
    console.log('\nExemple:');
    console.log('  node derive-device-keys.js "abc123def456..."');
    console.log('\nOu depuis le fichier .env:');
    console.log('  node derive-device-keys.js $(grep GROUP_PRIMARY_KEY dps-credentials.env | cut -d= -f2)');
    process.exit(1);
  }

  const groupPrimaryKey = process.argv[2].trim();

  // Validation basique
  if (groupPrimaryKey.length < 20) {
    console.error('❌ Erreur : la clé semble invalide (trop courte)');
    process.exit(1);
  }

  console.log('📋 Clé du groupe:');
  console.log(`   ${groupPrimaryKey.substring(0, 20)}... (${groupPrimaryKey.length} caractères)\n`);

  console.log('🔧 Génération des clés devices...\n');

  // Génération des clés pour chaque device
  DEVICES.forEach(device => {
    try {
      const deviceKey = deriveDeviceKey(groupPrimaryKey, device.registrationId);
      
      console.log('─────────────────────────────────────────');
      console.log(`📱 ${device.name}`);
      console.log('─────────────────────────────────────────');
      console.log(`Registration ID : ${device.registrationId}`);
      console.log(`Device Key      : ${deviceKey}`);
      console.log('');
    } catch (error) {
      console.error(`❌ Erreur lors de la génération pour ${device.name}:`, error.message);
    }
  });

  console.log('============================================');
  console.log('✅ CLÉS GÉNÉRÉES AVEC SUCCÈS');
  console.log('============================================\n');

  console.log('📝 Prochaines étapes:');
  console.log('  1. Copier la Device Key dans votre firmware');
  console.log('  2. Configurer les paramètres DPS :');
  console.log('     - DPS_ID_SCOPE');
  console.log('     - DPS_ENDPOINT (global.azure-devices-provisioning.net)');
  console.log('     - REGISTRATION_ID');
  console.log('     - DEVICE_KEY (générée ci-dessus)');
  console.log('  3. Flasher le firmware sur vos devices');
  console.log('');
}

// ============================================
// EXÉCUTION
// ============================================

main();