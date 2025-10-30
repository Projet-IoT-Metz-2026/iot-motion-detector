import { Card, CardContent, CardHeader, CardTitle } from '../ui/card';
import { Button } from '../ui/button';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '../ui/select';
import { Download } from 'lucide-react';
import { LineChart, Line, BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { useState } from 'react';

const pirData = [
  { time: '08:00', esp32_01: 1, esp32_02: 0, esp32_03: 1 },
  { time: '09:00', esp32_01: 0, esp32_02: 1, esp32_03: 1 },
  { time: '10:00', esp32_01: 1, esp32_02: 0, esp32_03: 0 },
  { time: '11:00', esp32_01: 1, esp32_02: 1, esp32_03: 1 },
  { time: '12:00', esp32_01: 0, esp32_02: 0, esp32_03: 1 },
  { time: '13:00', esp32_01: 1, esp32_02: 1, esp32_03: 0 },
  { time: '14:00', esp32_01: 1, esp32_02: 0, esp32_03: 1 },
];

const temperatureData = [
  { time: '08:00', esp32_01: 22.3, esp32_02: 21.8, esp32_03: 23.1 },
  { time: '09:00', esp32_01: 22.8, esp32_02: 22.1, esp32_03: 23.5 },
  { time: '10:00', esp32_01: 23.2, esp32_02: 22.5, esp32_03: 24.0 },
  { time: '11:00', esp32_01: 24.1, esp32_02: 23.2, esp32_03: 25.1 },
  { time: '12:00', esp32_01: 25.0, esp32_02: 24.0, esp32_03: 26.2 },
  { time: '13:00', esp32_01: 24.5, esp32_02: 23.5, esp32_03: 25.5 },
  { time: '14:00', esp32_01: 24.0, esp32_02: 23.0, esp32_03: 24.8 },
];

const humidityData = [
  { time: '08:00', esp32_01: 45, esp32_03: 48 },
  { time: '09:00', esp32_01: 44, esp32_03: 46 },
  { time: '10:00', esp32_01: 42, esp32_03: 44 },
  { time: '11:00', esp32_01: 40, esp32_03: 42 },
  { time: '12:00', esp32_01: 38, esp32_03: 40 },
  { time: '13:00', esp32_01: 39, esp32_03: 41 },
  { time: '14:00', esp32_01: 40, esp32_03: 43 },
];

export function DataPage() {
  const [period, setPeriod] = useState('24h');

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-gray-900 dark:text-white mb-1">Données capteurs</h2>
          <p className="text-gray-600 dark:text-gray-400">Visualisation temps réel des mesures</p>
        </div>
        <div className="flex items-center gap-3">
          <Select value={period} onValueChange={setPeriod}>
            <SelectTrigger className="w-32 bg-gray-800 border-gray-600 text-white">
              <SelectValue />
            </SelectTrigger>
            <SelectContent className="bg-gray-800 border-gray-600">
              <SelectItem value="1h">1 heure</SelectItem>
              <SelectItem value="24h">24 heures</SelectItem>
              <SelectItem value="7d">7 jours</SelectItem>
              <SelectItem value="30d">30 jours</SelectItem>
            </SelectContent>
          </Select>
          <Button className="bg-blue-600 hover:bg-blue-700">
            <Download className="h-4 w-4 mr-2" />
            Exporter CSV
          </Button>
        </div>
      </div>

      {/* PIR Sensor Activity */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">Capteur PIR - Détection de mouvement</CardTitle>
        </CardHeader>
        <CardContent>
          <ResponsiveContainer width="100%" height={300}>
            <BarChart data={pirData}>
              <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
              <XAxis dataKey="time" stroke="#9CA3AF" />
              <YAxis stroke="#9CA3AF" />
              <Tooltip 
                contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                labelStyle={{ color: '#F3F4F6' }}
              />
              <Legend />
              <Bar dataKey="esp32_01" fill="#3B82F6" name="ESP32-01" />
              <Bar dataKey="esp32_02" fill="#10B981" name="ESP32-02" />
              <Bar dataKey="esp32_03" fill="#F59E0B" name="ESP32-03" />
            </BarChart>
          </ResponsiveContainer>
        </CardContent>
      </Card>

      {/* Temperature */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">Température (°C)</CardTitle>
        </CardHeader>
        <CardContent>
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={temperatureData}>
              <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
              <XAxis dataKey="time" stroke="#9CA3AF" />
              <YAxis stroke="#9CA3AF" />
              <Tooltip 
                contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                labelStyle={{ color: '#F3F4F6' }}
              />
              <Legend />
              <Line type="monotone" dataKey="esp32_01" stroke="#EF4444" name="ESP32-01" strokeWidth={2} />
              <Line type="monotone" dataKey="esp32_02" stroke="#F59E0B" name="ESP32-02" strokeWidth={2} />
              <Line type="monotone" dataKey="esp32_03" stroke="#EC4899" name="ESP32-03" strokeWidth={2} />
            </LineChart>
          </ResponsiveContainer>
        </CardContent>
      </Card>

      {/* Humidity */}
      <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
        <CardHeader>
          <CardTitle className="text-gray-900 dark:text-white">Humidité (%)</CardTitle>
        </CardHeader>
        <CardContent>
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={humidityData}>
              <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
              <XAxis dataKey="time" stroke="#9CA3AF" />
              <YAxis stroke="#9CA3AF" />
              <Tooltip 
                contentStyle={{ backgroundColor: '#1F2937', border: '1px solid #374151', borderRadius: '8px' }}
                labelStyle={{ color: '#F3F4F6' }}
              />
              <Legend />
              <Line type="monotone" dataKey="esp32_01" stroke="#3B82F6" name="ESP32-01" strokeWidth={2} />
              <Line type="monotone" dataKey="esp32_03" stroke="#10B981" name="ESP32-03" strokeWidth={2} />
            </LineChart>
          </ResponsiveContainer>
        </CardContent>
      </Card>
    </div>
  );
}
