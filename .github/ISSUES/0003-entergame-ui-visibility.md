# Issue: Server/client UI widgets hidden by default in `entergame.otui`

- **Severity:** Major
- **Files affected:** `data/modules/client_entergame/entergame.otui`
- **Baseline commit:** `70e3abf2416935a9fe967050211d6e8beaa0fcdb`
- **Current branch/commit:** `Java-21-update` / `3af4ebe64`

## Description

Several UI elements used for manual server selection and client version input were set to `visible: false`, hiding them by default. These include `serverLabel`, `serverListButton`, `serverHostTextEdit`, `clientLabel`, `clientComboBox`, `portLabel`, and `serverPortTextEdit`.

## Impact

- Users lose the ability to manually input or select host, port, and client version from the main Enter Game UI.
- This reduces flexibility for connecting to custom or local servers and may impede debugging and local test workflows.

## Suggested remediation / next steps

1. If hiding was accidental, restore `visible: true` for the affected widgets (done in `Java-21-update`).
2. If the UI should be simplified, provide an alternate advanced/legacy UI switch or settings page instead of hiding controls without notice.
3. Add a small UI test to verify visibility and functionality for server selection UI elements.

## Notes / References

- Current file: `data/modules/client_entergame/entergame.otui`
- Restored visibility changes are in branch `Java-21-update` (commit `3af4ebe64`).
