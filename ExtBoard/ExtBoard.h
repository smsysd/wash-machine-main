/* Author: Jodzik */
#ifndef EXT_BOARD_H_
#define EXT_BOARD_H_

#include "../mspi-linux/Mspi.h"

#include <vector>
#include <string>
#include <stdint.h>

using namespace std;

class ExtBoard {

public:
	enum class LightCmd {

	};

	struct LightInstruction{
		LightCmd cmd;
		
	};

	ExtBoard(string driver = "/dev/spi0.0");
	virtual ~ExtBoard();

private:
	Mspi* _spi;
};

#endif