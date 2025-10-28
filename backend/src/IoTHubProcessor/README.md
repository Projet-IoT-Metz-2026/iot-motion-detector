# IoTHubProcessor (Azure Function)

Emplacement : `backend/src/IoTHubProcessor`

But :
- Traiter les évènements entrants depuis Azure IoT Hub / Event Grid.
- Contenir la logique d'ingestion et tout traitement serveur lié aux détecteurs.

Fichiers clés :
- `Program.cs` : configuration et démarrage de l'application Functions.
- `IoTHubProcessor.csproj` : définition du projet et des dépendances.
- `local.settings.json` : configuration locale (ne doit pas être committée).

Build & exécution (PowerShell) :

1) Construire :

   dotnet build .

2) Exécuter localement (Functions Core Tools requis) :

   func host start

Conseils:
- Vérifier les packages dans `obj/` si vous rencontrez des problèmes de dépendances.
- Pour déployer sur Azure Functions, utilisez `dotnet publish --configuration Release` puis la méthode de déploiement choisie (Azure CLI / GitHub Actions / VS Publish).

Sécurité:
- Ne stockez pas de clés dans le code. Utilisez Azure Key Vault ou les paramètres d'application d'Azure Functions.
