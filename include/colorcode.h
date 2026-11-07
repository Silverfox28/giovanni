// Codice colori dei resistori (IEC 60062), con i nomi dei colori in italiano.
//
//   nero 0 · marrone 1 · rosso 2 · arancione 3 · giallo 4 · verde 5
//   blu 6 · viola 7 · grigio 8 · bianco 9 · oro ×0,1 · argento ×0,01
//
// Resistore a 4 bande: cifra, cifra, moltiplicatore, tolleranza.
// Resistore a 5 bande: cifra, cifra, cifra, moltiplicatore, tolleranza.
// Resistore a 3 bande: cifra, cifra, moltiplicatore (tolleranza ±20%).
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace colorcode {

struct Decoded {
    double ohms = 0.0;
    std::string tolerance;  // es. "±5%"
};

struct Encoded {
    std::vector<std::string> bands;  // cifre + moltiplicatore (senza la banda di tolleranza)
    double ohms = 0.0;               // valore effettivamente rappresentato (eventualmente arrotondato)
};

// Nomi dei colori riconosciuti, in ordine.
std::vector<std::string> colorNames();

// Divide un testo come "giallo viola rosso oro" (anche con virgole o trattini) nelle singole bande.
std::vector<std::string> splitBands(const std::string& input);

// Da colori a valore. In caso di errore restituisce nullopt e descrive il problema in 'error'.
std::optional<Decoded> decode(const std::vector<std::string>& bands, std::string& error);

// Da valore a colori, con 2 cifre significative (resistori a 4 bande) o 3 (a 5 bande).
std::optional<Encoded> encode(double ohms, int significantDigits, std::string& error);

}  // namespace colorcode
