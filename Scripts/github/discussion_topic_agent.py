#!/usr/bin/env python3
"""Generate repository-grounded GitHub Discussion topics on a schedule."""

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

import discussion_agent

TEXT_ENCODING = "utf-8-sig"
DEFAULT_CONFIG_PATH = Path(".github") / "discussion_topic_generator.json"
DEFAULT_DISCUSSION_AGENT_CONFIG_PATH = Path(".github") / "discussion_agents.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Seed GitHub Discussions from repository wiki and documentation surfaces."
    )
    parser.add_argument("--root", default=os.getcwd(), help="Repository root path.")
    parser.add_argument(
        "--config",
        default=None,
        help="Optional path to the discussion topic generator config file.",
    )
    parser.add_argument(
        "--discussion-agent-config",
        default=None,
        help="Optional path to the discussion response agent config file.",
    )
    parser.add_argument(
        "--validate-config",
        action="store_true",
        help="Validate the discussion topic generator config and exit.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the topics that would be created instead of calling GitHub.",
    )
    return parser.parse_args()


def slugify(value: str) -> str:
    normalized = discussion_agent.normalize_text(value)
    normalized = re.sub(r"[^a-z0-9]+", "-", normalized).strip("-")
    return normalized or "topic"


def compact_summary(value: str, max_chars: int = 320) -> str:
    cleaned = value.replace("`", "'").replace("|", " ").replace("*", "")
    cleaned = re.sub(r"\s+", " ", cleaned).strip(" -:\n\t")
    if len(cleaned) <= max_chars:
        return cleaned
    return f"{cleaned[: max_chars - 3].rstrip()}..."


def clean_heading_title(value: str) -> str:
    return re.sub(r"^\d+\.\s*", "", value).strip()


def first_heading(content: str) -> str:
    for line in content.splitlines():
        stripped = line.strip()
        if stripped.startswith("# "):
            return stripped[2:].strip()
    return ""


def first_paragraph(content: str) -> str:
    paragraphs = re.split(r"\n\s*\n", content)
    for paragraph in paragraphs:
        stripped = paragraph.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            continue
        if stripped.startswith(">"):
            continue
        if stripped.startswith("|"):
            continue
        if stripped.startswith("- "):
            continue
        return stripped
    return ""


def extract_markdown_sections(content: str, heading_level: int) -> list[dict[str, str]]:
    sections: list[dict[str, str]] = []
    heading_prefix = "#" * heading_level + " "
    section_title = None
    section_lines: list[str] = []

    for line in content.splitlines():
        stripped = line.rstrip()
        if stripped.startswith(heading_prefix):
            if section_title:
                sections.append(
                    {
                        "heading": section_title,
                        "content": "\n".join(section_lines).strip(),
                    }
                )
            section_title = stripped[len(heading_prefix) :].strip()
            section_lines = []
            continue

        if section_title and re.match(r"^#{1,%d}\s+" % heading_level, stripped):
            sections.append(
                {
                    "heading": section_title,
                    "content": "\n".join(section_lines).strip(),
                }
            )
            section_title = None
            section_lines = []

        if section_title:
            section_lines.append(line)

    if section_title:
        sections.append(
            {
                "heading": section_title,
                "content": "\n".join(section_lines).strip(),
            }
        )

    return sections


def load_json_file(path: Path) -> Any:
    with path.open(encoding=TEXT_ENCODING) as handle:
        return json.load(handle)


def read_text_file(path: Path) -> str:
    return path.read_text(encoding=TEXT_ENCODING)


