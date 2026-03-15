#!/usr/bin/env python3
"""Route GitHub Discussion events to repo-grounded response agents."""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

TEXT_ENCODING = "utf-8-sig"
TEXT_EXTENSIONS = {
    ".md",
    ".txt",
    ".json",
    ".yaml",
    ".yml",
    ".cpp",
    ".cc",
    ".cxx",
    ".h",
    ".hpp",
    ".hh",
    ".hxx",
    ".ps1",
    ".cmake",
}
EXCLUDED_DIRS = {
    ".git",
    ".vs",
    "__pycache__",
    "node_modules",
    ".venv",
    "build",
    "out",
}
MAX_FILE_BYTES = 400_000
ALLOWED_SHORT_TOKENS = {"ai", "ci", "ui", "vr"}
STOPWORDS = {
    "a",
    "about",
    "an",
    "and",
    "are",
    "as",
    "at",
    "be",
    "back",
    "but",
    "by",
    "check",
    "comment",
    "comments",
    "current",
    "discussion",
    "for",
    "from",
    "how",
    "i",
    "if",
    "information",
    "in",
    "into",
    "is",
    "it",
    "its",
    "me",
    "my",
    "of",
    "on",
    "or",
    "please",
    "post",
    "posts",
    "question",
    "questions",
    "repo",
    "repository",
    "response",
    "responses",
    "question",
    "source",
    "sources",
    "soon",
    "that",
    "the",
    "their",
    "there",
    "these",
    "this",
    "time",
    "to",
    "topic",
    "available",
    "we",
    "what",
    "when",
    "where",
    "which",
    "who",
    "why",
    "with",
    "you",
    "your",
}
REQUIRED_AGENT_IDS = ("technical", "support", "framework_governance")
DEFAULT_CONFIG_PATH = Path(".github") / "discussion_agents.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Respond to GitHub Discussion events using repo-grounded agents."
    )
    parser.add_argument("--root", default=os.getcwd(), help="Repository root path.")
    parser.add_argument(
        "--config",
        default=None,
        help="Optional path to the discussion agent config file.",
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
        help="Validate the discussion agent config and exit.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the response instead of posting it to GitHub.",
    )
    return parser.parse_args()


def read_text_file(path: Path) -> str:
    return path.read_text(encoding=TEXT_ENCODING)


def load_json_file(path: Path) -> Any:
    with path.open(encoding=TEXT_ENCODING) as handle:
        return json.load(handle)


def normalize_text(value: str) -> str:
    return re.sub(r"\s+", " ", value or "").strip().lower()


def tokenize(value: str) -> list[str]:
    tokens = []
    for token in re.findall(r"[a-z0-9_]+", normalize_text(value)):
        if token in ALLOWED_SHORT_TOKENS or len(token) >= 3:
            if token not in STOPWORDS:
                tokens.append(token)
    return tokens


def derive_graphql_url(api_url: str) -> str:
    trimmed = (api_url or "https://api.github.com").rstrip("/")
    if trimmed.endswith("/api/v3"):
        return f"{trimmed[:-7]}/api/graphql"
    return f"{trimmed}/graphql"


def is_bot_sender(payload: dict[str, Any]) -> bool:
    sender = payload.get("sender") or {}
    login = (sender.get("login") or "").lower()
    sender_type = (sender.get("type") or "").lower()
    return sender_type == "bot" or login.endswith("[bot]")


