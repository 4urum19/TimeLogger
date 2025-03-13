#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <regex>
#include <filesystem>
#include <sys/wait.h>
#include <iomanip>
#include <fcntl.h>

int main(int argc, char* argv[]) {
	std::cout << "parser\n";

	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string calendarPath = execPath.string() + "/calendar.ics";

	std::cerr << calendarPath << '\n';

  pid_t pid = fork();
  if (pid == 0) {
		const char* calendarPathCStr = calendarPath.c_str();

		int fd = open(calendarPathCStr, O_RDWR | O_CREAT | O_TRUNC, 0644);

	  if (dup2(fd, STDOUT_FILENO) == -1) {
	    perror("dup2 failed");
	    close(fd);
	    return -1;	
	  }
	  close(fd); 
  	
		std::vector<char*> execArgs;
		execArgs.push_back((char*)"curl");
		execArgs.push_back((char*)"[URL]"); 
  	execArgs.push_back(NULL);
  	execvp(execArgs[0], execArgs.data());
  	
  	perror("Error executing git");
  	_exit(127);
  }
  else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
      std::cerr << "Curl was terminated by signal " << WTERMSIG(status) << "\n";
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