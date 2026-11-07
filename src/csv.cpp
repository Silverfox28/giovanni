#include "csv.h"

namespace csv {

std::string escapeField(const std::string& rawField, char separator) {
    // I ritorni a capo non sono ammessi all'interno di un campo: li sostituiamo con spazi.
    std::string field = rawField;
    for (char& c : field) {
        if (c == '\n' || c == '\r') c = ' ';
    }

    const bool needsQuotes = field.find(separator) != std::string::npos ||
                             field.find('"') != std::string::npos ||
                             (!field.empty() && (field.front() == ' ' || field.back() == ' '));
    if (!needsQuotes) return field;

    std::string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

std::string joinLine(const std::vector<std::string>& fields, char separator) {
    std::string line;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) line += separator;
        line += escapeField(fields[i], separator);
    }
    return line;
}

std::vector<std::string> parseLine(const std::string& line, char separator) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == separator) {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

}  // namespace csv
