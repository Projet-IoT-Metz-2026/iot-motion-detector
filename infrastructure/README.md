# Infrastructure (Bicep)

Emplacement : `infrastructure/`

But : déploiement des ressources Azure pour le projet (DPS, IoT Hub, etc.) à l'aide de Bicep.

Fichiers clés :
- `main.bicep` : orchestration principale qui inclut des modules.
- `modules/` : modules Bicep (ex : `iot-hub.bicep`, `iot-dps.bicep`).
- `parameters.json` : paramètres pour le déploiement.
- `dps-credentials.env.template` : template de fichier credentials (les secrets doivent rester hors repo).
- `SECURITY_INCIDENT.md` : historique / notes de sécurité (s'il existe).

Déploiement (exemple avec Azure CLI) :

1) S'assurer d'être connecté :

   az login

2) Déployer dans un groupe de ressources :

   az deployment group create --resource-group <RG_NAME> --template-file main.bicep --parameters @parameters.json

Remarques et sécurité :
- Évitez d'utiliser `listKeys()` dans Bicep pour propager des clés entre ressources (exposition de secrets). Préférez Managed Identity / Key Vault ou un post-deploy script sécurisé.
- `dps-credentials.env.template` est un template — ne commitez jamais le fichier réel contenant des clés.
