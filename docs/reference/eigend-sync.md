# EigenD Sync Reference

Context for keeping EigenLite's driver directories aligned with EigenD.

---

## Repository

Canonical EigenD fork: https://github.com/thetechnobear/EigenD

Local clone: `~/projects/EigenD`

The original Eigenlabs EigenD repo is unmaintained; all active work is in TheTechnobear's fork.

## Branches

| Branch | Status | Notes |
|--------|--------|-------|
| `2.2`  | Current release | EigenLite driver code is based on this |
| `3.0`  | Active development | Modernisations in progress; not yet the sync basis |

When syncing driver code into EigenLite, pull from the `2.2` branch unless specifically adopting 3.0 changes.

## Directories to Sync

| EigenLite path | EigenD equivalent |
|---|---|
| `eigenapi/lib_alpha2/` | `lib_alpha2/` |
| `eigenapi/lib_pico/` | `lib_pico/` |
| `eigenapi/picross/` | `picross/` |

**Do not modify these in EigenLite independently.** Make changes in EigenD first, then copy here.

No patches have been applied to these directories in EigenLite that would block a clean copy. Goal is zero divergence.

## EigenLite-Specific Code (not in EigenD)

Everything under `eigenapi/src/` is EigenLite-specific. The rough EigenD equivalents are:

| EigenLite file | EigenD equivalent |
|---|---|
| `ef_pico.cpp` | `plg_pkbd` (pico_manager Python module, C++ implementation) |
| `ef_basestation.cpp` / `ef_alpha.cpp` / `ef_tau.cpp` | `plg_keyboard` (alpha_manager) |
| `eigenlite.cpp` | no direct equivalent — EigenLite-specific orchestration |

## AI Agent Architecture (planned)

Rather than EigenLite agents reading EigenD source directly, the intended approach is:

- An **EigenD-side agent** reads the relevant EigenD modules and produces structured documentation (`docs/eigenlite-interface.md` in the EigenD repo), updated when EigenD changes
- **EigenLite agents** consume that output instead of EigenD source — lower context cost, stays current as EigenD 3.0 evolves
- EigenLite maintains `docs/reference/eigend-divergence.md` as its side of the contract

See `ai/work/260411-eigend-sync-agents.md` for full design.

## Sync Process (manual, no tooling yet)

1. Check EigenD `2.2` branch for changes to `lib_alpha2/`, `lib_pico/`, `picross/`
2. Diff against EigenLite copies
3. Copy changed files
4. Verify EigenLite still builds and Pico/Alpha/Tau still function

Automated tooling for this is tracked in `docs/roadmap.md`.
