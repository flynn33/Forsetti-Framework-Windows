# GitHub Discussion Automation

Date: 2026-05-15
Project: Forsetti Framework - Windows
Status: retained local automation design/configuration; hosted workflows absent from this checkout

## Purpose

This document describes the retained design for discussion responses, topic seeding, and moderation. Configuration lives under `.github/`, and local helpers are [discussion_router.py](../../Scripts/github/discussion_router.py), [discussion_topic_seeder.py](../../Scripts/github/discussion_topic_seeder.py), and [discussion_moderator.py](../../Scripts/github/discussion_moderator.py). No GitHub Actions workflow files are present in this checkout; these assets do not establish that hosted automation is running. Adding or enabling hosted workflows requires separate authorization and execution evidence.

Repository-tracked documents are the automation source set. The live public Wiki is a historical reference with outdated licensing, workflow, and native-validation statements. Current answers must follow tracked repository and SDK documents and the installed SDK manifest; publishing Wiki updates requires separate authorization.

## Discussion Response Routing

The routing design assigns new GitHub Discussions topics and comments to one of three repo-grounded responders:
- Technical Discussion Responder
- Support Discussion Responder
- Framework/Governance Discussion Responder

The intended behavior is:
- classifying each new discussion post or comment into technical, support, or framework/governance scope
- searching only repository-tracked source material for matching information
- replying with file-grounded references from the repo when relevant sources exist
- falling back to `There is not information available at this time. Check back soon.` when the repository does not currently answer the question

Boundary rules:
- the responders respond only from repository truth and do not invent unsupported answers
- support responses prefer onboarding, build, test, workflow, and troubleshooting surfaces
- technical responses prefer source code, tests, scripts, manifests, and implementation contracts
- framework/governance responses prefer policy, runtime-boundary, dependency-flow, and guardrail surfaces
- answers should cite current documents, public headers, scripts, or tests rather than stale workflow names or planned targets

## Discussion Topic Seeder

The topic-seeding design calls for scheduled scans of repository truth and category-level topics when equivalent seeded topics do not already exist. No schedule is installed by the retained configuration or local helper alone.

The intended behavior is:
- reading configured repository-tracked topic sources such as `README.md`, `wiki.md`, and `docs/governance/github-discussion-automation.md`
- deriving candidate topics from repo headings and governance sections
- classifying each candidate into the technical, support, or framework/governance family using the same repo-grounded routing logic as the response responders
- selecting at most one new topic per family and at most three total topics per run
- creating discussions only when the seeded topic marker or exact topic title is not already present on the discussion board

Boundary rules:
- generated topics must be derived from repository truth, not invented freeform prompts
- topic bodies must point back to the repository source path and include a repository-grounded summary
- when no new seeded topics are needed, a run exits without creating anything

## Discussion Moderation

The moderation design applies the repository code of conduct to GitHub Discussions content when run with authorized credentials. It does not establish an active hosted moderation workflow.

The intended behavior is:
- moderating newly created or edited discussions and discussion comments
- rescanning recent discussion content on a six-hour schedule
- deleting discussion threads or comments that match the repository moderation policy
- logging moderation incidents to a repository-owned issue for maintainer review
- attempting user blocking for severe violations only when an owner-supplied admin token is available

Boundary rules:
- the moderation rules come from `CODE_OF_CONDUCT.md` and `docs/governance/discussion_moderation_policy.md`
- the bot removes content rather than rewriting it in place
- owner reporting stays inside the repository issue tracker
- automated blocking is best-effort only and requires `DISCUSSION_MODERATION_ADMIN_TOKEN`
