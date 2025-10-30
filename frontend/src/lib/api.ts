// API configuration
const API_BASE_URL = import.meta.env.VITE_API_URL || 'http://localhost:5000';

// Types
export interface Device {
  id: string;
  name: string;
  type: string;
  status: string;
  lastSeen?: string;
  lastSignal?: string;
  createdAt: string;
}

export interface DeviceStats {
  totalDevices: number;
  activeDevices: number;
  eventsToday: number;
  activeAlerts: number;
  availability: number;
  deviceTypeBreakdown: string;
}

export interface ActivityDataPoint {
  time: string;
  value: number;
}

export interface TemperatureDataPoint {
  time: string;
  temp: number;
  humidity: number;
}

export interface EventLog {
  id: number;
  timestamp: string;
  message: string;
  level: string;
  deviceId?: string;
}

export interface SensorData {
  id: number;
  deviceId: string;
  eventType: string;
  value?: number;
  temperature?: number;
  humidity?: number;
  timestamp: string;
  rawData?: string;
}

// API functions
export const api = {
  // Devices
  async getDevices(): Promise<Device[]> {
    const response = await fetch(`${API_BASE_URL}/api/devices`);
    if (!response.ok) throw new Error('Failed to fetch devices');
    return response.json();
  },

  async getDevice(id: string): Promise<Device> {
    const response = await fetch(`${API_BASE_URL}/api/devices/${id}`);
    if (!response.ok) throw new Error('Failed to fetch device');
    return response.json();
  },

  async getDeviceStats(): Promise<DeviceStats> {
    const response = await fetch(`${API_BASE_URL}/api/devices/stats`);
    if (!response.ok) throw new Error('Failed to fetch device stats');
    return response.json();
  },

  // Telemetry
  async getActivityData(hours: number = 24): Promise<ActivityDataPoint[]> {
    const response = await fetch(`${API_BASE_URL}/api/telemetry/activity?hours=${hours}`);
    if (!response.ok) throw new Error('Failed to fetch activity data');
    return response.json();
  },

  async getTemperatureData(hours: number = 24): Promise<TemperatureDataPoint[]> {
    const response = await fetch(`${API_BASE_URL}/api/telemetry/temperature?hours=${hours}`);
    if (!response.ok) throw new Error('Failed to fetch temperature data');
    return response.json();
  },

  async getRecentTelemetry(limit: number = 100): Promise<SensorData[]> {
    const response = await fetch(`${API_BASE_URL}/api/telemetry/recent?limit=${limit}`);
    if (!response.ok) throw new Error('Failed to fetch recent telemetry');
    return response.json();
  },

  // Events
  async getRecentEvents(limit: number = 50): Promise<EventLog[]> {
    const response = await fetch(`${API_BASE_URL}/api/events/recent?limit=${limit}`);
    if (!response.ok) throw new Error('Failed to fetch recent events');
    return response.json();
  },

  async getEvents(level?: string, skip: number = 0, take: number = 100): Promise<EventLog[]> {
    let url = `${API_BASE_URL}/api/events?skip=${skip}&take=${take}`;
    if (level) url += `&level=${level}`;

    const response = await fetch(url);
    if (!response.ok) throw new Error('Failed to fetch events');
    return response.json();
  }
};
