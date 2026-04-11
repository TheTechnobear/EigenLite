# task-new

Create a new task file in `ai/work/`.

## Steps

1. **Clarify the task** (if `$ARGUMENTS` is empty, ask for a one-sentence description; otherwise use it as the starting point). Do not ask implementation questions — just enough to write a clear Goal.

2. **Check for existing references.** Read `docs/roadmap.md` and `docs/known-issues.md`. Show any items that appear related and ask:
   - Does this task link to an existing roadmap or known-issues entry? (if so, capture the ref)
   - Does a new entry need to be added to either doc? (if yes, propose the addition and ask for confirmation before writing)
   - **After creating the task file**, add a link to it in the matched roadmap/known-issues entry (e.g. append `— see \`ai/work/<filename>\`` to the Status line). Do this without asking.

3. **Ask for scope** — one brief question: is there anything explicitly out of scope worth noting?

4. **Create the file** at `ai/work/YYMMDD-<short-name>.md` using today's date and a slug derived from the goal. Use this template exactly:

```markdown
# Task: <Name>
<!-- status: active -->
<!-- created: <date>  refs: <roadmap/known-issues links, or none> -->

## Goal
<One clear sentence.>

## Context
<Why this matters. What triggered it. Any relevant background.>

## Scope
In: <what is included>
Out: <what is explicitly excluded, or "not defined" if none>

## Decisions
<!-- Updated during work -->

## Outcome
<!-- Filled on completion -->
```

5. Confirm the file path to the user and remind them to use `/task-update` to record decisions during work or close it out when done.

Keep the file short. The goal is a useful kickoff prompt and future reference, not a design document.
