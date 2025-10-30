import { useState } from 'react';
import { Header } from './components/Header';
import { Sidebar } from './components/Sidebar';
import { DashboardPage } from './components/pages/DashboardPage';
import { DevicesPage } from './components/pages/DevicesPage';
import { ConfigurationPage } from './components/pages/ConfigurationPage';
import { DataPage } from './components/pages/DataPage';
import { LogsPage } from './components/pages/LogsPage';
import { TeamPage } from './components/pages/TeamPage';
import { MonitoringPage } from './components/pages/MonitoringPage';

export default function App() {
  const [isDarkMode, setIsDarkMode] = useState(true);
  const [currentPage, setCurrentPage] = useState('dashboard');
  const [isSidebarOpen, setIsSidebarOpen] = useState(true);

  const renderPage = () => {
    switch (currentPage) {
      case 'dashboard':
        return <DashboardPage />;
      case 'devices':
        return <DevicesPage />;
      case 'configuration':
        return <ConfigurationPage />;
      case 'data':
        return <DataPage />;
      case 'logs':
        return <LogsPage />;
      case 'team':
        return <TeamPage />;
      case 'monitoring':
        return <MonitoringPage />;
      default:
        return <DashboardPage />;
    }
  };

  return (
    <div className={isDarkMode ? 'dark' : ''}>
      <div className="min-h-screen bg-gray-50 dark:bg-gray-900 transition-colors">
        <Header 
          isDarkMode={isDarkMode} 
          setIsDarkMode={setIsDarkMode}
          toggleSidebar={() => setIsSidebarOpen(!isSidebarOpen)}
        />
        
        <div className="flex">
          <Sidebar 
            currentPage={currentPage} 
            setCurrentPage={setCurrentPage}
            isOpen={isSidebarOpen}
          />
          
          <main className={`flex-1 transition-all duration-300 ${isSidebarOpen ? 'ml-64' : 'ml-0'}`}>
            <div className="p-6">
              {renderPage()}
            </div>
            
            <footer className="border-t border-gray-200 dark:border-gray-700 bg-white dark:bg-gray-800 mt-12">
              <div className="px-6 py-4 flex items-center justify-between">
                <p className="text-gray-600 dark:text-gray-400">
                  © 2025 IoT Detector – Projet universitaire M2 EEA
                </p>
                <a 
                  href="https://github.com" 
                  target="_blank" 
                  rel="noopener noreferrer"
                  className="text-blue-600 dark:text-blue-400 hover:underline"
                >
                  GitHub du projet
                </a>
              </div>
            </footer>
          </main>
        </div>
      </div>
    </div>
  );
}
