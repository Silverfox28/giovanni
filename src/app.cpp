#include "app.h"

#include <algorithm>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "colorcode.h"
#include "console.h"
#include "textutil.h"
#include "units.h"

namespace {

// Piccola tabella di testo con colonne allineate.
class Table {
public:
    struct Column {
        std::string header;
        bool alignRight;
        std::size_t maxWidth;
    };

    explicit Table(std::vector<Column> columns) : columns_(std::move(columns)) {}

    void addRow(std::vector<std::string> cells) { rows_.push_back(std::move(cells)); }

    void print(std::ostream& os) const {
        std::vector<std::size_t> widths;
        for (std::size_t i = 0; i < columns_.size(); ++i) {
            std::size_t w = text::displayWidth(columns_[i].header);
            for (const auto& row : rows_) w = std::max(w, text::displayWidth(row[i]));
            widths.push_back(std::min(w, columns_[i].maxWidth));
        }

        auto printRow = [&](const std::vector<std::string>& cells) {
            std::string line;
            for (std::size_t i = 0; i < columns_.size(); ++i) {
                const std::string cell = text::truncate(cells[i], widths[i]);
                line += columns_[i].alignRight ? text::padLeft(cell, widths[i]) : text::padRight(cell, widths[i]);
                if (i + 1 < columns_.size()) line += "  ";
            }
            line.erase(line.find_last_not_of(' ') + 1);
            os << ' ' << line << '\n';
        };

        std::vector<std::string> headers;
        std::size_t totalWidth = 0;
        for (std::size_t i = 0; i < columns_.size(); ++i) {
            headers.push_back(columns_[i].header);
            totalWidth += widths[i] + (i + 1 < columns_.size() ? 2 : 0);
        }
        printRow(headers);
        os << ' ' << std::string(totalWidth, '-') << '\n';
        for (const auto& row : rows_) printRow(row);
    }

private:
    std::vector<Column> columns_;
    std::vector<std::vector<std::string>> rows_;
};

std::string countOf(std::size_t n, const char* singular, const char* plural) {
    return std::to_string(n) + " " + (n == 1 ? singular : plural);
}

std::string quantityCell(const Component& c) {
    return std::to_string(c.quantity) + (c.isLowStock() ? " !" : "");
}

// Legge un valore come "4k7" o "100n"; restituisce nullopt se l'utente preme Invio.
std::optional<double> readOptionalValue(const std::string& label) {
    while (true) {
        const std::string input = console::readLine(label);
        if (input.empty()) return std::nullopt;
        if (const auto v = units::parseValue(input)) return v;
        std::cout << "  Valore non riconosciuto. Esempi validi: 4k7, 100n, 2,2u, 1M.\n";
    }
}

// Descrizione del codice colori di un resistore, es. "giallo viola rosso + tolleranza".
std::string colorCodeDescription(double ohms) {
    std::string error;
    for (int digits : {2, 3}) {
        const auto encoded = colorcode::encode(ohms, digits, error);
        if (!encoded || !units::approxEqual(encoded->ohms, ohms)) continue;
        std::string bands;
        for (const std::string& b : encoded->bands) bands += (bands.empty() ? "" : " ") + b;
        return bands + " + tolleranza (" + (digits == 2 ? "4" : "5") + " bande)";
    }
    return "";
}

}  // namespace

App::App(std::string dataPath) : dataPath_(std::move(dataPath)) {}

int App::run() {
    try {
        const auto warnings = inventory_.load(dataPath_);
        for (const std::string& w : warnings) std::cout << "Avviso (" << dataPath_ << ", " << w << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << '\n';
        return 1;
    }

    try {
        mainMenu();
    } catch (const console::InputClosed&) {
        std::cout << '\n';
    }
    std::cout << "Arrivederci!\n";
    return 0;
}

bool App::persist() {
    try {
        inventory_.save(dataPath_);
        return true;
    } catch (const std::exception& e) {
        std::cout << "\n*** ATTENZIONE: salvataggio non riuscito: " << e.what() << " ***\n";
        return false;
    }
}

