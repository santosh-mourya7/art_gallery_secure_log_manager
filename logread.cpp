#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <iomanip>

struct LogEntry {
    std::time_t timestamp;
    std::string action;
    std::string person;
    int room;

    LogEntry(std::time_t t, const std::string &a, const std::string &p, int r)
        : timestamp(t), action(a), person(p), room(r) {}
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

bool readLog(std::vector<LogEntry> &entries, const std::string &logFile) {
    std::ifstream file(logFile);
    if (!file) {
        std::cerr << "Log file does not exist or cannot be opened: " << logFile << std::endl;
        return false;
    }

    std::string timestampStr, action, person;
    int room;
    std::time_t timestamp;

    while (file >> timestampStr >> action >> person >> room) {
        // Modify the timestamp parsing to account for the format "YYYY-MM-DD_HH:MM:SS"
        std::tm tm = {};
        std::istringstream ss(timestampStr);

        // Parse the timestamp string (including the underscore between date and time)
        ss >> std::get_time(&tm, "%Y-%m-%d_%H:%M:%S");

        if (ss.fail()) {
            std::cerr << "Failed to parse timestamp: " << timestampStr << std::endl;
            continue;  // Skip invalid log entries
        }

        // Convert the tm struct to std::time_t
        timestamp = std::mktime(&tm);

        // Ensure proper format (action should be "enter" or "leave")
        if (action != "enter" && action != "leave") {
            std::cerr << "Invalid action in log entry: " << action << std::endl;
            continue;  // Skip invalid entries
        }

        entries.emplace_back(timestamp, action, person, room);
    }
    return true;
}

void printCurrentState(const std::vector<LogEntry> &entries) {
    std::set<std::string> employeesInGallery;
    std::set<std::string> guestsInGallery;
    std::map<int, std::set<std::string>> rooms;

    for (const auto &entry : entries) {
        if (entry.action == "enter") {
            if (entry.room == -1) {  // Assume employees are in the gallery (room -1)
                employeesInGallery.insert(entry.person);
            } else {  // Guests are assumed to be in rooms with numbers other than -1
                rooms[entry.room].insert(entry.person);
            }
        } else if (entry.action == "leave") {
            if (entry.room == -1) {
                employeesInGallery.erase(entry.person);
            } else {
                rooms[entry.room].erase(entry.person);
            }
        }
    }

    // Print Employees in the gallery
    std::cout << "Employees in gallery: ";
    if (employeesInGallery.empty()) {
        std::cout << "none\n";
    } else {
        for (auto it = employeesInGallery.begin(); it != employeesInGallery.end(); ++it) {
            std::cout << *it;
            if (std::next(it) != employeesInGallery.end()) std::cout << ", ";
        }
        std::cout << "\n";
    }

    // Print Guests in the gallery (room -1)
    std::cout << "Guests in gallery: ";
    if (guestsInGallery.empty()) {
        std::cout << "none\n";
    } else {
        for (auto it = guestsInGallery.begin(); it != guestsInGallery.end(); ++it) {
            std::cout << *it;
            if (std::next(it) != guestsInGallery.end()) std::cout << ", ";
        }
        std::cout << "\n";
    }

    // Print persons in each room (other than the gallery)
    for (const auto &room : rooms) {
        std::cout << "Room " << room.first << ": ";
        if (room.second.empty()) {
            std::cout << "none\n";
        } else {
            for (auto it = room.second.begin(); it != room.second.end(); ++it) {
                std::cout << *it;
                if (std::next(it) != room.second.end()) std::cout << ", ";
            }
            std::cout << "\n";
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4 || std::string(argv[1]) != "-K") {
        std::cerr << "Usage: logread -K <token> <log>" << std::endl;
        return 1;
    }

    std::string token = argv[2];
    std::string logFile = argv[3];

    if (!authenticate(token)) {
        std::cerr << "Authentication failed: invalid token" << std::endl;
        return 2;
    }

    std::vector<LogEntry> entries;
    if (!readLog(entries, logFile)) {
        std::cerr << "Failed to read log file: " << logFile << std::endl;
        return 1;
    }

    printCurrentState(entries);
    return 0;
    
}
