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

struct Event {
	std::string title;
	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;
	std::string timeZone;
};

bool isSameDate(const std::string& logEntry, const std::string& targetDate) {
  return logEntry.find("[" + targetDate + "]") == 0;
}

std::string parseTimeZone(
	const std::string &tz) 
{
	size_t tzPos = tz.find("TZID=");
	if (tzPos != std::string::npos) {
		return tz.substr(5, tz.length());
	}

	return "UTC";
}

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
  } else {
    try {
      const std::chrono::time_zone* timeZone = std::chrono::locate_zone(tz);
      std::chrono::local_seconds lt{std::chrono::seconds(std::mktime(&tm))};
      return timeZone->to_sys(lt);
    } catch (const std::runtime_error& e) {
      std::cerr << "Warning: Unknown timezone " << tz << ", defaulting to UTC\n";
      return std::chrono::sys_seconds(std::chrono::seconds(std::mktime(&tm)));
    }
  }
}

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


void printEvents(const std::vector<Event> events) {
  for (const auto& event : events) {
    std::time_t startTime = std::chrono::system_clock::to_time_t(event.start);
    std::time_t endTime = std::chrono::system_clock::to_time_t(event.end);
    std::cout << "Event:\t" << event.title << '\n'
              << "Start:\t" << std::ctime(&startTime)
              << "End:\t" << std::ctime(&endTime) << '\n';
  }
}

std::vector<Event> parse(
	const std::string& calendarPath,
	const std::string date) 
{
	std::ifstream calendarFile(calendarPath);
	if (!calendarFile.is_open()) {
		perror("Failed to open calendar file");
		return std::vector<Event>();
	}

	std::ostringstream oss;
	oss << calendarFile.rdbuf();
	std::string calendarData = oss.str();

	std::regex reg(R"(BEGIN:VEVENT[\s\S]*?(SUMMARY:(.*))[\s\S]*?(DTSTART;?(.*):([\dTZ]+))[\s\S]*?(DTEND;?(.*)?:([\dTZ]*))[\s\S]*?END:VEVENT)", std::regex::icase);

	std::vector<Event> events;
  auto matchesBegin = std::sregex_iterator(calendarData.begin(), calendarData.end(), reg);
  auto matchesEnd = std::sregex_iterator();

  for (std::sregex_iterator it = matchesBegin; it != matchesEnd; ++it) {
    std::smatch match = *it;
    
    std::string timeZone = parseTimeZone(match[4].str());
    std::chrono::system_clock::time_point start = parseIcalDateTime(match[5].str(), timeZone);
    std::chrono::system_clock::time_point end = parseIcalDateTime(match[8].str(), timeZone);

    std::string startDate = formatToDateYmd(start);

    if (date == date && startDate == formatToDateYmd(end)) {
	    Event event;

	    event.title = match[2].str();
	    event.timeZone = timeZone;
	    event.start = start;
	    event.end = end;
	    events.push_back(event);
    }
  }

  printEvents(events);

  return events;
}

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
    if (isSameDate(line, targetDate)) {
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

    for (const auto& entry : dateEntries) {
    	std::cerr << entry << '\n';
      tempFile << entry << '\n';
	  }
  }

  originalFile.close();
  tempFile.close();
  std::filesystem::rename(tempPath, logPath);
}

int main(int argc, char* argv[]) {
	auto execPath = std::filesystem::canonical("/proc/self/exe").parent_path();
	std::string calendarPath = execPath.string() + "/calendar.ics";

	std::cerr << calendarPath << '\n';

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

	//Current day for now as, later add support for opt arguments and default current day
  std::vector<Event> parsedEvents = parse(calendarPath, "15-03-2025");
  insertEventsIntoLog(execPath.string() + "/log.txt", parsedEvents, "15-03-2025");

  return 0;
}