// Test automatici della logica del catalogo (senza librerie esterne).

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "colorcode.h"
#include "csv.h"
#include "inventory.h"
#include "textutil.h"
#include "units.h"

namespace {

int checks = 0;
int failures = 0;

void report(bool ok, const char* file, int line, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << file << ":" << line << ": FALLITO: " << what << '\n';
    }
}

#define CHECK(cond) report((cond), __FILE__, __LINE__, #cond)
#define CHECK_EQ(actual, expected)                                                                   \
    do {                                                                                             \
        const auto a_ = (actual);                                                                    \
        const auto e_ = (expected);                                                                  \
        std::ostringstream msg_;                                                                     \
        msg_ << #actual << " == " << #expected << "  (ottenuto: " << a_ << ", atteso: " << e_ << ")"; \
        report(a_ == e_, __FILE__, __LINE__, msg_.str());                                            \
    } while (0)

void checkParse(const std::string& input, double expected, int line) {
    const auto v = units::parseValue(input);
    report(v && units::approxEqual(*v, expected), __FILE__, line,
           "parseValue(\"" + input + "\") == " + std::to_string(expected));
}

void checkParseFails(const std::string& input, int line) {
    report(!units::parseValue(input), __FILE__, line, "parseValue(\"" + input + "\") deve fallire");
}

Component make(Category cat, const std::string& name, const std::string& value, int qty, int min = 0) {
    Component c;
    c.category = cat;
    c.name = name;
    c.value = value;
    c.quantity = qty;
    c.minQuantity = min;
    return c;
}

void testText() {
    CHECK_EQ(text::trim("  ciao \t\r\n"), std::string("ciao"));
    CHECK_EQ(text::displayWidth("Quantità Ω"), std::size_t(10));
    CHECK_EQ(text::truncate("resistenza", 5), std::string("resi…"));
    CHECK_EQ(text::padLeft("7", 3), std::string("  7"));
    CHECK_EQ(text::formatDecimal(3.14159, 2), std::string("3,14"));
    CHECK_EQ(text::formatDecimalTrim(2.5, 4), std::string("2,5"));
    CHECK_EQ(text::formatDecimalTrim(3.0, 4), std::string("3"));
    CHECK_EQ(text::formatMoney(12.5), std::string("12,50 €"));
    CHECK(text::parseDecimal("1,5") == 1.5);
    CHECK(text::parseDecimal("0.02") == 0.02);
    CHECK(text::parseDecimal("0,35 €") == 0.35);
    CHECK(!text::parseDecimal("abc"));
    CHECK(!text::parseDecimal("1,2,3"));
    CHECK(text::parseInt(" 42 ") == 42);
    CHECK(text::parseInt("+7") == 7);
    CHECK(!text::parseInt("4k7"));
    CHECK(!text::parseInt(""));
}

void testUnitsParse() {
    checkParse("4k7", 4700, __LINE__);
    checkParse("4.7k", 4700, __LINE__);
    checkParse("4,7k", 4700, __LINE__);
    checkParse("10k", 10000, __LINE__);
    checkParse("10K", 10000, __LINE__);
    checkParse("220", 220, __LINE__);
    checkParse("220R", 220, __LINE__);
    checkParse("4R7", 4.7, __LINE__);
    checkParse("1M", 1e6, __LINE__);
    checkParse("2M2", 2.2e6, __LINE__);
    checkParse("100n", 100e-9, __LINE__);
    checkParse("100nF", 100e-9, __LINE__);
    checkParse("0.1uF", 100e-9, __LINE__);
    checkParse("4µ7", 4.7e-6, __LINE__);
    checkParse("4.7 µF", 4.7e-6, __LINE__);
    checkParse("22pF", 22e-12, __LINE__);
    checkParse("1mH", 1e-3, __LINE__);
    checkParse("10 kΩ", 10000, __LINE__);
    checkParse("10kohm", 10000, __LINE__);
    checkParse("16MHz", 16e6, __LINE__);
    checkParse("5V", 5, __LINE__);

    checkParseFails("", __LINE__);
    checkParseFails("abc", __LINE__);
    checkParseFails("k", __LINE__);
    checkParseFails("4.7k7", __LINE__);
    checkParseFails("1.2.3", __LINE__);
    checkParseFails("10x", __LINE__);
    checkParseFails("rosso 5mm", __LINE__);
}

