#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <cstring>

int logCommit(std::string msg) {
	std::ofstream logFile("log.txt", std::ios_base::in);
	if (!logFile.is_open()) {
		std::cerr << "Failed to open log\n";
		return 1;
	}

	auto now = std::chrono::system_clock::now();
	std::time_t curTime = std::chrono::system_clock::to_time_t(now);
	
	logFile << '[' << std::ctime(&curTime) << "] " << msg << '\n';
	logFile.close();

	return 0;
}

int main(int argc, char* argv[]) {
  std::vector<char*> execArgs;
  execArgs.push_back((char*)"/usr/bin/git"); 
  for (int i = 1; i < argc; ++i) {
    execArgs.push_back(argv[i]);  
  }
  execArgs.push_back(nullptr);

  for (int i = 0; i < (int)execArgs.size(); i += 1) {
  	printf("%s ", execArgs[i]);
  } printf("\n");

  if (argc > 2) {
	  if (strcmp(execArgs[1], "commit") == 0 && strcmp(execArgs[2], "-m") == 0) {
	  	logCommit(execArgs[3]);
  	}
  }

  execvp(execArgs[0], execArgs.data());
  perror("Error executing git");
  return 1;
}