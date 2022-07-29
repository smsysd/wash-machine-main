#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        let result = 2 + 2;
        assert_eq!(result, 4);
    }
}

use termios::*;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::os::unix::prelude::AsRawFd;

pub struct Tty {
    baudrate: speed_t,
    fd: i32,
    tctl: Termios,
    tty: File
}

impl Tty {
    pub fn new(driver: &str, baudrate: speed_t, blocking: bool) -> Self {
        let mut opt = fs::OpenOptions::new();
        opt.read(true);
        opt.write(true);
        let tty = opt.open(driver).unwrap();
        let fd = tty.as_raw_fd();
    
        let mut tctl = Termios::from_fd(fd).unwrap();
        tctl.c_cflag |= CREAD | CLOCAL;
        tctl.c_lflag &= !(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
        tctl.c_oflag &= !OPOST;
        tctl.c_iflag &= !(INLCR | IGNCR | ICRNL | IGNBRK);
        tctl.c_cc[VMIN] = if blocking {1} else {0};
        tctl.c_cc[VTIME] = 0;
        cfsetspeed( &mut tctl, baudrate).unwrap();
        tcsetattr(fd, TCSANOW, &tctl).unwrap();

        Self {
            baudrate: baudrate,
            fd: fd,
            tctl: tctl,
            tty: tty
        }

    }

    pub fn write(&mut self, buf: &[u8]) {
        self.tty.write(buf).unwrap();
        tcdrain(self.fd).unwrap();
    }

    pub fn read(&mut self, buf: &mut [u8]) -> usize {
        let rc = self.tty.read(buf).unwrap();
        return rc;
    }

    pub fn speed(&mut self, baudrate: speed_t) {
        cfsetspeed(&mut self.tctl, baudrate).unwrap();
        tcsetattr(self.fd, TCSANOW, &self.tctl).unwrap();
        self.baudrate = baudrate;
    }

    pub fn sbreak(&self, dur: i32) {
        tcsendbreak(self.fd, dur).unwrap();
    }
}