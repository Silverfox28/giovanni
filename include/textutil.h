// Funzioni di utilità per la gestione del testo e dei numeri in formato italiano.
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace text {

// Rimuove spazi e caratteri di controllo all'inizio e alla fine.
std::string trim(const std::string& s);

// Converte in minuscolo (solo caratteri ASCII; le lettere accentate restano invariate).
std::string toLower(std::string s);

bool equalsIgnoreCase(const std::string& a, const std::string& b);
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);

// Divide una stringa sugli spazi, scartando i pezzi vuoti.
std::vector<std::string> splitWords(const std::string& s);

// Larghezza "visiva" di una stringa UTF-8 (numero di caratteri, non di byte).
std::size_t displayWidth(const std::string& s);

// Accorcia la stringa a 'width' caratteri, terminandola con "…" se necessario.
std::string truncate(const std::string& s, std::size_t width);

std::string padRight(const std::string& s, std::size_t width);
std::string padLeft(const std::string& s, std::size_t width);

// Numero con la virgola come separatore decimale (es. 3,14).
std::string formatDecimal(double value, int decimals);

// Come formatDecimal, ma senza zeri finali inutili (es. 2,50 -> 2,5; 3,00 -> 3).
std::string formatDecimalTrim(double value, int maxDecimals);

// Importo in euro (es. "12,50 €").
std::string formatMoney(double value);

// Legge un numero decimale accettando sia la virgola sia il punto
// (ed eventualmente un simbolo "€" finale).
std::optional<double> parseDecimal(const std::string& s);

// Legge un numero intero; restituisce nullopt se il testo non è un intero valido.
std::optional<int> parseInt(const std::string& s);

}  // namespace text
