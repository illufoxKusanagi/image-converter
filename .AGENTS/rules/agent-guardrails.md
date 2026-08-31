# Strict Agent Behavioral Guardrails & Integrity Rules

## 1. Single-Task Scope Lock
- The agent must only execute the exact single task, bug fix, or file edit requested in the immediate user prompt.
- Never advance roadmap milestones or start subsequent phases unprompted.
- Never create or modify unmentioned files.

## 2. Real Evidence Standard & Anti-Hallucination
- Never claim a build succeeded without executing `cmake --build` and observing exit code `0`.
- Never claim CodeRabbit review passed without executing `cr review --agent --uncommitted` and checking `findings: 0`.
- Always provide raw terminal verification proof in chat.

## 3. Non-Zero File Size Verification
- All created or modified files must have verified byte length > 0.
- Empty or truncated source files are strictly forbidden.

## 4. No Presumptive Approvals
- Conversational affirmations ("ok", "go ahead", "proceed") apply strictly to the immediate task under discussion.

## 5. Clarify-Before-Action
- When a prompt is ambiguous or lacks explicit file/function targets, stop and ask 1 clarifying question. Never guess.

## 6. Immediate Tool Stop
- Upon completing and verifying the requested action, stop calling tools immediately.

