# Project Rules (SkidBase)

These rules govern how changes are made to this repository.

## Scope & minimalism
- Do exactly what is asked — no unrequested features, refactors, new files, or polish.
- Prefer editing existing files over creating new ones. Make the fewest changes that satisfy the request.
- Match the existing code style and conventions of the surrounding code.

## Offsets / structures / encryption
- The yubx set is the source of truth for Roblox offsets, client struct layouts, and the Luau pointer-encryption (encs) mapping. Do not silently revert these values.
- Swap offset values in place; do not restructure the `Main` namespace or replace whole files unless explicitly asked.
- Keep the `VMValue0..4` encryption templates and the alias macros intact.

## Code cleanliness
- Keep the codebase clean: no messy or throwaway code.
- No comments anywhere in the codebase at all.
- No em dashes anywhere (including prose and documentation).
- No useless or no-op print/log statements.

## Safety & authorization
- Ask before destructive or hard-to-undo actions: deleting files, force-pushing, publishing/deploying, or changing system-wide/git settings.
- Do not push or publish unless the user explicitly authorizes it.

## Verification
- Where the environment allows, run the relevant checks. Report failures honestly as failures, never as passes.
- This repo is a Windows/Visual Studio build; when it cannot be compiled in the current environment, say so explicitly and mark the change as unverified rather than claiming it builds.

## Dependencies
- Preserve lockfiles; do not upgrade dependencies just to restore them.
- Keep the Luau dependency on the swapped-in supQ source, with the project's own encryption (Encs.hpp) and the yubx struct layouts overlaid on top.
