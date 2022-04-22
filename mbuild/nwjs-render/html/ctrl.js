alert("ctrl.js");

function onframe(on, frame) {
	alert("onframe: " + frame);
}

events.sub('frame', onframe);
