// ============================================
// MAIN DEPLOYMENT - IoT Motion Detector
// ============================================
// Déploiement orchestré :
// 1. Resource Group (ou référence existant)
// 2. IoT Hub (référence existant)
// 3. Device Provisioning Service (DPS)

targetScope = 'subscription'

// ============================================
// PARAMETRES
// ============================================

@description('Nom du projet')
param projectName string = 'iotdetector'

@description('Environnement (dev, prod)')
@allowed([
  'dev'
  'prod'
])
param environment string = 'dev'

@description('Localisation Azure')
param location string = 'germanywestcentral'

@description('Créer un nouveau IoT Hub ou utiliser l\'existant ?')
param createNewIotHub bool = false

@description('Tags communs pour toutes les ressources')
param tags object = {
  project: 'IoT-Motion-Detector'
  environment: environment
  managedBy: 'Bicep'
}

// ============================================
// VARIABLES
// ============================================

var resourceGroupName = 'rg-${projectName}-${environment}'
var iotHubName = 'iot-${projectName}-${environment}'
var dpsName = 'dps-${projectName}'

// ============================================
// MODULE 1 : Resource Group
// ============================================

resource rg 'Microsoft.Resources/resourceGroups@2021-04-01' = {
  name: resourceGroupName
  location: location
  tags: tags
}

// ============================================
// MODULE 2 : IoT Hub (CONDITIONNEL)
// ============================================

// Création uniquement si demandé
module iotHub 'modules/iot-hub.bicep' = if (createNewIotHub) {
  name: 'deploy-iot-hub'
  scope: rg
  params: {
    iotHubName: iotHubName
    location: location
    tags: tags
    skuName: 'S1'
    skuCapacity: 1
  }
}

// ============================================
// MODULE 3 : Device Provisioning Service
// ============================================

module dps 'modules/iot-dps.bicep' = {
  name: 'deploy-dps'
  scope: rg
  params: {
    dpsName: dpsName
    environment: environment
    location: location
    iotHubName: iotHubName
    iotHubResourceGroup: resourceGroupName
    tags: tags
  }
  dependsOn: [
    iotHub // Même si conditionnel, la dépendance assure l'ordre
  ]
}

// ============================================
// OUTPUTS
// ============================================

@description('Nom du Resource Group')
output resourceGroupName string = rg.name

@description('Nom de l\'IoT Hub')
output iotHubName string = iotHubName

@description('Hostname de l\'IoT Hub')
output iotHubHostName string = '${iotHubName}.azure-devices.net'

@description('Nom du DPS')
output dpsName string = dps.outputs.dpsName

@description('ID Scope du DPS (pour le firmware)')
output dpsIdScope string = dps.outputs.idScope

@description('Endpoint du DPS (pour le firmware)')
output dpsEndpoint string = dps.outputs.deviceProvisioningHostName

// ============================================
// INSTRUCTIONS POST-DEPLOIEMENT
// ============================================

// Après le déploiement, exécuter ces commandes :
//
// 1. Créer l'Enrollment Group :
//    az iot dps enrollment-group create \
//      --dps-name $(dpsName) \
//      --resource-group $(resourceGroupName) \
//      --enrollment-id "auto-provisioning-group" \
//      --provisioning-status enabled
//
// 2. Récupérer la Primary Key :
//    az iot dps enrollment-group show \
//      --dps-name $(dpsName) \
//      --resource-group $(resourceGroupName) \
//      --enrollment-id "auto-provisioning-group" \
//      --query 'attestation.symmetricKey.primaryKey' -o tsv
//
// 3. Générer les clés devices (voir script derive-device-keys.js)
