import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Button } from '../ui/button';
import { Badge } from '../ui/badge';
import { Satellite, Settings, Wifi, WifiOff, Cpu } from 'lucide-react';
import { ImageWithFallback } from '../figma/ImageWithFallback';
import { useEffect, useState } from 'react';

const API_URL = import.meta.env.VITE_API_URL ?? '';


export function DevicesPage() {
  const [devices, setDevices] = useState<any[]>([]);

  useEffect(() => {
    const base = API_URL || '';
    async function loadDevices() {
      try {
        const res = await fetch(`${base}/api/devices`);
        if (res.ok) {
          setDevices(await res.json());
        }
      } catch (e) {
        console.error('Failed to load devices', e);
      }
    }
    loadDevices();
  }, []);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-gray-900 dark:text-white mb-1">Appareils connectés</h2>
          <p className="text-gray-600 dark:text-gray-400">Gestion et surveillance des dispositifs IoT</p>
        </div>
        <Button className="bg-blue-600 hover:bg-blue-700">
          <Satellite className="h-4 w-4 mr-2" />
          Synchroniser DPS
        </Button>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {devices.map((device) => (
          <Card key={device.id} className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
            <CardHeader>
              <div className="flex items-start justify-between">
                <div className="flex items-center gap-3">
                  <div className="w-12 h-12 bg-gray-700 rounded-lg flex items-center justify-center">
                    <Cpu className="h-6 w-6 text-blue-400" />
                  </div>
                  <div>
                    <CardTitle className="text-gray-900 dark:text-white">{device.name}</CardTitle>
                    <p className="text-gray-500 dark:text-gray-400">{device.type}</p>
                  </div>
                </div>
                <Badge 
                  variant={device.status === 'connected' ? 'default' : 'destructive'}
                  className={`flex items-center gap-1 ${device.status === 'connected' ? 'bg-green-600' : ''}`}
                >
                  {device.status === 'connected' ? (
                    <><Wifi className="h-3 w-3" /> Connecté</>
                  ) : (
                    <><WifiOff className="h-3 w-3" /> Déconnecté</>
                  )}
                </Badge>
              </div>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="grid grid-cols-2 gap-4">
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Device ID</p>
                  <p className="text-gray-900 dark:text-white">{device.deviceId}</p>
                </div>
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Certificat</p>
                  <p className="text-gray-900 dark:text-white">{device.certificate}</p>
                </div>
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Firmware</p>
                  <p className="text-gray-900 dark:text-white">{device.firmware}</p>
                </div>
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Dernière MAJ</p>
                  <p className="text-gray-900 dark:text-white">{device.lastUpdate}</p>
                </div>
              </div>

              <div>
                <p className="text-gray-500 dark:text-gray-400 mb-1">Localisation</p>
                <p className="text-gray-900 dark:text-white">{device.location}</p>
              </div>

              <div>
                <p className="text-gray-500 dark:text-gray-400 mb-2">Capteurs</p>
                <div className="flex flex-wrap gap-2">
                  {device.sensors.map((sensor) => (
                    <Badge key={sensor} variant="outline" className="border-gray-600 text-gray-300">
                      {sensor}
                    </Badge>
                  ))}
                </div>
              </div>

              <div className="flex gap-2 pt-2">
                <Button variant="outline" size="sm" className="flex-1 border-gray-600 text-gray-300 hover:bg-gray-700">
                  Voir détails
                </Button>
                <Button variant="outline" size="sm" className="border-gray-600 text-gray-300 hover:bg-gray-700">
                  <Settings className="h-4 w-4" />
                </Button>
              </div>
            </CardContent>
          </Card>
        ))}
      </div>
    </div>
  );
}
