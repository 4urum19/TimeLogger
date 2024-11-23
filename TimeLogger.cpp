#include <iostream>
#include <fstream>
#include <regex>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <vector>

std::vector<std::string> copySmatch(std::smatch match) {
	std::vector<std::string> v;
	for (size_t i = 0; i < match.size(); i += 1) {
		v.push_back(match[i]);
	}
	return v;
}

std::time_t stringToTimeT(std::string dateStr, std::string format) {
	std::tm time = {};
	std::istringstream ssi(dateStr);

	ssi >> std::get_time(&time, format.c_str());
	if (ssi.fail()) {
		perror("Failed to convert std::string to tm");
		return {};
	}

	return mktime(&time);
}

std::string getTimeSpent(std::smatch match, std::vector<std::string> prevMatch) {
	std::string prevDate = prevMatch[1] + " " + prevMatch[2];
	std::string date = (std::string)match[1] + " " + (std::string)match[2];
	std::string format = "%d-%m-%Y %H:%M:%S";

	std::time_t prevTime = stringToTimeT(prevDate, format);
	std::time_t time = stringToTimeT(date, format);

	uint32_t seconds = (uint32_t)difftime(time, prevTime);
	uint32_t hours = seconds / 3600;
	uint32_t minutes = (seconds % 3600) / 60;
	uint32_t sec = seconds % 60;

	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << hours << ":"
			<< std::setw(2) << std::setfill('0') << minutes << ":"
			<< std::setw(2) << std::setfill('0') << sec;
	return oss.str();
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

int printLog(std::string date) {
	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string logFileName = execPath.string() + "/log.txt";

	std::fstream logFile(logFileName, std::ios_base::in);
	std::string line;

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
	std::vector<std::string> prevMatch;
	oss << match[1] << ":\n";
	do {
    if (regex_match(line, match, reg) && match[1] == date) {
    	std::string timeSpent = "";
    	if (!prevMatch.empty()) {
    		timeSpent = getTimeSpent(match, prevMatch);
    	}
 			prevMatch = copySmatch(match);

 			oss << std::setw(8) << std::setfill(' ') << match[2] << " | " 
 					 << timeSpent << " | "
					<< match[3] << '\n';
    }
		if (logFile.eof()) break;
	} while(std::getline(logFile, line));
	std::cout << oss.str();
	oss.flush();
	return 0;
}

int main(int argc, char* argv[]) {
	clock_t tStart = clock();

	std::string date = getCurrentDate();
	std::string testDate = "23-11-2030";
	printLog(date);

	printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
	return 0;
}