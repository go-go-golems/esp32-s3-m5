# Design Patterns for Preact-Zustand Frontend

**Status:** active  
**Intent:** long-term  
**Created:** 2026-01-21  

## Executive Summary

During implementation of the MLED Controller web UI, we encountered three recurring issues that caused infinite render loops, connection storms, and runtime errors. This document defines design patterns to prevent these issues in future work.

The three patterns are:

1. **Stable Selectors** — Prevent Zustand re-renders from derived data
2. **Connection Manager** — Exponential backoff for SSE/WebSocket
3. **API Client with Transformers** — Explicit backend↔frontend contract

## Problem Statement

### Issue 1: Infinite Render Loops

When a Zustand selector returns a new object or array reference on every call, React/Preact re-renders the component, which calls the selector again, creating a loop.

```tsx
// BAD: Creates new array every render → infinite loop
const nodes = useAppStore((s) => Array.from(s.nodes.values()));

// BAD: Creates new object every render
const stats = useAppStore((s) => ({ 
  online: s.nodes.filter(n => n.status === 'online').length 
}));
```

### Issue 2: SSE Reconnection Storms

`EventSource` auto-reconnects on error. When the backend is down, this creates rapid-fire connection attempts, each triggering state updates.

```ts
// BAD: Rapid reconnection without backoff
eventSource.onerror = () => {
  connectSSE(); // Immediate retry → storm
};
```

### Issue 3: API Contract Drift

Frontend expects one response format, backend returns another. This causes silent failures or cryptic runtime errors.

```ts
// Frontend expected: { nodes: [...] }
// Backend returned:  [...]
const data = await fetchJson<{ nodes: Node[] }>('/nodes');
return data.nodes; // TypeError: data.nodes is undefined
```

## Proposed Solution

### Pattern 1: Stable Selectors

**Principle:** Select raw data (Map/Set) in the selector, derive arrays with `useMemo` outside.

```tsx
// src/lib/useStableSelector.ts

export function useNodes(): Node[] {
  // Select the Map (stable reference)
  const nodesMap = useAppStore((s) => s.nodes);
  
  // Derive array outside selector, memoized
  return useMemo(
    () => nodesMap ? Array.from(nodesMap.values()) : [],
    [nodesMap]
  );
}

// Usage in component:
function NodeList() {
  const nodes = useNodes(); // Stable reference
  return <ul>{nodes.map(n => <li key={n.node_id}>{n.name}</li>)}</ul>;
}
```

**Helper hooks provided:**
- `useNodes()` — All nodes as array
- `useOnlineNodes()` — Non-offline nodes
- `useSortedNodes()` — Sorted for display
- `useSelectedCount()` — Primitive value (no memo needed)
- `useIsNodeSelected(id)` — Boolean per node
- `usePresets()` — All presets

### Pattern 2: Connection Manager with Exponential Backoff

**Principle:** Wrap EventSource in a manager class that handles lifecycle and retry logic.

```ts
// src/lib/connectionManager.ts

class SSEConnectionManager {
  private retryDelay = 1000;     // Start at 1s
  private maxDelay = 30000;      // Cap at 30s
  
  handleError(): void {
    this.closeEventSource();
    
    // Schedule reconnect with backoff
    setTimeout(() => this.connect(), this.retryDelay);
    
    // Increase delay for next failure
    this.retryDelay = Math.min(this.retryDelay * 2, this.maxDelay);
  }
  
  handleSuccess(): void {
    this.retryDelay = 1000; // Reset on success
  }
}
```

**Features:**
- Exponential backoff (1s → 2s → 4s → ... → 30s max)
- Debounces rapid errors (ignores errors within 1s of previous)
- Clean connect/disconnect lifecycle
- Singleton pattern for app-wide use

### Pattern 3: API Client with Response Transformers

**Principle:** Explicitly define backend response types and transform to frontend types.

```ts
// src/lib/apiClient.ts

// 1. Define what backend actually returns
interface BackendNode {
  node_id: string;
  name: string;
  rssi: number;
  // ... exact backend fields
}

// 2. Transform to frontend type
function transformNode(backend: BackendNode): Node {
  return {
    node_id: backend.node_id,
    name: backend.name || backend.node_id, // Default if missing
    rssi: backend.rssi,
    // ... with safe defaults
  };
}

// 3. Export clean API
export const apiClient = {
  async fetchNodes(): Promise<Node[]> {
    const data = await fetchJson<BackendNode[] | null>('/nodes');
    return (data || []).map(transformNode);
  },
};
```

**Benefits:**
- Contract is explicit in TypeScript
- Handles null/undefined gracefully
- Easy to spot when backend changes
- Single place to update when format changes

## Design Decisions

### Why not use `shallow` from Zustand?

`shallow` comparison helps, but doesn't prevent the fundamental issue: if you create a new array in the selector, you get a new reference every time. `shallow` only checks if array *contents* changed, not if transformation is needed.

Best practice: Don't transform in selector. Select raw data, transform in useMemo.

### Why a class for ConnectionManager?

The connection lifecycle has state (retryDelay, timeout ID, EventSource instance). A class encapsulates this cleanly. Singleton pattern ensures only one connection exists.

### Why explicit Backend types?

TypeScript's structural typing means we could just use `Node` for both. But this hides the contract and makes drift silent. Explicit backend types make the boundary visible.

## Implementation Plan

1. ✅ Create `src/lib/useStableSelector.ts` with node/preset hooks
2. ✅ Create `src/lib/connectionManager.ts` with SSE manager
3. ✅ Create `src/lib/apiClient.ts` with transformers
4. ✅ Migrate screens to use new hooks (commit 79e4300)
5. ⬜ Add tests for edge cases (null data, rapid errors)

## File Reference

| File | Purpose |
|------|---------|
| `src/lib/useStableSelector.ts` | Pattern 1: Stable selector hooks |
| `src/lib/connectionManager.ts` | Pattern 2: SSE connection manager |
| `src/lib/apiClient.ts` | Pattern 3: API client with transformers |

## Related

- [Implementation Diary](../reference/01-implementation-diary.md) — See Steps 2-3 for debugging history
- [Parent Design Doc](../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/02-design-go-host-http-api-preact-zustand-ui-embedded.md) — Overall architecture
