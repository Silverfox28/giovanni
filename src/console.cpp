#include "console.h"

#include <iostream>

#include "textutil.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace console {

void setupUtf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void title(const std::string& text) {
    std::cout << "\n=== " << text << " ===\n\n";
}

void pause() {
    readLine("\nPremi Invio per continuare...");
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed{};
    return text::trim(line);
}

std::string readText(const std::string& label, const std::string& current) {
    const std::string shown = current.empty() ? "" : " [" + current + "]";
    const std::string input = readLine(label + shown + ": ");
    if (input.empty()) return current;
    if (input == "-") return "";
    return input;
}

std::string readRequiredText(const std::string& label, const std::string& current) {
    while (true) {
        const std::string value = readText(label, current);
        if (!value.empty()) return value;
        std::cout << "  Questo campo è obbligatorio.\n";
    }
}

int readInt(const std::string& label, int min, int max) {
    while (true) {
        const auto n = text::parseInt(readLine(label + ": "));
        if (n && *n >= min && *n <= max) return *n;
        std::cout << "  Inserisci un numero intero tra " << min << " e " << max << ".\n";
    }
}

int readIntDefault(const std::string& label, int current, int min, int max) {
    while (true) {
        const std::string input = readLine(label + " [" + std::to_string(current) + "]: ");
        if (input.empty()) return current;
        const auto n = text::parseInt(input);
        if (n && *n >= min && *n <= max) return *n;
        std::cout << "  Inserisci un numero intero tra " << min << " e " << max << ".\n";
    }
}

double readDecimalDefault(const std::string& label, double current, double min) {
    while (true) {
        const std::string input = readLine(label + " [" + text::formatDecimalTrim(current, 4) + "]: ");
        if (input.empty()) return current;
        const auto v = text::parseDecimal(input);
        if (v && *v >= min) return *v;
        std::cout << "  Inserisci un numero (es. 0,15) maggiore o uguale a " << text::formatDecimalTrim(min, 4)
                  << ".\n";
    }
}

bool confirm(const std::string& question, bool defaultYes) {
    while (true) {
        const std::string answer = text::toLower(readLine(question + (defaultYes ? " (S/n): " : " (s/N): ")));
        if (answer.empty()) return defaultYes;
        if (answer == "s" || answer == "si" || answer == "sì" || answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        std::cout << "  Rispondi s (sì) oppure n (no).\n";
    }
}

std::optional<Category> chooseCategory(std::optional<Category> current, bool allowAll) {
    const auto& categories = allCategories();
    const int count = static_cast<int>(categories.size());

    std::cout << "Categorie:\n";
    if (allowAll) std::cout << "   0) Tutte\n";
    const int rows = (count + 1) / 2;
    for (int r = 0; r < rows; ++r) {
        for (int col = 0; col < 2; ++col) {
            const int i = r + col * rows;
            if (i >= count) continue;
            std::string item = (i + 1 < 10 ? "   " : "  ") + std::to_string(i + 1) + ") " + categoryName(categories[i]);
            std::cout << (col == 0 ? text::padRight(item, 30) : item);
        }
        std::cout << '\n';
    }

    int defaultChoice = -1;
    if (current) {
        for (int i = 0; i < count; ++i) {
            if (categories[i] == *current) defaultChoice = i + 1;
        }
    } else if (allowAll) {
        defaultChoice = 0;
    }

    const int min = allowAll ? 0 : 1;
    const int choice = defaultChoice >= 0 ? readIntDefault("Scelta", defaultChoice, min, count)
                                          : readInt("Scelta", min, count);
    if (choice == 0) return std::nullopt;
    return categories[choice - 1];
}

}  // namespace console
