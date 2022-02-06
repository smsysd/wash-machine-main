from os import access
# import os
import sys
import json

try:
	f = open("./info.json", "r")
	info = json.loads(f.read())
	f.close()
	curver = info['version']
	if len(sys.argv) > 1:
		v = sys.argv[1]
		if v == 'mj':
			curver[1] += 1
			curver[2] = 0
			head = '##'
		elif v == 'mn':
			curver[0] += 1
			curver[1] = 0
			curver[2] = 0
			head = '###'
		elif v == 'p':
			curver[2] += 1
			head = '####'
		else:
			exit(-1)
	else:
		curver[2] += 1
		head = '####'

	info['version'] = curver
	f = open("./info.json", 'w')
	f.writelines(json.dumps(info, indent=4, separators=(',', ': '), sort_keys=False))
	f.close()
	if len(sys.argv) > 2:
		if sys.argv[2] == '-l':
			f = open("./changelog.md", "r")
			cl = f.readlines()
			f.close()
			f = open("./changelog.md", 'w')
			f.write(f"{head} {curver[0]}.{curver[1]}.{curver[2]}\n\n")
			f.writelines(cl)
			f.close()
	print(f"{curver[0]}.{curver[1]}.{curver[2]}")
except:
	exit(-1)
