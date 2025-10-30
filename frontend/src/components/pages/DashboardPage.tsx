import { useState, useEffect } from 'react';
import { StatCard } from '../StatCard';
import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '../ui/table';
import { Badge } from '../ui/badge';
import { Button } from '../ui/button';
import { Cpu, Power, Activity, AlertTriangle, Eye } from 'lucide-react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, AreaChart, Area } from 'recharts';
import { api, Device, DeviceStats, ActivityDataPoint, TemperatureDataPoint, EventLog } from '../../lib/api';

export function DashboardPage() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [stats, setStats] = useState<DeviceStats | null>(null);
  const [activityData, setActivityData] = useState<ActivityDataPoint[]>([]);
  const [temperatureData, setTemperatureData] = useState<TemperatureDataPoint[]>([]);
  const [recentEvents, setRecentEvents] = useState<EventLog[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    const fetchData = async () => {
      try {
        setLoading(true);
        setError(null);

        const [devicesData, statsData, activityResp, tempResp, eventsData] = await Promise.all([
          api.getDevices(),
          api.getDeviceStats(),
          api.getActivityData(24),
          api.getTemperatureData(24),
          api.getRecentEvents(4)
        ]);

        setDevices(devicesData);
        setStats(statsData);
        setActivityData(activityResp);
        setTemperatureData(tempResp);
        setRecentEvents(eventsData);
      } catch (err) {
        console.error('Error fetching dashboard data:', err);
        setError('Erreur lors du chargement des données');
      } finally {
        setLoading(false);
      }
    };

    fetchData();

    // Refresh data every 30 seconds
    const interval = setInterval(fetchData, 30000);
    return () => clearInterval(interval);
  }, []);

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="text-gray-600 dark:text-gray-400">Chargement...</div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="text-red-600 dark:text-red-400">{error}</div>
      </div>
    );
  }
  return (
    <div className="space-y-6">
      <div>
        <h2 className="text-gray-900 dark:text-white mb-1">Vue d'ensemble</h2>
        <p className="text-gray-600 dark:text-gray-400">Surveillance en temps réel de votre système IoT</p>
      </div>

      {/* Stats Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
        <StatCard
          title="Total appareils"
          value={stats?.totalDevices.toString() || '0'}
          icon={Cpu}
          iconColor="text-blue-600"
          subtitle={stats?.deviceTypeBreakdown || ''}
        />
        <StatCard
          title="Appareils actifs"
          value={stats?.activeDevices.toString() || '0'}
          icon={Power}
          iconColor="text-green-600"
          subtitle={`${stats?.availability || 0}% de disponibilité`}
        />
        <StatCard
          title="Événements aujourd'hui"
          value={stats?.eventsToday.toString() || '0'}
          icon={Activity}
          iconColor="text-purple-600"
          subtitle="Détections PIR"
        />
        <StatCard
          title="Alertes actives"
          value={stats?.activeAlerts.toString() || '0'}
          icon={AlertTriangle}
          iconColor="text-orange-600"
          subtitle="Warnings et erreurs"
        />
      </div>

      {/* Charts Section */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Activité capteur PIR</CardTitle>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={250}>
              <AreaChart data={activityData}>
                <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
                <XAxis dataKey="time" stroke="#9CA3AF" />
                <YAxis stroke="#9CA3AF" />
                <Tooltip 
                  contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                  labelStyle={{ color: '#F3F4F6' }}
                />
                <Area type="monotone" dataKey="value" stroke="#3B82F6" fill="#3B82F6" fillOpacity={0.3} />
              </AreaChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>

        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Température & Humidité</CardTitle>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={250}>
              <LineChart data={temperatureData}>
                <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
                <XAxis dataKey="time" stroke="#9CA3AF" />
                <YAxis stroke="#9CA3AF" />
                <Tooltip 
                  contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                  labelStyle={{ color: '#F3F4F6' }}
                />
                <Line type="monotone" dataKey="temp" stroke="#EF4444" name="Temp (°C)" />
                <Line type="monotone" dataKey="humidity" stroke="#3B82F6" name="Humidité (%)" />
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>
      </div>

      {/* Tables Section */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Appareils</CardTitle>
          </CardHeader>
          <CardContent>
            <Table>
              <TableHeader>
                <TableRow className="border-gray-700">
                  <TableHead className="text-gray-400">Nom</TableHead>
                  <TableHead className="text-gray-400">Type</TableHead>
                  <TableHead className="text-gray-400">Statut</TableHead>
                  <TableHead className="text-gray-400">Actions</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {devices.map((device) => (
                  <TableRow key={device.id} className="border-gray-700">
                    <TableCell className="text-gray-300">{device.name}</TableCell>
                    <TableCell className="text-gray-400">{device.type}</TableCell>
                    <TableCell>
                      <Badge variant={device.status === 'active' ? 'default' : 'destructive'} className={device.status === 'active' ? 'bg-green-600' : ''}>
                        {device.status === 'active' ? 'Actif' : 'Inactif'}
                      </Badge>
                    </TableCell>
                    <TableCell>
                      <Button variant="ghost" size="sm">
                        <Eye className="h-4 w-4" />
                      </Button>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </CardContent>
        </Card>

        <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
          <CardHeader>
            <CardTitle className="text-gray-900 dark:text-white">Derniers événements</CardTitle>
          </CardHeader>
          <CardContent>
            <Table>
              <TableHeader>
                <TableRow className="border-gray-700">
                  <TableHead className="text-gray-400">Timestamp</TableHead>
                  <TableHead className="text-gray-400">Message</TableHead>
                  <TableHead className="text-gray-400">Niveau</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {recentEvents.map((event) => (
                  <TableRow key={event.id} className="border-gray-700">
                    <TableCell className="text-gray-400">
                      {new Date(event.timestamp).toLocaleString('fr-FR')}
                    </TableCell>
                    <TableCell className="text-gray-300">{event.message}</TableCell>
                    <TableCell>
                      <Badge
                        variant={event.level === 'error' ? 'destructive' : 'default'}
                        className={
                          event.level === 'info' ? 'bg-blue-600' :
                          event.level === 'warning' ? 'bg-orange-600' : ''
                        }
                      >
                        {event.level}
                      </Badge>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
