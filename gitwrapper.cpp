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

std::string timeSpent(
	const std::time_t nowTimeT, 
	const std::string logFileName) 
{
	std::ifstream logFile(logFileName);
	if (!logFile.is_open()) {
		perror("Failed to open log");
		return "-1";
	}
	std::string line;
	std::string lastLine;
  while (std::getline(logFile, line)) {
    if (!line.empty()) {
      lastLine = line;
    }
  }
  logFile.close();

  std::regex reg(R"(\[(.*)\] - \[(.*)\] '(.*)')");
  std::smatch match;

  if (std::regex_match(lastLine, match, reg)) {
  	std::string lastTimeStampStr = match[1].str();

    std::tm lastTimeTm = {};
    std::istringstream ss(lastTimeStampStr);
    ss >> std::get_time(&lastTimeTm, "%a %b %d %H:%M:%S %Y");
    if (ss.fail()) {
      std::cerr << "Failed to parse timestamp: " << lastTimeStampStr << '\n';
      return "-1";
    }

    auto lastTimeT = std::mktime(&lastTimeTm);

    auto diff = std::difftime(nowTimeT, lastTimeT);
    int hours = static_cast<int>(diff) / 3600;
    int minutes = (static_cast<int>(diff) % 3600) / 60;
    int seconds = static_cast<int>(diff) % 60;

    std::ostringstream timeSpentStr;
    timeSpentStr << hours << "h " << minutes << "m " << seconds << "s";
    std::cout << hours << "h " << minutes << "m " << seconds << "s" << '\n';
    return timeSpentStr.str();
  }

  std::cerr << "No valid timestamp found in last log line\n";
  return "-1";
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
	if (!nowStr.empty() && nowStr.back() == '\n') {
		nowStr.pop_back();
	}

	logFile << '[' << nowStr << "] - [" << timeSpent(nowT, logFileName) << "] '" << msg << "'\n";
	logFile.close();

	std::cerr << "Added:\n";
	std::cerr << '[' << nowStr << "] - [" << timeSpent(nowT, logFileName) << "] '" << msg << "'\n";

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