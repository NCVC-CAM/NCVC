import glob

types = ('h', 'inl', 'c', 'cpp')
wc = 0

for t in types:
    for f in glob.iglob('NCVC/*.' + t):
        wc += sum([1 for _ in open(f)])

print(wc)
