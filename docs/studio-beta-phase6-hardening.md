# Studio Beta Phase 6: Quality and hardening

Opening a level now tears down an active playtest only after the JSON, its safe asset reference, and replacement texture have loaded successfully. A failed open therefore leaves the current document and session running. Creating a new level ends the session only after the new level validates. The toolbar Stop command and the application smoke path share the same RAII session reset.

The displayless editor smoke is now an integration check rather than a startup-only check. It draws Welcome, loads the packaged RPG example and starts its real gameplay session, stops and starts it again, then loads and renders the platformer session. It continues to resize the SDL window and rebuild the font atlas at all four supported scale presets. The asset path suite also asserts that missing relative assets fail clearly.

Local Linux checks: 81 level/serialization checks, 62 document/history checks, 74 gameplay checks, and all four CTest targets pass. AddressSanitizer and UndefinedBehaviorSanitizer run the three core CTest targets with leak detection and halt-on-error enabled; all three pass. Hosted Windows and Ubuntu workflows are required before this phase is complete. Windows native dialogs and interactive play still need manual verification.
