// Archivio dei componenti: inserimento, ricerca, statistiche e salvataggio su file CSV.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "component.h"

// Criteri di ricerca: quelli non impostati non vengono applicati.
struct SearchFilter {
    std::string query;                 // parole da cercare (tutte devono comparire)
    std::optional<Category> category;
    std::optional<double> minValue;    // valore minimo (es. 1000 per 1 kΩ)
    std::optional<double> maxValue;
    bool onlyLowStock = false;

    bool matches(const Component& c) const;
};

enum class SortKey {
    CategoryAndValue,
    Name,
    Quantity,
    Location,
    Id,
};

struct CategoryStats {
    Category category = Category::Other;
    int items = 0;
    long long pieces = 0;
    double value = 0.0;
};

struct Statistics {
    int items = 0;
    long long pieces = 0;
    double value = 0.0;
    int lowStock = 0;
    int withoutPrice = 0;
    int withoutLocation = 0;
    std::vector<CategoryStats> byCategory;  // solo le categorie presenti
};

void sortComponents(std::vector<const Component*>& list, SortKey key);

class Inventory {
public:
    static constexpr char kSeparator = ';';

    // Aggiunge un componente assegnandogli un nuovo ID; restituisce il componente inserito.
    Component& add(Component component);
    bool remove(int id);

    Component* findById(int id);
    const Component* findById(int id) const;

    // Cerca un componente "uguale" (stessa categoria, sigla, valore e package).
    const Component* findDuplicate(const Component& component) const;

    std::vector<const Component*> all() const;
    std::vector<const Component*> search(const SearchFilter& filter) const;
    std::vector<const Component*> lowStock() const;

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    Statistics statistics() const;

    // Carica i dati dal file. Se il file non esiste l'archivio resta vuoto.
    // Restituisce gli avvisi sulle righe problematiche; lancia std::runtime_error se il file è illeggibile.
    std::vector<std::string> load(const std::string& path);

    // Salva in modo sicuro (prima su un file temporaneo, poi lo sostituisce).
    void save(const std::string& path) const;

private:
    std::vector<Component> items_;
    int nextId_ = 1;
};