def validate_config(config: dict[str, Any], root: Path) -> list[str]:
    errors: list[str] = []

    if config.get("version") != "1.0":
        errors.append("discussion_agents.json must declare version 1.0.")

    default_agent_id = config.get("default_agent_id")
    agents = config.get("agents")
    response_policy = config.get("response_policy") or {}

    if not isinstance(agents, list):
        errors.append("discussion_agents.json must define an agents list.")
        return errors

    fallback_message = response_policy.get("fallback_message")
    if fallback_message != "There is not information available at this time. Check back soon.":
        errors.append(
            "discussion_agents.json must preserve the required fallback message."
        )

    seen_ids: set[str] = set()
    for agent in agents:
        if not isinstance(agent, dict):
            errors.append("Each discussion agent entry must be a JSON object.")
            continue

        agent_id = agent.get("id")
        if not agent_id:
            errors.append("Every discussion agent must define an id.")
            continue

        seen_ids.add(agent_id)

        if not agent.get("display_name"):
            errors.append(f"Discussion agent '{agent_id}' must define display_name.")

        category_hints = agent.get("category_hints")
        if not isinstance(category_hints, list) or not category_hints:
            errors.append(
                f"Discussion agent '{agent_id}' must define category_hints."
            )

        keywords = agent.get("keywords")
        if not isinstance(keywords, dict) or not keywords:
            errors.append(f"Discussion agent '{agent_id}' must define keywords.")

        source_paths = agent.get("source_paths")
        if not isinstance(source_paths, list) or not source_paths:
            errors.append(f"Discussion agent '{agent_id}' must define source_paths.")
            continue

        for source_path in source_paths:
            if not (root / source_path).exists():
                errors.append(
                    f"Discussion agent '{agent_id}' references a missing source path: {source_path}"
                )

    if tuple(sorted(seen_ids)) != tuple(sorted(REQUIRED_AGENT_IDS)):
        errors.append(
            "discussion_agents.json must define exactly the technical, support, and framework_governance agents."
        )

    if default_agent_id not in seen_ids:
        errors.append("discussion_agents.json default_agent_id must match an agent id.")

    max_sources = response_policy.get("max_sources")
    if not isinstance(max_sources, int) or max_sources < 1:
        errors.append("discussion_agents.json response_policy.max_sources must be >= 1.")

    max_excerpt_chars = response_policy.get("max_excerpt_chars")
    if not isinstance(max_excerpt_chars, int) or max_excerpt_chars < 120:
        errors.append(
            "discussion_agents.json response_policy.max_excerpt_chars must be >= 120."
        )

    return errors


def gather_event_context(
    payload: dict[str, Any], event_name: str | None
) -> dict[str, Any]:
    discussion = payload.get("discussion") or {}
    comment = payload.get("comment") or {}
    category = discussion.get("category") or {}
    sender = payload.get("sender") or {}

    discussion_id = discussion.get("node_id") or discussion.get("id")
    reply_to_id = None

    title = discussion.get("title") or ""
    discussion_body = discussion.get("body") or ""
    comment_body = comment.get("body") or ""
    category_name = category.get("name") or ""

    if event_name == "discussion_comment":
        reply_to_id = comment.get("node_id") or comment.get("id")
        query_text = "\n".join(
            part for part in (title, comment_body, discussion_body) if part
        )
    else:
        query_text = "\n".join(part for part in (title, discussion_body) if part)

    return {
        "discussion_id": discussion_id,
        "reply_to_id": reply_to_id,
        "title": title,
        "discussion_body": discussion_body,
        "comment_body": comment_body,
        "category_name": category_name,
        "query_text": query_text,
        "sender_login": sender.get("login") or "",
    }


def iter_source_files(root: Path, source_paths: list[str]) -> list[Path]:
    files: list[Path] = []
    seen: set[Path] = set()

    for source_path in source_paths:
        candidate = root / source_path
        if candidate.is_file():
            if candidate.suffix.lower() in TEXT_EXTENSIONS and candidate not in seen:
                files.append(candidate)
                seen.add(candidate)
            continue

        if not candidate.is_dir():
            continue

        for dirpath, dirnames, filenames in os.walk(candidate):
            dirnames[:] = [name for name in dirnames if name not in EXCLUDED_DIRS]
            current_dir = Path(dirpath)
            for filename in filenames:
                file_path = current_dir / filename
                if file_path.suffix.lower() not in TEXT_EXTENSIONS:
                    continue
                if file_path.stat().st_size > MAX_FILE_BYTES:
                    continue
                if file_path not in seen:
                    files.append(file_path)
                    seen.add(file_path)

    return files


