#include <iostream>
#include <fstream>
#include <regex>
#include <ctime>
#include <filesystem>
#include <iomanip>

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

int printLog(std::string date) {
	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string logFileName = execPath.string() + "/log.txt";

	std::fstream logFile(logFileName, std::ios_base::in);
	std::string line;
	std::getline(logFile, line);

	std::regex reg(R"(\[(.+)\] \[(.+)\] '(.*)')");
	std::smatch match;
	regex_match(line, match, reg);

	while (std::getline(logFile, line)) {
    if (regex_match(line, match, reg) && match[1] == date) {
      break;  
    }
		if (logFile.eof()) return 0;
	}

	std::ostringstream oss;
	oss << match[1] << ":\n";
	while (match[1] == date) {
    if (regex_match(line, match, reg) && match[1] == date) {
 			oss << std::setw(8) << std::setfill(' ') << match[2] << " | " 
					<< match[3] << '\n';
    }
		std::getline(logFile, line);
		if (logFile.eof()) break;
	}
	std::cout << oss.str();
	oss.flush();
	return 0;
}

int main(int argc, char* argv[]) {
	clock_t tStart = clock();

	std::string date = getCurrentDate();
	std::string testDate = "23-11-2030";
	printLog(testDate);

	printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
	return 0;
}