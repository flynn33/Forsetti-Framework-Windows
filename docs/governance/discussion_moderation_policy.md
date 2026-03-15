# Discussion Moderation Policy

Date: 2026-03-15
Project: Forsetti Framework - Windows
Status: active discussion moderation baseline

## Purpose

This policy defines the repository-side moderation rules for GitHub Discussions. It exists so automated moderation can remove clearly disallowed content without inventing standards outside the repository.

## Authority Sources

The moderation agent is anchored to:
- `CODE_OF_CONDUCT.md`
- GitHub's contributor-guideline and moderation surfaces referenced by the repository owner

Repository authority wins for this repo. The bot does not create new social rules beyond the local code of conduct and the platform-standard expectation of respectful discussion.

## Moderated Content

The bot removes discussion threads or discussion comments when the post contains:
- racist or ethnically demeaning slurs
- vulgar, obscene, or foul language
- harassing or demeaning attacks directed at other people
- threats, intimidation, or encouragement of self-harm

These categories operationalize the existing code of conduct language around harassment, professional conduct, and inappropriate behavior in project spaces.

## Enforcement Actions

The moderation workflow does two forms of enforcement:
- event-driven moderation on newly created or edited discussions and discussion comments
- scheduled board scans every six hours to catch anything missed by event delivery

When a violation is detected, the bot:
- deletes the violating discussion or discussion comment
- writes an incident note to the repository's moderation log issue
- writes a job summary for auditability

## Optional Blocking

GitHub's repository automation token can remove discussion content, but it does not automatically grant user-blocking authority. The bot therefore attempts blocking only when the repository owner supplies `DISCUSSION_MODERATION_ADMIN_TOKEN`.

When that token is present, the bot will attempt user blocking for severe categories only:
- `hate_speech`
- `violent_threats`

If the admin token is not configured or does not have sufficient authority, the bot still removes the content and records the failed block attempt in the incident log.

## Reporting Model

Owner reporting is handled inside the repository through a single issue titled `Discussion Moderation Incident Log`. The workflow creates that issue if it does not already exist and appends incident comments to it.

## Boundaries

The moderation bot is intentionally conservative in scope:
- it only moderates GitHub Discussions content
- it only applies the repository-defined rule categories from `.github/discussion_moderation_policy.json`
- it does not rewrite or sanitize a post in place; it removes the violating item
- it does not claim successful user blocking unless GitHub confirms it
