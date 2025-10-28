# Backend

Ce dossier contient le code serveur du projet : principalement le projet Azure Functions qui reçoit et traite la télémétrie IoT.

Structure importante :
- `src/IoTHubProcessor/` : Azure Function (C#/.NET) qui traite les messages IoT Hub.

À savoir / commandes utiles (PowerShell) :

- Construire le projet :

  dotnet build c:\Users\Miko\Desktop\projetm2\iot-motion-detector\backend\src\IoTHubProcessor

- Lancer localement (si vous avez l'Azure Functions Core Tools installé) :

  cd c:\Users\Miko\Desktop\projetm2\iot-motion-detector\backend\src\IoTHubProcessor ; func host start

Notes :
- `local.settings.json` contient des settings locaux (ignored par .gitignore). Ne pas committer de secrets.
- Le projet compile avec .NET 8 dans l'état actuel du dépôt (vérifié localement).
