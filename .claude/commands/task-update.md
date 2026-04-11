# task-update

Update an existing task file in `ai/work/`.

## Steps

1. **Find the task.** If `$ARGUMENTS` names a task (partial match is fine), locate the file in `ai/work/`. If ambiguous or empty, list the files in `ai/work/` and ask which to update.

2. **Determine update type.** Ask (or infer from context) whether this is:
   - **In-progress update** — record decisions, findings, or status changes made during this session
   - **Close out** — task is done; write the Outcome and archive

### In-progress update
Append concise entries to the `## Decisions` section. Use the format:
```
- <YYYY-MM-DD> <decision or finding, one line>
```
Do not rewrite the whole file. Do not add implementation detail — record *what was decided and why*, not *how the code works*.

### Close out
1. Write a brief `## Outcome` section: what was done, what was deferred, any key decisions future work should know.
2. Check `docs/roadmap.md` and `docs/known-issues.md`: if this task resolves or changes any entry there, propose the update and ask for confirmation before writing.
3. Move the file from `ai/work/` to `ai/archive/` (rename; keep the same filename).
4. Confirm the archive path to the user.

Keep all additions brief. The task file is a reference, not a log.
