# Incident de Sécurité - Exposition de Clés Azure DPS

## Date
24 octobre 2025, 19:41 UTC

## Description
GitGuardian a détecté l'exposition de clés Azure dans le commit `a0a3377` sur la branche `develop`.

## Clés Exposées
- **GROUP_PRIMARY_KEY** du DPS enrollment group (clé symétrique)
- **DEVICE_KEY** dérivées pour esp32-pir-01 et photon2-pir-01

## Impact
- Les clés exposées permettaient de provisionner des devices non autorisés via le DPS
- Accès potentiel à l'IoT Hub `iot-iotdetector-dev.azure-devices.net`

## Actions Correctives Prises

### 1. Régénération Immédiate des Clés
- ✅ Suppression de l'enrollment group `auto-provisioning-group`
- ✅ Recréation avec nouvelles clés symétriques
- ✅ Nouvelle PRIMARY_KEY générée : `na9UTLlpD6iG/F+RTMWWWhbXhwr38LhVNR3xpiNWIXmluychgWbmT0d6zHfaork+UXK2TBbIoTn3AIoTQg2D7g==`

### 2. Suppression des Devices Compromis
- ✅ Device `esp32-pir-01` supprimé de l'IoT Hub
- ✅ Re-provisionnement requis avec nouvelles clés

### 3. Correction du Repository
- ✅ Fichier `infrastructure/dps-credentials.env` retiré du tracking Git
- ✅ Le fichier est déjà couvert par `.gitignore` (`*.env`)
- ⚠️ Les anciennes clés restent dans l'historique Git (commits `a0a3377` et précédents)

### 4. Mise à Jour des Credentials Locaux
- ✅ Script `deploy-dps.sh` exécuté pour régénérer les credentials
- ✅ Fichier `secrets.h` mis à jour automatiquement
- ✅ Backup créé : `secrets.h.backup`

## Recommandations Futures

### Protection des Secrets
1. **NE JAMAIS** commiter de fichiers contenant des secrets :
   - `dps-credentials.env`
   - `secrets.h`
   - Fichiers `.env`
   - Certificats (`.pem`, `.der`, `.pfx`)

2. **Utiliser** Azure Key Vault pour stocker les secrets en production

3. **Rotation régulière** des clés (tous les 90 jours minimum)

### Monitoring
1. Activer Azure Security Center pour IoT
2. Configurer des alertes sur les connexions suspectes
3. Monitorer les tentatives de provisionnement échouées

## Timeline
- **19:30 UTC** : Commit `a0a3377` avec clés exposées pushed vers GitHub
- **19:35 UTC** : Alerte GitGuardian détectée
- **19:41 UTC** : Clés régénérées dans Azure
- **19:42 UTC** : Device supprimé de l'IoT Hub
- **19:43 UTC** : Fichier retiré du tracking Git
- **19:45 UTC** : Credentials locaux régénérés

## Status
🔒 **RÉSOLU** - Toutes les clés compromises ont été révoquées et régénérées.

## Contact
Pour toute question sur cet incident, contacter l'équipe DevSecOps.
