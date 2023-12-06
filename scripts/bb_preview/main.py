import os
os.environ['PATH'] += os.pathsep + 'C:\\Program Files\\Graphviz\\bin'

from pathlib import Path
import graphviz


root_dir = Path(__file__).parent / 'data'

dot = graphviz.Digraph(format='png')


for file in root_dir.glob('*.bb'):
    with open(file, 'r') as f:
        content = f.read().splitlines()

    start: int = 0
    end: int = 0
    instructions = []
    successors = []
    predecessors = []

    for line in content:
        if line.startswith('start:'):
            start = int(line.split(':')[1], 16)
        if line.startswith('end:'):
            end = int(line.split(':')[1], 16)
        if line.startswith('instruction:'):
            data = line.split(':')[1]
            instructions.append(data.replace(';', '\t'))
        if line.startswith('successor:'):
            successors.append(int(line.split(':')[1], 16))
        if line.startswith('predecessor:'):
            predecessors.append(int(line.split(':')[1], 16))

    body = '\n'.join(instructions)
    dot.node(str(start), body)

    for successor in successors:
        dot.edge(str(start), str(successor))

    os.remove(file)


dot.render('basic_blocks', directory=root_dir.parent, view=True)
