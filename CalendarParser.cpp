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
#include <utility>
#include <getopt.h>

struct Event {
	std::string title = "";
	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;
	std::string timeZone = "";
};

template <typename T>
std::vector<T> splitString(
	std::string str, 
	char delim,
	std::function<T(const std::string&)> converter) 
{
  std::vector<T> tokens;
  size_t pos = 0;
  std::string token;
  while ((pos = str.find(delim)) != std::string::npos) {
    token = str.substr(0, pos);
    tokens.push_back(converter(token));
    str.erase(0, pos + 1);
  }
  tokens.push_back(converter(str));

  return tokens;
}

bool isSameDate(const std::string& logEntry, const std::string& targetDate) {
  return logEntry.find("[" + targetDate + "]") == 0;
}

bool isLaterDate(const std::string& logEntry, const std::string& targetDate) {
	std::function converter = [](const std::string& str) { return std::stoi(str); };
	char delim = '-';

	std::vector<int> logDate = splitString(logEntry.substr(1, 11), delim, converter);
	if (logDate.size() != 3) {
		return false;
	}

	std::vector<int> target = splitString(targetDate, delim, converter);
	if (target.size() != 3) {
		return false;
	}	

  if (logDate[0] > target[0]) {
    return true;
  } 
  else if (logDate[1] > target[1] && logDate[0] == target[0]) {
    return true;
  } 
  else if (logDate[2] > target[2] && logDate[1] == target[1] && logDate[0] == target[0]) {
    return true;
  }
	return false;
}

bool isEarlierDate(const std::string& logEntry, const std::string& targetDate) {
	std::function converter = [](const std::string& str) { return std::stoi(str); };
	char delim = '-';

	std::vector<int> logDate = splitString(logEntry.substr(1, 11), delim, converter);
	if (logDate.size() != 3) {
		return false;
	}

	std::vector<int> target = splitString(targetDate, delim, converter);
	if (target.size() != 3) {
		return false;
	}	

  if (logDate[0] < target[0]) {
    return true;
  } 
  else if (logDate[1] < target[1] && logDate[0] == target[0]) {
    return true;
  } 
  else if (logDate[2] < target[2] && logDate[1] == target[1] && logDate[0] == target[0]) {
    return true;
  }
	return false;
}

/* Helper functions to format time_points
*/
std::string formatToDateYmd(const std::chrono::system_clock::time_point& date) {
  std::time_t time = std::chrono::system_clock::to_time_t(date);
  std::tm tm = *std::localtime(&time);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d");

  return oss.str();
}

std::string formatToDatedmY(const std::chrono::system_clock::time_point& date) {
  std::time_t time = std::chrono::system_clock::to_time_t(date);
  std::tm tm = *std::localtime(&time);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%d-%m-%Y");

  return oss.str();
}

std::string formatToHMS(const std::chrono::system_clock::time_point& time) {
	std::time_t t = std::chrono::system_clock::to_time_t(time);
  std::tm tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");

  return oss.str();
}

/*	Helper functions to convert events into log entries
*/
std::string startEventToLogEntry(const Event& event) {
  std::ostringstream oss;
  oss << "[" << formatToDatedmY(event.start) << ']'
      << " [" << formatToHMS(event.start) << ']'
      << " 'Start " << event.title << "'";
  return oss.str();
}

std::string endEventToLogEntry(const Event& event) {  
  std::ostringstream oss;
  oss << "[" << formatToDatedmY(event.end) << ']'
      << " [" << formatToHMS(event.end) << ']'
      << " 'end " << event.title << "'";
  return oss.str();
}

void printEvents(const std::vector<Event> events) {
  for (const auto& event : events) {
    std::time_t startTime = std::chrono::system_clock::to_time_t(event.start);
    std::time_t endTime = std::chrono::system_clock::to_time_t(event.end);
    std::cout << "Event:\t" << event.title << '\n'
              << "Start:\t" << std::ctime(&startTime)
              << "End:\t" << std::ctime(&endTime)
              << "Timezone:\t" << event.timeZone << "\n\n";
  }
}

/* Parse if possible to a timezone
 * Default return is "UTC"
*/
std::string parseTimeZone(
	const std::string &tz) 
{
	size_t tzPos = tz.find("TZID=");
	if (tzPos != std::string::npos) {
		return tz.substr(5, tz.length());
	}

	return "UTC";
}

/* Parse string to system time seconds
 * quite a mess of c_time & chrono types
*/
std::chrono::system_clock::time_point parseIcalDateTime(
	const std::string& dateStr, 
	const std::string& tz = "UTC") 
{
  std::tm tm = {};
  std::istringstream iss(dateStr);
  
  bool utc = !dateStr.empty() && dateStr.back() == 'Z';
  std::string format = utc ? "%Y%m%dT%H%M%SZ" : "%Y%m%dT%H%M%S";
  
  iss >> std::get_time(&tm, format.c_str());
  if (iss.fail()) {
     throw std::runtime_error("Failed to parse datetime: " + dateStr);
  }

  if (utc) {
    return std::chrono::sys_seconds(std::chrono::seconds(std::mktime(&tm)));
  } 
  else {
    try {
      const std::chrono::time_zone* timeZone = std::chrono::locate_zone(tz);
      std::chrono::local_seconds lt{std::chrono::seconds(std::mktime(&tm))};
      return timeZone->to_sys(lt);
    } 
    catch (const std::runtime_error& e) {
      std::cerr << "Warning: Unknown timezone " << tz << ", defaulting to UTC\n";
      return std::chrono::sys_seconds(std::chrono::seconds(std::mktime(&tm)));
    }
  }
}

