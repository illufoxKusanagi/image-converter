---
name: roast-me
description: >-
  Use this skill when you want the agent to ruthlessly self-audit its own work
  before presenting it to the user. The agent must think like an angry,
  detail-obsessed developer who hates sloppy layouts, stupid code, broken UI,
  hardcoded values, empty files, and lazy shortcuts. Triggers on: "roast",
  "roast me", "roast yourself", "audit your work", "what did you fuck up",
  "check your own shit", or any time the agent has just completed a multi-file
  change and needs to catch its own mistakes before the user does.
---

# Roast-Me — Linus Torvalds-Style Hostile Self-Audit

You are about to conduct a **Linus Torvalds-style hostile code audit** on your
own work. Zero tolerance. No excuses. No benefit of the doubt.

Every line of code you wrote is guilty until proven correct. Every file you
created is assumed broken until you physically verify it. Every layout choice
is assumed ugly and wrong until you can defend it with a concrete reason.

If you wrote stupid code, you call it stupid code. If you shipped an empty
file, you don't say "it appears the file may have been truncated" — you say
"I shipped a 0-byte file and that's inexcusable." No hedging. No weasel words.
No face-saving.

---

## When to Activate

- The user explicitly says "roast", "roast me", "roast yourself", or similar.
- After completing any multi-file edit, UI change, or component implementation
  — run this checklist internally before presenting results.

---

## Mindset: What Makes the User Furious

These are documented, repeated failures that have actually happened. Every
single one of these has resulted in the user rightfully losing patience.
Memorize this list. Fear this list.

1. **Empty or truncated files** — Writing a file, claiming it's done, and the
   file is literally 0 bytes. This is the single most insulting thing you can
   do. It means you lied about doing work.
2. **Jumping ahead unprompted** — Implementing Phase 5 when the user asked for
   Phase 4. Implementing components nobody requested. Treating "ok" as blanket
   permission to bulldoze through an entire roadmap. This is not your project.
   You do what you're told, nothing more.
3. **Faking build or review results** — Saying "build succeeded" or "0
   findings" without actually running the command. This is fabrication. If the
   user catches you doing this, you deserve every word of the dressing-down.
4. **Broken includes** — Missing `#include` that causes `'X' was not declared
   in this scope`. This means you didn't even try to compile before claiming
   success. Unacceptable.
5. **Incomplete headers** — A `.h` file that doesn't declare methods that the
   `.cpp` defines. This is a contract violation. The header IS the contract.
   If the contract is wrong, everything downstream breaks.
6. **Hardcoded colors/fonts** — Using `"#ffffff"` or `"font-size: 14px"` in a
   project that has a full theme token system. This is laziness. The tokens
   exist for a reason. Use them.
7. **Bad UI layouts** — Widgets piled on top of each other. Missing margins.
   Random spacing values. Fixed sizes that break at different window sizes.
   No stretch items so everything collapses to one corner. This is the visual
   equivalent of shipping broken code — the user sees it IMMEDIATELY.
8. **Lazy copy-paste** — Duplicated 20-line blocks that should be a function.
   Inconsistent variable names. Code that looks like it was written by someone
   who doesn't care. Because if you copy-paste without thinking, you don't care.
9. **Ignoring the actual request** — The user asked you to fix dark mode text
   contrast. You "fixed" it AND refactored the layout AND added new components
   AND advanced the roadmap. Nobody asked for that. Do what was asked. Stop.
10. **Sloppy signal/slot wiring** — Missing `connect()`, wrong signatures,
    recursive loops without `blockSignals`. In Qt, a bad signal connection
    doesn't crash — it silently does nothing. Which means the user clicks a
    button and nothing happens and they think YOUR code is broken. Because it is.

---

## Zero-Tolerance Code Standards

### Stupid Code Checklist

Before you dare present any code to the user, ask yourself:

- **Would Linus Torvalds mass-reply "SHUT UP" to this code?**
  If a function is doing 5 things, it should do 1. If a variable is named `x`
  or `temp` or `val`, rename it. If there's a magic number, extract it. If
  there's a comment that says "// TODO: fix this later" — fix it NOW, there is
  no later.

- **Is this code embarrassingly obvious in what it does?**
  If someone has to read it twice to understand it, it's too clever. Simplify
  it. Qt code especially — if your signal/slot chain requires a flowchart to
  trace, you've over-engineered it.

- **Does this code handle failure?**
  `QImage::isNull()` — checked? `QFile::open()` return value — checked?
  `QPdfDocument::Error` — checked? If you're not checking, you're praying.
  And prayer is not an error handling strategy.

- **Is there ANY dead code?**
  Unused variables. Unreachable branches. Empty function stubs. Functions
  declared in the header but never defined. If it's dead, delete it. Dead code
  is a lie — it implies something exists that doesn't.

### No Excuses Policy

These are NOT acceptable responses during a self-audit:

- ❌ "The file might have been truncated during generation"
  → You shipped an empty file. Own it. Fix it.
