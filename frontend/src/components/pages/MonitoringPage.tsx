import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Button } from '../ui/button';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '../ui/table';
import { Badge } from '../ui/badge';
import { Activity, CheckCircle, AlertTriangle, XCircle } from 'lucide-react';
import { LineChart, Line, BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

const messageRateData = [
  { time: '08:00', messages: 145, errors: 2 },
  { time: '09:00', messages: 168, errors: 1 },
  { time: '10:00', messages: 192, errors: 3 },
  { time: '11:00', messages: 205, errors: 0 },
  { time: '12:00', messages: 178, errors: 1 },
  { time: '13:00', messages: 195, errors: 2 },
  { time: '14:00', messages: 210, errors: 0 },
];

const latencyData = [
  { time: '08:00', latency: 45 },
  { time: '09:00', latency: 38 },
  { time: '10:00', latency: 52 },
  { time: '11:00', latency: 41 },
  { time: '12:00', latency: 48 },
  { time: '13:00', latency: 43 },
  { time: '14:00', latency: 39 },
];

const provisioningLogs = [
  { device: 'ESP32-001', status: 'success', timestamp: '2025-10-25 10:30:15', duration: '2.3s' },
  { device: 'ESP32-003', status: 'success', timestamp: '2025-10-25 10:32:42', duration: '1.8s' },
  { device: 'Photon2-001', status: 'success', timestamp: '2025-10-25 10:35:08', duration: '2.1s' },
  { device: 'ESP32-002', status: 'failed', timestamp: '2025-10-25 10:38:22', duration: '15.2s' },
];

const systemStatus = [
  { component: 'Azure IoT Hub', status: 'ok', message: 'Opérationnel' },
  { component: 'Device Provisioning Service', status: 'ok', message: 'Tous les appareils provisionnés' },
  { component: 'Message Routing', status: 'ok', message: 'Routes actives: 3/3' },
  { component: 'Diagnostic Settings', status: 'warning', message: 'Logs non activés' },
  { component: 'ESP32-002 Connectivity', status: 'error', message: 'Appareil déconnecté depuis 2h' },
];

export function MonitoringPage() {
  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'ok':
        return <CheckCircle className="h-5 w-5 text-green-600" />;
      case 'warning':
        return <AlertTriangle className="h-5 w-5 text-orange-600" />;
      case 'error':
        return <XCircle className="h-5 w-5 text-red-600" />;
      default:
        return <Activity className="h-5 w-5 text-gray-600" />;
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-gray-900 dark:text-white mb-1">Diagnostic & Monitoring</h2>
          <p className="text-gray-600 dark:text-gray-400">Surveillance Azure et diagnostic système</p>
        </div>
        <Button className="bg-blue-600 hover:bg-blue-700">
          <Activity className="h-4 w-4 mr-2" />
          Activer Diagnostic Settings
        </Button>
      </div>

      {/* System Status */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">Statut du système</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="space-y-3">
            {systemStatus.map((item, index) => (
              <div key={index} className="flex items-center justify-between p-3 bg-gray-700 rounded-lg">
                <div className="flex items-center gap-3">
                  {getStatusIcon(item.status)}
                  <div>
                    <p className="text-gray-100">{item.component}</p>
                    <p className="text-gray-400">{item.message}</p>
                  </div>
                </div>
                <Badge 
                  variant={item.status === 'error' ? 'destructive' : 'default'}
                  className={
                    item.status === 'ok' ? 'bg-green-600' : 
                    item.status === 'warning' ? 'bg-orange-600' : ''
                  }
                >
                  {item.status === 'ok' ? '✅ OK' : item.status === 'warning' ? '⚠️ Warning' : '❌ Erreur'}
                </Badge>
              </div>
            ))}
          </div>
        </CardContent>
      </Card>

      {/* Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Taux de messages Azure</CardTitle>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={250}>
              <BarChart data={messageRateData}>
                <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
                <XAxis dataKey="time" stroke="#9CA3AF" />
                <YAxis stroke="#9CA3AF" />
                <Tooltip 
                  contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                  labelStyle={{ color: '#F3F4F6' }}
                />
                <Bar dataKey="messages" fill="#3B82F6" name="Messages" />
                <Bar dataKey="errors" fill="#EF4444" name="Erreurs" />
              </BarChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>

        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Latence (ms)</CardTitle>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={250}>
              <LineChart data={latencyData}>
                <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
                <XAxis dataKey="time" stroke="#9CA3AF" />
                <YAxis stroke="#9CA3AF" />
                <Tooltip 
                  contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                  labelStyle={{ color: '#F3F4F6' }}
                />
                <Line type="monotone" dataKey="latency" stroke="#10B981" strokeWidth={2} />
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>
      </div>

      {/* Provisioning Logs */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">Logs de provisioning DPS</CardTitle>
        </CardHeader>
        <CardContent>
          <Table>
            <TableHeader>
              <TableRow className="border-gray-700">
                <TableHead className="text-gray-400">Appareil</TableHead>
                <TableHead className="text-gray-400">Statut</TableHead>
                <TableHead className="text-gray-400">Timestamp</TableHead>
                <TableHead className="text-gray-400">Durée</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {provisioningLogs.map((log, index) => (
                <TableRow key={index} className="border-gray-700">
                  <TableCell className="text-gray-300">{log.device}</TableCell>
                  <TableCell>
                    <Badge 
                      variant={log.status === 'failed' ? 'destructive' : 'default'}
                      className={log.status === 'success' ? 'bg-green-600' : ''}
                    >
                      {log.status === 'success' ? 'Succès' : 'Échec'}
                    </Badge>
                  </TableCell>
                  <TableCell className="text-gray-400">{log.timestamp}</TableCell>
                  <TableCell className="text-gray-400">{log.duration}</TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </CardContent>
      </Card>
    </div>
  );
}
