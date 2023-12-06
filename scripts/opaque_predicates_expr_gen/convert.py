
with open('./zasm_generated.cpp', 'r') as f:
    content = f.read()


cases = []
cur = []

for line in content.splitlines():
    line = line.strip()
    if line in ['', ' ']:
        cases.append(list(cur))  # list for da copy
        cur.clear()
        continue

    cur.append(line)


print(f'switch (rnd::number<std::size_t>(0, {len(cases) - 1})) ' + '{')

for i in range(len(cases)):
    if i > 0:
        print()
    print('\t' + cases[i][0])
    print('\tcase ' + str(i) + ': {')
    for line in cases[i][1:]:
        print('\t\t' + line.replace('as->xor(', 'as->xor_(') + ';')
        if line.startswith('as->cmp'):
            print('\t\tvar_alloc.pop(as);')
    print('\t\tbreak;')
    print('\t}')

print()
print('\tdefault:')
print('\t\tthrow std::runtime_error("gen_predicate: invalid random index");')

print('};')
