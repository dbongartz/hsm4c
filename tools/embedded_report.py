#!/usr/bin/env python3
"""Code size and worst-case stack report for an embedded build of hsm4c.

Reads the static library and the call graph GCC writes with -fcallgraph-info=su (the cortex-m
presets set it) and prints a Markdown report:

- code and data size of the library (arm-none-eabi-size),
- worst-case stack per public function: the deepest chain of library stack frames. Callbacks
  (indirect calls) are the user's and come on top, the report marks where they are called.

Fails if the call graph has recursion or a frame of dynamic size, as then no bound exists.

Usage: embedded_report.py <build dir> [--title TITLE] [--size-tool arm-none-eabi-size]
"""

import argparse
import pathlib
import re
import subprocess
import sys

NODE = re.compile(r'node: \{ title: "([^"]+)" label: "([^"]*)"')
EDGE = re.compile(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"')
STACK = re.compile(r"\\n(\d+) bytes \((static|dynamic[^)]*)\)")
INDIRECT = "__indirect_call"


def parse_callgraph(path):
    """Returns {node: (function name, frame bytes, kind)} and {node: {callees}}."""
    nodes, edges = {}, {}
    for line in path.read_text().splitlines():
        if m := NODE.match(line):
            title, label = m.groups()
            name = label.split("\\n")[0]
            stack = STACK.search(label)
            nodes[title] = (name, int(stack.group(1)) if stack else 0,
                            stack.group(2) if stack else "none")
        elif m := EDGE.match(line):
            edges.setdefault(m.group(1), set()).add(m.group(2))
    return nodes, edges


def worst_case(nodes, edges):
    """Worst-case stack per node as (bytes, calls callbacks). Raises on recursion."""
    memo, active = {}, set()

    def visit(node):
        if node == INDIRECT:
            return 0, True
        if node in memo:
            return memo[node]
        if node in active:
            raise ValueError(f"recursion through {nodes[node][0]}")
        active.add(node)
        deepest, callbacks = 0, False
        for callee in edges.get(node, ()):
            depth, cb = visit(callee)
            deepest = max(deepest, depth)
            callbacks = callbacks or cb
        active.remove(node)
        memo[node] = (nodes[node][1] + deepest, callbacks)
        return memo[node]

    return {node: visit(node) for node in nodes if node != INDIRECT}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("build_dir", type=pathlib.Path)
    parser.add_argument("--title", default="hsm4c")
    parser.add_argument("--size-tool", default="arm-none-eabi-size")
    args = parser.parse_args()

    library = args.build_dir / "src" / "libhsm4c.a"
    callgraph = next(args.build_dir.glob("src/**/hsm4c.c.ci"))

    size = subprocess.run([args.size_tool, "-B", "-t", str(library)], check=True,
                          capture_output=True, text=True).stdout.splitlines()[-1].split()
    text, data, bss = (int(x) for x in size[:3])

    nodes, edges = parse_callgraph(callgraph)
    errors = [f"{name}: frame size is {kind}" for name, _, kind in nodes.values()
              if kind.startswith("dynamic")]
    try:
        depths = worst_case(nodes, edges)
    except ValueError as e:
        errors.append(str(e))
        depths = {}

    print(f"### {args.title}\n")
    print("| Code (text) | Data | BSS |\n|---|---|---|")
    print(f"| {text} B | {data} B | {bss} B |\n")
    print("| Function | Worst-case stack | Calls callbacks |\n|---|---|---|")
    for node, (depth, callbacks) in sorted(depths.items(), key=lambda kv: nodes[kv[0]][0]):
        name = nodes[node][0]
        if name.startswith("hsm4c_"):
            print(f"| `{name}` | {depth} B | {'yes, plus their stack' if callbacks else 'no'} |")
    print()

    for error in errors:
        print(f"**Error:** {error}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