void testUnitsFormat() {
    CHECK_EQ(units::formatValue(4700, "Ω"), std::string("4,7 kΩ"));
    CHECK_EQ(units::formatValue(220, "Ω"), std::string("220 Ω"));
    CHECK_EQ(units::formatValue(1000, "Ω"), std::string("1 kΩ"));
    CHECK_EQ(units::formatValue(2.2e6, "Ω"), std::string("2,2 MΩ"));
    CHECK_EQ(units::formatValue(4.7, "Ω"), std::string("4,7 Ω"));
    CHECK_EQ(units::formatValue(100e-9, "F"), std::string("100 nF"));
    CHECK_EQ(units::formatValue(4.7e-6, "F"), std::string("4,7 µF"));
    CHECK_EQ(units::formatValue(22e-12, "F"), std::string("22 pF"));
    CHECK_EQ(units::formatValue(1000e-6, "F"), std::string("1000 µF"));
    CHECK_EQ(units::formatValue(4700e-6, "F"), std::string("4700 µF"));
    CHECK_EQ(units::formatValue(1.0, "F"), std::string("1 F"));
    CHECK_EQ(units::formatValue(1e-3, "Ω"), std::string("1 mΩ"));
    CHECK_EQ(units::formatValue(1e-3, "H"), std::string("1 mH"));
    CHECK_EQ(units::formatValue(12345, "Ω"), std::string("12,3 kΩ"));
    CHECK_EQ(units::formatValue(999.96, "Ω"), std::string("1 kΩ"));
    CHECK_EQ(units::formatValue(0, "Ω"), std::string("0 Ω"));
    CHECK_EQ(units::formatValue(4700, ""), std::string("4,7 k"));
}

void testColorCode() {
    std::string error;

    auto d = colorcode::decode({"giallo", "viola", "rosso", "oro"}, error);
    CHECK(d && units::approxEqual(d->ohms, 4700) && d->tolerance == "±5%");

    d = colorcode::decode({"marrone", "nero", "nero", "rosso", "marrone"}, error);
    CHECK(d && units::approxEqual(d->ohms, 10000) && d->tolerance == "±1%");

    d = colorcode::decode(colorcode::splitBands("Marrone, Nero, Oro"), error);
    CHECK(d && units::approxEqual(d->ohms, 1.0) && d->tolerance == "±20%");

    d = colorcode::decode({"rosso", "rosso", "argento", "argento"}, error);
    CHECK(d && units::approxEqual(d->ohms, 0.22) && d->tolerance == "±10%");

    CHECK(!colorcode::decode({"oro", "viola", "rosso", "oro"}, error));
    CHECK(!colorcode::decode({"giallo", "viola"}, error));
    CHECK(!colorcode::decode({"giallo", "fucsia", "rosso"}, error));
    CHECK(!colorcode::decode({"giallo", "viola", "rosso", "giallo"}, error));  // giallo non è una tolleranza

    auto e = colorcode::encode(4700, 2, error);
    CHECK(e && e->bands == std::vector<std::string>({"giallo", "viola", "rosso"}));

    e = colorcode::encode(10000, 3, error);
    CHECK(e && e->bands == std::vector<std::string>({"marrone", "nero", "nero", "rosso"}));

    e = colorcode::encode(4.7, 2, error);
    CHECK(e && e->bands == std::vector<std::string>({"giallo", "viola", "oro"}));

    e = colorcode::encode(0.47, 2, error);
    CHECK(e && e->bands == std::vector<std::string>({"giallo", "viola", "argento"}));

    e = colorcode::encode(1000, 2, error);
    CHECK(e && e->bands == std::vector<std::string>({"marrone", "nero", "rosso"}));

    e = colorcode::encode(12345, 2, error);  // non rappresentabile esattamente con 2 cifre
    CHECK(e && units::approxEqual(e->ohms, 12000) &&
          e->bands == std::vector<std::string>({"marrone", "rosso", "arancione"}));

    CHECK(!colorcode::encode(0.001, 2, error));
    CHECK(!colorcode::encode(-5, 2, error));

    // Andata e ritorno su tutta la serie E12.
    for (double base : {1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2}) {
        for (double decade : {1.0, 10.0, 100.0, 1e3, 1e4, 1e5, 1e6}) {
            const double ohms = base * decade;
            auto enc = colorcode::encode(ohms, 2, error);
            CHECK(enc && units::approxEqual(enc->ohms, ohms));
            if (!enc) continue;
            auto bands = enc->bands;
            bands.push_back("oro");
            auto dec = colorcode::decode(bands, error);
            CHECK(dec && units::approxEqual(dec->ohms, ohms));
        }
    }
}

