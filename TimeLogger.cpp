#include <iostream>
#include <fstream>
#include <regex>
#include <ctime>
#include <filesystem>
#include <iomanip>

// std::string timeSpent(
// 	const std::time_t nowTimeT, 
// 	const std::string logFileName) 
// {
// 	std::ifstream logFile(logFileName);
// 	if (!logFile.is_open()) {
// 		perror("Failed to open log");
// 		return "-1";
// 	}
// 	std::string line;
// 	std::string lastLine;
//   while (std::getline(logFile, line)) {
//     if (!line.empty()) {
//       lastLine = line;
//     }
//   }
//   logFile.close();

//   std::regex reg(R"(\[(.*)\] - \[(.*)\] '(.*)')");
//   std::smatch match;

//   if (std::regex_match(lastLine, match, reg)) {
//   	std::string lastTimeStampStr = match[1].str();

//     std::tm lastTimeTm = {};
//     std::istringstream ss(lastTimeStampStr);
//     ss >> std::get_time(&lastTimeTm, "%a %b %d %H:%M:%S %Y");
//     if (ss.fail()) {
//       std::cerr << "Failed to parse timestamp: " << lastTimeStampStr << '\n';
//       return "-1";
//     }

//     auto lastTimeT = std::mktime(&lastTimeTm);

//     auto diff = std::difftime(nowTimeT, lastTimeT);
//     int hours = static_cast<int>(diff) / 3600;
//     int minutes = (static_cast<int>(diff) % 3600) / 60;

//     std::ostringstream timeSpentStr;
//     timeSpentStr << hours << "h " << minutes << "m";
//     std::cout << hours << "h " << minutes << "m " << '\n';
//     return timeSpentStr.str();
//   }

//   std::cerr << "No valid timestamp found in last log line\n";
//   return "-1";
// }

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

	std::regex reg(R"(\[(.+)\] \[(.+)\] '(.*?)')");
	std::smatch match;
	regex_match(line, match, reg);

	while (match[1] != date) {
		regex_match(line, match, reg);
		std::getline(logFile, line);
		if (logFile.eof()) break;
	}
	std::cout << match[1] << ":\n";

	while (match[1] == date) {
		regex_match(line, match, reg);
		std::cout << match[2] << "|" << match[3] << '\n';
		std::getline(logFile, line);
		if (logFile.eof()) break;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	const char* progName = argv[0];

	std::string date = getCurrentDate();

	printLog("23-10-2023");

	return printLog(date);
}