---
name: Ask before significant changes
description: Always ask before making large/significant changes to existing files — never rewrite a working file without confirmation
type: feedback
---

Never make significant changes to existing working files without asking first. Adding an #include is fine. Rewriting the entire file is not — ask first.

**Why:** User was frustrated when .ino was fully rewritten (JSON-RPC → binary) without confirmation. The instruction was just to add the header, not rewire everything.
**How to apply:** When the scope of a change goes beyond what was explicitly asked (e.g., "wire" could mean "add include" vs "rewrite"), clarify before touching the file. Small additive changes are safe. Rewrites require explicit approval.