def validate_config(config: dict[str, Any], root: Path) -> list[str]:
    errors: list[str] = []

    if config.get("version") != "1.0":
        errors.append("discussion_topic_generator.json must declare version 1.0.")

    max_topics_per_run = config.get("max_topics_per_run")
    if not isinstance(max_topics_per_run, int) or max_topics_per_run < 1:
        errors.append("discussion_topic_generator.json max_topics_per_run must be >= 1.")

    max_topics_per_family = config.get("max_topics_per_family_per_run")
    if not isinstance(max_topics_per_family, int) or max_topics_per_family < 1:
        errors.append(
            "discussion_topic_generator.json max_topics_per_family_per_run must be >= 1."
        )

    family_order = config.get("family_order")
    if family_order != ["technical", "support", "framework_governance"]:
        errors.append(
            "discussion_topic_generator.json family_order must be ['technical', 'support', 'framework_governance']."
        )

    if config.get("seed_marker_prefix") != "forsetti-framework-windows-discussion-seed":
        errors.append(
            "discussion_topic_generator.json must preserve the forsetti-framework-windows-discussion-seed marker prefix."
        )

    category_preferences = config.get("category_preferences")
    if not isinstance(category_preferences, dict):
        errors.append("discussion_topic_generator.json must define category_preferences.")
    else:
        for family in ("technical", "support", "framework_governance"):
            values = category_preferences.get(family)
            if not isinstance(values, list) or not values:
                errors.append(
                    f"discussion_topic_generator.json category_preferences.{family} must be a non-empty list."
                )

    seed_sources = config.get("seed_sources")
    if not isinstance(seed_sources, list) or not seed_sources:
        errors.append("discussion_topic_generator.json must define seed_sources.")
    else:
        for source in seed_sources:
            if not isinstance(source, dict):
                errors.append("Each seed source must be a JSON object.")
                continue

            source_id = source.get("id")
            kind = source.get("kind")
            source_path = source.get("source_path")
            title_prefix = source.get("title_prefix")
            body_intro = source.get("body_intro")

            if not source_id:
                errors.append("Each seed source must define id.")
            if kind not in {"markdown_headings", "wiki_sync_pages"}:
                errors.append(f"Seed source '{source_id}' uses unsupported kind '{kind}'.")
            if not source_path:
                errors.append(f"Seed source '{source_id}' must define source_path.")
            elif not (root / source_path).exists():
                errors.append(
                    f"Seed source '{source_id}' references missing source path: {source_path}"
                )
            if not title_prefix:
                errors.append(f"Seed source '{source_id}' must define title_prefix.")
            if not body_intro:
                errors.append(f"Seed source '{source_id}' must define body_intro.")
            if kind == "markdown_headings":
                heading_level = source.get("heading_level")
                if not isinstance(heading_level, int) or heading_level < 2:
                    errors.append(
                        f"Seed source '{source_id}' must define heading_level >= 2."
                    )

    return errors


def classify_family(
    discussion_agent_config: dict[str, Any], title: str, summary: str, source_path: str
) -> str:
    query_text = "\n".join(part for part in (title, summary, source_path) if part)
    ranked: list[tuple[int, str]] = []
    for agent in discussion_agent_config["agents"]:
        score = discussion_agent.score_agent(agent, "", query_text)
        ranked.append((score, agent["id"]))

    ranked.sort(key=lambda item: (-item[0], item[1]))
    if ranked and ranked[0][0] > 0:
        return ranked[0][1]
    return discussion_agent_config["default_agent_id"]


def build_markdown_heading_candidates(
    root: Path,
    source: dict[str, Any],
    discussion_agent_config: dict[str, Any],
) -> list[dict[str, Any]]:
    path = root / source["source_path"]
    content = read_text_file(path)
    candidates: list[dict[str, Any]] = []
    for section in extract_markdown_sections(content, source["heading_level"]):
        summary = first_paragraph(section["content"]) or compact_summary(section["content"])
        if not summary:
            continue
        heading_title = clean_heading_title(section["heading"])
        title = f"{source['title_prefix']}: {heading_title}"
        family = classify_family(
            discussion_agent_config,
            title=title,
            summary=summary,
            source_path=source["source_path"],
        )
        candidates.append(
            {
                "seed_id": f"{source['id']}:{slugify(heading_title)}",
                "title": title,
                "family": family,
                "source_path": source["source_path"],
                "source_label": heading_title,
                "summary": compact_summary(summary),
                "body_intro": source["body_intro"],
            }
        )
    return candidates


def build_wiki_sync_candidates(
    root: Path,
    source: dict[str, Any],
    discussion_agent_config: dict[str, Any],
) -> list[dict[str, Any]]:
    config = load_json_file(root / source["source_path"])
    pages = config.get("pages", [])
    candidates: list[dict[str, Any]] = []
    for page in pages:
        source_path = page.get("source")
        destination = page.get("destination")
        if not source_path or not destination:
            continue

        file_path = root / source_path
        if not file_path.is_file():
            continue

        content = read_text_file(file_path)
        heading = first_heading(content) or destination.replace(".md", "").replace("-", " ")
        summary = first_paragraph(content) or compact_summary(content)
        if not summary:
            continue

        title = f"{source['title_prefix']}: {destination.replace('.md', '').replace('-', ' ')}"
        family = classify_family(
            discussion_agent_config,
            title=title,
            summary=summary,
            source_path=source_path,
        )
        candidates.append(
            {
                "seed_id": f"{source['id']}:{slugify(destination.replace('.md', ''))}",
                "title": title,
                "family": family,
                "source_path": source_path,
                "source_label": heading,
                "summary": compact_summary(summary),
                "body_intro": source["body_intro"],
            }
        )
    return candidates


