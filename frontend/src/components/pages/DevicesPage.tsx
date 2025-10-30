import { useState, useEffect } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Button } from '../ui/button';
import { Badge } from '../ui/badge';
import { Satellite, Settings, Wifi, WifiOff, Cpu } from 'lucide-react';
import { api, Device } from '../../lib/api';

export function DevicesPage() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchDevices = async () => {
      try {
        const data = await api.getDevices();
        setDevices(data);
      } catch (error) {
        console.error('Error fetching devices:', error);
      } finally {
        setLoading(false);
      }
    };

    fetchDevices();
    const interval = setInterval(fetchDevices, 30000);
    return () => clearInterval(interval);
  }, []);

  if (loading) {
    return <div className="text-gray-400">Chargement...</div>;
  }
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
                  variant={device.status === 'active' ? 'default' : 'destructive'}
                  className={`flex items-center gap-1 ${device.status === 'active' ? 'bg-green-600' : ''}`}
                >
                  {device.status === 'active' ? (
                    <><Wifi className="h-3 w-3" /> Actif</>
                  ) : (
                    <><WifiOff className="h-3 w-3" /> Inactif</>
                  )}
                </Badge>
              </div>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="grid grid-cols-2 gap-4">
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Device ID</p>
                  <p className="text-gray-900 dark:text-white">{device.id}</p>
                </div>
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Type</p>
                  <p className="text-gray-900 dark:text-white">{device.type}</p>
                </div>
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Statut</p>
                  <p className="text-gray-900 dark:text-white">{device.status}</p>
                </div>
                <div>
                  <p className="text-gray-500 dark:text-gray-400">Dernier signal</p>
                  <p className="text-gray-900 dark:text-white">{device.lastSignal || 'Jamais vu'}</p>
                </div>
              </div>

              <div>
                <p className="text-gray-500 dark:text-gray-400 mb-1">Créé le</p>
                <p className="text-gray-900 dark:text-white">{new Date(device.createdAt).toLocaleString('fr-FR')}</p>
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
