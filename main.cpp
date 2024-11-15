#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <sys/wait.h>

int logCommit(std::string msg) {
	std::ofstream logFile("log.txt", std::ios_base::app);
	if (!logFile.is_open()) {
		perror("Failed to open log");
		return 1;
	}

	auto now = std::chrono::system_clock::now();
	auto nowT = std::chrono::system_clock::to_time_t(now);
	std::string nowStr = std::ctime(&nowT);
	if (!nowStr.empty() && nowStr.back() == '\n') {
		nowStr.pop_back();
	}

	logFile << '[' << nowStr << "] '" << msg << "'\n";
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

  pid_t pid = fork();
  if (pid == 0) {
  	//child process to execute git
  	execvp(execArgs[0], execArgs.data());
  	perror("Error executing git");
  	return 1;
  }
  else if (pid > 0) {
		//Log if commit is made
		if (argc > 2) {
		  if (strcmp(execArgs[1], "commit") == 0 && strcmp(execArgs[2], "-m") == 0) {
		  	logCommit(execArgs[3]);
			}
		}

		//Wait for child process to exit
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        std::cout << "Git was terminated by signal " << WTERMSIG(status) << "\n";
    }
  }
  else {
  	perror("Fork failed");
  } 

  return 0;
}