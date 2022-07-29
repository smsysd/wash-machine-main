use std::os::unix::net::UnixStream;
use std::os::unix::net::UnixListener;
use std::fs::File;
use std::io::prelude::*;
use std::sync::mpsc::Receiver;
use std::sync::mpsc::Sender;
use std::time::Instant;
use std::time::Duration;
use std::sync::mpsc::channel;
use std::thread;

extern crate ctrlc;

extern crate bdf;

extern crate image;
use image::RgbImage;
use image::Rgb;
use image::imageops;

use termios;

extern crate serde;
#[macro_use]
extern crate serde_derive;
extern crate serde_json;

mod tty;
use tty::Tty;

mod bdf_writer;
mod rgb111;

const CONFIG_PATH: &str = "./config/render.json";
const SERVER_SOCK: &str = "./render.ipc";
const MAIN_SOCK: &str = "./main.ipc";

const CMD_EXIT: &str = "e";
const CMD_FRAME: &str = "f";
const CMD_TEMP_FRAME: &str = "t";

const DUMMY_STR: &str = "??";

const DISPLAY_WIDTH: u32 = 64;
const DISPLAY_HEIGHT: u32 = 32;
const DRAW_DATA_SIZE: usize = (DISPLAY_HEIGHT*DISPLAY_WIDTH/2 + 2) as usize; 

const TTY_DATA_SPEED: u32 = termios::os::linux::B500000;
const TTY_BREAK_SPEED: u32 = termios::os::linux::B1200;
const BREAK_DELAY: Duration = Duration::from_millis(10);

const MAIN_THREAD_SLEEP: Duration = Duration::from_millis(10);
const REDRAW_PERIOD: Duration = Duration::from_millis(250);
const CHECK_MAIN_PERIOD: Duration = Duration::from_millis(2500);

#[derive(Debug, Deserialize, Serialize)]
struct RawGenOptions {
    start_frame: String,
    no_main_frame: String,
    default_color: [u8;3],
    default_font: String,
    tty_driver: String,
    char_gap: Option<i32>,
    temp_frame_dur: u32
}

struct GenOptions {
    start_frame: String,
    no_main_frame: String,
    default_color: Rgb<u8>,
    default_font: String,
    tty_driver: String,
    char_gap: Option<i32>,
    temp_frame_dur: Duration
}

#[derive(Debug, Deserialize, Serialize)]
struct RawLine {
    x: Option<u32>,
    y: Option<u32>,
    color: Option<[u8;3]>,
    font: Option<String>,
    data: String
}

struct Line {
    x: Option<u32>,
    y: Option<u32>,
    color: Option<Rgb<u8>>,
    font: Option<String>,
    varnames: Vec<String>,
    data: String
}

#[derive(Debug, Deserialize, Serialize)]
struct RawFrame {
    name: String,
    bg: Option<String>,
    lines: Option<Vec<RawLine>>
}

struct Frame {
    name: String,
    bg: Option<RgbImage>,
    lines: Option<Vec<Line>>
}

#[derive(Debug, Deserialize, Serialize)]
struct RawFont {
    name: String,
    font: String
}

struct Font {
    name: String,
    font: bdf::Font
}

#[derive(Debug, Deserialize, Serialize)]
struct RawConfig {
    gen_options: RawGenOptions,
    fonts: Vec<RawFont>,
    frames: Vec<RawFrame>
}

struct CurrentFrame {
    main_i: usize,
    temp_i: Option<usize>,
    ton: Instant,
    dur: Duration,
    tredraw: Instant
}

enum SrvCmd {
    Exit
}

enum MainCmd {
    Exit,
    Frame(String),
    TempFrame(String)
}


fn get_font(name: String, fonts: &Vec<Font>) -> Result<usize, String> {
    for i in 0..fonts.len() {
        if fonts[i].name == name {
            return Ok(i);
        }
    }

    return Err(format!("font {} not found", name));
}

fn get_frame(name: String, frames: &Vec<Frame>) -> Result<usize, String> {
    for i in 0..frames.len() {
        if frames[i].name == name {
            return Ok(i);
        }
    }

    return Err(format!("frame {} not found", name));
}

/// Send encoded draw data to display
fn send_draw_data(tty: &mut Tty, buf: &[u8]) {
    let break_byte: [u8;1] = [0;1];
    tty.speed(TTY_BREAK_SPEED);
    tty.write(&break_byte);
    thread::sleep(BREAK_DELAY);
    tty.speed(TTY_DATA_SPEED);
    tty.write(&buf);
}