def score_text_block(text: str, query_tokens: list[str]) -> int:
    lowered = normalize_text(text)
    if not lowered:
        return 0

    score = 0
    for token in query_tokens:
        count = lowered.count(token)
        if count:
            score += min(count, 3)
    return score


def compact_excerpt(text: str, max_chars: int) -> str:
    cleaned = text.replace("`", "'").replace("|", " ").replace("*", "")
    cleaned = re.sub(r"\s+", " ", cleaned).strip(" -:\n\t")
    if len(cleaned) <= max_chars:
        return cleaned
    return f"{cleaned[: max_chars - 3].rstrip()}..."


def best_excerpt(text: str, query_tokens: list[str], max_chars: int) -> tuple[int, str]:
    best_score = 0
    best_text = ""
    lines = [line.strip() for line in text.splitlines()]

    for index, line in enumerate(lines):
        if not line:
            continue
        window_lines = lines[max(0, index - 1) : min(len(lines), index + 3)]
        candidate = " ".join(part for part in window_lines if part)
        score = score_text_block(candidate, query_tokens)
        if score > best_score:
            best_score = score
            best_text = candidate

    if best_score == 0:
        paragraphs = [paragraph.strip() for paragraph in re.split(r"\n\s*\n", text) if paragraph.strip()]
        if paragraphs:
            best_text = paragraphs[0]

    return best_score, compact_excerpt(best_text, max_chars)


def search_sources(
    root: Path, source_paths: list[str], query_text: str, max_sources: int, max_chars: int
) -> list[dict[str, Any]]:
    query_tokens = tokenize(query_text)
    if not query_tokens:
        return []

    results: list[dict[str, Any]] = []
    for path in iter_source_files(root, source_paths):
        try:
            content = read_text_file(path)
        except (OSError, UnicodeDecodeError):
            continue

        excerpt_score, excerpt = best_excerpt(content, query_tokens, max_chars)
        path_score = score_text_block(path.as_posix(), query_tokens)
        total_score = excerpt_score + path_score
        if total_score < 2 or not excerpt:
            continue

        results.append(
            {
                "path": path.relative_to(root).as_posix(),
                "score": total_score,
                "excerpt": excerpt,
            }
        )

    results.sort(key=lambda item: (-item["score"], item["path"]))
    return results[:max_sources]


def score_agent(agent: dict[str, Any], category_name: str, query_text: str) -> int:
    query_lower = normalize_text(query_text)
    category_lower = normalize_text(category_name)
    query_tokens = tokenize(query_text)
    score = 0

    for hint in agent.get("category_hints", []):
        hint_lower = normalize_text(hint)
        if hint_lower and hint_lower in category_lower:
            score += 10
        if hint_lower and hint_lower in query_lower:
            score += 3

    for phrase, weight in (agent.get("keywords") or {}).items():
        phrase_lower = normalize_text(phrase)
        if not phrase_lower:
            continue
        if " " in phrase_lower:
            if phrase_lower in query_lower:
                score += int(weight)
        else:
            score += int(weight) * min(query_tokens.count(phrase_lower), 2)

    return score


