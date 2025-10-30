import { Card, CardContent, CardHeader, CardTitle } from './ui/card';
import { LucideIcon } from 'lucide-react';

interface StatCardProps {
  title: string;
  value: string | number;
  icon: LucideIcon;
  iconColor?: string;
  subtitle?: string;
  trend?: 'up' | 'down' | 'neutral';
}

export function StatCard({ title, value, icon: Icon, iconColor = 'text-blue-600', subtitle, trend }: StatCardProps) {
  return (
    <Card className="bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700">
      <CardHeader className="flex flex-row items-center justify-between pb-2">
        <CardTitle className="text-gray-600 dark:text-gray-400">{title}</CardTitle>
        <Icon className={`h-5 w-5 ${iconColor}`} />
      </CardHeader>
      <CardContent>
        <div className="text-gray-900 dark:text-white">{value}</div>
        {subtitle && (
          <p className="text-gray-500 dark:text-gray-400 mt-1">{subtitle}</p>
        )}
      </CardContent>
    </Card>
  );
}
