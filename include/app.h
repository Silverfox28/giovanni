// Interfaccia a menu del catalogo.
#pragma once

#include <string>
#include <vector>

#include "inventory.h"

class App {
public:
    explicit App(std::string dataPath);

    // Avvia il programma; restituisce il codice di uscita.
    int run();

private:
    Inventory inventory_;
    std::string dataPath_;

    void mainMenu();
    bool persist();

    void listComponents();
    void searchComponents();
    void showComponent();
    void addComponent();
    void editComponent();
    void adjustQuantity();
    void deleteComponent();
    void lowStockReport();
    void showStatistics();
    void colorCodeTool();

    void decodeColors();
    void encodeColors();
    void showResistorsWithValue(double ohms);

    // Chiede i campi di un componente (usato sia per l'inserimento sia per la modifica).
    void fillFields(Component& c);

    // Chiede all'utente quale componente usare (per ID oppure cercandolo per testo).
    Component* askComponent();

    void printTable(const std::vector<const Component*>& list) const;
    void printDetails(const Component& c) const;
};
