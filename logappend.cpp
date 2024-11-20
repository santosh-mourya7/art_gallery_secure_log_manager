#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>    // for getenv
#include <chrono>     // for getting current time
#include <iomanip>    // for std::put_time
#include <sstream>    // for std::ostringstream
#include <ctime>      // for std::localtime
#include <stdexcept>  // for exception handling

struct LogEntry {
    std::time_t timestamp;  // Use std::time_t for timestamp
    std::string action;     // "enter" or "leave"
    std::string person;
    int room;

    // Convert timestamp to "YYYY-MM-DD HH:MM:SS" format
    std::string getFormattedTimestamp() const {
        std::tm *localTime = std::localtime(&timestamp);
        std::ostringstream oss;
        oss << std::put_time(localTime, "%Y-%m-%d_%H:%M:%S");
        return oss.str();
    }

    std::string toString() const {
        return getFormattedTimestamp() + " " + action + " " + person + " " + std::to_string(room);
    }
};

// Get token from environment variable for better security
std::string getEnvVar(const std::string &key) {
    const char *val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

bool authenticate(const std::string &token) {
    std::string secureToken = getEnvVar("SECURE_TOKEN");
    return token == secureToken;
}

bool isValidRoomId(const std::string &roomId) {
    if (roomId.empty() || roomId.length() > 10) return false;
    if (roomId[0] == '-') return false;  // Ensure non-negative IDs
    return std::all_of(roomId.begin(), roomId.end(), ::isdigit);
}

bool isValidName(const std::string &name) {
    return !name.empty() && std::all_of(name.begin(), name.end(), ::isalpha) && name.length() <= 50;
}

bool readLog(std::vector<LogEntry> &entries, const std::string &logFile) {
    std::ifstream file(logFile);
    if (!file) return false;

    LogEntry entry;
    while (file >> entry.timestamp >> entry.action >> entry.person >> entry.room) {
        entries.push_back(entry);
    }
    return true;
}

bool appendLog(const std::string &logFile, const LogEntry &entry) {
    std::ofstream file(logFile, std::ios::app);  // Open file in append mode
    if (!file) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        return false;
    }

    file << entry.toString() << std::endl;
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: logappend -K <token> (-E <employee-name> | -G <guest-name>) (-A | -L) [-R <room-id>] <log>" << std::endl;
        return 1;  // Return 1 for general errors
    }

    std::string logFile;
    std::string token;
    std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string personName;
    int room = -1;  // Default for gallery
    std::string action;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        try {
            if (arg == "-K") {
                if (i + 1 < argc) {
                    token = argv[++i];
                }
            } else if (arg == "-E" || arg == "-G") {
                if (i + 1 < argc) {
                    personName = argv[++i];
                    if (!isValidName(personName)) {
                        std::cerr << "Invalid name" << std::endl;
                        return 1;
                    }
                }
            } else if (arg == "-A") {
                action = "enter";
            } else if (arg == "-L") {
                action = "leave";
            } else if (arg == "-R") {
                if (i + 1 < argc) {
                    std::string roomId = argv[++i];
                    if (!isValidRoomId(roomId)) {
                        std::cerr << "Invalid room id" << std::endl;
                        return 1;
                    }
                    room = std::stoi(roomId);
                }
            } else {
                logFile = arg;
            }
        } catch (const std::exception &e) {
            std::cerr << "Invalid argument format" << std::endl;
            return 1;
        }
    }

    if (!authenticate(token)) {
        std::cerr << "Authentication failed" << std::endl;
        return 2;  // Return 2 for authentication failure
    }

    std::vector<LogEntry> entries;
    bool logExists = readLog(entries, logFile);
    if (!logExists) {
        // If log file doesn't exist, we create it and add the first log entry
        if (!appendLog(logFile, {timestamp, action, personName, room})) {
            std::cerr << "Log file creation failed" << std::endl;
            return 1;
        }
        std::cout << "Log created and entry added." << std::endl;
        return 0;
    }

    LogEntry newEntry = {timestamp, action, personName, room};

    if (!appendLog(logFile, newEntry)) {
        std::cerr << "Failed to append log entry" << std::endl;
        return 1;
    }

    std::cout << "Log entry added." << std::endl;
    return 0;
    
}