void testCsv() {
    const std::vector<std::string> fields = {"1", "testo; con separatore", "con \"virgolette\"", "", " spazi "};
    const std::string line = csv::joinLine(fields, ';');
    CHECK(csv::parseLine(line, ';') == fields);
    CHECK(csv::parseLine("a;;c", ';') == std::vector<std::string>({"a", "", "c"}));
    CHECK_EQ(csv::escapeField("riga1\nriga2", ';'), std::string("riga1 riga2"));
}

void testInventory() {
    Inventory inv;
    CHECK(inv.empty());

    const int r1 = inv.add(make(Category::Resistor, "Film metallico", "4700", 100, 20)).id;
    const int r2 = inv.add(make(Category::Resistor, "Film metallico", "10k", 5, 20)).id;
    const int c1 = inv.add(make(Category::Capacitor, "Ceramico", "100n", 50, 10)).id;
    const int t1 = inv.add(make(Category::Transistor, "BC547", "NPN", 0)).id;
    CHECK_EQ(inv.size(), std::size_t(4));
    CHECK(r1 == 1 && r2 == 2 && c1 == 3 && t1 == 4);

    // Visualizzazione del valore.
    CHECK_EQ(inv.findById(r1)->displayValue(), std::string("4,7 kΩ"));
    CHECK_EQ(inv.findById(c1)->displayValue(), std::string("100 nF"));
    CHECK_EQ(inv.findById(t1)->displayValue(), std::string("NPN"));

    // Ricerca per testo e per valore equivalente.
    SearchFilter f;
    f.query = "4k7";
    auto res = inv.search(f);
    CHECK(res.size() == 1 && res[0]->id == r1);

    f.query = "bc547";
    res = inv.search(f);
    CHECK(res.size() == 1 && res[0]->id == t1);

    f.query = "film 10k";
    res = inv.search(f);
    CHECK(res.size() == 1 && res[0]->id == r2);

    f.query = "inesistente";
    CHECK(inv.search(f).empty());

    // Filtro per categoria e intervallo di valori.
    SearchFilter range;
    range.category = Category::Resistor;
    range.minValue = 1000;
    range.maxValue = 5000;
    res = inv.search(range);
    CHECK(res.size() == 1 && res[0]->id == r1);

    range.maxValue = 10000;  // estremo incluso
    CHECK_EQ(inv.search(range).size(), std::size_t(2));

    // Scorte in esaurimento: 10k (5 <= 20) e BC547 (esaurito).
    res = inv.lowStock();
    CHECK_EQ(res.size(), std::size_t(2));

    // Doppioni: stesso valore scritto in modo diverso.
    Component dup = make(Category::Resistor, "film METALLICO", "4k7", 10);
    const Component* found = inv.findDuplicate(dup);
    CHECK(found && found->id == r1);
    dup.package = "0805";
    CHECK(!inv.findDuplicate(dup));

    // Ordinamento per categoria e valore.
    auto list = inv.all();
    sortComponents(list, SortKey::Quantity);
    CHECK(list.front()->id == t1);
    sortComponents(list, SortKey::CategoryAndValue);
    CHECK(list[0]->id == r1 && list[1]->id == r2 && list[2]->id == c1 && list[3]->id == t1);

    // Statistiche.
    inv.findById(r1)->unitPrice = 0.02;
    inv.findById(c1)->unitPrice = 0.05;
    const Statistics s = inv.statistics();
    CHECK_EQ(s.items, 4);
    CHECK_EQ(s.pieces, 155LL);
    CHECK(std::fabs(s.value - 4.5) < 1e-9);
    CHECK_EQ(s.lowStock, 2);
    CHECK_EQ(s.byCategory.size(), std::size_t(3));

    // Eliminazione.
    CHECK(inv.remove(r2));
    CHECK(!inv.remove(r2));
    CHECK(inv.findById(r2) == nullptr);
}