void App::mainMenu() {
    while (true) {
        const auto low = inventory_.lowStock().size();
        std::cout << "\n==============================================\n"
                  << "     CATALOGO COMPONENTI ELETTRONICI\n"
                  << "==============================================\n"
                  << " Archivio: " << dataPath_ << " (" << countOf(inventory_.size(), "componente", "componenti")
                  << ")\n";
        if (low > 0) std::cout << " ! " << countOf(low, "componente", "componenti") << " sotto la scorta minima\n";
        std::cout << "----------------------------------------------\n"
                  << "  1) Elenco componenti\n"
                  << "  2) Cerca\n"
                  << "  3) Scheda di un componente\n"
                  << "  4) Aggiungi componente\n"
                  << "  5) Modifica componente\n"
                  << "  6) Preleva / carica quantità\n"
                  << "  7) Elimina componente\n"
                  << "  8) Scorte in esaurimento\n"
                  << "  9) Statistiche\n"
                  << " 10) Codice colori resistori\n"
                  << "  0) Esci\n";

        switch (console::readInt("Scelta", 0, 10)) {
            case 1: listComponents(); break;
            case 2: searchComponents(); break;
            case 3: showComponent(); break;
            case 4: addComponent(); break;
            case 5: editComponent(); break;
            case 6: adjustQuantity(); break;
            case 7: deleteComponent(); break;
            case 8: lowStockReport(); break;
            case 9: showStatistics(); break;
            case 10: colorCodeTool(); break;
            case 0: return;
        }
    }
}

void App::printTable(const std::vector<const Component*>& list) const {
    Table table({
        {"ID", true, 6},
        {"Categoria", false, 18},
        {"Sigla/Descrizione", false, 28},
        {"Valore", false, 16},
        {"Package", false, 10},
        {"Q.tà", true, 8},
        {"Min", true, 6},
        {"Posizione", false, 28},
    });
    long long pieces = 0;
    for (const Component* c : list) {
        table.addRow({
            std::to_string(c->id), categoryName(c->category), c->name, c->displayValue(), c->package,
            quantityCell(*c), std::to_string(c->minQuantity), c->location,
        });
        pieces += c->quantity;
    }
    table.print(std::cout);
    std::cout << "\n " << countOf(list.size(), "componente", "componenti") << ", "
              << countOf(static_cast<std::size_t>(pieces), "pezzo", "pezzi") << " in totale.";
    if (std::any_of(list.begin(), list.end(), [](const Component* c) { return c->isLowStock(); })) {
        std::cout << "  (! = sotto la scorta minima)";
    }
    std::cout << '\n';
}

void App::printDetails(const Component& c) const {
    auto row = [](const std::string& label, const std::string& value) {
        std::cout << "  " << text::padRight(label, 16) << ": " << value << '\n';
    };

    if (c.id > 0) std::cout << "\n--- Componente #" << c.id << " ---\n";
    else std::cout << "\n--- Nuovo componente ---\n";
    row("Categoria", categoryName(c.category));
    row("Sigla/descr.", c.name);
    std::string value = c.displayValue();
    if (!c.value.empty() && value != c.value) value += "   (inserito come \"" + c.value + "\")";
    row("Valore", value);
    if (c.category == Category::Resistor) {
        if (const auto ohms = c.numericValue(); ohms && *ohms > 0) {
            const std::string code = colorCodeDescription(*ohms);
            if (!code.empty()) row("Codice colori", code);
        }
    }
    row("Package", c.package);
    row("Quantità", std::to_string(c.quantity) + "   (scorta minima " + std::to_string(c.minQuantity) + ")" +
                        (c.isLowStock() ? "  ! DA RIORDINARE" : ""));
    row("Posizione", c.location);
    row("Fornitore", c.supplier);
    if (c.unitPrice > 0.0) {
        row("Prezzo unitario", text::formatDecimalTrim(c.unitPrice, 4) + " €   (valore scorta " +
                                   text::formatMoney(c.stockValue()) + ")");
    } else {
        row("Prezzo unitario", "-");
    }
    row("Note", c.notes);
}

Component* App::askComponent() {
    if (inventory_.empty()) {
        std::cout << "L'archivio è vuoto.\n";
        return nullptr;
    }
    while (true) {
        const std::string input = console::readLine("ID del componente, oppure testo da cercare (Invio = annulla): ");
        if (input.empty()) return nullptr;

        if (const auto id = text::parseInt(input)) {
            if (Component* c = inventory_.findById(*id)) return c;
            std::cout << "  Nessun componente con ID " << *id << ".\n";
            continue;
        }

        SearchFilter filter;
        filter.query = input;
        auto results = inventory_.search(filter);
        sortComponents(results, SortKey::CategoryAndValue);
        if (results.empty()) {
            std::cout << "  Nessun componente corrisponde a \"" << input << "\".\n";
        } else if (results.size() == 1) {
            printTable(results);
            if (console::confirm("Usare questo componente?", true)) return inventory_.findById(results.front()->id);
        } else {
            printTable(results);
            std::cout << "  Più risultati: indica l'ID del componente desiderato.\n";
        }
    }
}

