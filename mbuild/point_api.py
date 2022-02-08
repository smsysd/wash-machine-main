import requests
import sys
import json

# # # # # # # # # # # # # # # # # # # # # # # # # #		DESCRIPTION		# # # # # # # # # # # # # # # # # # # # # # # #
# point API for bonus system 'AQUANTER'
# run as independet python module
# run options passed in argv, argv description:
# [1]: path to point certificate, issued by owner of bonus system
# [2]: command, may be 'open', 'close', 'writeoff', each command can have additional args
#	info: info about acrd
#		argv [3]: access token, convey client of current session (by QR code or any), may be static access, receipt text or promo code
#	open: open transaction, return card info
#		argv [3]: access token, convey client of current session (by QR code or any), may be static access, receipt text or promo code
#	close: close transaction
#		argv[3]: number of bonuses for acrue
#	writeoff: write off specefied count of bonuses
#		argv[3]: desired number of bonuses to be debited, real debited number of bonuses may be different
#	onetime: perform onetime card, this used only once for one card, it may be receipt or promo code
#		argv [3]: access token, convey client of current session (by QR code or any), may be static access, receipt text or promo code
# conditional example:
# ..$ python3 point_api.py localhost path_to_point_certificate cmd additional_arg
# real examples:
# ..$ python3 point_api.py ../config/point_cert.json open -DCA8594632929236741037+5
# ..$ python3 point_api.py ../config/point_cert.json writeoff 120
# result will be written in stdout as json
# result format dpend of perform command, but always exists:
#	rc: int, return code, see it before any
#	rt: string, return text - clarification of return code
# result fields for 'info': 'type', 'count', 'id'
#	type: string, type of card
#	count: double, number of available bonuses
#	id: uint64 uid of card
# result fields for 'open': none
# result fields for 'close': none
# result fields for 'writeoff': 'count'
#	count: double, number of real debited bonuses
# result fields for 'onetime': 'count'
#	count: double, number of bonuses
# sequence bonus cards:
# 1. open transaction
# 2. optionaly several times perform 'writeoff' cmd
# 3. close transaction
# return codes:
# 	0: OK
#	-1: ERROR, no necessary arg
#	-2: ERROR, incorrect arg
#	-3: ERROR, access denied
#	-4: ERROR, internal error
#	-5: ERROR, request fault
#	-7: ERROR, conditions not met
#	-8: ERROR, transaction for this point already open and not close
#	-9: ERROR, not found
#	-32: ERROR, parsing error
# card types:
#	p: bonus, personal
#	o: bonus, from organization
#	s: service card
#	t: onetime


def rete(rc, rt):
	print('{"rc": ' + str(rc) + ', "rt": "' + str(rt) + '"}')
	exit(rc)


def retinfo(type, count, id):
	print('{"rc": 0, "rt": null, "type": "' + str(type) + '", "count": ' + str(count) + ', "id": ' + str(id) + '}')
	exit(0)


def retok():
	print('{"rc": 0, "rt": null}')
	exit(0)


def retwoff(count):
	print('{"rc": 0, "rt": null, "count": ' + str(count) + '}')
	exit(0)


def req(hn, cert, id, cmd, acs = None, count = None):
	data = {"cert": cert, "id": id, "cmd": cmd}
	if acs != None:
		data["access"] = acs
	if count != None:
		data["count"] = count

	try:
		r = requests.post(hn, json=data)
	except:
		rete(-5, "request fault")
	if r.status_code != 200:
		rete(-5, "requset fault: " + str(r.status_code))

	try:
		jr = r.json()
	except:
		rete(-32, "parse error, resp text: " + r.text)
	if jr["rc"] == None:
		rete(-32, "parse error: no 'rc'")
	if jr["rc"] != 0:
		rete(jr["rc"], jr["rt"])
	return jr


def getarg(index, name):
	if len(sys.argv) <= index:
		rete(-1, "too less args: no " + name)
	return sys.argv[index]


if len(sys.argv) < 3:
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
cmd = sys.argv[2]

if cmd == "info":
	acs = getarg(3, 'access')
	r = req(hostname, cert, id, 4, acs)
	if r["type"] == None or r["count"] == None or r["id"] == None:
		rete(-5, "parse error: incorrect response")
	retinfo(r["type"], r["count"], r["id"])
if cmd == "open":
	acs = getarg(3, 'access')
	r = req(hostname, cert, id, 1, acs)
	retok()
elif cmd == "close":
	count = getarg(3, 'count')
	req(hostname, cert, id, 3, None, count)
	retok()
elif cmd == "writeoff":
	count = getarg(3, 'count')
	r = req(hostname, cert, id, 2, None, count)
	if r["count"] == None:
		rete(-5, "parse error: incorrect response")
	retwoff(r["count"])
elif cmd == "onetime":
	acs = getarg(3, 'access')
	r = req(hostname, cert, id, 5, acs)
	if r["count"] == None:
		rete(-5, "parse error: incorrect response")
	retwoff(r["count"])
else:
	rete(-2, "incorrect command")

