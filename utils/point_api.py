from os import access
import os
import requests
import sys
import json
import time

# # # # # # # # # # # # # # # # # # # # # # # # # #		DESCRIPTION		# # # # # # # # # # # # # # # # # # # # # # # #
# point API for bonus system 'AQUANTER'
# run as independet python module
# run options passed in argv, argv description:
# [1]: path to point certificate, issued by owner of bonus system
# [2]: access token, dynamic token, convey client of current session (by QR code or any)
# [3]: command, may be 'open', 'close', 'writeoff', each command can have additional args
#	open: open transaction, return card info
#	close: close transaction
#		argv[4]: number of bonuses for acrue
#	writeoff: write off specefied count of bonuses
#		argv[4]: desired number of bonuses to be debited, real debited number of bonuses may be different
# conditional example:
# ..$ python3 point_api.py localhost path_to_point_certificate access_val cmd additional_arg
# real examples:
# ..$ python3 point_api.py ../config/point_cert.json -8594632929236741037+999135 open
# ..$ python3 point_api.py ../config/point_cert.json -8594632929236741037+999135 writeoff 120
# result will be written in stdout as json
# result format dpend of perform command
# result fields for 'open': 'rc', 'rt', 'type', 'count', 'id'
#	rc: int, return code, see it before any
#	rt: string, return text - clarification of return code
#	type: int, type of card
#	count: double, number of available bonuses
#	id: id of card
# result fields for 'close': 'rc', 'rt'
#	rc: int, return code, see it before any
#	rt: string, return text - clarification of return code
# result fields for 'writeoff': 'rc', 'rt', 'count'
#	rc: int, return code, see it before any
#	rt: string, return text - clarification of return code
#	count: double, number of real debited bonuses
# sequence:
# 1. open transaction
# 2. optionaly several times perform 'writeoff' cmd
# 3. close transaction
# return codes:
# 	0: OK
#	6: OK, but desired bonuses to be debit and real debited is different
#	-1: ERROR, no necessary arg
#	-2: ERROR, incorrect arg
#	-3: ERROR, permission denied
#	-4: ERROR, internal error
#	-5: ERROR, request fault
#	-32: ERROR, parsing error
# card types:
#	0: standard, personal
#	1: standard, from organization
#	5: service card


def rete(rc, rt):
	print('{"rc": ' + str(rc) + ', "rt": "' + str(rt) + '"}')
	exit(rc)


def reto(type, count, uid):
	print('{"rc": 0, "rt": null, "type": ' + str(type) + ', "count": ' + str(count) + ', "uid": "' + str(uid) + '"}')
	exit(0)


def retc():
	print('{"rc": 0, "rt": null}')
	exit(0)


def retwoff(count):
	print('{"rc": 0, "rt": null, "count": ' + str(count) + '}')
	exit(0)


def push_save(hn, cert, id, acs, cmd, count):
	try:
		if count == None:
			sd = {"hostname": hn, "req": {"cert": cert, "id": id, "access": acs, "cmd": cmd}}
		else:
			sd = {"hostname": hn, "req": {"cert": cert, "id": id, "access": acs, "cmd": cmd, "count": count}}
		if (not os.path.isdir("./.bonus_temp")):
			os.makedirs("./.bonus_temp")
		sf = open("./.bonus_temp/do_" + str(time.time()), 'w')
		sf.write(json.dumps(sd))
		sf.close()
	except:
		rete(-4, "fail to push save")


def send_saves():
	try:
		if not os.path.isdir("./.bonus_temp"):
			return
		files = os.listdir("./.bonus_temp")
		for f in files:
			if 'do_' in f:
				jdo = open("./.bonus_temp/" + f, "r")
				do = json.loads(jdo.read())
				jdo.close()
				r = requests.post(do["hostname"], json=do["req"])
				if r.status_code == 200:
					os.remove("./.bonus_temp/" + f)
	except:
		return


def req(hn, cert, id, acs, cmd, count, save = False):
	try:
		if count == None:
			r = requests.post(hn, json={"cert": cert, "id": id, "access": acs, "cmd": cmd})
		else:
			r = requests.post(hn, json={"cert": cert, "id": id, "access": acs, "cmd": cmd, "count": count})
	except:
		if save:
			push_save(hn, cert, id, acs, cmd, count)
			return None
		else:
			rete(-5, "request fault")
	if r.status_code != 200:
		if save:
			push_save(hn, cert, id, acs, cmd, count)
			return None
		else:
			rete(-5, "requset fault: " + r.status_code)
	
	try:
		jr = r.json()
	except:
		rete(-32, "parse error")
	if jr["rc"] == None:
		rete(-32, "parse error: no 'rc'")
	if jr["rc"] != 0:
		rete(jr["rc"], jr["rt"])
	return jr


send_saves()

if len(sys.argv) < 4:
	rete(-1, "too less args")

try:
	f = open(sys.argv[1], "r")
	data = f.read()
	f.close()
except:
	rete(-2, "fail to read point certificate")

try:
	jcert = json.loads(data)
	hostname = jcert["hostname"]
	id = jcert["id"]
	cert = jcert["cert"]
except:
	rete(-32, "fail to parse point certificate")

hostname = hostname + "/bonus/point.php"
acs = sys.argv[2]
cmd = sys.argv[3]

if cmd == "open":
	r = req(hostname, id, cert, acs, 1, None)
	if r["type"] == None or r["count"] == None or r["uid"] == None:
		rete(-5, "parse error: incorrect response")
	reto(r["type"], r["count"], r["uid"])
elif cmd == "close":
	if len(sys.argv) < 5:
		rete(-1, "too less args: no count")
	req(hostname, id, cert, acs, 3, sys.argv[4], True)
	retc()
elif cmd == "writeoff":
	if len(sys.argv) < 5:
		rete(-1, "too less args: no count")
	r = req(hostname, id, cert, acs, 2, sys.argv[4])
	if r["count"] == None:
		rete(-5, "parse error: incorrect response")
	retwoff(r["count"])
else:
	rete(-2, "incorrect command")