void testSaveLoad() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "test_catalogo_componenti";
    fs::create_directories(dir);
    const std::string path = (dir / "archivio.csv").string();
    fs::remove(path);

    Inventory empty;
    CHECK(empty.load(path).empty());  // file inesistente: archivio vuoto, nessun avviso
    CHECK(empty.empty());

    Inventory inv;
    Component c = make(Category::Resistor, "Film; metallico \"1/4 W\"", "4k7", 100, 20);
    c.package = "THT";
    c.location = "Cassetto A3";
    c.supplier = "Fornitore à è ì";
    c.unitPrice = 0.0125;
    c.notes = "Tolleranza ±1%";
    inv.add(c);
    inv.add(make(Category::Led, "LED", "rosso 5 mm", 30));
    inv.remove(1);
    inv.add(make(Category::Module, "ESP32", "", 2));  // riceve ID 3
    inv.save(path);

    Inventory loaded;
    CHECK(loaded.load(path).empty());
    CHECK_EQ(loaded.size(), std::size_t(2));
    CHECK(loaded.findById(2) && loaded.findById(2)->value == "rosso 5 mm");
    CHECK(loaded.findById(3) && loaded.findById(3)->category == Category::Module);
    // Il prossimo ID continua dopo il più alto presente.
    CHECK_EQ(loaded.add(make(Category::Other, "x", "", 1)).id, 4);

    // Andata e ritorno di tutti i campi.
    Inventory full;
    full.add(c);
    full.save(path);
    Inventory reread;
    reread.load(path);
    const Component* r = reread.findById(1);
    CHECK(r != nullptr);
    if (r) {
        CHECK_EQ(r->name, c.name);
        CHECK_EQ(r->value, c.value);
        CHECK_EQ(r->package, c.package);
        CHECK_EQ(r->quantity, c.quantity);
        CHECK_EQ(r->minQuantity, c.minQuantity);
        CHECK_EQ(r->location, c.location);
        CHECK_EQ(r->supplier, c.supplier);
        CHECK(std::fabs(r->unitPrice - c.unitPrice) < 1e-9);
        CHECK_EQ(r->notes, c.notes);
    }

    // File scritto a mano (o modificato con Excel) con qualche errore.
    {
        std::ofstream out(path, std::ios::binary);
        out << "id;categoria;nome;valore;package;quantita;scorta_minima;posizione;fornitore;prezzo_unitario;note\r\n"
            << "5;resistore;Carbone;220;THT;10;2;A1;;0,01;\r\n"
            << "5;Condensatore;Elettrolitico;100u;THT;3;1;B2;;;\r\n"   // ID duplicato
            << ";Sconosciuta;Cosa;;;abc;;;;;\r\n"                      // categoria e quantità non valide
            << "x\r\n"                                                 // riga troppo corta
            << "\r\n";
    }
    Inventory messy;
    const auto warnings = messy.load(path);
    CHECK_EQ(messy.size(), std::size_t(3));
    CHECK(warnings.size() >= 4);
    CHECK(messy.findById(5) && messy.findById(5)->category == Category::Resistor);
    CHECK(messy.findById(6) && messy.findById(6)->category == Category::Capacitor);
    CHECK(messy.findById(7) && messy.findById(7)->category == Category::Other && messy.findById(7)->quantity == 0);
    CHECK(std::fabs(messy.findById(5)->unitPrice - 0.01) < 1e-9);

    fs::remove_all(dir);
}

}  // namespace

int main() {
    testText();
    testUnitsParse();
    testUnitsFormat();
    testColorCode();
    testCsv();
    testInventory();
    testSaveLoad();

    if (failures == 0) {
        std::cout << "Tutti i " << checks << " controlli superati.\n";
        return 0;
    }
    std::cout << failures << " controlli falliti su " << checks << ".\n";
    return 1;
}
