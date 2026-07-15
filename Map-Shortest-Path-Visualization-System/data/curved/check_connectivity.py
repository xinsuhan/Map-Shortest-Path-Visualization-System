#!/usr/bin/env python3
"""Check that every curved-campus POI entrance can reach every other POI."""

import csv
import sys
from collections import defaultdict, deque
from pathlib import Path


DATA_DIR = Path(__file__).resolve().parent


def load_csv(name):
    with (DATA_DIR / name).open("r", encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def reachable(adjacency, start):
    seen = {start}
    queue = deque([start])
    while queue:
        current = queue.popleft()
        for neighbor in adjacency[current]:
            if neighbor not in seen:
                seen.add(neighbor)
                queue.append(neighbor)
    return seen


def main():
    nodes = load_csv("nodes.csv")
    edges = load_csv("edges_curved.csv")
    pois = load_csv("pois.csv")
    node_ids = {node["id"] for node in nodes}
    adjacency = defaultdict(set)
    incoming = defaultdict(set)
    errors = []

    for edge in edges:
        source = edge["source"]
        target = edge["target"]
        if source not in node_ids or target not in node_ids:
            errors.append(f"edge {edge['id']} references unknown node {source}->{target}")
            continue
        if edge["walkable"] not in {"0", "1"}:
            errors.append(f"edge {edge['id']} has invalid walkable={edge['walkable']!r}")
            continue
        if edge["walkable"] == "0":
            continue
        adjacency[source].add(target)
        incoming[target].add(source)
        if edge["oneway"] in {"False", "false", "0", "wide"}:
            adjacency[target].add(source)
            incoming[source].add(target)

    print("POI entrances:")
    for poi in pois:
        entrance = poi["nearest_node"]
        if entrance not in node_ids:
            errors.append(
                f"{poi['id']} {poi['name']} references missing entrance {entrance}"
            )
            continue
        out_degree = len(adjacency[entrance])
        in_degree = len(incoming[entrance])
        print(
            f"  {poi['id']} {poi['name']}: {entrance} "
            f"out={out_degree} in={in_degree}"
        )
        if out_degree == 0 or in_degree == 0:
            errors.append(
                f"{poi['id']} {poi['name']} entrance {entrance} is isolated "
                f"in one direction (out={out_degree}, in={in_degree})"
            )

    reachable_by_entrance = {
        poi["nearest_node"]: reachable(adjacency, poi["nearest_node"])
        for poi in pois
        if poi["nearest_node"] in node_ids
    }
    unreachable_pairs = []
    for index, start in enumerate(pois):
        start_id = start["nearest_node"]
        if start_id not in reachable_by_entrance:
            continue
        for goal in pois[index + 1 :]:
            goal_id = goal["nearest_node"]
            if goal_id not in reachable_by_entrance:
                continue
            missing_directions = []
            if goal_id not in reachable_by_entrance[start_id]:
                missing_directions.append(f"{start['id']}->{goal['id']}")
            if start_id not in reachable_by_entrance[goal_id]:
                missing_directions.append(f"{goal['id']}->{start['id']}")
            if missing_directions:
                unreachable_pairs.append(
                    f"{start['id']} {start['name']} ({start_id}) <-> "
                    f"{goal['id']} {goal['name']} ({goal_id}); "
                    f"missing {', '.join(missing_directions)}"
                )

    if unreachable_pairs:
        print("Unreachable POI pairs:")
        for pair in unreachable_pairs:
            print(f"  {pair}")
        errors.append(f"{len(unreachable_pairs)} unreachable POI pair(s)")
    else:
        print(f"OK: all {len(pois)} POIs are mutually reachable.")

    if errors:
        print("Connectivity check failed:", file=sys.stderr)
        for error in errors:
            print(f"  {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