void App::listComponents() {
    console::title("Elenco componenti");
    if (inventory_.empty()) {
        std::cout << "L'archivio è vuoto: usa \"Aggiungi componente\" per iniziare.\n";
        console::pause();
        return;
    }
    std::cout << "Ordina per: 1) categoria e valore  2) sigla  3) quantità  4) posizione  5) ID\n";
    const int choice = console::readIntDefault("Ordinamento", 1, 1, 5);
    static const SortKey keys[] = {SortKey::CategoryAndValue, SortKey::Name, SortKey::Quantity, SortKey::Location,
                                   SortKey::Id};
    auto list = inventory_.all();
    sortComponents(list, keys[choice - 1]);
    std::cout << '\n';
    printTable(list);
    console::pause();
}

void App::searchComponents() {
    console::title("Cerca componenti");
    std::cout << "Lascia vuoto (Invio) un criterio per non applicarlo.\n\n";

    SearchFilter filter;
    filter.category = console::chooseCategory(std::nullopt, true);
    filter.query = console::readLine("Parole da cercare (sigla, valore, package, posizione, note...): ");

    if (filter.category && !categoryUnit(*filter.category).empty()) {
        const std::string unit = categoryUnit(*filter.category);
        std::cout << "Intervallo di valori in " << unit << " (es. 1k, 100n):\n";
        filter.minValue = readOptionalValue("  Valore minimo: ");
        filter.maxValue = readOptionalValue("  Valore massimo: ");
    }

    auto results = inventory_.search(filter);
    std::cout << '\n';
    if (results.empty()) {
        std::cout << "Nessun componente trovato.\n";
    } else {
        sortComponents(results, SortKey::CategoryAndValue);
        printTable(results);
    }
    console::pause();
}

void App::showComponent() {
    console::title("Scheda componente");
    if (const Component* c = askComponent()) {
        printDetails(*c);
        console::pause();
    }
}

void App::fillFields(Component& c) {
    const auto category = console::chooseCategory(c.name.empty() ? std::nullopt : std::optional(c.category), false);
    c.category = category.value_or(Category::Other);

    c.name = console::readRequiredText("Sigla o descrizione (es. BC547, NE555, film metallico 1/4 W)", c.name);

    const std::string unit = categoryUnit(c.category);
    c.value = console::readText("Valore (" + categoryValueHint(c.category) + ")", c.value);
    if (!unit.empty() && !c.value.empty()) {
        if (const auto v = c.numericValue()) {
            std::cout << "  -> interpretato come " << units::formatValue(*v, unit) << '\n';
        } else {
            std::cout << "  (non è un valore numerico: verrà conservato come testo)\n";
        }
    }

    c.package = console::readText("Package (es. THT, 0805, TO-92, DIP-8, SOT-23)", c.package);
    c.quantity = console::readIntDefault("Quantità disponibile", c.quantity, 0, INT_MAX);
    c.minQuantity = console::readIntDefault("Scorta minima (avviso quando si scende a questo livello)",
                                            c.minQuantity, 0, INT_MAX);
    c.location = console::readText("Posizione (es. cassettiera A, cassetto 3)", c.location);
    c.supplier = console::readText("Fornitore", c.supplier);
    c.unitPrice = console::readDecimalDefault("Prezzo unitario in €", c.unitPrice, 0.0);
    c.notes = console::readText("Note (es. link al datasheet)", c.notes);
}

