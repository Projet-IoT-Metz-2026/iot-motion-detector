import { StatCard } from '../StatCard';
import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '../ui/table';
import { Badge } from '../ui/badge';
import { Button } from '../ui/button';
import { Cpu, Power, Activity, AlertTriangle, Eye } from 'lucide-react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, AreaChart, Area } from 'recharts';
import { useEffect, useState } from 'react';

const API_URL = import.meta.env.VITE_API_URL ?? '';

// default fallbacks while fetching
const defaultActivity = [
  { time: '00:00', value: 45 },
  { time: '04:00', value: 32 },
  { time: '08:00', value: 68 },
  { time: '12:00', value: 85 },
  { time: '16:00', value: 72 },
  { time: '20:00', value: 55 },
];

const defaultTemperature = [
  { time: '00:00', temp: 22.5, humidity: 45 },
  { time: '04:00', temp: 21.8, humidity: 48 },
  { time: '08:00', temp: 23.2, humidity: 42 },
  { time: '12:00', temp: 25.1, humidity: 38 },
  { time: '16:00', temp: 24.5, humidity: 40 },
  { time: '20:00', temp: 23.0, humidity: 44 },
];


export function DashboardPage() {
  const [activityData, setActivityData] = useState(defaultActivity);
  const [temperatureData, setTemperatureData] = useState(defaultTemperature);
  const [devices, setDevices] = useState<any[]>([]);
  const [recentEvents, setRecentEvents] = useState<any[]>([]);

  useEffect(() => {
    const base = API_URL || '';
    async function load() {
      try {
        const metricsRes = await fetch(`${base}/api/metrics`);
        if (metricsRes.ok) {
          const m = await metricsRes.json();
          setActivityData(m.activityData ?? defaultActivity);
          setTemperatureData(m.temperatureData ?? defaultTemperature);
        }

        const devicesRes = await fetch(`${base}/api/devices`);
        if (devicesRes.ok) {
          setDevices(await devicesRes.json());
        }

        const eventsRes = await fetch(`${base}/api/events`);
        if (eventsRes.ok) {
          setRecentEvents(await eventsRes.json());
        }
      } catch (e) {
        console.error('Failed to load dashboard data', e);
      }
    }
    load();
  }, []);

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
          value="4"
          icon={Cpu}
          iconColor="text-blue-600"
          subtitle="3 ESP32, 1 Photon2"
        />
        <StatCard
          title="Appareils actifs"
          value="3"
          icon={Power}
          iconColor="text-green-600"
          subtitle="75% de disponibilité"
        />
        <StatCard
          title="Événements aujourd'hui"
          value="127"
          icon={Activity}
          iconColor="text-purple-600"
          subtitle="+12% vs hier"
        />
        <StatCard
          title="Alertes actives"
          value="2"
          icon={AlertTriangle}
          iconColor="text-orange-600"
          subtitle="1 warning, 1 error"
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
                {recentEvents.map((event, index) => (
                  <TableRow key={index} className="border-gray-700">
                    <TableCell className="text-gray-400">{event.timestamp}</TableCell>
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
