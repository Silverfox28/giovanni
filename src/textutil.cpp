#include "textutil.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace text {

namespace {

bool isUtf8LeadByte(char c) {
    return (static_cast<unsigned char>(c) & 0xC0) != 0x80;
}

}  // namespace

std::string trim(const std::string& s) {
    const char* whitespace = " \t\r\n\f\v";
    const auto start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(c < 128 ? std::tolower(c) : c);
    });
    return s;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return a.size() == b.size() && toLower(a) == toLower(b);
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

std::vector<std::string> splitWords(const std::string& s) {
    std::vector<std::string> words;
    std::istringstream iss(s);
    std::string word;
    while (iss >> word) words.push_back(word);
    return words;
}

std::size_t displayWidth(const std::string& s) {
    return static_cast<std::size_t>(std::count_if(s.begin(), s.end(), isUtf8LeadByte));
}

std::string truncate(const std::string& s, std::size_t width) {
    if (displayWidth(s) <= width) return s;
    if (width == 0) return "";

    std::string out;
    std::size_t chars = 0;
    for (char c : s) {
        if (isUtf8LeadByte(c)) {
            if (chars == width - 1) break;
            ++chars;
        }
        out += c;
    }
    return out + "…";
}

std::string padRight(const std::string& s, std::size_t width) {
    const std::size_t w = displayWidth(s);
    return w >= width ? s : s + std::string(width - w, ' ');
}

std::string padLeft(const std::string& s, std::size_t width) {
    const std::size_t w = displayWidth(s);
    return w >= width ? s : std::string(width - w, ' ') + s;
}

std::string formatDecimal(double value, int decimals) {
    // Evita di stampare "-0,00" per valori negativi piccolissimi.
    if (std::fabs(value) < 0.5 * std::pow(10.0, -decimals)) value = 0.0;

    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::fixed << std::setprecision(decimals) << value;
    std::string result = oss.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

std::string formatDecimalTrim(double value, int maxDecimals) {
    std::string result = formatDecimal(value, maxDecimals);
    if (result.find(',') != std::string::npos) {
        while (!result.empty() && result.back() == '0') result.pop_back();
        if (!result.empty() && result.back() == ',') result.pop_back();
    }
    return result;
}

std::string formatMoney(double value) {
    return formatDecimal(value, 2) + " €";
}

std::optional<double> parseDecimal(const std::string& input) {
    std::string s = trim(input);
    const std::string euro = "€";
    if (s.size() >= euro.size() && s.compare(s.size() - euro.size(), euro.size(), euro) == 0) {
        s = trim(s.substr(0, s.size() - euro.size()));
    }
    if (s.empty()) return std::nullopt;
    std::replace(s.begin(), s.end(), ',', '.');

    std::istringstream iss(s);
    iss.imbue(std::locale::classic());
    double value = 0.0;
    if (!(iss >> value)) return std::nullopt;
    iss >> std::ws;
    if (!iss.eof() || !std::isfinite(value)) return std::nullopt;
    return value;
}

std::optional<int> parseInt(const std::string& input) {
    std::string s = trim(input);
    if (s.size() > 1 && s[0] == '+' && std::isdigit(static_cast<unsigned char>(s[1]))) s.erase(0, 1);
    if (s.empty()) return std::nullopt;

    int value = 0;
    const char* begin = s.data();
    const char* end = s.data() + s.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc() || ptr != end) return std::nullopt;
    return value;
}

}  // namespace text
