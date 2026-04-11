# Task: EigenD Sync Agent(s)
<!-- status: active -->
<!-- created: 2026-04-11  refs: docs/reference/eigend-sync.md, docs/roadmap.md -->

## Goal
Design and implement agent(s) that periodically compare EigenD and EigenLite sources, surface drift in directly-copied files, and track intentional divergences where the two projects solve the same problem differently.

## Context
EigenLite shares three driver directories with EigenD (`lib_alpha2/`, `lib_pico/`, `picross/`) that are manually kept in sync. With EigenD 3.0 seeing active development, drift risk is increasing.

Beyond direct copies, some EigenLite code is a C++ reimplementation of EigenD Python modules (e.g. firmware loading is Python in EigenD, C++ in EigenLite; key normalisation in plg_pkbd/plg_keyboard maps to ef_pico/ef_alpha/ef_tau). These equivalences are currently undocumented and untracked. Divergence here can be intentional (EigenLite simplifies, e.g. hysteresis) or accidental — but either way should be explicit.

The devnotes explicitly called this out as a known gap. EigenD 3.0 activity makes it timely.

## Scope
In: agent design, divergence registry document, periodic scheduling
Out: actually merging any changes from EigenD (that's a separate decision each time)

## Cross-Project Architecture

The user is actively using AI agents on the EigenD project. This creates an opportunity to split responsibility at the project boundary rather than giving a single EigenLite agent the burden of understanding both codebases:

```
EigenD project
  └── Agent: EigenD Module Documenter
        reads: plg_pkbd, plg_keyboard, firmware Python scripts, lib_alpha2/lib_pico
        writes: docs/eigenlite-interface.md  (structured, EigenLite-facing)
              → committed to EigenD repo (or a shared location)

EigenLite project
  └── Agent 2: Equivalence Audit
        reads: eigenapi/src/, EigenD's docs/eigenlite-interface.md
        does NOT need to read EigenD source directly
```

**Benefits:**
- EigenLite agents don't need EigenD source in context — lower cost, more accurate
- EigenD documentation stays current as 3.0 evolves (EigenD agent regenerates it)
- EigenD module documenter is useful to EigenD independently, not just for EigenLite
- Clear contract between projects: EigenD publishes what it does; EigenLite consumes it

**What `docs/eigenlite-interface.md` in EigenD should contain** (per relevant module):
- What the module does (key normalisation, strip handling, firmware loading, etc.)
- Input: raw sensor data format / ranges
- Output: normalised values and their semantics
- Any configurable parameters (hysteresis windows, gains, thresholds)
- Known changes between 2.2 and 3.0

**Coordination needed:** align with EigenD AI setup so the EigenD-side agent uses a compatible output format. The EigenLite divergence registry (`docs/reference/eigend-divergence.md`) becomes the EigenLite-side counterpart.

## Proposed Agents / Tasks

### Agent 1: Direct Copy Diff
**Trigger:** periodic (e.g. weekly), or manually before any sync work  
**Inputs:** path to local EigenD clone (`~/projects/EigenD`), branch (`2.2` or `3.0`)  
**What it does:**
- Diffs `eigenapi/lib_alpha2/` vs EigenD `lib_alpha2/`
- Diffs `eigenapi/lib_pico/` vs EigenD `lib_pico/`
- Diffs `eigenapi/picross/` vs EigenD `picross/`
- Reports: files changed, files added/removed in EigenD since last sync
- Optionally shows per-file diffs

**Output:** structured report — which files differ, size of diff, whether changes look mechanical (whitespace/rename) or substantive

---

### Agent 2: Equivalence Audit
**Trigger:** manually, or when Agent 1 reports significant EigenD changes  
**What it does:**
- Compares EigenLite `eigenapi/src/` files against their known EigenD equivalents (see table below)
- For each pair: summarise what EigenD does vs what EigenLite does, highlight semantic differences
- Flags new functionality in EigenD that EigenLite lacks

**Known equivalences to track:**

| EigenLite | EigenD equivalent | Notes |
|---|---|---|
| `ef_pico.cpp` | `plg_pkbd` (C++), pico_manager Python glue | Key normalisation, strip, breath |
| `ef_alpha.cpp` / `ef_tau.cpp` | `plg_keyboard` (C++), alpha_manager Python glue | Key layout, bitmap diff |
| `ef_harp.cpp::loadFirmware` | `lib_alpha2/alpha2_usb`, EigenD Python firmware scripts | C++ vs Python IHX loading |
| `eigenlite.cpp::poll` | EigenD agent/clock model | Polling vs event-driven |

**Output:** per-pair divergence notes, tagged as: `intentional-simplification`, `unreviewed`, `needs-porting`, `EigenLite-specific`

---

### Agent 3: Divergence Registry Maintainer
**Trigger:** after Agent 2 runs, or when a known divergence changes  
**What it does:**
- Maintains `docs/reference/eigend-divergence.md` — a living registry of known intentional divergences
- Each entry: what differs, why, whether EigenLite should eventually adopt EigenD's approach
- Flags entries that are now stale (EigenD changed the thing EigenLite diverged from)

---

## Suggested Implementation Approach

1. Start with **Agent 1** — pure diff, no judgement needed, high value immediately
2. Create `docs/reference/eigend-divergence.md` manually first (seed it from devnotes + known-issues) so Agent 3 has a baseline to work from
3. Agent 2 is the hardest — requires reading and understanding both codebases. Best run as an explore-type subagent with both repos in context
4. Use `/schedule` for periodic Agent 1 runs once it's working

## Decisions
<!-- Updated during work -->

## Outcome
<!-- Filled on completion -->
