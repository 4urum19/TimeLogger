#include <fstream>
#include <random>
#include <string>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <iostream>

std::string random_string(std::size_t length) {
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
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

int main() {
	clock_t tStart = clock();

	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string logFileName = execPath.string() + "/log.txt";

	std::fstream logFile(logFileName, std::ios_base::app);
	if (!logFile.is_open()) {
		perror("Failed to open log");
		return 1;
	}

	for (int i = 0; i < 1000000; i += 1) {
		std::string curDate = getCurrentDate();
		std::string curTime = getCurrentTime();
		std::string msg = random_string(255);

		logFile << '[' << curDate << "] [" << curTime << "] '" << msg << "'\n";
	}
	logFile.close();

	printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
	return 0;
}