def choose_agent(
    config: dict[str, Any], root: Path, category_name: str, query_text: str
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    response_policy = config["response_policy"]
    max_sources = response_policy["max_sources"]
    max_chars = response_policy["max_excerpt_chars"]

    ranked: list[tuple[int, int, dict[str, Any], list[dict[str, Any]]]] = []
    for agent in config["agents"]:
        results = search_sources(
            root,
            agent["source_paths"],
            query_text,
            max_sources=max_sources,
            max_chars=max_chars,
        )
        retrieval_score = results[0]["score"] if results else 0
        classification_score = score_agent(agent, category_name, query_text)
        total_score = classification_score + retrieval_score
        ranked.append((total_score, classification_score, retrieval_score, agent, results))

    ranked.sort(key=lambda item: (-item[0], -item[2], item[3]["id"]))

    if ranked and ranked[0][0] > 0:
        _, classification_score, retrieval_score, agent, results = ranked[0]
        if classification_score == 0 and retrieval_score < 8:
            break_glass_agent_id = config["default_agent_id"]
            for _, _, _, fallback_agent, _ in ranked:
                if fallback_agent["id"] == break_glass_agent_id:
                    return fallback_agent, []
            return agent, []
        return agent, results

    default_agent_id = config["default_agent_id"]
    for _, _, _, agent, results in ranked:
        if agent["id"] == default_agent_id:
            return agent, results

    raise RuntimeError("No default discussion agent could be selected.")


def render_response(
    agent: dict[str, Any], results: list[dict[str, Any]], fallback_message: str
) -> str:
    lines = [
        f"### {agent['display_name']}",
        "",
        "I checked the current repository sources for this topic.",
    ]

    if not results:
        lines.extend(["", fallback_message])
        return "\n".join(lines)

    lines.append("")
    lines.append("Relevant repository references:")
    for result in results:
        lines.append(f"- `{result['path']}`: {result['excerpt']}")

    return "\n".join(lines)


def post_discussion_comment(
    graphql_url: str,
    token: str,
    discussion_id: str,
    body: str,
    reply_to_id: str | None,
) -> None:
    mutation = """
    mutation AddDiscussionComment($discussionId: ID!, $body: String!, $replyToId: ID) {
      addDiscussionComment(
        input: {discussionId: $discussionId, body: $body, replyToId: $replyToId}
      ) {
        comment {
          id
        }
      }
    }
    """

    payload = json.dumps(
        {
            "query": mutation,
            "variables": {
                "discussionId": discussion_id,
                "body": body,
                "replyToId": reply_to_id,
            },
        }
    ).encode("utf-8")

    request = urllib.request.Request(
        graphql_url,
        data=payload,
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
            "Accept": "application/json",
            "User-Agent": "forsetti-framework-windows-discussion-agents",
        },
        method="POST",
    )

    try:
        with urllib.request.urlopen(request) as response:
            response_payload = json.load(response)
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="ignore")
        raise RuntimeError(f"Failed to post discussion comment: {detail}") from exc

    if response_payload.get("errors"):
        raise RuntimeError(
            f"GitHub GraphQL error: {json.dumps(response_payload['errors'])}"
        )


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()
    config_path = Path(args.config) if args.config else root / DEFAULT_CONFIG_PATH

    if not config_path.is_file():
        print(f"Missing discussion agent config: {config_path}", file=sys.stderr)
        return 1

    config = load_json_file(config_path)
    config_errors = validate_config(config, root)
    if config_errors:
        for error in config_errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    if args.validate_config:
        print("Discussion agent configuration is valid.")
        return 0

    if not args.event_path:
        print("Missing GitHub event payload path.", file=sys.stderr)
        return 1

    payload = load_json_file(Path(args.event_path))
    if is_bot_sender(payload):
        print("Event sender is a bot. Skipping response.")
        return 0

    context = gather_event_context(payload, args.event_name)
    if not context["discussion_id"]:
        print("Event payload does not include a discussion id.", file=sys.stderr)
        return 1

    agent, results = choose_agent(
        config,
        root,
        category_name=context["category_name"],
        query_text=context["query_text"],
    )
    body = render_response(
        agent,
        results,
        config["response_policy"]["fallback_message"],
    )

    if args.dry_run:
        print(body)
        return 0

    token = os.getenv("GITHUB_TOKEN")
    if not token:
        print("Missing GITHUB_TOKEN for posting discussion comments.", file=sys.stderr)
        return 1

    graphql_url = derive_graphql_url(os.getenv("GITHUB_API_URL", "https://api.github.com"))
    post_discussion_comment(
        graphql_url=graphql_url,
        token=token,
        discussion_id=context["discussion_id"],
        body=body,
        reply_to_id=context["reply_to_id"],
    )
    print(
        f"Posted {agent['display_name']} response for discussion event from {context['sender_login']}."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
