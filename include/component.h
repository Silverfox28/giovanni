// Definizione di un componente elettronico e delle sue categorie.
#pragma once

#include <optional>
#include <string>
#include <vector>

enum class Category {
    Resistor,
    Capacitor,
    Inductor,
    Diode,
    Led,
    Transistor,
    IntegratedCircuit,
    Connector,
    Electromechanical,  // interruttori, pulsanti, relè, buzzer...
    Sensor,
    Module,             // moduli e schede (Arduino, ESP32, driver...)
    Other,
};

const std::vector<Category>& allCategories();

// Nome in italiano (es. "Resistore"), usato anche nel file CSV.
std::string categoryName(Category category);

// Riconosce il nome di una categoria (senza distinguere maiuscole e minuscole).
std::optional<Category> categoryFromName(const std::string& name);

// Unità di misura del valore principale: "Ω", "F", "H" oppure "" se non applicabile.
std::string categoryUnit(Category category);

// Esempio di come inserire il valore per quella categoria.
std::string categoryValueHint(Category category);

struct Component {
    int id = 0;
    Category category = Category::Other;
    std::string name;        // sigla o descrizione (es. "BC547", "Film metallico 1/4 W")
    std::string value;       // valore così come inserito (es. "4k7", "100nF")
    std::string package;     // contenitore (es. "THT", "0805", "TO-92", "DIP-8")
    int quantity = 0;
    int minQuantity = 0;     // scorta minima: sotto questa soglia il componente va riordinato
    std::string location;    // dove si trova (es. "Cassettiera A, cassetto 3")
    std::string supplier;
    double unitPrice = 0.0;  // in euro
    std::string notes;

    // Valore numerico (in unità SI) se il campo "valore" è interpretabile, es. "4k7" -> 4700.
    std::optional<double> numericValue() const;

    // Valore da mostrare: formattato con l'unità (es. "4,7 kΩ") se possibile, altrimenti come inserito.
    std::string displayValue() const;

    // Vero se la quantità è pari o inferiore alla scorta minima (o se il componente è esaurito).
    bool isLowStock() const { return quantity <= minQuantity; }

    double stockValue() const { return quantity * unitPrice; }
};
