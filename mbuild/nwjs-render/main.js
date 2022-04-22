var nw = require("nw.gui");

var net = require("net");
var fs = require("fs");
var os = require("os");
var process = require("process");

var evl = require("./event_loop");

var server = undefined;
var curframe = undefined;
var events = evl.createEventLoop();

if (nw.App.argv.length < 2) {
    nw.App.quit();
}

function onbutton (name, data) {
    var con = net.createConnection(nw.App.argv[0], () => {
        con.write("b" + data);
    });
}

function onsrvcon(sock) {
    console.log('client connected');
    sock.on('data', (data) => {
        data = data.toString();
        console.log(data);
        if (data[0] == 'f') {
            let f = data.slice(1);
            if (f != curframe) {
                let path = './html/' + f;
                path = path.replace(/\s/g, '');
                if (fs.existsSync(path)) {
                    let html = fs.readFileSync(path);
                    events.event('frame', f);
                        // body.innerHTML = html;
                        let dom = new DOMParser().parseFromString(html, 'text/html');
                        // let bodydom = document.createElement(html);
                        // document.body.removeChild();
                        document.body = dom.body;
                }
                curframe = f;
            }
        } else
        if (data[0] == 'v') {
            let name = data.slice(0, data.indexOf(':'));
            let val = data.slice(data.indexOf(':') + 1);
            events.event(name, val);
            // alert("var in: '" + name + "': '" + val + "'");
            let el = document.getElementById(name);
            if (el !== undefined && el !== null) {
                // alert("var found");
                el.innerHTML = val;
            }
        }
    });
}

fs.unlinkSync(nw.App.argv[1]);

server = net.createServer(onsrvcon);
server.on('error', (err) => {
    alert(err)
});
server.listen(nw.App.argv[1], () => {
    console.log('server bound');
    var con = net.createConnection(nw.App.argv[0], () => {
        con.write("init");
    });
});

events.sub('button', onbutton);
