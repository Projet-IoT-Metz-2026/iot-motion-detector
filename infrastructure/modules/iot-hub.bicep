// ============================================
// MODULE: IoT Hub
// ============================================
// Ce module déploie un IoT Hub Azure

@description('Nom de l\'IoT Hub')
param iotHubName string

@description('Localisation Azure')
param location string = resourceGroup().location

@description('SKU de l\'IoT Hub')
@allowed([
  'F1'
  'S1'
  'S2'
  'S3'
])
param skuName string = 'S1'

@description('Capacité (nombre d\'unités)')
param skuCapacity int = 1

@description('Tags pour les ressources')
param tags object = {}

// ============================================
// RESSOURCE : IoT Hub
// ============================================

resource iotHub 'Microsoft.Devices/IotHubs@2023-06-30' = {
  name: iotHubName
  location: location
  tags: tags
  sku: {
    name: skuName
    capacity: skuCapacity
  }
  properties: {
    eventHubEndpoints: {
      events: {
        retentionTimeInDays: 1
        partitionCount: 2
      }
    }
    cloudToDevice: {
      maxDeliveryCount: 10
      defaultTtlAsIso8601: 'PT1H'
      feedback: {
        lockDurationAsIso8601: 'PT1M'
        ttlAsIso8601: 'PT1H'
        maxDeliveryCount: 10
      }
    }
    messagingEndpoints: {
      fileNotifications: {
        lockDurationAsIso8601: 'PT1M'
        ttlAsIso8601: 'PT1H'
        maxDeliveryCount: 10
      }
    }
    enableFileUploadNotifications: false
    minTlsVersion: '1.2'
    features: 'None'
  }
}

// ============================================
// OUTPUTS
// ============================================

@description('ID de l\'IoT Hub')
output iotHubId string = iotHub.id

@description('Nom de l\'IoT Hub')
output iotHubName string = iotHub.name

@description('Hostname de l\'IoT Hub')
output hostName string = iotHub.properties.hostName

@description('Event Hub-compatible endpoint')
output eventHubEndpoint string = iotHub.properties.eventHubEndpoints.events.endpoint

@description('Event Hub-compatible name')
output eventHubPath string = iotHub.properties.eventHubEndpoints.events.path
