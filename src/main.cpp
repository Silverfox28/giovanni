// Catalogo dei componenti elettronici: punto di ingresso del programma.
//
// Uso:  catalogo [file.csv]
// Se il file non viene indicato si usa "componenti.csv" nella cartella corrente.

#include <iostream>
#include <string>

#include "app.h"
#include "console.h"

int main(int argc, char* argv[]) {
    console::setupUtf8();

    std::string path = "componenti.csv";
    if (argc > 1) {
        const std::string arg = argv[1];
        if (arg == "-h" || arg == "--help" || arg == "/?") {
            std::cout << "Catalogo componenti elettronici\n\n"
                      << "Uso: " << argv[0] << " [file.csv]\n\n"
                      << "I dati vengono letti e salvati automaticamente nel file indicato\n"
                      << "(predefinito: componenti.csv nella cartella corrente).\n";
            return 0;
        }
        path = arg;
    }

    App app(path);
    return app.run();
}