/// Get variable value by name from main program
fn get_var(name: &String) -> String {
    let cmd = format!("gv {}\n", name);
    let mut sock = match UnixStream::connect(MAIN_SOCK) {
        Ok(sock) => sock,
        _ => {
            println!("fail get var: no connecntion");
            return String::from(DUMMY_STR)
        }
    };
    match sock.set_read_timeout(Some(Duration::from_millis(250))) {
        Ok(()) => (),
        Err(_) => {
            return String::from(DUMMY_STR)
        }
    }
    match sock.write_all(cmd.as_bytes()) {
        Ok(()) => {
            let mut buf:[u8;64] = [0;64];
            match sock.read(&mut buf) {
                Err(e) => {
                    println!("fail read:");
                    dbg!(e);
                    return String::from(DUMMY_STR)
                },
                Ok(len) => {
                    let res = String::from_utf8(buf[..len].to_vec());
                    match res {
                        Ok(s) => return s,
                        _ => return String::from(DUMMY_STR)
                    }
                }
            }
        },
        Err(e) => {
            dbg!(e);
            return String::from(DUMMY_STR)
        }
    }
}

fn poll_main_srv() -> Option<i32> {
    let cmd = format!("poll");
    let mut sock = match UnixStream::connect(MAIN_SOCK) {
        Ok(sock) => sock,
        _ => {
            return None
        }
    };
    match sock.set_read_timeout(Some(Duration::from_millis(250))) {
        Ok(()) => (),
        Err(_) => {
            return None
        }
    }
    match sock.write_all(cmd.as_bytes()) {
        Ok(()) => {
            let mut buf:[u8;64] = [0;64];
            match sock.read(&mut buf) {
                Err(_) => {
                    return None
                },
                Ok(len) => {
                    let res = String::from_utf8(buf[..len].to_vec());
                    match res {
                        Ok(s) => return match s.parse::<i32>() {
                            Ok(num) => Some(num),
                            _ => None
                        },
                        _ => return None
                    }
                }
            }
        },
        Err(_) => {
            return None
        }
    }
}

