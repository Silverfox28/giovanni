// Lettura e scrittura di righe CSV (con campi tra virgolette quando serve).
#pragma once

#include <string>
#include <vector>

namespace csv {

std::string escapeField(const std::string& field, char separator);
std::string joinLine(const std::vector<std::string>& fields, char separator);
std::vector<std::string> parseLine(const std::string& line, char separator);

}  // namespace csv
