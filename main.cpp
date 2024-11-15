#include <iostream>
#include <fstream>
#include <ctime>
#include <unistd.h>


int main(int argc, char* argv[]) {
	std::cout << "Entered custom git wrapper\n";

	std::string cmd = "/usr/bin/git";

	for(int i = 1; i < argc; i += 1) {
		cmd += " ";
		cmd += argv[i];
	}

	std::cout << cmd << '\n';

  int ret = std::system(cmd.c_str());

  return WEXITSTATUS(ret);
}