///
fn redraw(frame: &Frame, fonts: &Vec<Font>, tty: &mut Tty, gen_opt: &GenOptions) {
    let mut cnv = match frame.bg.clone() {
        Some(bg) => bg,
        None => RgbImage::new(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    };
    match &frame.lines {
        Some(lines) => {
            for l in lines {
                // replace variables
                let mut ld = l.data.clone();
                for vn in &l.varnames {
                    let vpat = format!("{}{}{}", "{", vn, "}");
                    let val = get_var(&vn);
                    ld = String::from(ld.replace(vpat.as_str(), val.as_str()));
                }
                // get font
                let font_name = match l.font.clone() {
                    Some(f) => f,
                    None => gen_opt.default_font.clone()
                };
                let font = match get_font(font_name, fonts) {
                    Ok(i) => {
                        &fonts[i]
                    },
                    Err(e) => {
                        dbg!(e);
                        return;
                    }
                };
                // get color
                let color = match l.color {
                    Some(c) => c,
                    None => gen_opt.default_color
                };
                // draw line to canvas
                bdf_writer::write_str(&font.font, &mut cnv, color, l.x, l.y, &ld, gen_opt.char_gap);
            }
        },
        None => ()
    }

    // DEBUG
    // for h in 0..cnv.height() {
    //     for w in 0..cnv.width() {
    //         let pix = cnv.get_pixel(w, h);
    //         if pix.0[0] > 127 || pix.0[1] > 127 || pix.0[2] > 127 {
    //             print!("#");
    //         } else {
    //             print!(" ");
    //         }
    //     }
    //     println!("");
    // }

    // convert image canvas to draw data buffer
    let mut dd_buf: [u8;DRAW_DATA_SIZE] = [0;DRAW_DATA_SIZE];
    match rgb111::to_rgb111(&cnv, &mut dd_buf) {
        None => (),
        _ => {
            send_draw_data(tty, &dd_buf);
        }
    };
}


fn server(tx: Sender<MainCmd>, rx: Receiver<SrvCmd>) {
    let mut buf: [u8;1024] = [0;1024];
    if std::path::Path::new(SERVER_SOCK).exists() {
        match std::fs::remove_file(SERVER_SOCK) {
            Ok(()) => println!("previous sock removed"),
            Err(e) => println!("fail to remove previous sock: {:?}", e)
        };
    }
    let listener = UnixListener::bind(SERVER_SOCK).unwrap();
    loop {
        match listener.accept() {
            Ok((mut socket, addr)) => {
                dbg!(addr);
                let res = socket.read(&mut buf);
                match res {
                    Ok(len) => {
                        let res = String::from_utf8(buf[..len].to_vec());
                        match res {
                            Ok(s) => {
                                let mut s = s;
                                if s.chars().last().unwrap() == '\n' {
                                    s = String::from(&s[..s.len()-1]);
                                }
                                let (cmd, body) = match s.find(" ") {
                                    Some(p) => {
                                        (String::from(&s[..p]), String::from(&s[p+1..]))
                                    },
                                    None => {
                                        (s, String::new())
                                    }
                                };
                                dbg!(cmd.clone());
                                dbg!(body.clone());
                                let need_rx = match cmd.as_str() {
                                    CMD_EXIT => {
                                        tx.send(MainCmd::Exit).unwrap();
                                        true
                                    },
                                    CMD_FRAME => {
                                        tx.send(MainCmd::Frame(body)).unwrap();
                                        false
                                    },
                                    CMD_TEMP_FRAME => {
                                        tx.send(MainCmd::TempFrame(body)).unwrap();
                                        false
                                    },
                                    _ => false
                                };

                                if need_rx {
                                    match rx.recv().unwrap() {
                                        SrvCmd::Exit => break
                                    }
                                }
                            },
                            _ => ()
                        }
                    },
                    Err(e) => print!("fail to read: {:?}", e)
                }
            },
            Err(e) => {
                dbg!("accept function failed", e);
            },
        }
    }
}

fn main() {
    // read config - construct RawConfig
    let mut file = File::open(CONFIG_PATH).unwrap();
    let mut content = String::new();
    file.read_to_string(&mut content).unwrap();
    let raw_config: RawConfig = serde_json::from_str(&content).unwrap();

    // construct Font, GenOptions, Frame objects
    // construct GenOptions
    let gen_options: GenOptions;
    {
        let raw_def_color = raw_config.gen_options.default_color;
        let def_color = Rgb::<u8>(raw_def_color);
        let def_font = raw_config.gen_options.default_font;
        let start_frame = raw_config.gen_options.start_frame;
        let tty_driver = raw_config.gen_options.tty_driver;
        let char_gap = raw_config.gen_options.char_gap;
        let temp_frame_dur = Duration::from_millis(raw_config.gen_options.temp_frame_dur as u64);
        let no_main_frame = raw_config.gen_options.no_main_frame;
        gen_options = GenOptions {
            default_color: def_color,
            default_font: def_font,
            start_frame: start_frame,
            tty_driver: tty_driver,
            char_gap: char_gap,
            temp_frame_dur: temp_frame_dur,
            no_main_frame: no_main_frame
        };
    }

    // construct Fonts
    let mut fonts = Vec::<Font>::new();
    for rf in raw_config.fonts {
        let t = bdf::open(rf.font.clone());
        let bdf_font = match t {
            Ok(font) => font,
            Err(e) => {
                dbg!("fail construct font", rf.font.clone(), e);
                continue
            }
        };
        fonts.push(Font {name: rf.name, font: bdf_font});
    }

    // assert availability default font
    get_font(gen_options.default_font.clone(), &fonts).expect("default font not found");

    // construct Frames
    let mut frames = Vec::<Frame>::new();
    for rf in raw_config.frames {
        let bg: Option<RgbImage> = match rf.bg {
            Some(p) => {
                match image::open(p) {
                    Ok(dimg) => {
                        let mut mutdimg = dimg;
                        if mutdimg.width() != DISPLAY_WIDTH || mutdimg.height() != DISPLAY_HEIGHT {
                            mutdimg = mutdimg.resize(DISPLAY_WIDTH, DISPLAY_HEIGHT, imageops::FilterType::Nearest);
                        }
                        Some(mutdimg.to_rgb8())
                    },
                    Err(e) => {
                        dbg!(e);
                        None
                    }
                }
            },
            None => None
        };

        let lines = match rf.lines {
            Some(lines) => {
                let mut ls = Vec::<Line>::new();
                for rl in lines {
                    let color = match rl.color {
                        Some(c) => Some(Rgb::<u8>(c)),
                        None => None
                    };
                    let font = match rl.font {
                        Some(f) => Some(f),
                        None => None
                    };
                    // found varnames
                    let mut varns = Vec::<String>::new();
                    let mut temp_data = rl.data.clone();
                    loop {
                        let ob = temp_data.find("{");
                        let oe = temp_data.find("}");
                        let b: usize;
                        let e: usize;
                        match ob {
                            Some(i) => b = i,
                            None => break
                        }
                        match oe {
                            Some(i) => e = i,
                            None => break
                        }
                        // extract var name
                        let varn = String::from(&temp_data[b+1..e]);
                        // push to varns
                        if !varns.contains(&varn) {
                            varns.push(varn);
                        }
                        // delte this var
                        temp_data.replace_range(b..e, "");
                    }
                    // push
                    ls.push(Line { x: rl.x, y: rl.y, color: color, font: font, varnames: varns, data: rl.data });
                }
                Some(ls)
            },
            None => None
        };

        frames.push(Frame { name: rf.name, bg: bg, lines: lines });
    }

    // assert availability start frame
    let start_frame_i = get_frame(gen_options.start_frame.clone(), &frames).expect("start frame not found");
    // assert availability no_main frame
    let no_main_frame_i = get_frame(gen_options.no_main_frame.clone(), &frames).expect("no_main frame not found");

    // init tty
    let mut tty = Tty::new(&gen_options.tty_driver.clone(), termios::os::linux::B500000, true);

    // start server
    let (txs, rxm) = channel::<MainCmd>();
    let (txm, rxs) = channel::<SrvCmd>();
    let srv = std::thread::spawn(move || {server(txs, rxs)});

    // register ctrl+c handler
    let (cli_tx, cli_rx) = channel::<bool>();
    ctrlc::set_handler(move || {
        cli_tx.send(true).unwrap();
    }).unwrap();

    let mut cur_frame = CurrentFrame {main_i: start_frame_i, temp_i: None, dur: Duration::from_secs(1), ton: Instant::now(), tredraw: Instant::now()};
    let mut force_redraw = true;
    let mut tl_check_main = Instant::now();
    loop {
        // redraw current frame
        if cur_frame.tredraw.elapsed() >= REDRAW_PERIOD || force_redraw {
            cur_frame.tredraw = Instant::now();
            let i = match cur_frame.temp_i {
                Some(ti) => ti,
                None => cur_frame.main_i
            };
            redraw(&frames[i], &fonts, &mut tty, &gen_options);
            force_redraw = false;
        }

        // check duration if cur. frame is temp frame
        match cur_frame.temp_i {
            Some(ti) => {
                if cur_frame.ton.elapsed() > cur_frame.dur {
                    dbg!("remove temp frame ", ti);
                    cur_frame.temp_i = None;
                    redraw(&frames[cur_frame.main_i], &fonts, &mut tty, &gen_options);
                }
            },
            None => ()
        }

        // check server messages
        let res = rxm.try_recv();
        match res {
            Ok(cmd) => {
                match cmd {
                    MainCmd::Exit => {
                        txm.send(SrvCmd::Exit).unwrap();
                        srv.join().unwrap();
                        break;
                    },
                    MainCmd::Frame(f) => {
                        match get_frame(f, &frames) {
                            Ok(fi) => {
                                cur_frame.main_i = fi;
                                force_redraw = true;
                            },
                            _ => ()
                        }
                    },
                    MainCmd::TempFrame(f) => {
                        match get_frame(f, &frames) {
                            Ok(fi) => {
                                cur_frame.temp_i = Some(fi);
                                cur_frame.ton = Instant::now();
                                cur_frame.dur = gen_options.temp_frame_dur;
                                force_redraw = true;
                            },
                            _ => ()
                        }
                    }
                }
            },
            _ => ()
        }

        match cli_rx.try_recv() {
            Ok(s) => {
                if s {
                    println!("terminate");
                    break;
                }
            },
            _ => ()
        }

        if tl_check_main.elapsed() > CHECK_MAIN_PERIOD {
            match poll_main_srv() {
                None => {
                    cur_frame.main_i = no_main_frame_i;
                    force_redraw = true;
                },
                _ => ()
            }
            tl_check_main = Instant::now();
        }

        if !force_redraw {
            std::thread::sleep(MAIN_THREAD_SLEEP);
        }
    }
}
