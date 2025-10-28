import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '../ui/table';
import { Badge } from '../ui/badge';
import { Input } from '../ui/input';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '../ui/select';
import { Search } from 'lucide-react';
import { useState } from 'react';

const logs = [
  { id: 1, timestamp: '2025-10-27 14:32:15', device: 'ESP32-001', message: 'Détection PIR activée - mouvement détecté', level: 'info', category: 'sensor' },
  { id: 2, timestamp: '2025-10-27 14:28:42', device: 'ESP32-003', message: 'Température supérieure au seuil: 26.2°C', level: 'warning', category: 'sensor' },
  { id: 3, timestamp: '2025-10-27 14:15:08', device: 'Photon2-001', message: 'Connexion établie avec Azure IoT Hub', level: 'info', category: 'connection' },
  { id: 4, timestamp: '2025-10-27 14:02:33', device: 'ESP32-002', message: 'Échec de connexion - timeout', level: 'error', category: 'connection' },
  { id: 5, timestamp: '2025-10-27 13:55:21', device: 'ESP32-001', message: 'Message telemetry envoyé avec succès', level: 'info', category: 'telemetry' },
  { id: 6, timestamp: '2025-10-27 13:42:18', device: 'ESP32-003', message: 'Capteur PIR - aucune activité détectée', level: 'info', category: 'sensor' },
  { id: 7, timestamp: '2025-10-27 13:30:05', device: 'DPS', message: 'Provisioning réussi pour ESP32-003', level: 'info', category: 'provisioning' },
  { id: 8, timestamp: '2025-10-27 13:15:42', device: 'ESP32-002', message: 'Certificat X.509 expirera dans 30 jours', level: 'warning', category: 'security' },
  { id: 9, timestamp: '2025-10-27 12:58:33', device: 'Photon2-001', message: 'Twin update reçu depuis le cloud', level: 'info', category: 'twin' },
  { id: 10, timestamp: '2025-10-27 12:45:11', device: 'ESP32-001', message: 'Firmware version 2.4.1 confirmée', level: 'info', category: 'system' },
  { id: 11, timestamp: '2025-10-27 12:30:28', device: 'ESP32-003', message: 'Humidité mesurée: 43%', level: 'info', category: 'sensor' },
  { id: 12, timestamp: '2025-10-27 12:12:15', device: 'ESP32-002', message: 'Tentative de reconnexion échouée', level: 'error', category: 'connection' },
];

export function LogsPage() {
  const [filterLevel, setFilterLevel] = useState('all');
  const [searchQuery, setSearchQuery] = useState('');

  const filteredLogs = logs.filter(log => {
    const matchesLevel = filterLevel === 'all' || log.level === filterLevel;
    const matchesSearch = log.message.toLowerCase().includes(searchQuery.toLowerCase()) ||
                         log.device.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesLevel && matchesSearch;
  });

  return (
    <div className="space-y-6">
      <div>
        <h2 className="text-gray-900 dark:text-white mb-1">Logs & Historique</h2>
        <p className="text-gray-600 dark:text-gray-400">Suivi des événements et activités système</p>
      </div>

      {/* Filters */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardContent className="pt-6">
          <div className="flex gap-4">
            <div className="relative flex-1">
              <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 h-4 w-4 text-gray-400" />
              <Input 
                placeholder="Rechercher dans les logs..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="pl-10 bg-gray-700 border-gray-600 text-white"
              />
            </div>
            <Select value={filterLevel} onValueChange={setFilterLevel}>
              <SelectTrigger className="w-48 bg-gray-700 border-gray-600 text-white">
                <SelectValue placeholder="Filtrer par niveau" />
              </SelectTrigger>
              <SelectContent className="bg-gray-800 border-gray-600">
                <SelectItem value="all">Tous les niveaux</SelectItem>
                <SelectItem value="info">Info</SelectItem>
                <SelectItem value="warning">Warning</SelectItem>
                <SelectItem value="error">Error</SelectItem>
              </SelectContent>
            </Select>
          </div>
        </CardContent>
      </Card>

      {/* Logs Table */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">
            Événements récents ({filteredLogs.length})
          </CardTitle>
        </CardHeader>
        <CardContent>
          <Table>
            <TableHeader>
              <TableRow className="border-gray-700">
                <TableHead className="text-gray-400">Timestamp</TableHead>
                <TableHead className="text-gray-400">Appareil</TableHead>
                <TableHead className="text-gray-400">Message</TableHead>
                <TableHead className="text-gray-400">Catégorie</TableHead>
                <TableHead className="text-gray-400">Niveau</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {filteredLogs.map((log) => (
                <TableRow key={log.id} className="border-gray-700">
                  <TableCell className="text-gray-400">{log.timestamp}</TableCell>
                  <TableCell className="text-gray-300">{log.device}</TableCell>
                  <TableCell className="text-gray-300">{log.message}</TableCell>
                  <TableCell>
                    <Badge variant="outline" className="border-gray-600 text-gray-400">
                      {log.category}
                    </Badge>
                  </TableCell>
                  <TableCell>
                    <Badge 
                      variant={log.level === 'error' ? 'destructive' : 'default'}
                      className={
                        log.level === 'info' ? 'bg-blue-600' : 
                        log.level === 'warning' ? 'bg-orange-600' : ''
                      }
                    >
                      {log.level}
                    </Badge>
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </CardContent>
      </Card>
    </div>
  );
}
