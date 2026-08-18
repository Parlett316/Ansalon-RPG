# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 33 (adaptive layout — the fixed frame
size finally got checked against a real terminal and didn't fit, since
Milestones 29-32 had grown it repeatedly without ever querying one. New
`Console::currentWindowSize()` reads the real visible console window
(`srWindow`, not the taller scrollback `dwSize`), and
`MapRenderer::configureLayout` sizes the frame to it once at startup —
shrinks the map viewport toward a measured real floor (44×16, the
widest/tallest authored zone) before ever shrinking the log panel below
20 columns, fails fast with a clear message below the absolute minimum
(70×23). Adapts once at launch, not continuously. See
`docs/ARCHITECTURE.md`).

**Important — this is the one milestone that actually needs the user's
real terminal to confirm it worked.** The reported bug was "the game is
bigger than my console window" — ask them to relaunch and confirm the
frame now fits without scrolling before considering this closed. Every
milestone since 29 has carried some form of "needs a real interactive
check," but this one specifically exists *because* that check was
skipped for too long — worth being more insistent about it this time.

Next step: confirm the sizing fix actually worked for the user, then
ask what's next (backlog menu).
