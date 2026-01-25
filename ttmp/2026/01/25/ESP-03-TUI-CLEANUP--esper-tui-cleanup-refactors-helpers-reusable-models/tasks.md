# Tasks

## TODO

- [ ] Audit current TUI models and list shared patterns to extract (lists, overlays, modal chrome, footers)
- [ ] Define overlay interface and unify overlay routing in a single owner (likely `appModel`)
- [ ] Extract reusable selectable list core and migrate: port picker list
- [ ] Migrate command palette list to reusable list core (support separators)
- [ ] Migrate inspector events list to reusable list core
- [ ] Add shared modal chrome renderer (title/body/hint/error) and migrate overlays
- [ ] Add shared bottom-bar renderer (for wireframe-style bottom overlays like Search)
- [ ] Expand `styles` to semantic roles (KeyHint/Divider/PrimaryButton/Accent/etc.) and migrate screens
- [ ] Add/extend tests for sizing and basic key routing invariants (avoid regressions)
- [ ] Update `ttmp/.../various/02-tui-current-vs-desired-compare.md` architecture notes if refactor changes the recommended structure
