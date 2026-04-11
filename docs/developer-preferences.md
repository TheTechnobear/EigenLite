# Developer Approach and Preferences

This document captures the developer's preferred approach to design, planning, implementation, and review. It is about *how* to think about and approach work on this codebase — not C++ formatting or language conventions.

It is intentionally lightweight and can evolve.

## Current Preferences

### Architecture and Design
- Prefer strong encapsulation over casual simplicity when they materially conflict.
- Prefer event driven architecture, model pushing changes to ui, and also other 'clients'
- Prefer abstractions that preserve future extension paths without introducing speculative framework layers.
- Prefer explicit ownership and clear boundaries over convenience shortcuts.
- Favor designs that reduce future large-scale rewrites.

### Implementation Style
- Keep changes minimal and focused.
- Prefer clean domain ownership of state rather than spreading type-specific behavior across coordinator classes.
- Avoid speculative generalization when the current requirement can be met with a smaller, well-bounded abstraction.

### Review and Planning Preferences
- Surface tradeoffs explicitly when multiple architectural directions are viable.
- Distinguish between current-state design, future options, and deferred decisions.
- Preserve long-term architectural clarity even when implementing iteratively.

Context-dependent preferences above are a summary; canonical task mode instructions live in `ai/tasks/`.