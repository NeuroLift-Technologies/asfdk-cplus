# Agent Contribution Checklist

## Pre-Commit

- [ ] Commit message follows NLT format: `[AGENT_NAME] type(scope): description`
- [ ] Task scope confirmed and does not conflict with existing work
- [ ] No architecture decisions made without human approval
- [ ] No LLM provider lock-in committed
- [ ] No credentials, secrets, or tokens in code
- [ ] No external service integrations without approval

## PR Requirements

- [ ] All status checks pass (green)
- [ ] At least 1 human approval given
- [ ] No unresolved review comments
- [ ] PR description is complete using this template
- [ ] Branch is up to date with target branch
- [ ] Scope declaration documented if new top-level directories added

## Governance Checks

- [ ] `validate-governance.sh` passes (run locally before pushing)
- [ ] NLT-DEV-OTOI.md version is current (1.0.3)
- [ ] AGENTS.md agent registry is up to date
- [ ] CLAUDE.md commit format is correct
- [ ] No OTOI self-amendment without formal process

## Post-Merge

- [ ] Handoff record written if session ended
- [ ] Active threads updated (`docs/active-threads.md`)
- [ ] Any escalations documented in `docs/escalations/`