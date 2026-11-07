#include "componentdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <climits>

#include "qtutil.h"
#include "textutil.h"
#include "units.h"

namespace {

QComboBox* makeEditableCombo(const QStringList& items, const QString& current, const QString& placeholder) {
    auto* combo = new QComboBox;
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);
    combo->addItems(items);
    combo->setCurrentText(current);
    combo->lineEdit()->setPlaceholderText(placeholder);
    return combo;
}

}  // namespace

ComponentDialog::ComponentDialog(const QString& title, const Component& initial, const QStringList& locations,
                                 const QStringList& suppliers, QWidget* parent)
    : QDialog(parent), base_(initial) {
    setWindowTitle(title);

    category_ = new QComboBox;
    const auto& categories = allCategories();
    for (std::size_t i = 0; i < categories.size(); ++i) {
        category_->addItem(toQ(categoryName(categories[i])), static_cast<int>(i));
        if (categories[i] == initial.category) category_->setCurrentIndex(static_cast<int>(i));
    }

    name_ = new QLineEdit(toQ(initial.name));
    name_->setPlaceholderText(QStringLiteral("es. BC547, NE555, film metallico 1/4 W"));

    value_ = new QLineEdit(toQ(initial.value));
    valuePreview_ = new QLabel;
    valuePreview_->setMinimumWidth(170);
    auto* valueRow = new QHBoxLayout;
    valueRow->addWidget(value_, 1);
    valueRow->addWidget(valuePreview_);

    package_ = makeEditableCombo({"THT", "SMD", "0402", "0603", "0805", "1206", "SOT-23", "TO-92", "TO-220",
                                  "DO-35", "DO-41", "DIP-8", "DIP-14", "DIP-16", "SOIC-8", "modulo", "scheda"},
                                 toQ(initial.package), QStringLiteral("es. THT, 0805, TO-92, DIP-8"));

    quantity_ = new QSpinBox;
    quantity_->setRange(0, INT_MAX);
    quantity_->setValue(initial.quantity);

    minQuantity_ = new QSpinBox;
    minQuantity_->setRange(0, INT_MAX);
    minQuantity_->setValue(initial.minQuantity);
    minQuantity_->setToolTip(QStringLiteral("Quando la quantità scende a questo livello il componente va riordinato"));

    location_ = makeEditableCombo(locations, toQ(initial.location), QStringLiteral("es. cassettiera A, cassetto 3"));
    supplier_ = makeEditableCombo(suppliers, toQ(initial.supplier), QString());

    price_ = new QDoubleSpinBox;
    price_->setRange(0.0, 1e6);
    price_->setDecimals(4);
    price_->setSuffix(QStringLiteral(" €"));
    price_->setValue(initial.unitPrice);

    notes_ = new QLineEdit(toQ(initial.notes));
    notes_->setPlaceholderText(QStringLiteral("es. link al datasheet"));

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Categoria:"), category_);
    form->addRow(QStringLiteral("Sigla / descrizione:"), name_);
    form->addRow(QStringLiteral("Valore:"), valueRow);
    form->addRow(QStringLiteral("Package:"), package_);
    form->addRow(QStringLiteral("Quantità:"), quantity_);
    form->addRow(QStringLiteral("Scorta minima:"), minQuantity_);
    form->addRow(QStringLiteral("Posizione:"), location_);
    form->addRow(QStringLiteral("Fornitore:"), supplier_);
    form->addRow(QStringLiteral("Prezzo unitario:"), price_);
    form->addRow(QStringLiteral("Note:"), notes_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Salva"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Annulla"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ComponentDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ComponentDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    setMinimumWidth(520);

    connect(category_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        updateValueHint();
        updateValuePreview();
    });
    connect(value_, &QLineEdit::textChanged, this, &ComponentDialog::updateValuePreview);
    updateValueHint();
    updateValuePreview();
    name_->setFocus();
}

Category ComponentDialog::selectedCategory() const {
    const int index = category_->currentData().toInt();
    return allCategories()[static_cast<std::size_t>(index)];
}

void ComponentDialog::updateValueHint() {
    value_->setPlaceholderText(toQ(categoryValueHint(selectedCategory())));
}

void ComponentDialog::updateValuePreview() {
    const std::string unit = categoryUnit(selectedCategory());
    const std::string value = text::trim(toStd(value_->text()));
    if (unit.empty() || value.empty()) {
        valuePreview_->clear();
        return;
    }
    if (const auto v = units::parseValue(value)) {
        valuePreview_->setText(QStringLiteral("= <b>%1</b>").arg(toQ(units::formatValue(*v, unit)).toHtmlEscaped()));
    } else {
        valuePreview_->setText(QStringLiteral("<i>non numerico: salvato come testo</i>"));
    }
}

Component ComponentDialog::component() const {
    Component c = base_;
    c.category = selectedCategory();
    c.name = text::trim(toStd(name_->text()));
    c.value = text::trim(toStd(value_->text()));
    c.package = text::trim(toStd(package_->currentText()));
    c.quantity = quantity_->value();
    c.minQuantity = minQuantity_->value();
    c.location = text::trim(toStd(location_->currentText()));
    c.supplier = text::trim(toStd(supplier_->currentText()));
    c.unitPrice = price_->value();
    c.notes = text::trim(toStd(notes_->text()));
    return c;
}

void ComponentDialog::accept() {
    if (name_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Indica la sigla o una descrizione del componente."));
        name_->setFocus();
        return;
    }
    QDialog::accept();
}
