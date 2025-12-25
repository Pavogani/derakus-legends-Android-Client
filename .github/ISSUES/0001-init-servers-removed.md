# Issue: Removed local server entries from `Servers_init` in `data/init.lua`

- **Severity:** Major
- **Files affected:** `data/init.lua`
- **Baseline commit:** `70e3abf2416935a9fe967050211d6e8beaa0fcdb`
- **Current branch/commit:** `Java-21-update` / `3af4ebe64`

## Description

Several local server entries ("Local 7.72", "Local 8.60", "Local 10.98", "Local 12.64") were removed from the `Servers_init` table in `data/init.lua`. This change prevents those servers from appearing in the built-in server selector and may block local development or testing workflows that rely on these entries.

## Impact

- Users will not see or be able to select the removed local servers from the UI.
- Automated or local test setups that expect these presets may fail or require manual configuration.

## Suggested remediation / next steps

1. Confirm whether removal was intentional. If not intentional, restore the local entries from the baseline commit.
2. If removal is intentional, document the change in the release notes and provide instructions for adding local entries back to `config.otml` or runtime settings.
3. Optionally add a small regression test or a scripted check to ensure expected server presets exist for developer builds.

## Notes / References

- Baseline snippet: `git show 70e3abf2416935a9fe967050211d6e8beaa0fcdb:data/init.lua`
- Current file: `data/init.lua`
