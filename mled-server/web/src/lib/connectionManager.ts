/**
 * Pattern 2: Connection Manager with Exponential Backoff
 * 
 * Problem: EventSource auto-reconnects on error, causing rapid state updates
 * and potential infinite loops when backend is unavailable.
 * 
 * Solution: Wrap EventSource in a manager that:
 * - Uses exponential backoff for reconnection
 * - Tracks connection state explicitly
 * - Provides clean connect/disconnect lifecycle
 * - Debounces error handling
 */

import { useAppStore } from '../store';
import type { Node } from '../types';

interface ConnectionConfig {
  url: string;
  minRetryDelay: number;    // Initial retry delay (ms)
  maxRetryDelay: number;    // Maximum retry delay (ms)
  backoffMultiplier: number; // Multiply delay on each failure
}

const DEFAULT_CONFIG: ConnectionConfig = {
  url: '/api/events',
  minRetryDelay: 1000,
  maxRetryDelay: 30000,
  backoffMultiplier: 2,
};

type ConnectionState = 'disconnected' | 'connecting' | 'connected' | 'error';

class SSEConnectionManager {
  private eventSource: EventSource | null = null;
  private config: ConnectionConfig;
  private state: ConnectionState = 'disconnected';
  private retryDelay: number;
  private retryTimeout: number | null = null;
  private lastErrorTime: number = 0;

  constructor(config: Partial<ConnectionConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.retryDelay = this.config.minRetryDelay;
  }

  /**
   * Start the SSE connection with automatic reconnection.
   */
  connect(): void {
    if (this.state === 'connecting' || this.state === 'connected') {
      return;
    }

    this.setState('connecting');
    this.createEventSource();
  }

  /**
   * Stop the SSE connection and cancel any pending reconnection.
   */
  disconnect(): void {
    this.cancelRetry();
    this.closeEventSource();
    this.setState('disconnected');
    this.retryDelay = this.config.minRetryDelay;
  }

  /**
   * Get current connection state.
   */
  getState(): ConnectionState {
    return this.state;
  }

  private createEventSource(): void {
    this.closeEventSource();

    const store = useAppStore.getState();
    this.eventSource = new EventSource(this.config.url);

    this.eventSource.onopen = () => {
      this.setState('connected');
      this.retryDelay = this.config.minRetryDelay; // Reset backoff on success
      store.setConnected(true);
      store.setLastError(null);
      store.addLogEntry('Connected to server');
    };

    this.eventSource.onerror = () => {
      this.handleError();
    };

    // Register event handlers
    this.registerEventHandlers();
  }

  private registerEventHandlers(): void {
    if (!this.eventSource) return;

    const store = useAppStore.getState();

    // Node update event
    this.eventSource.addEventListener('node', (event: MessageEvent) => {
      try {
        const node: Node = JSON.parse(event.data);
        store.updateNode(node);
      } catch (e) {
        console.error('Failed to parse node event:', e);
      }
    });

    // Node offline event
    this.eventSource.addEventListener('node.offline', (event: MessageEvent) => {
      try {
        const { node_id } = JSON.parse(event.data);
        const existingNode = store.nodes.get(node_id);
        if (existingNode) {
          store.updateNode({ ...existingNode, status: 'offline' });
          store.addLogEntry(`Node ${existingNode.name || node_id} went offline`);
        }
      } catch (e) {
        console.error('Failed to parse offline event:', e);
      }
    });

    // Ack event
    this.eventSource.addEventListener('ack', (event: MessageEvent) => {
      try {
        const { node_id, success } = JSON.parse(event.data);
        const node = store.nodes.get(node_id);
        const name = node?.name || node_id;
        store.addLogEntry(`ACK from ${name}: ${success ? 'OK' : 'FAILED'}`);
      } catch (e) {
        console.error('Failed to parse ack event:', e);
      }
    });

    // Log event
    this.eventSource.addEventListener('log', (event: MessageEvent) => {
      try {
        const { message } = JSON.parse(event.data);
        store.addLogEntry(message);
      } catch (e) {
        console.error('Failed to parse log event:', e);
      }
    });
  }

  private handleError(): void {
    // Debounce rapid errors
    const now = Date.now();
    if (now - this.lastErrorTime < 1000) {
      return;
    }
    this.lastErrorTime = now;

    this.closeEventSource();
    this.setState('error');

    const store = useAppStore.getState();
    store.setConnected(false);
    store.setLastError(`Connection lost. Retrying in ${this.retryDelay / 1000}s...`);
    store.addLogEntry('Connection lost');

    // Schedule reconnection with backoff
    this.scheduleReconnect();
  }

  private scheduleReconnect(): void {
    this.cancelRetry();

    this.retryTimeout = window.setTimeout(() => {
      this.retryTimeout = null;
      this.connect();
    }, this.retryDelay);

    // Increase delay for next failure (exponential backoff)
    this.retryDelay = Math.min(
      this.retryDelay * this.config.backoffMultiplier,
      this.config.maxRetryDelay
    );
  }

  private cancelRetry(): void {
    if (this.retryTimeout !== null) {
      clearTimeout(this.retryTimeout);
      this.retryTimeout = null;
    }
  }

  private closeEventSource(): void {
    if (this.eventSource) {
      this.eventSource.close();
      this.eventSource = null;
    }
  }

  private setState(state: ConnectionState): void {
    this.state = state;
  }
}

// Singleton instance
let connectionManager: SSEConnectionManager | null = null;

/**
 * Get the global connection manager instance.
 */
export function getConnectionManager(): SSEConnectionManager {
  if (!connectionManager) {
    connectionManager = new SSEConnectionManager();
  }
  return connectionManager;
}

/**
 * Connect to SSE stream.
 */
export function connectSSE(): void {
  getConnectionManager().connect();
}

/**
 * Disconnect from SSE stream.
 */
export function disconnectSSE(): void {
  getConnectionManager().disconnect();
}
