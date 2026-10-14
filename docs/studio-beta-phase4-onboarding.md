# Studio Beta Phase 4: Onboarding and demos

PixelForge opens a dismissible Welcome workspace offering New RPG, New Platformer, Open, and one-click loading for each bundled example. The demo buttons load the same JSON documents through the existing validated Open path; dirty documents use the save/discard/cancel prompt before switching. The welcome text summarizes the RPG objective (talk with Mira and find the Moon crystal), the platformer objective (cross the hazard gap, touch the checkpoint, and reach Finish), and the controls.

Both sample levels are already installed with the Windows package. Their content and runtime behavior are covered by model/gameplay tests, and `editor_smoke` verifies the Welcome UI can render with SDL/ImGui in the headless Linux environment. Manual first-run and demo playthrough validation remains necessary on Windows.
