#include "inventory.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>

#include "csv.h"
#include "textutil.h"
#include "units.h"

namespace {

const char* const kUtf8Bom = "\xEF\xBB\xBF";

const std::vector<std::string> kHeader = {
    "id", "categoria", "nome", "valore", "package", "quantita", "scorta_minima",
    "posizione", "fornitore", "prezzo_unitario", "note",
};

bool sameValue(const Component& a, const Component& b) {
    const auto va = a.numericValue();
    const auto vb = b.numericValue();
    if (va && vb) return units::approxEqual(*va, *vb);
    return text::equalsIgnoreCase(text::trim(a.value), text::trim(b.value));
}

bool matchesWord(const Component& c, const std::string& word) {
    const std::string fields[] = {
        categoryName(c.category), c.name, c.value, c.displayValue(),
        c.package, c.location, c.supplier, c.notes,
    };
    for (const std::string& f : fields) {
        if (text::containsIgnoreCase(f, word)) return true;
    }
    // "4k7" trova anche un resistore inserito come "4700" o "4,7k".
    const auto wanted = units::parseValue(word);
    const auto actual = c.numericValue();
    return wanted && actual && units::approxEqual(*wanted, *actual);
}

// Ordine "naturale" dei valori: prima quelli numerici, crescenti, poi quelli testuali.
bool lessByValue(const Component& a, const Component& b) {
    const auto va = a.numericValue();
    const auto vb = b.numericValue();
    if (va && vb && !units::approxEqual(*va, *vb)) return *va < *vb;
    if (va.has_value() != vb.has_value()) return va.has_value();
    return text::toLower(a.value) < text::toLower(b.value);
}

}  // namespace

bool SearchFilter::matches(const Component& c) const {
    if (category && c.category != *category) return false;
    if (onlyLowStock && !c.isLowStock()) return false;

    if (minValue || maxValue) {
        const auto v = c.numericValue();
        if (!v) return false;
        if (minValue && *v < *minValue && !units::approxEqual(*v, *minValue)) return false;
        if (maxValue && *v > *maxValue && !units::approxEqual(*v, *maxValue)) return false;
    }

    for (const std::string& word : text::splitWords(query)) {
        if (!matchesWord(c, word)) return false;
    }
    return true;
}

void sortComponents(std::vector<const Component*>& list, SortKey key) {
    // Resistori, condensatori e induttori: per valore; le altre categorie: per sigla.
    auto byCategoryAndValue = [](const Component* a, const Component* b) {
        if (a->category != b->category) return a->category < b->category;
        const std::string na = text::toLower(a->name);
        const std::string nb = text::toLower(b->name);
        const bool valueFirst = !categoryUnit(a->category).empty();
        if (!valueFirst && na != nb) return na < nb;
        if (lessByValue(*a, *b)) return true;
        if (lessByValue(*b, *a)) return false;
        if (na != nb) return na < nb;
        return a->id < b->id;
    };

    switch (key) {
        case SortKey::CategoryAndValue:
            std::sort(list.begin(), list.end(), byCategoryAndValue);
            break;
        case SortKey::Name:
            std::sort(list.begin(), list.end(), [](const Component* a, const Component* b) {
                const std::string na = text::toLower(a->name);
                const std::string nb = text::toLower(b->name);
                return na != nb ? na < nb : a->id < b->id;
            });
            break;
        case SortKey::Quantity:
            std::sort(list.begin(), list.end(), [](const Component* a, const Component* b) {
                return a->quantity != b->quantity ? a->quantity < b->quantity : a->id < b->id;
            });
            break;
        case SortKey::Location:
            std::sort(list.begin(), list.end(), [&](const Component* a, const Component* b) {
                const std::string la = text::toLower(a->location);
                const std::string lb = text::toLower(b->location);
                return la != lb ? la < lb : byCategoryAndValue(a, b);
            });
            break;
        case SortKey::Id:
            std::sort(list.begin(), list.end(),
                      [](const Component* a, const Component* b) { return a->id < b->id; });
            break;
    }
}

Component& Inventory::add(Component component) {
    component.id = nextId_++;
    items_.push_back(std::move(component));
    return items_.back();
}

bool Inventory::remove(int id) {
    const auto it = std::find_if(items_.begin(), items_.end(), [id](const Component& c) { return c.id == id; });
    if (it == items_.end()) return false;
    items_.erase(it);
    return true;
}