/*	Parse the ics file into a vector of events for a given date
 *	line by line, possibly later simplify while loop
 *	& add more conditions before events being pushed (status for example)
*/
std::vector<Event> parse(
	const std::string& calendarPath,
	const std::string date) 
{
	std::ifstream calendarFile(calendarPath);
	if (!calendarFile.is_open()) {
		perror("Failed to open calendar file");
		return std::vector<Event>();
	}
	
	std::vector<Event> events;
	std::string line;
	std::function converter = [](const std::string& str) { return str; };

	while (std::getline(calendarFile, line)) {
		if (line == "BEGIN:VEVENT") {
			Event event;
			
			while (std::getline(calendarFile, line) && line != "END:VEVENT") {
				std::vector<std::string> v = splitString(line, ':', converter);
				if (v.size() < 2) {
					continue;
				}

				if (v[0] == "SUMMARY") {
					event.title = v[1];
				}
				else if (v[0].rfind("DTSTART", 0) == 0) {
					if (event.timeZone.empty()) {
						std::vector<std::string> tzv = splitString(v[0], ';', converter);
						if (tzv.size() > 1) {
							event.timeZone = parseTimeZone(tzv[1]);
						}
						else {
							event.timeZone = "UTC";
						}
					}
					event.start = parseIcalDateTime(v[1], event.timeZone);
				}
				else if (v[0].rfind("DTEND", 0) == 0) {
					if (event.timeZone.empty()) {
						std::vector<std::string> tzv = splitString(v[0], ';', converter);
						if (tzv.size() > 1) {
							event.timeZone = parseTimeZone(tzv[1]);
						}
						else {
							event.timeZone = "UTC";
						}
					}
					event.end = parseIcalDateTime(v[1], event.timeZone);
				}
			}

			if (date == formatToDatedmY(event.start) && formatToDatedmY(event.start) == formatToDatedmY(event.end)) {
				events.push_back(event);
			}
		}
	}

  printEvents(events);

  return events;
}

/*	Insert the events for a given date into the log
 *	by reading line by line & copying the lines from the original log
 *	into a temp file, inserting when the target date is reached
*/
void insertEventsIntoLog(
	const std::string& logPath, 
  const std::vector<Event>& newEvents,
  const std::string& targetDate) 
{
  std::string tempPath = logPath + ".tmp";
  std::ofstream tempFile(tempPath);
  
  std::vector<std::string> dateEntries;
  
  std::ifstream originalFile(logPath);
  std::string line;
  bool processingTargetDate = false;

  while (std::getline(originalFile, line)) {
    if (isSameDate(line, targetDate) || isLaterDate(line, targetDate) || isEarlierDate(line, targetDate)) {
      if (!processingTargetDate) {
        processingTargetDate = true;
        dateEntries.clear();
      }
      dateEntries.push_back(line);
    } 
    else {
      if (processingTargetDate) {
        processingTargetDate = false;
        
        std::vector<std::string> newEntries;
        for (const auto& event : newEvents) {
          newEntries.push_back(startEventToLogEntry(event));
          newEntries.push_back(endEventToLogEntry(event));
        }
        
        dateEntries.insert(dateEntries.end(), newEntries.begin(), newEntries.end());
        std::sort(dateEntries.begin(), dateEntries.end());
        auto lastUnique = std::unique(dateEntries.begin(), dateEntries.end());
    		dateEntries.erase(lastUnique, dateEntries.end());

        for (const auto& entry : dateEntries) {
          tempFile << entry << '\n';
        }
      }
      tempFile << line << '\n';
    }
  }

  if (processingTargetDate) {
    std::vector<std::string> newEntries;
    for (const auto& event : newEvents) {
      newEntries.push_back(startEventToLogEntry(event));
      newEntries.push_back(endEventToLogEntry(event));
    }
    
    dateEntries.insert(dateEntries.end(), newEntries.begin(), newEntries.end());
    std::sort(dateEntries.begin(), dateEntries.end());
    auto lastUnique = std::unique(dateEntries.begin(), dateEntries.end());
    dateEntries.erase(lastUnique, dateEntries.end());

    for (const auto& entry : dateEntries) {
      tempFile << entry << '\n';
	  }
  }

  originalFile.close();
  tempFile.close();
  std::filesystem::rename(tempPath, logPath);
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

void showHelp(const char* progName) {
  std::cerr << progName << "{-d DD-MM-YYYY}\n";
  std::cerr <<
R"HERE(
    -h		Print help menu
    -d		Print log van een gegeven dag (DD-MM-YYYY format)
    			Default huidige datum
)HERE";
}


int main(int argc, char* argv[]) {
	const char *progName = argv[0];
	char c;
	std::string date = getCurrentDate();
	std::regex dmYPattern(R"(\d{2}-\d{2}-\d{4})");

	while((c = getopt(argc, argv, "d:h:")) != -1) {
		switch(c) {
			case 'd':
				date = optarg;
				if (!std::regex_match(date, dmYPattern)) {
					std::cerr << "Datum format matched niet";
					return 1;
				}
				break;
			case '?':
				std::cerr << "Onbekende optie\n\n";
			case 'h':
			default:
				showHelp(progName);
				return 1;
		}
	}

	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string calendarPath = execPath.string() + "/calendar.ics";

	std::cerr << calendarPath << ' ' << date << '\n';

	/*
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
  	
  	perror("Error executing curl");
  	_exit(127);
  }
  else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    parse(calendarPath);
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
  */

  std::vector<Event> parsedEvents = parse(calendarPath, date);
  //insertEventsIntoLog(execPath.string() + "/log.txt", parsedEvents, date);

  return 0;
}