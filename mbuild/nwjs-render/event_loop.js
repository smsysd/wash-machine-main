class EventLoop {
	_subs;
	constructor () {
		this._subs = new Array();
	}

	event(name, data) {
		this._subs.forEach((v, i) => {
			if (v['name'] == name) {
				v['cb'](name, data);
			}
		});
	}

	sub(name, cb) {
		this._subs.push({'name': name, 'cb': cb});
	}

	unsub(cb) {
		this._subs.forEach((v, i) => {
			if (v['cb'] == cb) {
				this._subs.splice(i, 1);
				return;
			}
		});
	}
}

module.exports.createEventLoop = function() {
	return new EventLoop();
}