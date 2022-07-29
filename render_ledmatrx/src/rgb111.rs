use image::RgbImage;

fn pix_to_rgb111(color: [u8;3], order: bool) -> u8 {
	let mut c = 0;
	if color[2] > 127 {
		c |= 0b1;
	}
	if color[1] > 127 {
		c |= 0b10;
	}
	if color[0] > 127 {
		c |= 0b100;
	}
	if !order {
		c = c << 3;
	}
	c
}

pub fn to_rgb111(img: &RgbImage, buf: &mut [u8]) -> Option<usize> {
	if buf.len() < (img.width()*img.height()/2 + 1) as usize {
		return None;
	}

	let mut i = 0;
	buf.fill(0);
	for y in 0..img.height() {
		for x in 0..img.width() {
			let c = pix_to_rgb111(img.get_pixel(x, y).0, i % 2 == 0);
			buf[i/2] |= c;
			i += 1;
		}
	}

	return Some((img.width()*img.height()/2 + 1) as usize);
}