void App::addComponent() {
    console::title("Aggiungi componente");
    Component c;
    fillFields(c);

    if (const Component* existing = inventory_.findDuplicate(c)) {
        std::cout << "\nEsiste già un componente uguale:";
        printDetails(*existing);
        if (console::confirm("Vuoi sommare " + std::to_string(c.quantity) +
                                 " pezzi a quello esistente invece di creare un doppione?",
                             true)) {
            Component* target = inventory_.findById(existing->id);
            target->quantity += c.quantity;
            if (persist()) std::cout << "Quantità aggiornata: ora ci sono " << target->quantity << " pezzi.\n";
            console::pause();
            return;
        }
    }

    printDetails(c);
    if (!console::confirm("Aggiungere questo componente?", true)) {
        std::cout << "Inserimento annullato.\n";
        return;
    }
    const int id = inventory_.add(std::move(c)).id;
    if (persist()) std::cout << "Componente aggiunto con ID " << id << ".\n";
    console::pause();
}

void App::editComponent() {
    console::title("Modifica componente");
    Component* found = askComponent();
    if (!found) return;

    Component edited = *found;
    printDetails(edited);
    std::cout << "\nPremi Invio per mantenere il valore attuale, scrivi \"-\" per cancellare un campo.\n\n";
    fillFields(edited);

    if (const Component* dup = inventory_.findDuplicate(edited)) {
        std::cout << "Nota: il componente #" << dup->id << " ha gli stessi dati (potrebbe essere un doppione).\n";
    }
    if (!console::confirm("Salvare le modifiche?", true)) {
        std::cout << "Modifiche annullate.\n";
        return;
    }
    *found = edited;
    if (persist()) std::cout << "Modifiche salvate.\n";
    console::pause();
}

void App::adjustQuantity() {
    console::title("Preleva / carica quantità");
    Component* c = askComponent();
    if (!c) return;

    std::cout << "\n#" << c->id << "  " << c->name << "  " << c->displayValue() << "  -> disponibili: " << c->quantity
              << " pezzi\n";
    std::cout << "Scrivi -N per prelevare, +N per caricare, =N per impostare la quantità (es. -5, +100, =40).\n";

    while (true) {
        const std::string input = console::readLine("Variazione (Invio = annulla): ");
        if (input.empty()) return;

        const char op = input[0];
        const auto amount = text::parseInt(input.substr(1));
        if ((op != '+' && op != '-' && op != '=') || !amount || *amount < 0) {
            std::cout << "  Formato non valido: usa ad esempio -5, +100 oppure =40.\n";
            continue;
        }

        long long newQuantity = c->quantity;
        if (op == '+') newQuantity += *amount;
        else if (op == '-') newQuantity -= *amount;
        else newQuantity = *amount;

        if (newQuantity < 0) {
            std::cout << "  Quantità insufficiente: ne sono disponibili solo " << c->quantity << ".\n";
            continue;
        }
        if (newQuantity > INT_MAX) {
            std::cout << "  Quantità troppo grande.\n";
            continue;
        }

        c->quantity = static_cast<int>(newQuantity);
        if (persist()) std::cout << "Fatto: ora ci sono " << c->quantity << " pezzi.\n";
        if (c->isLowStock()) {
            std::cout << "! Attenzione: sei al di sotto della scorta minima (" << c->minQuantity
                      << "). Ricordati di riordinarlo.\n";
        }
        console::pause();
        return;
    }
}

void App::deleteComponent() {
    console::title("Elimina componente");
    const Component* c = askComponent();
    if (!c) return;

    printDetails(*c);
    const int id = c->id;
    if (!console::confirm("Eliminare definitivamente questo componente?", false)) {
        std::cout << "Eliminazione annullata.\n";
        return;
    }
    inventory_.remove(id);
    if (persist()) std::cout << "Componente #" << id << " eliminato.\n";
    console::pause();
}

void App::lowStockReport() {
    console::title("Scorte in esaurimento");
    auto list = inventory_.lowStock();
    if (list.empty()) {
        std::cout << "Tutti i componenti sono sopra la scorta minima.\n";
        console::pause();
        return;
    }
    sortComponents(list, SortKey::CategoryAndValue);

    Table table({
        {"ID", true, 6},
        {"Categoria", false, 18},
        {"Sigla/Descrizione", false, 28},
        {"Valore", false, 16},
        {"Package", false, 10},
        {"Q.tà", true, 8},
        {"Min", true, 6},
        {"Fornitore", false, 20},
    });
    for (const Component* c : list) {
        table.addRow({
            std::to_string(c->id), categoryName(c->category), c->name, c->displayValue(), c->package,
            std::to_string(c->quantity), std::to_string(c->minQuantity), c->supplier,
        });
    }
    table.print(std::cout);
    std::cout << "\n " << countOf(list.size(), "componente", "componenti") << " da riordinare.\n";
    console::pause();
}

