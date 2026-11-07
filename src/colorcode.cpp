#include "colorcode.h"

#include <cmath>

#include "textutil.h"

namespace colorcode {

namespace {

struct Color {
    const char* name;
    int digit;               // -1 se il colore non può rappresentare una cifra
    int exponent;            // potenza di 10 come moltiplicatore
    const char* tolerance;   // nullptr se il colore non è usato per la tolleranza
};

const Color kColors[] = {
    {"nero", 0, 0, nullptr},
    {"marrone", 1, 1, "±1%"},
    {"rosso", 2, 2, "±2%"},
    {"arancione", 3, 3, nullptr},
    {"giallo", 4, 4, nullptr},
    {"verde", 5, 5, "±0,5%"},
    {"blu", 6, 6, "±0,25%"},
    {"viola", 7, 7, "±0,1%"},
    {"grigio", 8, 8, "±0,05%"},
    {"bianco", 9, 9, nullptr},
    {"oro", -1, -1, "±5%"},
    {"argento", -1, -2, "±10%"},
};

struct Alias {
    const char* alias;
    const char* name;
};

const Alias kAliases[] = {
    {"marron", "marrone"}, {"arancio", "arancione"}, {"violetto", "viola"},
    {"azzurro", "blu"},    {"dorato", "oro"},        {"argentato", "argento"},
};

const Color* findColor(const std::string& input) {
    std::string name = text::toLower(text::trim(input));
    for (const Alias& a : kAliases) {
        if (name == a.alias) name = a.name;
    }
    for (const Color& c : kColors) {
        if (name == c.name) return &c;
    }
    return nullptr;
}

const Color* colorForExponent(int exponent) {
    for (const Color& c : kColors) {
        if (c.exponent == exponent) return &c;
    }
    return nullptr;
}

}  // namespace

std::vector<std::string> colorNames() {
    std::vector<std::string> names;
    for (const Color& c : kColors) names.emplace_back(c.name);
    return names;
}

std::vector<std::string> splitBands(const std::string& input) {
    std::string normalized = input;
    for (char& c : normalized) {
        if (c == ',' || c == '-' || c == ';' || c == '/') c = ' ';
    }
    return text::splitWords(normalized);
}

std::optional<Decoded> decode(const std::vector<std::string>& bands, std::string& error) {
    if (bands.size() < 3 || bands.size() > 5) {
        error = "servono da 3 a 5 bande (ne sono state indicate " + std::to_string(bands.size()) + ")";
        return std::nullopt;
    }

    std::vector<const Color*> colors;
    for (const std::string& band : bands) {
        const Color* c = findColor(band);
        if (!c) {
            error = "colore sconosciuto: \"" + band + "\"";
            return std::nullopt;
        }
        colors.push_back(c);
    }

    const std::size_t digitCount = bands.size() == 5 ? 3 : 2;
    long long significand = 0;
    for (std::size_t i = 0; i < digitCount; ++i) {
        if (colors[i]->digit < 0) {
            error = "il colore \"" + std::string(colors[i]->name) + "\" non può essere una cifra (banda " +
                    std::to_string(i + 1) + ")";
            return std::nullopt;
        }
        significand = significand * 10 + colors[i]->digit;
    }

    Decoded result;
    result.ohms = static_cast<double>(significand) * std::pow(10.0, colors[digitCount]->exponent);

    if (bands.size() == 3) {
        result.tolerance = "±20%";
    } else {
        const Color* tol = colors.back();
        if (!tol->tolerance) {
            error = "il colore \"" + std::string(tol->name) + "\" non indica una tolleranza";
            return std::nullopt;
        }
        result.tolerance = tol->tolerance;
    }
    return result;
}

std::optional<Encoded> encode(double ohms, int significantDigits, std::string& error) {
    if (significantDigits != 2 && significantDigits != 3) {
        error = "le cifre significative devono essere 2 o 3";
        return std::nullopt;
    }
    if (!(ohms > 0.0) || !std::isfinite(ohms)) {
        error = "il valore deve essere maggiore di zero";
        return std::nullopt;
    }

    const long long lowest = significantDigits == 2 ? 10 : 100;  // più piccolo numero con N cifre
    const long long highest = lowest * 10;                       // primo numero con N+1 cifre

    int exponent = static_cast<int>(std::floor(std::log10(ohms))) - significantDigits + 1;
    long long significand = std::llround(ohms / std::pow(10.0, exponent));
    // Corregge eventuali errori di arrotondamento del logaritmo.
    if (significand >= highest) {
        significand = std::llround(static_cast<double>(significand) / 10.0);
        ++exponent;
    } else if (significand < lowest) {
        --exponent;
        significand = std::llround(ohms / std::pow(10.0, exponent));
    }

    const Color* multiplier = colorForExponent(exponent);
    if (!multiplier) {
        error = "valore fuori dall'intervallo rappresentabile con il codice colori";
        return std::nullopt;
    }

    Encoded result;
    const std::string digits = std::to_string(significand);
    for (char d : digits) result.bands.emplace_back(kColors[d - '0'].name);
    result.bands.emplace_back(multiplier->name);
    result.ohms = static_cast<double>(significand) * std::pow(10.0, exponent);
    return result;
}

}  // namespace colorcode