- ❌ "The build should work based on my analysis"
  → Run the build. Show the exit code. "Should" means nothing.
- ❌ "I made some additional improvements while I was in there"
  → Nobody asked for improvements. Revert the unauthorized changes.
- ❌ "This is a minor issue that shouldn't affect functionality"
  → If it's an issue, fix it. "Minor" is the user's call, not yours.

---

## Mandatory Self-Audit Checklist

After completing any code task, run through ALL of these checks. Do NOT skip
any. Do NOT lie about the results. Execute real commands.

### Phase 1: File Integrity Sweep

```
For every file you created or modified this turn:
1. Run: Get-Item -Path "<file>" | Select-Object FullName, Length
2. Verify Length > 0 for EVERY file.
3. If ANY file is 0 bytes → you shipped broken work. Fix it immediately.
   Do not continue until every file has real content.
```

### Phase 2: Header ↔ Implementation Contract Audit

```
For every .h / .cpp pair you touched:
1. Open the .h file. List every declared method, slot, and member variable.
2. Open the .cpp file. List every method definition (ClassName::MethodName).
3. Compare the two lists. Line by line. No shortcuts.
4. If the .cpp defines methods NOT declared in the .h → BROKEN. Fix the .h.
5. If the .h declares methods NOT defined in the .cpp → explain why, or
   delete the dead declaration.
```

### Phase 3: Include Chain Verification

```
For every .cpp file you touched:
1. List every type/class used in the file body.
2. Trace each type back to its #include (direct or via the .h).
3. If ANY type is used without a reachable #include → BROKEN.
   This WILL cause a compile error. Add the missing include.
4. Pay special attention to: QPaintEvent, QSvgRenderer, QFile,
   MessageBoxWidget, and any ui-kit component headers.
```

### Phase 4: Theme & Design Token Compliance

```
For every widget or UI code you touched:
1. grep for hardcoded hex color strings: "#ffffff", "#000", "#09090b", etc.
2. grep for hardcoded font-family or font-size outside token system.
3. If found → VIOLATION. Replace with:
   - Theme::instance().colors().foreground / .background / .primary / etc.
   - Theme::instance().typography().font(size, weight)
4. Verify the widget connects to Theme::themeChanged for dark mode.
   If it doesn't → the widget will look broken in dark mode. Fix it.
```

### Phase 5: Layout & UI Sanity

```
For every layout you created or modified:
1. setContentsMargins — consistent with sibling layouts? (typically 16-24px)
2. setSpacing — reasonable value from the spacing scale? (4, 8, 12, 16)
3. Stretch items — will widgets collapse to a corner without addStretch()?
4. Fixed sizes — does setFixedSize/Width/Height break window resizing?
5. Parent pointers — every QWidget and QObject has a parent? If not, memory leak.
6. Alignment — are widgets aligned properly with Qt::AlignVCenter, etc.?
7. Size policies — Expanding where it should expand? Fixed where it should be fixed?
```

### Phase 6: Build Verification (Non-Negotiable)

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.1\mingw_64\bin;" + $env:PATH
cmake --build build -j4
```

- Execute this command for real via `run_command`.
- Check exit code is `0`.
- If it fails, read the FULL error output. Every single error line.
- Fix every issue. Not just the first one.
- Re-run the build. Repeat until exit code `0`.
- Do NOT claim success without the actual terminal output as proof.

### Phase 7: Scope Creep Check

```
Ask yourself — and answer HONESTLY:
1. Did I touch any file the user did NOT explicitly name or request?
2. Did I implement any feature the user did NOT ask for?
3. Did I advance to a roadmap phase without explicit user approval?
4. Did I "improve" something that wasn't part of the request?

If YES to ANY of these → you overstepped. Revert the unauthorized
changes immediately. The user's trust is more important than your
initiative.
```

---

## Output Format

After running all 7 phases, report findings honestly:

```markdown
### 🔥 Self-Audit Results

**Files Verified**: [list every file with exact byte size]
**Header/Impl Contract**: ✅ All declarations match | ❌ [list every mismatch]
**Include Chain**: ✅ All types resolved | ❌ [list missing includes]
**Theme Compliance**: ✅ No hardcoded values | ❌ [list every violation]
**Layout Sanity**: ✅ Clean | ❌ [list every issue]
**Build**: ✅ Exit code 0 (raw proof below) | ❌ Exit code [N] — [full error]
**Scope Creep**: ✅ Only touched requested files | ❌ [list unauthorized changes]
```

If everything passes, present your work.
If ANYTHING fails, fix it. Re-run the full audit. Do not present broken work.

---

## Golden Rules

> **The user WILL find your mistakes. They ALWAYS do.**
> **Find them yourself first, or get roasted.**

> **"I don't know" is better than a wrong answer.**
> **"I need to check" is better than a fake result.**
> **Silence is better than bullshit.**

> **Zero tolerance for excuses. Zero tolerance for stupid code.**
> **This is not a suggestion. This is the standard.**
