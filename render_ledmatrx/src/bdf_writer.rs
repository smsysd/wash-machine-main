extern crate encoding;
use encoding::{Encoding, EncoderTrap};
use encoding::all::KOI8_U;

use image::RgbImage;
use image::Rgb;
use bdf;

pub fn write_gliph(x: u32, y: u32, glyph: &bdf::Glyph, img: &mut RgbImage, color: Rgb<u8>) -> u32 {
	if img.width() - x < glyph.width() {
		return 0;
	}

	for gy in 0..glyph.height() {
		for gx in 0..glyph.width() {
			if glyph.get(gx, gy) {
				img.get_pixel_mut(x+gx, y+gy).0 = color.0;
			}
		}
	}

	return glyph.width();
}

fn get_chars(str: &String) -> Vec<char> {
	match KOI8_U.encode(str.as_str(), EncoderTrap::Ignore) {
		Ok(v) => {
			let mut chars = Vec::<char>::new();
			for c in v {
				unsafe {
					let cv = char::from_u32_unchecked(c as u32);
					chars.push(cv);
				}
			}
			chars
		},
		_ => {
			vec!['?']
		}
	}
}

/// -> (width, height)
pub fn get_line_bound(font: &bdf::Font, str: &String) -> (u32, u32) {
	let chars = get_chars(str);
	let mut w = 0;
	let mut h = 0;
	for c in chars {
		let gliph = font.glyphs().get(&c);
		match gliph {
			Some(g) => {
				w += g.width();
				if g.height() > h {
					h = g.height();
				}
			},
			None => ()
		}
	}

	(w, h)
}

pub fn write_str(font: &bdf::Font, img: &mut RgbImage, color: Rgb<u8>, x: Option<u32>, y: Option<u32>, str: &String, char_gap: Option<i32>) {
	let chars = get_chars(str);
	let (w, h) = get_line_bound(font, str);
	let mut x = match x {
		Some(val) => val,
		None => {
			let center = ((img.width() as i32) - (w as i32))/2i32;
			if center < 0 {
				0
			} else {
				center as u32
			}
		}
	};
	let y = match y {
		Some(val) => val,
		None => (img.height() - h)/2
	};
	let gap = match char_gap {
		Some(val) => val,
		None => 1
	};
	for c in chars {
		let gliph = font.glyphs().get(&c);
		match gliph {
			Some(g) => {
				x += ((write_gliph(x, y, g, img, color) as i32) + gap) as u32;
			},
			None => ()
		}	
	}
}