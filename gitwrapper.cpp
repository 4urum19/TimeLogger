#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <regex>
#include <sys/wait.h>
#include <filesystem>

std::string getLastLine(std::string logFileName) {
	std::fstream logFile(logFileName, std::ios_base::in);
	std::string lastLine = "";

	if (!logFile.is_open()) {
		perror("Failed to open log");
		return "";
	}

	logFile.seekg(0, std::ios_base::end);
	auto pos = logFile.tellg();
	if (pos == 0) {
		return "";
	}

	pos -= 2;
	while(pos >= 0) {
		logFile.seekg(pos);
		char c;
		logFile.get(c);
		if (c == '\n') {
			pos += 1;
			logFile.seekg(pos);
			break;
		}
		pos -= 1;
	}
	std::getline(logFile, lastLine);

	return lastLine;
}

int logCommit(std::string msg) {
	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string logFileName = execPath.string() + "/log.txt";

	std::fstream logFile(logFileName, std::ios_base::app);
	if (!logFile.is_open()) {
		perror("Failed to open log");
		return 1;
	}

	auto now = std::chrono::system_clock::now();
	auto nowT = std::chrono::system_clock::to_time_t(now);
	
	std::string nowStr = std::ctime(&nowT);
	std::string curDate;
	std::string curTime;

	if (!nowStr.empty() && nowStr.back() == '\n') {
		nowStr.pop_back();
	}

	std::regex reg(R"([A-Za-z]+ ([A-Za-z]+) ([0-9]+) ([0-9:]+) ([0-9]+))");
	std::smatch match;
	if (std::regex_match(nowStr, match, reg)) {
		curDate = (std::string)match[2] + " " + (std::string)match[1] + " " + (std::string)match[4];
		curTime = match[3];
	}


	logFile << '[' << curTime << "] '" << msg << "'\n";
	logFile.close();

	std::cerr << "Added:\n";
	std::cerr << '[' << nowStr << "] '" << msg << "'\n";

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
		  	std::cerr << "Logging " << execArgs[3] << '\n';
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