Component* Inventory::findById(int id) {
    for (Component& c : items_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

const Component* Inventory::findById(int id) const {
    return const_cast<Inventory*>(this)->findById(id);
}

const Component* Inventory::findDuplicate(const Component& component) const {
    for (const Component& c : items_) {
        if (c.id != component.id && c.category == component.category &&
            text::equalsIgnoreCase(text::trim(c.name), text::trim(component.name)) &&
            text::equalsIgnoreCase(text::trim(c.package), text::trim(component.package)) &&
            sameValue(c, component)) {
            return &c;
        }
    }
    return nullptr;
}

std::vector<const Component*> Inventory::all() const {
    std::vector<const Component*> result;
    result.reserve(items_.size());
    for (const Component& c : items_) result.push_back(&c);
    return result;
}

std::vector<const Component*> Inventory::search(const SearchFilter& filter) const {
    std::vector<const Component*> result;
    for (const Component& c : items_) {
        if (filter.matches(c)) result.push_back(&c);
    }
    return result;
}

std::vector<const Component*> Inventory::lowStock() const {
    SearchFilter filter;
    filter.onlyLowStock = true;
    return search(filter);
}

Statistics Inventory::statistics() const {
    Statistics s;
    for (Category cat : allCategories()) {
        CategoryStats cs;
        cs.category = cat;
        for (const Component& c : items_) {
            if (c.category != cat) continue;
            ++cs.items;
            cs.pieces += c.quantity;
            cs.value += c.stockValue();
        }
        if (cs.items > 0) s.byCategory.push_back(cs);
    }
    for (const Component& c : items_) {
        ++s.items;
        s.pieces += c.quantity;
        s.value += c.stockValue();
        if (c.isLowStock()) ++s.lowStock;
        if (c.unitPrice <= 0.0) ++s.withoutPrice;
        if (text::trim(c.location).empty()) ++s.withoutLocation;
    }
    return s;
}

std::vector<std::string> Inventory::load(const std::string& path) {
    std::vector<std::string> warnings;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            items_.clear();
            nextId_ = 1;
            return warnings;  // primo avvio: nessun file, archivio vuoto
        }
        throw std::runtime_error("impossibile aprire il file \"" + path + "\"");
    }

    std::vector<Component> loaded;
    std::vector<std::size_t> withoutId;  // componenti a cui assegnare un nuovo ID
    std::set<int> usedIds;

    std::string line;
    int lineNumber = 0;
    bool firstDataLine = true;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.rfind(kUtf8Bom, 0) == 0) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (text::trim(line).empty()) continue;

        if (firstDataLine) {
            firstDataLine = false;
            if (text::toLower(text::trim(line)).rfind("id", 0) == 0) continue;  // intestazione
        }

        const std::string where = "riga " + std::to_string(lineNumber) + ": ";
        std::vector<std::string> f = csv::parseLine(line, kSeparator);
        if (f.size() < 3) {
            warnings.push_back(where + "campi insufficienti, riga ignorata");
            continue;
        }
        f.resize(kHeader.size());
        for (std::string& field : f) field = text::trim(field);

        Component c;
        if (const auto cat = categoryFromName(f[1])) {
            c.category = *cat;
        } else {
            c.category = Category::Other;
            warnings.push_back(where + "categoria \"" + f[1] + "\" sconosciuta, impostata ad \"Altro\"");
        }
        c.name = f[2];
        c.value = f[3];
        c.package = f[4];

        auto readCount = [&](const std::string& field, const char* label) {
            if (field.empty()) return 0;
            const auto n = text::parseInt(field);
            if (!n || *n < 0) {
                warnings.push_back(where + label + " \"" + field + "\" non valida, impostata a 0");
                return 0;
            }
            return *n;
        };
        c.quantity = readCount(f[5], "quantità");
        c.minQuantity = readCount(f[6], "scorta minima");
        c.location = f[7];
        c.supplier = f[8];
        if (!f[9].empty()) {
            const auto price = text::parseDecimal(f[9]);
            if (price && *price >= 0.0) {
                c.unitPrice = *price;
            } else {
                warnings.push_back(where + "prezzo \"" + f[9] + "\" non valido, impostato a 0");
            }
        }
        c.notes = f[10];

        const auto id = text::parseInt(f[0]);
        if (id && *id > 0 && usedIds.insert(*id).second) {
            c.id = *id;
        } else {
            if (!f[0].empty()) warnings.push_back(where + "ID \"" + f[0] + "\" non valido o duplicato, ne assegno uno nuovo");
            withoutId.push_back(loaded.size());
        }
        loaded.push_back(std::move(c));
    }
    if (in.bad()) throw std::runtime_error("errore durante la lettura di \"" + path + "\"");

    int next = usedIds.empty() ? 1 : *usedIds.rbegin() + 1;
    for (std::size_t index : withoutId) loaded[index].id = next++;

    items_ = std::move(loaded);
    nextId_ = next;
    return warnings;
}

void Inventory::save(const std::string& path) const {
    namespace fs = std::filesystem;
    const std::string tmpPath = path + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("impossibile scrivere il file \"" + tmpPath + "\"");

        // Il BOM permette a Excel di riconoscere la codifica UTF-8 (lettere accentate, Ω, µ...).
        out << kUtf8Bom << csv::joinLine(kHeader, kSeparator) << '\n';
        for (const Component& c : items_) {
            out << csv::joinLine({
                       std::to_string(c.id), categoryName(c.category), c.name, c.value, c.package,
                       std::to_string(c.quantity), std::to_string(c.minQuantity), c.location,
                       c.supplier, text::formatDecimalTrim(c.unitPrice, 4), c.notes,
                   }, kSeparator)
                << '\n';
        }
        out.flush();
        if (!out) throw std::runtime_error("errore durante la scrittura di \"" + tmpPath + "\"");
    }

    std::error_code ec;
    fs::rename(tmpPath, path, ec);
    if (ec) {
        std::error_code ignored;
        fs::remove(tmpPath, ignored);
        throw std::runtime_error("impossibile salvare \"" + path + "\": " + ec.message());
    }
}
