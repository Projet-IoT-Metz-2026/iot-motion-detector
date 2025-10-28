# Firmware

Ce dossier contient le code embarqué pour les capteurs (ESP32, Photon2). Chaque sous-dossier correspond à un microcontrôleur/projet PlatformIO ou équivalent.

Structure :
- `esp32-pir-sensor/` : projet PlatformIO pour ESP32 (détecteur PIR).
- `photon2-pir-sensor/` : firmware pour Photon2 (si présent).

Commandes utiles (PlatformIO & général) :

- Pour builder/flasher ESP32 (depuis le dossier `ESP32`):

  pio run -e <env>    # build
  pio run -e <env> -t upload  # flasher

Remarques :
- Le code contient des handlers pour DPS et Azure IoT (ex : `dps_handler.cpp`, `azure_handler.cpp`).
- Les certificats racines nécessaires pour TLS sont référencés dans les sources — vérifiez les constantes CA.

Bonnes pratiques :
- Ne pas committer de credentials dans le firmware (les clés doivent être injectées au moment de la production ou via DPS). Utilisez DPS ou X.509 si possible.
