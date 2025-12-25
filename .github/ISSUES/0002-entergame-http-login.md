# Issue: Change to HTTP-login behavior in `EnterGame.doLogin()` (`entergame.lua`)

- **Severity:** Major
- **Files affected:** `data/modules/client_entergame/entergame.lua`
- **Baseline commit:** `70e3abf2416935a9fe967050211d6e8beaa0fcdb`
- **Current branch/commit:** `Java-21-update` / `3af4ebe64`

## Description

Behavior was changed so that HTTP login attempts are now conditional on the `httpLogin` checkbox. Historically the client attempted HTTP login for client versions >= 1281 on non-7171 ports regardless of that checkbox. The code has been restored in this branch to preserve the previous behavior.

## Impact

- If the conditional behavior had been deliberate, restoring the previous behavior may re-enable HTTP login attempts that were intentionally disabled.
- If the behavior is left as-is (restored), this preserves backward compatibility for servers requiring HTTP login on non-standard ports.

## Suggested remediation / next steps

1. Confirm desired policy: should HTTP login be gated by the UI checkbox, or should it be attempted automatically for newer clients on non-standard ports?
2. If gating is desired, update UI/text to make the option obvious to users and update tests accordingly.
3. Add a small unit/integration test around `EnterGame.doLogin()` behavior for both paths.

## Notes / References

- Affected code location: `EnterGame.doLogin()` near decision to call `EnterGame.tryHttpLogin`.
