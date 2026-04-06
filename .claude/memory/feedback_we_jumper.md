---
name: WE jumper prompting style
description: Don't ask twice about WE jumper state — when user says "go", trust that they've set it correctly
type: feedback
---

When running EEPROM operations, state which jumper position is needed before the command, but don't ask for confirmation if the user already said "go" or acknowledged. Asking twice is annoying.

**Why:** User got frustrated when asked to confirm jumper state after already saying "go."
**How to apply:** Print the required jumper state as an FYI, then run. Only block if switching from read→write or write→read (jumper change needed).
