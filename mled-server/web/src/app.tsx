import { useEffect } from 'preact/hooks';
import 'bootstrap/dist/css/bootstrap.min.css';
import './index.css';

import { useAppStore, TabId } from './store';
import { connectSSE, loadInitialData } from './api';
import { NodesScreen } from './screens/Nodes';
import { PatternsScreen } from './screens/Patterns';
import { StatusScreen } from './screens/Status';
import { SettingsModal } from './screens/Settings';

const TABS: { id: TabId; label: string; icon: string }[] = [
  { id: 'nodes', label: 'Nodes', icon: '🏠' },
  { id: 'patterns', label: 'Patterns', icon: '🎨' },
  { id: 'status', label: 'Status', icon: 'ℹ️' },
];

export function App() {
  const currentTab = useAppStore((s) => s.currentTab);
  const setCurrentTab = useAppStore((s) => s.setCurrentTab);
  const connected = useAppStore((s) => s.connected);
  const lastError = useAppStore((s) => s.lastError);
  const showSettings = useAppStore((s) => s.showSettings);
  const setShowSettings = useAppStore((s) => s.setShowSettings);

  // Initialize on mount
  useEffect(() => {
    loadInitialData();
    connectSSE();

    // Refresh data periodically
    const interval = setInterval(() => {
      loadInitialData();
    }, 5000);

    return () => clearInterval(interval);
  }, []);

  const renderScreen = () => {
    switch (currentTab) {
      case 'nodes':
        return <NodesScreen />;
      case 'patterns':
        return <PatternsScreen />;
      case 'status':
        return <StatusScreen />;
      default:
        return <NodesScreen />;
    }
  };

  return (
    <>
      {/* Header */}
      <nav class="navbar navbar-dark bg-dark px-3">
        <span class="navbar-brand mb-0">
          🎛️ MLED Controller
        </span>
        <div class="connection-indicator">
          <span class={`dot ${connected ? 'connected' : 'disconnected'}`} />
          <span>{connected ? 'Connected' : 'Disconnected'}</span>
        </div>
      </nav>

      {/* Error banner */}
      {lastError && (
        <div class="error-banner">
          {lastError}
        </div>
      )}

      {/* Main content */}
      <main class="main-content">
        {renderScreen()}
      </main>

      {/* Tab navigation */}
      <nav class="navbar navbar-dark bg-dark fixed-bottom">
        <div class="container-fluid justify-content-center">
          <ul class="nav nav-tabs border-0 w-100 justify-content-center">
            {TABS.map((tab) => (
              <li class="nav-item" key={tab.id}>
                <button
                  class={`nav-link ${currentTab === tab.id ? 'active' : ''}`}
                  onClick={() => setCurrentTab(tab.id)}
                  type="button"
                >
                  {tab.icon} {tab.label}
                </button>
              </li>
            ))}
          </ul>
        </div>
      </nav>

      {/* Settings modal */}
      {showSettings && (
        <SettingsModal onClose={() => setShowSettings(false)} />
      )}
    </>
  );
}
