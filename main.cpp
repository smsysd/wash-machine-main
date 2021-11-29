#include "extboard/ExtBoard.h"
#include "general-tools/general_tools.h"
#include "jparser-linux/JParser.h"
#include "utils/utils.h"

#include <sstream>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

enum class Mode {
	GIVE_CASH,
	PROGRAM
};

Mode mode = Mode::GIVE_CASH;

int main(int argc, char const *argv[]) {
	init();
	printLogo();
	setGiveMoneyMode();
	
	while (true) {
		
	}

	return 0;
}
