// Funzioni per dialogare con l'utente dal terminale.
#pragma once

#include <exception>
#include <optional>
#include <string>

#include "component.h"

namespace console {

// Lanciata quando l'input termina (es. Ctrl+D / Ctrl+Z): il programma si chiude in modo ordinato.
struct InputClosed : std::exception {
    const char* what() const noexcept override { return "input terminato"; }
};

// Su Windows abilita l'UTF-8 nella console (per lettere accentate, Ω, µ, €).
void setupUtf8();

void title(const std::string& text);
void pause();

// Legge una riga (già ripulita dagli spazi iniziali e finali).
std::string readLine(const std::string& prompt);

// Chiede un testo mostrando il valore attuale tra parentesi quadre:
// Invio lo mantiene, "-" lo cancella.
std::string readText(const std::string& label, const std::string& current);

// Testo obbligatorio (non può restare vuoto).
std::string readRequiredText(const std::string& label, const std::string& current);

int readInt(const std::string& label, int min, int max);
int readIntDefault(const std::string& label, int current, int min, int max);
double readDecimalDefault(const std::string& label, double current, double min);

bool confirm(const std::string& question, bool defaultYes);

// Mostra l'elenco delle categorie e ne fa scegliere una.
// Con allowAll = true si può rispondere 0 ("tutte") e si ottiene nullopt.
std::optional<Category> chooseCategory(std::optional<Category> current, bool allowAll);

}  // namespace console
