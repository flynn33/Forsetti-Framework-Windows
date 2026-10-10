# Discussion Moderation Policy

Date: 2026-05-15
Project: Forsetti Framework - Windows
Status: retained moderation policy and local helper; hosted workflow absent from this checkout

## Purpose

This policy defines the repository-side moderation rules for GitHub Discussions. It exists so automated moderation can remove clearly disallowed content without inventing standards outside the repository.

The checkout retains [discussion_moderator.py](../../Scripts/github/discussion_moderator.py) and [moderation configuration](../../.github/discussion_moderation_policy.json), but no GitHub Actions workflow. The actions and schedule below describe the intended authorized automation; they are not evidence that a hosted bot or schedule is active. Hosted setup requires separate authorization and execution verification.

The moderation policy applies to discussions about repository code, documentation, Wiki pages, governance, and support topics.

## Authority Sources

The moderation responder is anchored to:
- `CODE_OF_CONDUCT.md`
- GitHub's contributor-guideline and moderation surfaces referenced by the repository owner

Repository authority wins for this repo. The bot does not create new social rules beyond the local code of conduct and the platform-standard expectation of respectful discussion.

## Moderated Content

The moderation design calls for removing threads or comments when the post contains:
- racist or ethnically demeaning slurs
- vulgar, obscene, or foul language
- harassing or demeaning attacks directed at other people
- threats, intimidation, or encouragement of self-harm

These categories operationalize the existing code of conduct language around harassment, professional conduct, and inappropriate behavior in project spaces.

## Enforcement Actions

The planned hosted workflow has two forms of enforcement:
- event-driven moderation on newly created or edited discussions and discussion comments
- scheduled board scans every six hours to catch anything missed by event delivery

When an authorized run detects a violation, the intended actions are:
- deletes the violating discussion or discussion comment
- writes an incident note to the repository's moderation log issue
- writes a job summary for auditability

## Optional Blocking

User blocking requires separately authorized credentials. The retained design permits blocking attempts only when the repository owner supplies `DISCUSSION_MODERATION_ADMIN_TOKEN` with the required permissions.

When that token is present in an authorized run, the design limits blocking attempts to:
- `hate_speech`
- `violent_threats`

A failed blocking attempt must be recorded separately from any confirmed content removal. Missing or insufficient credentials do not establish that either action succeeded.

## Reporting Model

The reporting design uses a repository issue titled `Discussion Moderation Incident Log`. An authorized run should create that issue when needed and append incident comments; the retained files do not establish that this issue exists or that reporting is active.

## Boundaries

The moderation bot is intentionally conservative in scope:
- it only moderates GitHub Discussions content
- it only applies the repository-defined rule categories from `.github/discussion_moderation_policy.json`
- it does not rewrite or sanitize a post in place; it removes the violating item
- it does not claim successful user blocking unless GitHub confirms it
