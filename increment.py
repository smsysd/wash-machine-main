from os import access
# import os
import sys
import json

f = open("./info.json", "r")
info = json.loads(f.read())
f.close()
curver = info['version']
if len(sys.argv) > 1:
	v = sys.argv[1]
	if v == 'mj':
		curver[1] += 1
		curver[2] = 0
	elif v == 'mn':
		curver[0] += 1
		curver[1] = 0
		curver[2] = 0
	elif v == 'p':
		curver[2] += 1
	else:
		print('Incorrect version arg')
		exit(-1)
else:
	curver[2] += 1

info['version'] = curver
f = open("./info.json", 'w')
f.writelines(json.dumps(info, indent=4, separators=(',', ': '), sort_keys=False))
f.close()

