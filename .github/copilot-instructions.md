
In all interactions and commit messages, be extremely concise and sacrifice grammar for the sake of concision


## Task Workflow

Active tasks live in `ai/work/`. Closed tasks move to `ai/archive/`. Cross-references are maintained in `docs/roadmap.md` and `docs/known-issues.md`.

## Task Mode Selection

- Infer task mode by default from user intent.
- If intent is ambiguous, ask one clarifying questions
- Canonical task instructions live in `ai/tasks/`.

## Documentation Source of Truth

When guidance conflicts, use this precedence:

1. `docs/api-reference.md` - user facing api description and usage
2. `docs/technical-requirements.md` — required behavior and constraints
3. `docs/design-notes.md` — current architecture and boundaries
4. `docs/product-overview.md` — product intent and direction
5. `docs/reference/*` — domain-specific deep dives
6. `docs/developer-preferences.md` — developer approach and design preferences
7. `docs/architectural-patterns.md` - patterns used in design

## Validation Policy

- Prefer real validation when tools and environment allow it.
- Standard loop: build → test → validate.
- If real execution is unavailable, state this explicitly and provide concrete validation steps.
- Do not claim successful validation without running it.

## Execution Guidelines

- Reason before producing output:
  1. Understand the scope and constraints
  2. Propose alternatives if multiple approaches exist
  3. Surface tradeoffs and potential risks
  4. Execute according to the inferred or explicit task mode
- Always clarify if the task or requirements are ambiguous.

## Planning & Decision-Making Rules

- Preserve long-term clarity over short-term expedience.
- Maintain alignment with product goals and architectural principles.
- Prefer deeper inspection for large or cross-cutting changes.
- Explicitly document recommendations, tradeoffs, and next steps.
- At the end of each plan, give me a list of unresolved questions to answer, if any. Make the questions extremely concise. Sacrifice grammar for the sake of concision.


## Codebase Navigation

| Location | Purpose |
|---|---|
| `eigenapi/eigenapi.h` | **Public API** — single header users include |
| `eigenapi/src/eigenapi.cpp` | Thin PIMPL facade; delegates to EigenLite |
| `eigenapi/src/eigenlite_impl.h` + `eigenlite.cpp` | Core orchestrator: USB discovery, device lifecycle, event dispatch |
| `eigenapi/src/ef_harp.h` + `ef_harp.cpp` | Device base class; IHX firmware loading; hysteresis |
| `eigenapi/src/ef_pico.cpp` | Pico implementation + strip state machine + breath warmup |
| `eigenapi/src/ef_basestation.cpp` | BaseStation (Alpha/Tau); auto-detects instrument type |
| `eigenapi/src/ef_alpha.cpp` | Alpha key layout + key-off bitmap diff |
| `eigenapi/src/ef_tau.cpp` | Tau key layout + mode→button mapping |
| `eigenapi/src/fwr_embedded.cpp` + `fwr_posix.cpp` | Firmware reader implementations |
| `eigenapi/lib_alpha2/` | EigenD driver for Alpha/Tau — **sync with EigenD, don't modify independently** |
| `eigenapi/lib_pico/` | EigenD driver for Pico — **sync with EigenD, don't modify independently** |
| `eigenapi/picross/` | EigenD platform abstraction — **sync with EigenD, don't modify independently** |
| `eigenapi/resources/firmware/` | Firmware: `ihx/` = source files; `cpp/` = compiled-in arrays |
| `resources/picodecoder/` | Closed-source picodecoder prebuilt libs per platform/arch |
| `tests/eigenapitest.cpp` | Reference usage example |
| `docs/archive/devnotes.md` | Historical design rationale and original goals |

## Optional References

- `docs/known-issues.md` — Active technical debt
- `docs/roadmap.md` — Planned features and design considerations
- `ai/work/*` - active tasks
- `ai/archive/*` - completed tasks