void App::showStatistics() {
    console::title("Statistiche");
    const Statistics s = inventory_.statistics();

    auto row = [](const std::string& label, const std::string& value) {
        std::cout << "  " << text::padRight(label, 28) << ": " << value << '\n';
    };
    row("Componenti diversi", std::to_string(s.items));
    row("Pezzi totali", std::to_string(s.pieces));
    row("Valore stimato del magazzino", text::formatMoney(s.value));
    row("Sotto la scorta minima", std::to_string(s.lowStock));
    row("Senza prezzo", std::to_string(s.withoutPrice));
    row("Senza posizione", std::to_string(s.withoutLocation));

    if (!s.byCategory.empty()) {
        std::cout << '\n';
        Table table({
            {"Categoria", false, 20},
            {"Articoli", true, 10},
            {"Pezzi", true, 10},
            {"Valore", true, 14},
        });
        for (const CategoryStats& cs : s.byCategory) {
            table.addRow({categoryName(cs.category), std::to_string(cs.items), std::to_string(cs.pieces),
                          text::formatMoney(cs.value)});
        }
        table.print(std::cout);
    }
    console::pause();
}

void App::colorCodeTool() {
    while (true) {
        console::title("Codice colori resistori");
        std::cout << "  1) Dai colori al valore\n"
                  << "  2) Dal valore ai colori\n"
                  << "  0) Torna al menu principale\n";
        switch (console::readInt("Scelta", 0, 2)) {
            case 1: decodeColors(); break;
            case 2: encodeColors(); break;
            case 0: return;
        }
    }
}

void App::decodeColors() {
    std::cout << "\nColori disponibili: ";
    const auto names = colorcode::colorNames();
    for (std::size_t i = 0; i < names.size(); ++i) std::cout << (i ? ", " : "") << names[i];
    std::cout << "\nIndica le bande in ordine, separate da spazi (es. giallo viola rosso oro).\n";

    while (true) {
        const std::string input = console::readLine("Bande (Invio = indietro): ");
        if (input.empty()) return;

        std::string error;
        const auto decoded = colorcode::decode(colorcode::splitBands(input), error);
        if (!decoded) {
            std::cout << "  Errore: " << error << ".\n";
            continue;
        }
        std::cout << "\n  Valore: " << units::formatValue(decoded->ohms, "Ω") << "  " << decoded->tolerance << "\n";
        showResistorsWithValue(decoded->ohms);
        console::pause();
        return;
    }
}

void App::encodeColors() {
    while (true) {
        const std::string input = console::readLine("\nValore della resistenza (es. 4k7, 220, 1M; Invio = indietro): ");
        if (input.empty()) return;

        const auto ohms = units::parseValue(input);
        if (!ohms || *ohms <= 0) {
            std::cout << "  Valore non riconosciuto.\n";
            continue;
        }

        std::cout << '\n';
        for (int digits : {2, 3}) {
            std::string error;
            const auto encoded = colorcode::encode(*ohms, digits, error);
            std::cout << "  " << (digits == 2 ? "4 bande: " : "5 bande: ");
            if (!encoded) {
                std::cout << "non rappresentabile (" << error << ")\n";
                continue;
            }
            for (const std::string& b : encoded->bands) std::cout << b << ' ';
            std::cout << "+ tolleranza";
            if (!units::approxEqual(encoded->ohms, *ohms)) {
                std::cout << "   (approssimato a " << units::formatValue(encoded->ohms, "Ω") << ")";
            }
            std::cout << '\n';
        }
        std::cout << "  Tolleranza: marrone ±1%, rosso ±2%, oro ±5%, argento ±10%\n";
        showResistorsWithValue(*ohms);
        console::pause();
        return;
    }
}

void App::showResistorsWithValue(double ohms) {
    SearchFilter filter;
    filter.category = Category::Resistor;
    filter.minValue = ohms;
    filter.maxValue = ohms;
    auto found = inventory_.search(filter);
    if (found.empty()) {
        std::cout << "\n  Nessun resistore da " << units::formatValue(ohms, "Ω") << " in magazzino.\n";
        return;
    }
    std::cout << "\n  In magazzino:\n";
    sortComponents(found, SortKey::CategoryAndValue);
    printTable(found);
}
