#include "colorcodedialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "colorcode.h"
#include "inventory.h"
#include "qtutil.h"
#include "resistorwidget.h"
#include "theme.h"
#include "units.h"

namespace {

constexpr int kMaxBands = 5;

QString joinBands(const std::vector<std::string>& bands) {
    QStringList list;
    for (const std::string& b : bands) list << toQ(b);
    return list.join(QStringLiteral(" – "));
}

QLabel* makeResultLabel() {
    auto* label = new QLabel;
    QFont font = label->font();
    font.setPointSizeF(font.pointSizeF() * 1.5);
    font.setBold(true);
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

}  // namespace

ColorCodeDialog::ColorCodeDialog(const Inventory& inventory, QWidget* parent)
    : QDialog(parent), inventory_(inventory) {
    setWindowTitle(QStringLiteral("Codice colori dei resistori"));

    auto* tabs = new QTabWidget;
    tabs->addTab(createDecodeTab(), QStringLiteral("Dai colori al valore"));
    tabs->addTab(createEncodeTab(), QStringLiteral("Dal valore ai colori"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("Chiudi"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);
    setMinimumWidth(560);

    bandCount_->setCurrentIndex(bandCount_->findData(4));  // resistore a 4 bande, il più comune
    setBandCount(4);
    updateEncode();
}

QWidget* ColorCodeDialog::createDecodeTab() {
    auto* page = new QWidget;

    bandCount_ = new QComboBox;
    bandCount_->addItem(QStringLiteral("3 bande"), 3);
    bandCount_->addItem(QStringLiteral("4 bande"), 4);
    bandCount_->addItem(QStringLiteral("5 bande"), 5);

    auto* grid = new QGridLayout;
    const auto names = colorcode::colorNames();
    for (int i = 0; i < kMaxBands; ++i) {
        auto* label = new QLabel;
        auto* combo = new QComboBox;
        for (const std::string& name : names) combo->addItem(bandIcon(name), toQ(name), toQ(name));
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorCodeDialog::updateDecode);
        grid->addWidget(label, 0, i);
        grid->addWidget(combo, 1, i);
        bandLabels_.push_back(label);
        bandCombos_.push_back(combo);
    }

    decodeResistor_ = new ResistorWidget;
    decodeResult_ = makeResultLabel();
    decodeStock_ = new QLabel;
    decodeStock_->setAlignment(Qt::AlignCenter);
    decodeStock_->setWordWrap(true);

    auto* countRow = new QHBoxLayout;
    countRow->addWidget(new QLabel(QStringLiteral("Tipo di resistore:")));
    countRow->addWidget(bandCount_);
    countRow->addStretch();

    auto* layout = new QVBoxLayout(page);
    layout->addLayout(countRow);
    layout->addLayout(grid);
    layout->addSpacing(8);
    layout->addWidget(decodeResistor_);
    layout->addWidget(decodeResult_);
    layout->addWidget(decodeStock_);
    layout->addStretch();

    connect(bandCount_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this] { setBandCount(bandCount_->currentData().toInt()); });
    return page;
}

QWidget* ColorCodeDialog::createEncodeTab() {
    auto* page = new QWidget;

    encodeInput_ = new QLineEdit(QStringLiteral("4k7"));
    encodeInput_->setPlaceholderText(QStringLiteral("es. 4k7, 220, 1M, 4R7"));
    connect(encodeInput_, &QLineEdit::textChanged, this, &ColorCodeDialog::updateEncode);

    encode4_ = new ResistorWidget;
    encode5_ = new ResistorWidget;
    encode4Text_ = new QLabel;
    encode5Text_ = new QLabel;
    encode4Text_->setWordWrap(true);
    encode5Text_->setWordWrap(true);
    encodeStock_ = new QLabel;
    encodeStock_->setWordWrap(true);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Valore della resistenza:"), encodeInput_);

    auto* box4 = new QGroupBox(QStringLiteral("Resistore a 4 bande (tolleranza oro, ±5%)"));
    auto* l4 = new QVBoxLayout(box4);
    l4->addWidget(encode4_);
    l4->addWidget(encode4Text_);

    auto* box5 = new QGroupBox(QStringLiteral("Resistore a 5 bande (tolleranza marrone, ±1%)"));
    auto* l5 = new QVBoxLayout(box5);
    l5->addWidget(encode5_);
    l5->addWidget(encode5Text_);

    auto* layout = new QVBoxLayout(page);
    layout->addLayout(form);
    layout->addWidget(box4);
    layout->addWidget(box5);
    layout->addWidget(encodeStock_);
    layout->addStretch();
    return page;
}

void ColorCodeDialog::setBandCount(int count) {
    static const std::vector<std::vector<const char*>> kRoles = {
        {"1ª cifra", "2ª cifra", "Moltiplicatore"},
        {"1ª cifra", "2ª cifra", "Moltiplicatore", "Tolleranza"},
        {"1ª cifra", "2ª cifra", "3ª cifra", "Moltiplicatore", "Tolleranza"},
    };
    static const std::vector<std::vector<const char*>> kDefaults = {
        {"giallo", "viola", "rosso"},
        {"giallo", "viola", "rosso", "oro"},
        {"giallo", "viola", "nero", "marrone", "marrone"},
    };
    const auto& roles = kRoles[static_cast<std::size_t>(count - 3)];
    const auto& defaults = kDefaults[static_cast<std::size_t>(count - 3)];

    for (int i = 0; i < kMaxBands; ++i) {
        const bool visible = i < count;
        bandLabels_[i]->setVisible(visible);
        bandCombos_[i]->setVisible(visible);
        if (!visible) continue;
        bandLabels_[i]->setText(QString::fromUtf8(roles[static_cast<std::size_t>(i)]));
        const QSignalBlocker blocker(bandCombos_[i]);
        bandCombos_[i]->setCurrentIndex(bandCombos_[i]->findData(QString::fromUtf8(defaults[static_cast<std::size_t>(i)])));
    }
    updateDecode();
}

void ColorCodeDialog::updateDecode() {
    const int count = bandCount_->currentData().toInt();
    std::vector<std::string> bands;
    for (int i = 0; i < count; ++i) bands.push_back(toStd(bandCombos_[i]->currentData().toString()));

    std::vector<std::string> valueBands = bands;
    std::string tolerance;
    if (count >= 4) {
        tolerance = valueBands.back();
        valueBands.pop_back();
    }
    decodeResistor_->setBands(valueBands, tolerance);

    std::string error;
    const auto decoded = colorcode::decode(bands, error);
    if (!decoded) {
        decodeResult_->setStyleSheet(QStringLiteral("color: %1;").arg(theme().dangerFg.name()));
        decodeResult_->setText(QStringLiteral("Combinazione non valida"));
        decodeStock_->setText(toQ(error));
        return;
    }
    decodeResult_->setStyleSheet(QString());
    decodeResult_->setText(toQ(units::formatValue(decoded->ohms, "Ω") + "  " + decoded->tolerance));
    decodeStock_->setText(stockText(decoded->ohms));
}

void ColorCodeDialog::updateEncode() {
    const auto ohms = units::parseValue(toStd(encodeInput_->text()));
    if (!ohms || *ohms <= 0) {
        encode4_->clear();
        encode5_->clear();
        encode4Text_->setText(encodeInput_->text().trimmed().isEmpty() ? QString()
                                                                         : QStringLiteral("Valore non riconosciuto."));
        encode5Text_->clear();
        encodeStock_->clear();
        return;
    }

    auto show = [&](int digits, ResistorWidget* widget, QLabel* label, const char* toleranceBand) {
        std::string error;
        const auto encoded = colorcode::encode(*ohms, digits, error);
        if (!encoded) {
            widget->clear();
            label->setText(QStringLiteral("Non rappresentabile: %1.").arg(toQ(error)));
            return;
        }
        widget->setBands(encoded->bands, toleranceBand);
        QString textLine = QStringLiteral("<b>%1</b> – %2").arg(joinBands(encoded->bands), toQ(toleranceBand));
        if (!units::approxEqual(encoded->ohms, *ohms)) {
            textLine += QStringLiteral("<br><i>Valore non esatto con %1 cifre: approssimato a %2</i>")
                            .arg(digits)
                            .arg(toQ(units::formatValue(encoded->ohms, "Ω")));
        }
        label->setText(textLine);
    };
    show(2, encode4_, encode4Text_, "oro");
    show(3, encode5_, encode5Text_, "marrone");
    encodeStock_->setText(stockText(*ohms));
}

QString ColorCodeDialog::stockText(double ohms) const {
    SearchFilter filter;
    filter.category = Category::Resistor;
    filter.minValue = ohms;
    filter.maxValue = ohms;
    const auto found = inventory_.search(filter);
    const QString value = toQ(units::formatValue(ohms, "Ω"));
    if (found.empty()) return QStringLiteral("Nessun resistore da %1 in magazzino.").arg(value);

    long long pieces = 0;
    QStringList where;
    for (const Component* c : found) {
        pieces += c->quantity;
        QString entry = QStringLiteral("#%1").arg(c->id);
        if (!c->location.empty()) entry += QStringLiteral(" (%1)").arg(toQ(c->location));
        where << entry;
    }
    return QStringLiteral("In magazzino: %1 pezzi da %2 – %3").arg(pieces).arg(value, where.join(QStringLiteral(", ")));
}
