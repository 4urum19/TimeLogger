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
#include <iomanip>

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

std::string getCurrentDate() {
	std::time_t now = std::time(NULL);
	std::tm localTime;
	localtime_r(&now, &localTime);

	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << localTime.tm_mday << '-' 
			<< std::setw(2) << std::setfill('0') << (localTime.tm_mon + 1) << '-' 
			<< std::setw(4) << std::setfill('0') << (localTime.tm_year + 1900);
	return oss.str();
}

std::string getCurrentTime() {
	std::time_t now = std::time(NULL);
	std::tm localTime;
	localtime_r(&now, &localTime);

	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << localTime.tm_hour << ':'
			<< std::setw(2) << std::setfill('0') << localTime.tm_min << ':'
			<< std::setw(2) << std::setfill('0') << localTime.tm_sec;
	return oss.str();
}

int logCommit(std::string msg) {
	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string logFileName = execPath.string() + "/log.txt";

	std::fstream logFile(logFileName, std::ios_base::app);
	if (!logFile.is_open()) {
		perror("Failed to open log");
		return 1;
	}

	std::string curDate = getCurrentDate();
	std::string curTime = getCurrentTime();

	logFile << '[' << curDate << "] [" << curTime << "] '" << msg << "'\n";
	logFile.close();

	return 0;
}

std::string getFileContent(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    perror("File failed to open");
    return "-1";
  }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  return content;
}

int debugLogGitCmd(std::string msg) {

	std::string commitMsg = getFileContent(msg);

	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string logFileName = execPath.string() + "/debugLog.txt";

	std::fstream logFile(logFileName, std::ios_base::app);
	if (!logFile.is_open()) {
		perror("Failed to open debug log");
		return 1;
	}

	logFile << "[" << commitMsg << "]\n";
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
  	execvp(execArgs[0], execArgs.data());
  	perror("Error executing git");
  	_exit(127);
  }
  else if (pid > 0) {
    if (argc > 2) {
      if (strcmp(execArgs[1], "commit") == 0 && strcmp(execArgs[2], "-m") == 0) {
        logCommit(execArgs[3]);
      } //CLI git integration
      else if (argc > 6) {
	      if (strcmp(execArgs[5], "commit") == 0 && strcmp(execArgs[6], "-F") == 0) {
	      	logCommit(getFileContent(execArgs[7]));
	      } //Jetbrain git integration
      }
    }
    
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        std::cerr << "Git was terminated by signal " << WTERMSIG(status) << "\n";
    } 
    else if (WIFEXITED(status)) {
    	return WEXITSTATUS(status);
    }
  }
  else {
  	perror("Fork failed");
  	return 1;
  } 

  return 0;
}
