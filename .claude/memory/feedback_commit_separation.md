---
name: Separate context and code commits
description: Never mix .claude/ memory/settings changes with source code or README changes in the same commit
type: feedback
---

Always separate context commits from code/README commits. Never mix them unless explicitly requested.

- **Context commits**: `.claude/memory/`, `.claude/settings.json`, `.claude/plans/`, `CLAUDE.md` — these are project knowledge and instructions
- **Code commits**: source files (`.ino`, `.h`, `.py`, etc.) and `README.md`

**Why:** The user considers these fundamentally different types of changes. Context is metadata about the project; code/README is the actual project. Mixing them makes git history unclear.

**How to apply:** When committing, always check `git diff --cached --stat` to ensure only one category is staged. If both need committing, make two separate commits.
