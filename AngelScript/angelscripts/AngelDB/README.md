AngelDB SQL Update System
========================

## How it works

1. Place `.sql` files in the `pending/` directory.
2. When the server starts or AngelScript is reloaded:
   - Files are sorted alphabetically
   - Each file is executed against the AngelDB database
   - On success, the file is moved to `applied/`
   - On failure, the file stays in `pending/` (check server logs)
3. Files in `applied/` are considered done and are never re-executed.

## File naming

Use numbered prefixes to control execution order:
  `0001_initial_schema.sql`
  `0002_add_spawns_table.sql`
  `0003_update_configs.sql`

Only `.sql` files are processed. Other files are ignored.

## Safety

- Use `IF NOT EXISTS` / `IF EXISTS` / `IGNORE` to make updates idempotent
- Failed files are NOT moved — they stay in `pending/` for retry
- Empty files are moved to `applied/` silently
- Files in `applied/` with duplicate names get a timestamp suffix

## Manual invocation

From AngelScript:
  AngelDB_RunPendingUpdates("path/to/updates/dir");
