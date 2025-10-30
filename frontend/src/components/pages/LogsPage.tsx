import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '../ui/table';
import { Badge } from '../ui/badge';
import { Input } from '../ui/input';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '../ui/select';
import { Search } from 'lucide-react';
import { useState, useEffect } from 'react';
import { api, EventLog } from '../../lib/api';

export function LogsPage() {
  const [logs, setLogs] = useState<EventLog[]>([]);
  const [filterLevel, setFilterLevel] = useState('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchLogs = async () => {
      try {
        const data = await api.getRecentEvents(100);
        setLogs(data);
      } catch (error) {
        console.error('Error fetching logs:', error);
      } finally {
        setLoading(false);
      }
    };

    fetchLogs();
    const interval = setInterval(fetchLogs, 30000);
    return () => clearInterval(interval);
  }, []);

  const filteredLogs = logs.filter(log => {
    const matchesLevel = filterLevel === 'all' || log.level === filterLevel;
    const matchesSearch = log.message.toLowerCase().includes(searchQuery.toLowerCase()) ||
                         (log.deviceId?.toLowerCase().includes(searchQuery.toLowerCase()) || false);
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
                <TableHead className="text-gray-400">Niveau</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {filteredLogs.map((log) => (
                <TableRow key={log.id} className="border-gray-700">
                  <TableCell className="text-gray-400">
                    {new Date(log.timestamp).toLocaleString('fr-FR')}
                  </TableCell>
                  <TableCell className="text-gray-300">{log.deviceId || 'Système'}</TableCell>
                  <TableCell className="text-gray-300">{log.message}</TableCell>
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