def build_topic_candidates(
    root: Path,
    config: dict[str, Any],
    discussion_agent_config: dict[str, Any],
) -> list[dict[str, Any]]:
    candidates: list[dict[str, Any]] = []
    for source in config["seed_sources"]:
        if source["kind"] == "markdown_headings":
            candidates.extend(
                build_markdown_heading_candidates(root, source, discussion_agent_config)
            )
        elif source["kind"] == "wiki_sync_pages":
            candidates.extend(
                build_wiki_sync_candidates(root, source, discussion_agent_config)
            )

    seen_ids: set[str] = set()
    unique_candidates: list[dict[str, Any]] = []
    for candidate in candidates:
        if candidate["seed_id"] in seen_ids:
            continue
        seen_ids.add(candidate["seed_id"])
        unique_candidates.append(candidate)
    return unique_candidates


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
            "User-Agent": "forsetti-framework-windows-discussion-topic-agent",
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


def load_repository_discussion_state(
    graphql_url: str, token: str, repository: str
) -> dict[str, Any]:
    if "/" not in repository:
        raise RuntimeError("GITHUB_REPOSITORY must be in owner/name format.")

    owner, name = repository.split("/", 1)
    query = """
    query RepositoryDiscussionState($owner: String!, $name: String!) {
      repository(owner: $owner, name: $name) {
        id
        discussionCategories(first: 25) {
          nodes {
            id
            name
            slug
            isAnswerable
          }
        }
        discussions(first: 100, orderBy: {field: CREATED_AT, direction: DESC}) {
          nodes {
            id
            title
            body
            category {
              id
              slug
              name
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
        {"owner": owner, "name": name},
    )
    repository_data = data.get("repository")
    if not repository_data:
        raise RuntimeError(f"Repository '{repository}' was not found via GraphQL.")
    return repository_data


def choose_category(
    family: str,
    categories: list[dict[str, Any]],
    category_preferences: dict[str, list[str]],
) -> dict[str, Any]:
    normalized = {
        category["id"]: {
            **category,
            "slug_normalized": discussion_agent.normalize_text(category["slug"]),
            "name_normalized": discussion_agent.normalize_text(category["name"]),
        }
        for category in categories
    }

    for preferred in category_preferences.get(family, []):
        preferred_normalized = discussion_agent.normalize_text(preferred)
        for category in normalized.values():
            if (
                category["slug_normalized"] == preferred_normalized
                or category["name_normalized"] == preferred_normalized
            ):
                return category

    if categories:
        return normalized[categories[0]["id"]]

    raise RuntimeError("No discussion categories are available in this repository.")


def discussion_exists(
    candidate: dict[str, Any],
    existing_discussions: list[dict[str, Any]],
    marker_prefix: str,
) -> bool:
    marker = f"<!-- {marker_prefix}:{candidate['seed_id']} -->"
    normalized_title = discussion_agent.normalize_text(candidate["title"])
    for discussion in existing_discussions:
        title = discussion_agent.normalize_text(discussion.get("title") or "")
        body = discussion.get("body") or ""
        if title == normalized_title:
            return True
        if marker in body:
            return True
    return False


def build_discussion_body(candidate: dict[str, Any], marker_prefix: str) -> str:
    marker = f"<!-- {marker_prefix}:{candidate['seed_id']} -->"
    prompt_lines = {
        "technical": [
            "- Which engine, schema, or workflow surfaces should stay aligned here?",
            "- What implementation or validation concerns should contributors watch?",
        ],
        "support": [
            "- What onboarding or usage questions should this topic help answer?",
            "- Which docs should community members review first for this area?",
        ],
        "framework_governance": [
            "- Which framework contracts, guardrails, or dependency boundaries should stay fixed here?",
            "- What governance, manifest, or policy surfaces should stay aligned with this discussion?",
        ],
    }

    lines = [
        marker,
        candidate["body_intro"],
        "",
        f"- Topic family: `{candidate['family']}`",
        f"- Source file: `{candidate['source_path']}`",
        f"- Source label: `{candidate['source_label']}`",
        "",
        "Repository summary:",
        f"> {candidate['summary']}",
        "",
        "Suggested discussion angles:",
    ]
    lines.extend(prompt_lines.get(candidate["family"], prompt_lines["support"]))
    return "\n".join(lines)


def create_discussion(
    graphql_url: str,
    token: str,
    repository_id: str,
    category_id: str,
    title: str,
    body: str,
) -> dict[str, Any]:
    mutation = """
    mutation CreateDiscussion(
      $repositoryId: ID!,
      $categoryId: ID!,
      $title: String!,
      $body: String!
    ) {
      createDiscussion(
        input: {
          repositoryId: $repositoryId,
          categoryId: $categoryId,
          title: $title,
          body: $body
        }
      ) {
        discussion {
          id
          url
          title
        }
      }
    }
    """
    data = graphql_request(
        graphql_url,
        token,
        mutation,
        {
            "repositoryId": repository_id,
            "categoryId": category_id,
            "title": title,
            "body": body,
        },
    )
    return data["createDiscussion"]["discussion"]


def select_topics_to_create(
    candidates: list[dict[str, Any]],
    existing_discussions: list[dict[str, Any]],
    config: dict[str, Any],
) -> list[dict[str, Any]]:
    selected: list[dict[str, Any]] = []
    family_counts = {family: 0 for family in config["family_order"]}

    for family in config["family_order"]:
        for candidate in candidates:
            if candidate["family"] != family:
                continue
            if discussion_exists(
                candidate,
                existing_discussions,
                config["seed_marker_prefix"],
            ):
                continue
            if family_counts[family] >= config["max_topics_per_family_per_run"]:
                continue
            selected.append(candidate)
            family_counts[family] += 1
            break

    if len(selected) > config["max_topics_per_run"]:
        return selected[: config["max_topics_per_run"]]
    return selected


def print_dry_run(topics: list[dict[str, Any]], config: dict[str, Any]) -> None:
    if not topics:
        print("No new discussion topics would be created.")
        return

    print(f"Would create {len(topics)} discussion topic(s):")
    for topic in topics:
        body = build_discussion_body(topic, config["seed_marker_prefix"])
        print("")
        print(f"- {topic['title']} [{topic['family']}]")
        print(body)


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()
    config_path = Path(args.config) if args.config else root / DEFAULT_CONFIG_PATH
    discussion_agent_config_path = (
        Path(args.discussion_agent_config)
        if args.discussion_agent_config
        else root / DEFAULT_DISCUSSION_AGENT_CONFIG_PATH
    )

    if not config_path.is_file():
        print(f"Missing discussion topic generator config: {config_path}", file=sys.stderr)
        return 1
    if not discussion_agent_config_path.is_file():
        print(
            f"Missing discussion agent config required for topic classification: {discussion_agent_config_path}",
            file=sys.stderr,
        )
        return 1

    config = load_json_file(config_path)
    config_errors = validate_config(config, root)
    if config_errors:
        for error in config_errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    discussion_agent_config = load_json_file(discussion_agent_config_path)
    discussion_agent_errors = discussion_agent.validate_config(
        discussion_agent_config, root
    )
    if discussion_agent_errors:
        for error in discussion_agent_errors:
            print(
                f"ERROR: discussion_topic_agent depends on a valid discussion agent config. {error}",
                file=sys.stderr,
            )
        return 1

    if args.validate_config:
        print("Discussion topic generator configuration is valid.")
        return 0

    candidates = build_topic_candidates(root, config, discussion_agent_config)

    if args.dry_run:
        topics = select_topics_to_create(candidates, [], config)
        print_dry_run(topics, config)
        return 0

    token = os.getenv("GITHUB_TOKEN")
    repository = os.getenv("GITHUB_REPOSITORY")
    if not token:
        print("Missing GITHUB_TOKEN for creating discussion topics.", file=sys.stderr)
        return 1
    if not repository:
        print("Missing GITHUB_REPOSITORY for creating discussion topics.", file=sys.stderr)
        return 1

    graphql_url = discussion_agent.derive_graphql_url(
        os.getenv("GITHUB_API_URL", "https://api.github.com")
    )
    repository_state = load_repository_discussion_state(graphql_url, token, repository)
    categories = repository_state["discussionCategories"]["nodes"]
    existing_discussions = repository_state["discussions"]["nodes"]

    topics = select_topics_to_create(candidates, existing_discussions, config)
    if not topics:
        print("No new repo-grounded discussion topics needed.")
        return 0

    created_count = 0
    for topic in topics:
        category = choose_category(
            topic["family"],
            categories,
            config["category_preferences"],
        )
        body = build_discussion_body(topic, config["seed_marker_prefix"])
        created = create_discussion(
            graphql_url=graphql_url,
            token=token,
            repository_id=repository_state["id"],
            category_id=category["id"],
            title=topic["title"],
            body=body,
        )
        created_count += 1
        print(
            f"Created discussion '{created['title']}' in category '{category['name']}'."
        )

    print(f"Created {created_count} discussion topic(s).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
