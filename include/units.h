// Lettura e scrittura di valori elettrici in notazione ingegneristica.
//
// Esempi riconosciuti da parseValue:
//   "4k7" -> 4700        "4,7k" -> 4700      "220R" -> 220      "4R7" -> 4,7
//   "100n" -> 1e-7       "100nF" -> 1e-7     "0.1uF" -> 1e-7    "2M2" -> 2,2e6
//   "10 kΩ" -> 10000     "1mH" -> 0,001      "4µ7" -> 4,7e-6
//
// Prefissi: p (1e-12), n (1e-9), u/µ (1e-6), m (1e-3), R (1), k/K (1e3), M (1e6), G (1e9).
// Attenzione: "m" minuscola = milli, "M" maiuscola = mega.
#pragma once

#include <optional>
#include <string>

namespace units {

// Interpreta un valore come "4k7" o "100nF". Restituisce nullopt se non è un valore numerico.
std::optional<double> parseValue(const std::string& text);

// Formatta un valore con il prefisso SI più adatto e 3 cifre significative
// (es. 4700, "Ω" -> "4,7 kΩ"; 1e-7, "F" -> "100 nF").
std::string formatValue(double value, const std::string& unit);

// Confronto tra due valori con tolleranza relativa (per gli errori di arrotondamento).
bool approxEqual(double a, double b);

}  // namespace units
