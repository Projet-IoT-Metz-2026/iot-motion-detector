import { Home, Wifi, Settings, LineChart, FileText, Users, Activity } from 'lucide-react';

interface SidebarProps {
  currentPage: string;
  setCurrentPage: (page: string) => void;
  isOpen: boolean;
}

const menuItems = [
  { id: 'dashboard', label: 'Tableau de bord', icon: Home },
  { id: 'devices', label: 'Appareils connectés', icon: Wifi },
  { id: 'configuration', label: 'Configuration', icon: Settings },
  { id: 'data', label: 'Données', icon: LineChart },
  { id: 'logs', label: 'Logs & Historique', icon: FileText },
  { id: 'team', label: 'Équipe / Projet', icon: Users },
  { id: 'monitoring', label: 'Diagnostic & Monitoring', icon: Activity },
];

export function Sidebar({ currentPage, setCurrentPage, isOpen }: SidebarProps) {
  if (!isOpen) return null;

  return (
    <aside className="w-64 bg-gray-800 dark:bg-gray-900 border-r border-gray-700 h-[calc(100vh-4rem)] fixed top-16 left-0 overflow-y-auto">
      <nav className="p-4 space-y-2">
        {menuItems.map((item) => {
          const Icon = item.icon;
          const isActive = currentPage === item.id;
          
          return (
            <button
              key={item.id}
              onClick={() => setCurrentPage(item.id)}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-colors ${
                isActive
                  ? 'bg-blue-600 text-white'
                  : 'text-gray-300 hover:bg-gray-700 hover:text-white'
              }`}
            >
              <Icon className="h-5 w-5" />
              <span>{item.label}</span>
            </button>
          );
        })}
      </nav>
    </aside>
  );
}
