#include "component.h"

#include <iterator>

#include "textutil.h"
#include "units.h"

namespace {

struct CategoryInfo {
    Category category;
    const char* name;
    const char* unit;
    const char* hint;
};

const CategoryInfo kCategoryInfo[] = {
    {Category::Resistor, "Resistore", "Ω", "es. 220, 4k7, 10k, 1M"},
    {Category::Capacitor, "Condensatore", "F", "es. 22p, 100n, 4u7, 470uF"},
    {Category::Inductor, "Induttore", "H", "es. 10u, 100uH, 1m"},
    {Category::Diode, "Diodo", "", "es. 1 A 1000 V, Zener 5,1 V"},
    {Category::Led, "LED", "", "es. rosso 5 mm, RGB, bianco 3 mm"},
    {Category::Transistor, "Transistor", "", "es. NPN, PNP, MOSFET canale N"},
    {Category::IntegratedCircuit, "Circuito integrato", "", "es. timer, op-amp, regolatore 5 V"},
    {Category::Connector, "Connettore", "", "es. pin header 2,54 mm 40 poli"},
    {Category::Electromechanical, "Elettromeccanico", "", "es. relè 5 V, pulsante 6x6 mm"},
    {Category::Sensor, "Sensore", "", "es. temperatura, ultrasuoni, PIR"},
    {Category::Module, "Modulo", "", "es. Wi-Fi, display OLED 0,96\""},
    {Category::Other, "Altro", "", "testo libero"},
};

const CategoryInfo& infoFor(Category category) {
    for (const CategoryInfo& info : kCategoryInfo) {
        if (info.category == category) return info;
    }
    return kCategoryInfo[std::size(kCategoryInfo) - 1];
}

}  // namespace

const std::vector<Category>& allCategories() {
    static const std::vector<Category> categories = [] {
        std::vector<Category> v;
        for (const CategoryInfo& info : kCategoryInfo) v.push_back(info.category);
        return v;
    }();
    return categories;
}

std::string categoryName(Category category) {
    return infoFor(category).name;
}

std::optional<Category> categoryFromName(const std::string& name) {
    const std::string wanted = text::trim(name);
    for (const CategoryInfo& info : kCategoryInfo) {
        if (text::equalsIgnoreCase(wanted, info.name)) return info.category;
    }
    return std::nullopt;
}

std::string categoryUnit(Category category) {
    return infoFor(category).unit;
}

std::string categoryValueHint(Category category) {
    return infoFor(category).hint;
}

std::optional<double> Component::numericValue() const {
    return units::parseValue(value);
}

std::string Component::displayValue() const {
    const std::string unit = categoryUnit(category);
    if (!unit.empty()) {
        if (const auto v = numericValue()) return units::formatValue(*v, unit);
    }
    return value;
}
