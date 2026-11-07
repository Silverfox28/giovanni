#include "units.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <iterator>
#include <locale>
#include <sstream>

#include "textutil.h"

namespace units {

namespace {

void replaceAll(std::string& s, const std::string& from, const std::string& to) {
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

bool endsWithIgnoreCase(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return text::equalsIgnoreCase(s.substr(s.size() - suffix.size()), suffix);
}

std::optional<double> prefixMultiplier(char prefix) {
    switch (prefix) {
        case 'p': return 1e-12;
        case 'n': return 1e-9;
        case 'u': return 1e-6;
        case 'm': return 1e-3;
        case 'R':
        case 'r': return 1.0;
        case 'k':
        case 'K': return 1e3;
        case 'M': return 1e6;
        case 'G': return 1e9;
        default: return std::nullopt;
    }
}

// Arrotonda a un certo numero di cifre significative.
double roundSignificant(double x, int digits) {
    if (x == 0.0) return 0.0;
    const int magnitude = static_cast<int>(std::ceil(std::log10(std::fabs(x))));
    const double scale = std::pow(10.0, digits - magnitude);
    return std::round(x * scale) / scale;
}

}  // namespace

std::optional<double> parseValue(const std::string& input) {
    // Rimuove tutti gli spazi ("10 kΩ" -> "10kΩ").
    std::string s;
    for (char c : input) {
        if (!std::isspace(static_cast<unsigned char>(c))) s += c;
    }

    // Il simbolo micro può essere scritto in vari modi.
    replaceAll(s, "\xC2\xB5", "u");  // µ (MICRO SIGN)
    replaceAll(s, "\xCE\xBC", "u");  // μ (lettera greca mu)

    // Toglie l'eventuale unità di misura finale.
    static const char* const kUnitSuffixes[] = {
        "\xE2\x84\xA6",  // Ω (OHM SIGN)
        "\xCE\xA9",      // Ω (lettera greca omega)
        "ohm", "hz", "f", "h", "v", "a", "w",
    };
    for (const char* suffix : kUnitSuffixes) {
        if (endsWithIgnoreCase(s, suffix)) {
            s.erase(s.size() - std::strlen(suffix));
            break;
        }
    }
    if (s.empty()) return std::nullopt;

    // Parte numerica iniziale (accetta virgola o punto come separatore decimale).
    std::size_t i = 0;
    std::string number;
    bool seenDecimalPoint = false;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == ',')) {
        if (s[i] == '.' || s[i] == ',') {
            if (seenDecimalPoint) return std::nullopt;
            seenDecimalPoint = true;
            number += '.';
        } else {
            number += s[i];
        }
        ++i;
    }
    if (number.empty() || number == ".") return std::nullopt;

    double multiplier = 1.0;
    if (i < s.size()) {
        const auto m = prefixMultiplier(s[i]);
        if (!m) return std::nullopt;
        multiplier = *m;
        ++i;

        // Notazione "4k7": le cifre dopo il prefisso sono la parte decimale.
        std::string fraction;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) fraction += s[i++];
        if (i != s.size()) return std::nullopt;
        if (!fraction.empty()) {
            if (seenDecimalPoint) return std::nullopt;  // es. "4.7k7" non ha senso
            number += "." + fraction;
        }
    }

    std::istringstream iss(number);
    iss.imbue(std::locale::classic());
    double value = 0.0;
    if (!(iss >> value)) return std::nullopt;
    return value * multiplier;
}

std::string formatValue(double value, const std::string& unit) {
    const std::string suffix = unit.empty() ? "" : " " + unit;
    if (value == 0.0 || !std::isfinite(value)) return text::formatDecimalTrim(value, 0) + suffix;

    struct Prefix {
        double factor;
        const char* symbol;
    };
    static const Prefix kPrefixes[] = {
        {1e9, "G"}, {1e6, "M"}, {1e3, "k"}, {1.0, ""},
        {1e-3, "m"}, {1e-6, "µ"}, {1e-9, "n"}, {1e-12, "p"},
    };

    const double rounded = roundSignificant(value, 3);
    const double magnitude = std::fabs(rounded);

    const Prefix* chosen = &kPrefixes[std::size(kPrefixes) - 1];
    for (const Prefix& p : kPrefixes) {
        if (magnitude >= p.factor * 0.999999) {
            chosen = &p;
            break;
        }
    }
    // Per i condensatori si usa "1000 µF" e non "1 mF", come scritto sui componenti.
    if (unit == "F" && chosen->factor == 1e-3 && magnitude < 0.1) chosen = &kPrefixes[5];

    const double mantissa = rounded / chosen->factor;
    const double absMantissa = std::fabs(mantissa);
    const int decimals = absMantissa >= 100 ? 0 : absMantissa >= 10 ? 1 : absMantissa >= 1 ? 2 : 4;
    const std::string sep = unit.empty() && chosen->symbol[0] == '\0' ? "" : " ";
    return text::formatDecimalTrim(mantissa, decimals) + sep + chosen->symbol + unit;
}

bool approxEqual(double a, double b) {
    if (a == b) return true;
    return std::fabs(a - b) <= 1e-9 * std::fmax(std::fabs(a), std::fabs(b));
}

}  // namespace units
