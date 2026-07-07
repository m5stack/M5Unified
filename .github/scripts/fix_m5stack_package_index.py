#!/usr/bin/env python3
"""Patch stale toolsDependencies in the M5Stack CN package index.

Older platform releases (<= 3.2.1) in package_m5stack_index_cn.json still
reference tool versions without the "-cn" suffix, while the index itself only
provides the renamed "-cn" tools, making those platforms uninstallable
(arduino-cli fails with "tool version ... not found"). Rewrite such
dependencies to the "-cn" version when that is the one available.
"""
import json
import sys

path = sys.argv[1]
with open(path) as f:
    data = json.load(f)

patched = 0
for package in data['packages']:
    tools = {(t['name'], t['version']) for t in package.get('tools', [])}
    for platform in package['platforms']:
        for dep in platform.get('toolsDependencies', []):
            if dep['packager'] != package['name']:
                continue
            if (dep['name'], dep['version']) in tools:
                continue
            if (dep['name'], dep['version'] + '-cn') in tools:
                dep['version'] += '-cn'
                patched += 1

with open(path, 'w') as f:
    json.dump(data, f)

print(f'patched {patched} tool dependencies')
