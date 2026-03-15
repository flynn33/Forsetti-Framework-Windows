#!/usr/bin/env python3
"""Moderate GitHub Discussions content against repository-defined standards."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any

import discussion_agent

TEXT_ENCODING = "utf-8-sig"
DEFAULT_CONFIG_PATH = Path(".github") / "discussion_moderation_policy.json"
ALLOWED_EVENT_NAMES = {"discussion", "discussion_comment"}
LEET_MAP = {
    "a": "[a4@]",
    "e": "[e3]",
    "i": "[i1!|]",
    "o": "[o0]",
    "s": "[s5$]",
    "t": "[t7+]",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Moderate GitHub Discussions content against repo policy."
    )
    parser.add_argument("--root", default=os.getcwd(), help="Repository root path.")
    parser.add_argument(
        "--config",
        default=None,
        help="Optional path to the discussion moderation policy config.",
    )
    parser.add_argument(
        "--event-path",
        default=os.getenv("GITHUB_EVENT_PATH"),
        help="Path to the GitHub event payload JSON.",
    )
    parser.add_argument(
        "--event-name",
        default=os.getenv("GITHUB_EVENT_NAME"),
        help="GitHub event name.",
    )
    parser.add_argument(
        "--validate-config",
        action="store_true",
        help="Validate the moderation config and exit.",
    )
    parser.add_argument(
        "--analyze-text",
        default=None,
        help="Analyze plain text locally without calling GitHub.",
    )
    parser.add_argument(
        "--analyze-title",
        default="",
        help="Optional discussion title when using --analyze-text.",
    )
    parser.add_argument(
        "--analyze-kind",
        default="discussion_comment",
        choices=["discussion", "discussion_comment"],
        help="Synthetic content kind for --analyze-text.",
    )
    return parser.parse_args()


def read_text_file(path: Path) -> str:
    return path.read_text(encoding=TEXT_ENCODING)


def load_json_file(path: Path) -> Any:
    with path.open(encoding=TEXT_ENCODING) as handle:
        return json.load(handle)


def normalize_text(value: str) -> str:
    return re.sub(r"\s+", " ", value or "").strip().lower()


def validate_config(config: dict[str, Any], root: Path) -> list[str]:
    errors: list[str] = []

    if config.get("version") != "1.0":
        errors.append("discussion_moderation_policy.json must declare version 1.0.")

    policy_sources = config.get("policy_sources")
    if not isinstance(policy_sources, dict):
        errors.append(
            "discussion_moderation_policy.json must define policy_sources."
        )
    else:
        for key in ("code_of_conduct_path", "moderation_policy_path"):
            source_path = policy_sources.get(key)
            if not source_path:
                errors.append(
                    f"discussion_moderation_policy.json must define policy_sources.{key}."
                )
            elif not (root / source_path).exists():
                errors.append(
                    f"discussion_moderation_policy.json references a missing policy source: {source_path}"
                )

    scan_limits = config.get("scan_limits")
    if not isinstance(scan_limits, dict):
        errors.append("discussion_moderation_policy.json must define scan_limits.")
    else:
        max_discussions = scan_limits.get("max_discussions_per_scan")
        max_comments = scan_limits.get("max_comments_per_discussion")
        if not isinstance(max_discussions, int) or max_discussions < 1:
            errors.append(
                "discussion_moderation_policy.json scan_limits.max_discussions_per_scan must be >= 1."
            )
        if not isinstance(max_comments, int) or max_comments < 1:
            errors.append(
                "discussion_moderation_policy.json scan_limits.max_comments_per_discussion must be >= 1."
            )

    if not isinstance(config.get("exempt_logins"), list):
        errors.append("discussion_moderation_policy.json must define exempt_logins.")

    owner_notification = config.get("owner_notification")
    if not isinstance(owner_notification, dict):
        errors.append(
            "discussion_moderation_policy.json must define owner_notification."
        )
    else:
        if owner_notification.get("enabled") is not True:
            errors.append(
                "discussion_moderation_policy.json must keep owner_notification.enabled true."
            )
        if not owner_notification.get("issue_title"):
            errors.append(
                "discussion_moderation_policy.json must define owner_notification.issue_title."
            )

    admin_blocking = config.get("admin_blocking")
    if not isinstance(admin_blocking, dict):
        errors.append("discussion_moderation_policy.json must define admin_blocking.")
    else:
        if admin_blocking.get("enabled_when_token_present") is not True:
            errors.append(
                "discussion_moderation_policy.json must keep admin_blocking.enabled_when_token_present true."
            )
        if not admin_blocking.get("token_env_var"):
            errors.append(
                "discussion_moderation_policy.json must define admin_blocking.token_env_var."
            )

    rules = config.get("rules")
    if not isinstance(rules, list) or not rules:
        errors.append("discussion_moderation_policy.json must define rules.")
        return errors

    required_rule_ids = {
        "hate_speech",
        "vulgar_language",
        "harassment",
        "violent_threats",
    }
    seen_rule_ids: set[str] = set()
    for rule in rules:
        if not isinstance(rule, dict):
            errors.append("Each moderation rule must be a JSON object.")
            continue

        rule_id = rule.get("id")
        if not rule_id:
            errors.append("Each moderation rule must define id.")
            continue

        seen_rule_ids.add(rule_id)
        if not rule.get("description"):
            errors.append(f"Moderation rule '{rule_id}' must define description.")
        if not isinstance(rule.get("block_candidate"), bool):
            errors.append(
                f"Moderation rule '{rule_id}' must define boolean block_candidate."
            )
        if not isinstance(rule.get("terms"), list) or not rule.get("terms"):
            errors.append(
                f"Moderation rule '{rule_id}' must define a non-empty terms list."
            )

    if seen_rule_ids != required_rule_ids:
        errors.append(
            "discussion_moderation_policy.json must define exactly hate_speech, vulgar_language, harassment, and violent_threats rules."
        )

    return errors


def github_request(
    url: str,
    token: str,
    method: str = "GET",
    payload: dict[str, Any] | None = None,
) -> dict[str, Any] | list[Any] | None:
    data = None
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "User-Agent": "forsetti-framework-windows-discussion-moderation-agent",
    }
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"

    request = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(request) as response:
            body = response.read().decode("utf-8", errors="ignore")
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="ignore")
        raise RuntimeError(
            f"GitHub API request failed: {method} {url} :: {detail}"
        ) from exc

    if not body:
        return None
    return json.loads(body)


def graphql_request(
    graphql_url: str, token: str, query: str, variables: dict[str, Any]
) -> dict[str, Any]:
    payload = json.dumps({"query": query, "variables": variables}).encode("utf-8")
    request = urllib.request.Request(
        graphql_url,
        data=payload,
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
            "Accept": "application/json",
            "User-Agent": "forsetti-framework-windows-discussion-moderation-agent",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(request) as response:
            result = json.load(response)
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="ignore")
        raise RuntimeError(f"GitHub GraphQL request failed: {detail}") from exc

    if result.get("errors"):
        raise RuntimeError(f"GitHub GraphQL error: {json.dumps(result['errors'])}")
    return result["data"]


def compile_term_pattern(term: str) -> re.Pattern[str]:
    pieces: list[str] = []
    separator = r"[\W_]*"
    for char in term.lower():
        if char.isalnum():
            if pieces:
                pieces.append(separator)
            pieces.append(LEET_MAP.get(char, re.escape(char)))
        else:
            if pieces and pieces[-1] != separator:
                pieces.append(separator)
    pattern = "".join(pieces)
    return re.compile(rf"(?<![a-z0-9]){pattern}(?![a-z0-9])", re.IGNORECASE)


def compile_rules(config: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        {**rule, "patterns": [compile_term_pattern(term) for term in rule["terms"]]}
        for rule in config["rules"]
    ]


def collect_matches(text: str, compiled_rules: list[dict[str, Any]]) -> list[dict[str, Any]]:
    findings: list[dict[str, Any]] = []
    for rule in compiled_rules:
        matched_terms = [
            term
            for term, pattern in zip(rule["terms"], rule["patterns"])
            if pattern.search(text)
        ]
        if matched_terms:
            findings.append(
                {
                    "id": rule["id"],
                    "description": rule["description"],
                    "block_candidate": rule["block_candidate"],
                    "matched_terms": matched_terms[:5],
                }
            )
    return findings


def redact_excerpt(
    text: str, compiled_rules: list[dict[str, Any]], max_chars: int = 280
) -> str:
    redacted = text
    for rule in compiled_rules:
        for pattern in rule["patterns"]:
            redacted = pattern.sub("[redacted]", redacted)
    redacted = re.sub(r"\s+", " ", redacted).strip()
    if len(redacted) <= max_chars:
        return redacted
    return f"{redacted[: max_chars - 3].rstrip()}..."


def is_exempt_login(login: str, exempt_logins: list[str]) -> bool:
    normalized = normalize_text(login)
    return normalized in {normalize_text(entry) for entry in exempt_logins}


def build_event_item(payload: dict[str, Any], event_name: str | None) -> list[dict[str, Any]]:
    discussion = payload.get("discussion") or {}
    comment = payload.get("comment") or {}
    sender = payload.get("sender") or {}

    if event_name == "discussion":
        author = (discussion.get("user") or {}).get("login") or sender.get("login") or ""
        return [
            {
                "kind": "discussion",
                "id": discussion.get("node_id") or discussion.get("id"),
                "discussion_id": discussion.get("node_id") or discussion.get("id"),
                "discussion_number": discussion.get("number"),
                "discussion_title": discussion.get("title") or "",
                "text": "\n".join(
                    part
                    for part in (
                        discussion.get("title") or "",
                        discussion.get("body") or "",
                    )
                    if part
                ),
                "author_login": author,
                "url": discussion.get("html_url") or discussion.get("url") or "",
            }
        ]

    if event_name == "discussion_comment":
        author = (comment.get("user") or {}).get("login") or sender.get("login") or ""
        return [
            {
                "kind": "discussion_comment",
                "id": comment.get("node_id") or comment.get("id"),
                "discussion_id": discussion.get("node_id") or discussion.get("id"),
                "discussion_number": discussion.get("number"),
                "discussion_title": discussion.get("title") or "",
                "text": comment.get("body") or "",
                "author_login": author,
                "url": comment.get("html_url") or comment.get("url") or "",
            }
        ]

    return []


def load_repository_state(
    graphql_url: str,
    token: str,
    repository: str,
    max_discussions: int,
    max_comments: int,
) -> dict[str, Any]:
    owner, name = repository.split("/", 1)
    query = """
    query DiscussionModerationScan(
      $owner: String!,
      $name: String!,
      $discussionsFirst: Int!,
      $commentsFirst: Int!
    ) {
      repository(owner: $owner, name: $name) {
        name
        owner {
          __typename
          login
        }
        discussions(first: $discussionsFirst, orderBy: {field: UPDATED_AT, direction: DESC}) {
          nodes {
            id
            number
            title
            body
            url
            author {
              login
            }
            comments(first: $commentsFirst) {
              nodes {
                id
                body
                url
                author {
                  login
                }
              }
            }
          }
        }
      }
    }
    """
    data = graphql_request(
        graphql_url,
        token,
        query,
        {
            "owner": owner,
            "name": name,
            "discussionsFirst": max_discussions,
            "commentsFirst": max_comments,
        },
    )
    repository_data = data.get("repository")
    if not repository_data:
        raise RuntimeError(f"Repository '{repository}' was not found via GraphQL.")
    return repository_data


def build_scan_items(repository_state: dict[str, Any]) -> list[dict[str, Any]]:
    items: list[dict[str, Any]] = []
    for discussion in repository_state["discussions"]["nodes"]:
        items.append(
            {
                "kind": "discussion",
                "id": discussion["id"],
                "discussion_id": discussion["id"],
                "discussion_number": discussion["number"],
                "discussion_title": discussion["title"] or "",
                "text": "\n".join(
                    part
                    for part in (discussion["title"] or "", discussion["body"] or "")
                    if part
                ),
                "author_login": (discussion.get("author") or {}).get("login") or "",
                "url": discussion.get("url") or "",
            }
        )
        for comment in discussion["comments"]["nodes"]:
            items.append(
                {
                    "kind": "discussion_comment",
                    "id": comment["id"],
                    "discussion_id": discussion["id"],
                    "discussion_number": discussion["number"],
                    "discussion_title": discussion["title"] or "",
                    "text": comment.get("body") or "",
                    "author_login": (comment.get("author") or {}).get("login") or "",
                    "url": comment.get("url") or "",
                }
            )
    return items


def delete_discussion(graphql_url: str, token: str, discussion_id: str) -> None:
    mutation = """
    mutation DeleteDiscussion($id: ID!) {
      deleteDiscussion(input: {id: $id}) {
        clientMutationId
      }
    }
    """
    graphql_request(graphql_url, token, mutation, {"id": discussion_id})


def delete_discussion_comment(graphql_url: str, token: str, comment_id: str) -> None:
    mutation = """
    mutation DeleteDiscussionComment($id: ID!) {
      deleteDiscussionComment(input: {id: $id}) {
        clientMutationId
      }
    }
    """
    graphql_request(graphql_url, token, mutation, {"id": comment_id})


def load_repo_metadata(api_base_url: str, token: str, repository: str) -> dict[str, Any]:
    result = github_request(f"{api_base_url}/repos/{repository}", token)
    return result if isinstance(result, dict) else {}


def list_open_issues(api_base_url: str, token: str, repository: str) -> list[dict[str, Any]]:
    result = github_request(
        f"{api_base_url}/repos/{repository}/issues?state=open&per_page=100",
        token,
    )
    return result if isinstance(result, list) else []


def create_issue(
    api_base_url: str, token: str, repository: str, title: str, body: str
) -> dict[str, Any]:
    result = github_request(
        f"{api_base_url}/repos/{repository}/issues",
        token,
        method="POST",
        payload={"title": title, "body": body},
    )
    return result if isinstance(result, dict) else {}


def create_issue_comment(
    api_base_url: str, token: str, repository: str, issue_number: int, body: str
) -> None:
    github_request(
        f"{api_base_url}/repos/{repository}/issues/{issue_number}/comments",
        token,
        method="POST",
        payload={"body": body},
    )


def ensure_alert_issue(
    api_base_url: str, token: str, repository: str, title: str
) -> int | None:
    repo_metadata = load_repo_metadata(api_base_url, token, repository)
    if not repo_metadata.get("has_issues"):
        return None

    for issue in list_open_issues(api_base_url, token, repository):
        if issue.get("title") == title and "pull_request" not in issue:
            return int(issue["number"])

    issue = create_issue(
        api_base_url,
        token,
        repository,
        title,
        (
            "# Discussion Moderation Incident Log\n\n"
            "This issue is maintained by the automated discussion moderation agent.\n"
            "It records deleted discussion threads and comments that violated the repository code of conduct.\n"
        ),
    )
    return int(issue["number"])


def block_user(
    api_base_url: str,
    token: str,
    owner_login: str,
    owner_type: str,
    username: str,
) -> None:
    if owner_type == "Organization":
        url = f"{api_base_url}/orgs/{owner_login}/blocks/{urllib.parse.quote(username)}"
    else:
        url = f"{api_base_url}/user/blocks/{urllib.parse.quote(username)}"
    github_request(url, token, method="PUT")


def render_alert_comment(
    item: dict[str, Any],
    findings: list[dict[str, Any]],
    compiled_rules: list[dict[str, Any]],
    deletion_result: str,
    block_result: str,
) -> str:
    reasons = ", ".join(finding["id"] for finding in findings)
    matched_terms = ", ".join(
        sorted(
            {
                matched_term
                for finding in findings
                for matched_term in finding["matched_terms"]
            }
        )
    )
    excerpt = redact_excerpt(item["text"], compiled_rules)
    return "\n".join(
        [
            f"### Moderation incident for @{item['author_login'] or 'unknown-user'}",
            "",
            f"- Time (UTC): `{dt.datetime.now(dt.timezone.utc).isoformat()}`",
            f"- Item kind: `{item['kind']}`",
            f"- Discussion: `#{item.get('discussion_number')}` {item.get('discussion_title') or ''}".rstrip(),
            f"- Source URL: {item.get('url') or 'Unavailable'}",
            f"- Reasons: `{reasons}`",
            f"- Matched terms: `{matched_terms}`",
            f"- Deletion result: `{deletion_result}`",
            f"- Block result: `{block_result}`",
            "",
            "Redacted excerpt:",
            f"> {excerpt or '[no excerpt available]'}",
        ]
    )


def write_summary(summary_lines: list[str]) -> None:
    summary_path = os.getenv("GITHUB_STEP_SUMMARY")
    if not summary_path:
        return
    Path(summary_path).write_text("\n".join(summary_lines) + "\n", encoding="utf-8")


def analyze_and_print(
    text: str, title: str, kind: str, compiled_rules: list[dict[str, Any]]
) -> int:
    analysis_text = (
        "\n".join(part for part in (title, text) if part)
        if kind == "discussion"
        else text
    )
    findings = collect_matches(analysis_text, compiled_rules)
    if not findings:
        print("No moderation violations detected.")
        return 0

    print(f"Detected {len(findings)} moderation rule violation(s):")
    for finding in findings:
        matched = ", ".join(finding["matched_terms"])
        print(f"- {finding['id']}: {matched}")
    print("")
    print("Redacted excerpt:")
    print(redact_excerpt(analysis_text, compiled_rules))
    return 0


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()
    config_path = Path(args.config) if args.config else root / DEFAULT_CONFIG_PATH

    if not config_path.is_file():
        print(f"Missing moderation config: {config_path}", file=sys.stderr)
        return 1

    config = load_json_file(config_path)
    config_errors = validate_config(config, root)
    if config_errors:
        for error in config_errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    compiled_rules = compile_rules(config)

    if args.validate_config:
        print("Discussion moderation configuration is valid.")
        return 0

    if args.analyze_text is not None:
        return analyze_and_print(
            text=args.analyze_text,
            title=args.analyze_title,
            kind=args.analyze_kind,
            compiled_rules=compiled_rules,
        )

    token = os.getenv("GITHUB_TOKEN")
    repository = os.getenv("GITHUB_REPOSITORY")
    api_base_url = (os.getenv("GITHUB_API_URL") or "https://api.github.com").rstrip("/")
    graphql_url = discussion_agent.derive_graphql_url(api_base_url)
    if not token:
        print("Missing GITHUB_TOKEN for moderation actions.", file=sys.stderr)
        return 1
    if not repository or "/" not in repository:
        print("Missing GITHUB_REPOSITORY in owner/name format.", file=sys.stderr)
        return 1

    event_name = args.event_name or ""
    if event_name in ALLOWED_EVENT_NAMES and args.event_path:
        payload = load_json_file(Path(args.event_path))
        items = build_event_item(payload, event_name)
    else:
        repository_state = load_repository_state(
            graphql_url,
            token,
            repository,
            config["scan_limits"]["max_discussions_per_scan"],
            config["scan_limits"]["max_comments_per_discussion"],
        )
        items = build_scan_items(repository_state)

    if not items:
        print("No moderation targets found for this event.")
        write_summary(["## Discussion Moderation", "", "No moderation targets found."])
        return 0

    alert_issue_number = None

    admin_token_env = config["admin_blocking"]["token_env_var"]
    admin_token = os.getenv(admin_token_env) or ""
    owner_login = ""
    owner_type = ""
    if admin_token:
        try:
            repo_metadata = load_repo_metadata(api_base_url, token, repository)
            owner = repo_metadata.get("owner") or {}
            owner_login = owner.get("login") or repository.split("/", 1)[0]
            owner_type = owner.get("type") or "User"
        except RuntimeError as exc:
            print(
                f"Warning: unable to load repo metadata for blocking: {exc}",
                file=sys.stderr,
            )

    deleted_discussions: set[str] = set()
    deleted_count = 0
    flagged_count = 0
    block_attempts = 0
    block_successes = 0
    summary_lines = ["## Discussion Moderation", ""]

    for item in items:
        if not item.get("id"):
            continue
        if (
            item["kind"] == "discussion_comment"
            and item["discussion_id"] in deleted_discussions
        ):
            continue
        if is_exempt_login(item.get("author_login") or "", config["exempt_logins"]):
            continue

        findings = collect_matches(item.get("text") or "", compiled_rules)
        if not findings:
            continue

        flagged_count += 1
        deletion_result = "not_attempted"
        block_result = "not_attempted"
        try:
            if item["kind"] == "discussion":
                delete_discussion(graphql_url, token, item["id"])
                deleted_discussions.add(item["discussion_id"])
            else:
                delete_discussion_comment(graphql_url, token, item["id"])
            deletion_result = "deleted"
            deleted_count += 1
        except RuntimeError as exc:
            deletion_result = f"delete_failed: {exc}"

        if any(finding["block_candidate"] for finding in findings):
            block_attempts += 1
            if admin_token and owner_login:
                try:
                    block_user(
                        api_base_url,
                        admin_token,
                        owner_login,
                        owner_type or "User",
                        item["author_login"],
                    )
                    block_result = "blocked"
                    block_successes += 1
                except RuntimeError as exc:
                    block_result = f"block_failed: {exc}"
            else:
                block_result = f"skipped_no_{admin_token_env.lower()}"

        summary_lines.append(
            f"- Flagged `{item['kind']}` from `@{item['author_login'] or 'unknown-user'}` for "
            + ", ".join(f"`{finding['id']}`" for finding in findings)
            + f"; deletion result: `{deletion_result}`."
        )

        if config["owner_notification"]["enabled"] and alert_issue_number is None:
            try:
                alert_issue_number = ensure_alert_issue(
                    api_base_url,
                    token,
                    repository,
                    config["owner_notification"]["issue_title"],
                )
            except RuntimeError as exc:
                print(
                    f"Warning: unable to prepare moderation issue log: {exc}",
                    file=sys.stderr,
                )

        if alert_issue_number is not None:
            try:
                create_issue_comment(
                    api_base_url,
                    token,
                    repository,
                    alert_issue_number,
                    render_alert_comment(
                        item=item,
                        findings=findings,
                        compiled_rules=compiled_rules,
                        deletion_result=deletion_result,
                        block_result=block_result,
                    ),
                )
            except RuntimeError as exc:
                print(
                    f"Warning: unable to write moderation issue comment: {exc}",
                    file=sys.stderr,
                )

    if flagged_count == 0:
        summary_lines.append("No violating discussion content was found.")
    else:
        summary_lines.extend(
            [
                "",
                f"- Total violations handled: `{flagged_count}`",
                f"- Total items deleted: `{deleted_count}`",
                f"- Block attempts: `{block_attempts}`",
                f"- Successful blocks: `{block_successes}`",
            ]
        )

    write_summary(summary_lines)
    if flagged_count == 0:
        print("No violating discussion content found.")
    else:
        print(
            f"Handled {flagged_count} violating discussion item(s); deleted {deleted_count}."
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
