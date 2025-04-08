import json
from sys import argv


if len(argv) <= 1:
    exit(1)


with open(argv[1], 'r') as f:
    data = json.load(f)

data = [
    item
    for item in data
    if not item['output'].startswith('_deps/')
]

with open(argv[1], 'w') as f:
    json.dump(data, f)
