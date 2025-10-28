# Scripts

Emplacement : `scripts/`

Contenu :
- `clean-repo.ps1` : script PowerShell pour nettoyer artefacts locaux et simuler la suppression de fichiers sensibles (dry-run par défaut).

Usage (PowerShell) :

1) Pour voir ce que ferait le script (dry-run) :

   pwsh -File .\\scripts\\clean-repo.ps1 -WhatIf

2) Pour exécuter (attention, destructive) :

   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass; pwsh -File .\\scripts\\clean-repo.ps1

Notes :
- Le script est conçu pour être sûr par défaut mais vérifiez toujours les actions avant d'exécuter.
