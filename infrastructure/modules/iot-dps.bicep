// ============================================
// MODULE: IoT Device Provisioning Service (DPS)
// ============================================
// Ce module déploie un DPS avec :
// - Liaison automatique à l'IoT Hub
// - Configuration pour Enrollment Group (créé via CLI après)
// - Outputs pour le firmware (idScope, primaryKey, etc.)

@description('Nom du DPS (sera suffixé par l\'environnement)')
param dpsName string

@description('Environnement (dev, prod, etc.)')
param environment string

@description('Localisation Azure')
param location string = resourceGroup().location

@description('Nom de l\'IoT Hub existant à lier')
param iotHubName string

@description('Nom du groupe de ressources de l\'IoT Hub')
param iotHubResourceGroup string

@description('Tags pour les ressources')
param tags object = {}

// ============================================
// VARIABLES
// ============================================

var dpsFullName = '${dpsName}-${environment}'

// ============================================
// RESSOURCE EXISTANTE : IoT Hub
// ============================================

resource iotHub 'Microsoft.Devices/IotHubs@2023-06-30' existing = {
  name: iotHubName
  scope: resourceGroup(iotHubResourceGroup)
}

// ============================================
// RESSOURCE : Device Provisioning Service
// ============================================

resource dps 'Microsoft.Devices/provisioningServices@2022-12-12' = {
  name: dpsFullName
  location: location
  tags: tags
  sku: {
    name: 'S1'
    capacity: 1
  }
  properties: {
    allocationPolicy: 'Static'
    iotHubs: [
      {
        connectionString: 'HostName=${iotHub.properties.hostName};SharedAccessKeyName=iothubowner;SharedAccessKey=${iotHub.listKeys().value[0].primaryKey}'
        location: location
      }
    ]
    state: 'Active'
  }
}

// ============================================
// OUTPUTS
// ============================================

@description('ID du DPS')
output dpsId string = dps.id

@description('Nom complet du DPS')
output dpsName string = dps.name

@description('ID Scope (à utiliser dans le firmware)')
output idScope string = dps.properties.idScope

@description('Endpoint du DPS')
output serviceOperationsHostName string = dps.properties.serviceOperationsHostName

@description('Endpoint du Device Endpoint')
output deviceProvisioningHostName string = dps.properties.deviceProvisioningHostName

@description('Nom de l\'IoT Hub lié')
output linkedIotHubHostName string = iotHub.properties.hostName

// ============================================
// NOTES IMPORTANTES
// ============================================

// 🔑 ENROLLMENT GROUP :
// L'Enrollment Group (Symmetric Key) ne peut PAS être créé via ARM/Bicep.
// Il doit être créé via Azure CLI ou Portal après le déploiement.
//
// Le script deploy-dps.sh gère automatiquement cette étape.
