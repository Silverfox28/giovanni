#include "mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <climits>
#include <set>

#include "colorcode.h"
#include "colorcodedialog.h"
#include "componentdialog.h"
#include "componentmodel.h"
#include "qtutil.h"
#include "resistorwidget.h"
#include "statisticsdialog.h"
#include "textutil.h"
#include "units.h"

namespace {

const char* const kLastFileKey = "ultimoArchivio";
const char* const kHiddenColumnsKey = "colonneNascoste";

// Testo HTML con i link (http://, https://) resi cliccabili.
QString linkify(const std::string& raw) {
    QStringList parts;
    for (const QString& word : toQ(raw).split(QLatin1Char(' '))) {
        if (word.startsWith(QLatin1String("http://")) || word.startsWith(QLatin1String("https://"))) {
            parts << QStringLiteral("<a href=\"%1\">%2</a>").arg(word.toHtmlEscaped(), word.toHtmlEscaped());
        } else {
            parts << word.toHtmlEscaped();
        }
    }
    return parts.join(QLatin1Char(' '));
}

// Bande del valore di un resistore (2 cifre se basta, altrimenti 3); vuoto se non rappresentabile.
std::vector<std::string> resistorBands(const Component& c) {
    if (c.category != Category::Resistor) return {};
    const auto ohms = c.numericValue();
    if (!ohms || *ohms <= 0) return {};
    std::string error;
    for (int digits : {2, 3}) {
        const auto encoded = colorcode::encode(*ohms, digits, error);
        if (encoded && units::approxEqual(encoded->ohms, *ohms)) return encoded->bands;
    }
    return {};
}

QString ensureCsvSuffix(QString path) {
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".csv");
    return path;
}

QFrame* makeCard() {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("card"));
    return card;
}

// Riquadro riassuntivo con un titolo piccolo e un numero grande.
QFrame* makeKpi(const QString& title, QLabel*& valueLabel) {
    QFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(0);
    auto* titleLabel = new QLabel(title.toUpper());
    titleLabel->setObjectName(QStringLiteral("kpiTitle"));
    valueLabel = new QLabel(QStringLiteral("0"));
    valueLabel->setObjectName(QStringLiteral("kpiValue"));
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    return card;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    model_ = new ComponentModel(this);
    createCentralWidget();
    createActions();
    applyIcons();
    statusLabel_ = new QLabel;
    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->setSizeGripEnabled(false);
    resize(1320, 780);
    refresh();
}

void MainWindow::createActions() {
    // --- Menu File ---
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&Nuovo archivio..."), this, &MainWindow::newArchive);
    QAction* actOpen = fileMenu->addAction(QStringLiteral("&Apri archivio..."), this, &MainWindow::openArchive);
    actOpen->setShortcut(QKeySequence::Open);
    actSaveAs_ = fileMenu->addAction(QStringLiteral("&Salva una copia con nome..."), this, &MainWindow::saveArchiveAs);
    fileMenu->addSeparator();
    QAction* actQuit = fileMenu->addAction(QStringLiteral("&Esci"), this, &QWidget::close);
    actQuit->setShortcut(QKeySequence::Quit);

    // --- Menu Componenti ---
    QMenu* compMenu = menuBar()->addMenu(QStringLiteral("&Componenti"));
    actAdd_ = compMenu->addAction(QStringLiteral("Nuovo"), this, &MainWindow::addComponent);
    actAdd_->setShortcut(QKeySequence::New);
    actAdd_->setToolTip(QStringLiteral("Aggiungi un nuovo componente (Ctrl+N)"));
    actEdit_ = compMenu->addAction(QStringLiteral("Modifica"), this, &MainWindow::editComponent);
    actEdit_->setShortcut(Qt::Key_F2);
    actEdit_->setToolTip(QStringLiteral("Modifica il componente selezionato (F2 o doppio clic)"));
    actDuplicate_ = compMenu->addAction(QStringLiteral("Duplica"), this, &MainWindow::duplicateComponent);
    actDuplicate_->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    actDuplicate_->setToolTip(QStringLiteral("Crea un nuovo componente partendo da quello selezionato"));
    compMenu->addSeparator();
    actWithdraw_ = compMenu->addAction(QStringLiteral("Preleva"), this, [this] { changeQuantity(true); });
    actWithdraw_->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));
    actWithdraw_->setToolTip(QStringLiteral("Togli dei pezzi dal magazzino"));
    actRestock_ = compMenu->addAction(QStringLiteral("Carica"), this, [this] { changeQuantity(false); });
    actRestock_->setShortcut(QKeySequence(QStringLiteral("Ctrl++")));
    actRestock_->setToolTip(QStringLiteral("Aggiungi dei pezzi al magazzino"));
    compMenu->addSeparator();
    actDelete_ = compMenu->addAction(QStringLiteral("Elimina"), this, &MainWindow::deleteComponent);
    actDelete_->setShortcut(QKeySequence::Delete);
    actDelete_->setToolTip(QStringLiteral("Elimina il componente selezionato"));
    compMenu->addSeparator();
    QAction* actFind = compMenu->addAction(QStringLiteral("Cerca"), this, [this] {
        search_->setFocus();
        search_->selectAll();
    });
    actFind->setShortcut(QKeySequence::Find);

    // --- Menu Strumenti ---
    QMenu* toolsMenu = menuBar()->addMenu(QStringLiteral("&Strumenti"));
    actLowStock_ = toolsMenu->addAction(QStringLiteral("Da riordinare"), this, &MainWindow::showLowStock);
    actLowStock_->setToolTip(QStringLiteral("Mostra solo i componenti sotto la scorta minima"));
    actStatistics_ = toolsMenu->addAction(QStringLiteral("Statistiche"), this, &MainWindow::showStatistics);
    actColorCode_ = toolsMenu->addAction(QStringLiteral("Codice colori"), this, &MainWindow::showColorCode);
    actColorCode_->setToolTip(QStringLiteral("Codice colori dei resistori"));

    // --- Menu Visualizza: scelta del tema ---
    QMenu* viewMenu = menuBar()->addMenu(QStringLiteral("&Visualizza"));
    QMenu* themeMenu = viewMenu->addMenu(QStringLiteral("Tema"));
    auto* themeGroup = new QActionGroup(this);
    const std::pair<ThemeMode, QString> themes[] = {
        {ThemeMode::Light, QStringLiteral("Chiaro")},
        {ThemeMode::Dark, QStringLiteral("Scuro")},
        {ThemeMode::System, QStringLiteral("Come il sistema")},
    };
    const ThemeMode savedMode = loadThemeMode();
    for (const auto& [mode, label] : themes) {
        QAction* action = themeMenu->addAction(label);
        action->setCheckable(true);
        action->setChecked(mode == savedMode);
        themeGroup->addAction(action);
        const ThemeMode m = mode;
        connect(action, &QAction::triggered, this, [this, m] { setThemeMode(m); });
    }

    QMenu* helpMenu = menuBar()->addMenu(QStringLiteral("&?"));
    helpMenu->addAction(QStringLiteral("&Informazioni"), this, &MainWindow::showAbout);

    // --- Barra degli strumenti ---
    QToolBar* toolbar = addToolBar(QStringLiteral("Principale"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->addAction(actAdd_);
    toolbar->addAction(actEdit_);
    toolbar->addAction(actDuplicate_);
    toolbar->addAction(actDelete_);
    toolbar->addSeparator();
    toolbar->addAction(actWithdraw_);
    toolbar->addAction(actRestock_);
    toolbar->addSeparator();
    toolbar->addAction(actLowStock_);
    toolbar->addAction(actStatistics_);
    toolbar->addAction(actColorCode_);

    // Menu contestuale della tabella (tasto destro).
    table_->setContextMenuPolicy(Qt::ActionsContextMenu);
    table_->addAction(actEdit_);
    table_->addAction(actDuplicate_);
    table_->addAction(actWithdraw_);
    table_->addAction(actRestock_);
    table_->addAction(actDelete_);

    connect(detailWithdraw_, &QPushButton::clicked, actWithdraw_, &QAction::trigger);
    connect(detailRestock_, &QPushButton::clicked, actRestock_, &QAction::trigger);
    connect(detailEdit_, &QPushButton::clicked, actEdit_, &QAction::trigger);
}

void MainWindow::applyIcons() {
    const ThemeColors& t = theme();
    actAdd_->setIcon(makeIcon(IconKind::Add, t.accent));
    actEdit_->setIcon(makeIcon(IconKind::Edit, t.text));
    actDuplicate_->setIcon(makeIcon(IconKind::Duplicate, t.text));
    actDelete_->setIcon(makeIcon(IconKind::Delete, t.dangerFg));
    actWithdraw_->setIcon(makeIcon(IconKind::Withdraw, t.text));
    actRestock_->setIcon(makeIcon(IconKind::Restock, t.text));
    actLowStock_->setIcon(makeIcon(IconKind::Reorder, t.dangerFg));
    actStatistics_->setIcon(makeIcon(IconKind::Statistics, t.text));
    actColorCode_->setIcon(makeIcon(IconKind::ColorCode, t.text));
    searchIcon_->setIcon(makeIcon(IconKind::Search, t.muted));
    detailWithdraw_->setIcon(makeIcon(IconKind::Withdraw, t.text));
    detailRestock_->setIcon(makeIcon(IconKind::Restock, t.text));
    detailEdit_->setIcon(makeIcon(IconKind::Edit, t.accentText));
}

void MainWindow::restoreHiddenColumns() {
    // Per impostazione predefinita fornitore e prezzo si vedono solo nella scheda a destra.
    const QVariant saved = QSettings().value(kHiddenColumnsKey);
    QList<int> hidden = {ComponentModel::ColSupplier, ComponentModel::ColPrice};
    if (saved.isValid()) {
        hidden.clear();
        for (const QVariant& v : saved.toList()) hidden << v.toInt();
    }
    for (int col = 0; col < ComponentModel::ColumnCount; ++col) {
        table_->setColumnHidden(col, hidden.contains(col) && col != ComponentModel::ColName);
    }
}

void MainWindow::showColumnMenu(const QPoint& position) {
    QMenu menu(this);
    menu.addSection(QStringLiteral("Colonne visibili"));
    for (int col = 0; col < ComponentModel::ColumnCount; ++col) {
        QAction* action = menu.addAction(model_->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString());
        action->setCheckable(true);
        action->setChecked(!table_->isColumnHidden(col));
        action->setEnabled(col != ComponentModel::ColName);  // la sigla resta sempre visibile
        connect(action, &QAction::toggled, this, [this, col](bool visible) {
            table_->setColumnHidden(col, !visible);
            QVariantList hidden;
            for (int c = 0; c < ComponentModel::ColumnCount; ++c) {
                if (table_->isColumnHidden(c)) hidden << c;
            }
            QSettings().setValue(kHiddenColumnsKey, hidden);
        });
    }
    menu.exec(table_->horizontalHeader()->viewport()->mapToGlobal(position));
}

void MainWindow::setThemeMode(ThemeMode mode) {
    saveThemeMode(mode);
    applyTheme(mode);
    applyIcons();
    model_->refreshAppearance();
    updateSummary();
    updateDetails();
}

QWidget* MainWindow::createSummaryBar() {
    auto* bar = new QWidget;
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(makeKpi(QStringLiteral("Componenti"), kpiItems_));
    layout->addWidget(makeKpi(QStringLiteral("Pezzi in magazzino"), kpiPieces_));
    layout->addWidget(makeKpi(QStringLiteral("Da riordinare"), kpiLow_));
    layout->addWidget(makeKpi(QStringLiteral("Valore stimato"), kpiValue_));
    return bar;
}

QWidget* MainWindow::createFilterBar() {
    search_ = new QLineEdit;
    search_->setPlaceholderText(QStringLiteral("Cerca per sigla, valore (es. 4k7), package, posizione, note..."));
    search_->setClearButtonEnabled(true);
    searchIcon_ = search_->addAction(QIcon(), QLineEdit::LeadingPosition);

    categoryFilter_ = new QComboBox;
    categoryFilter_->addItem(QStringLiteral("Tutte le categorie"), -1);
    const auto& categories = allCategories();
    for (std::size_t i = 0; i < categories.size(); ++i) {
        categoryFilter_->addItem(categoryIcon(categories[i]), toQ(categoryName(categories[i])), static_cast<int>(i));
    }

    rangeLabel_ = new QLabel(QStringLiteral("Valore da"));
    minValue_ = new QLineEdit;
    maxValue_ = new QLineEdit;
    for (QLineEdit* e : {minValue_, maxValue_}) {
        e->setFixedWidth(90);
        e->setClearButtonEnabled(true);
    }
    minValue_->setPlaceholderText(QStringLiteral("min"));
    maxValue_->setPlaceholderText(QStringLiteral("max"));
    lowStockOnly_ = new QCheckBox(QStringLiteral("Solo da riordinare"));

    QFrame* card = makeCard();
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);
    layout->addWidget(search_, 1);
    layout->addWidget(categoryFilter_);
    layout->addSpacing(6);
    layout->addWidget(rangeLabel_);
    layout->addWidget(minValue_);
    layout->addWidget(new QLabel(QStringLiteral("a")));
    layout->addWidget(maxValue_);
    layout->addSpacing(6);
    layout->addWidget(lowStockOnly_);
    return card;
}

QWidget* MainWindow::createDetailPanel() {
    QFrame* card = makeCard();
    card->setFixedWidth(340);

    detailTitle_ = new QLabel;
    detailTitle_->setObjectName(QStringLiteral("detailTitle"));
    detailTitle_->setWordWrap(true);

    detailCategory_ = new QLabel;
    detailStatus_ = new QLabel;
    auto* pills = new QHBoxLayout;
    pills->setSpacing(6);
    pills->addWidget(detailCategory_);
    pills->addWidget(detailStatus_);
    pills->addStretch();

    details_ = new QLabel;
    details_->setWordWrap(true);
    details_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    details_->setTextFormat(Qt::RichText);
    details_->setOpenExternalLinks(true);
    details_->setTextInteractionFlags(Qt::TextBrowserInteraction);

    detailResistor_ = new ResistorWidget;

    detailWithdraw_ = new QPushButton(QStringLiteral("Preleva"));
    detailRestock_ = new QPushButton(QStringLiteral("Carica"));
    detailEdit_ = new QPushButton(QStringLiteral("Modifica"));
    detailEdit_->setObjectName(QStringLiteral("primary"));
    detailButtons_ = new QWidget;
    auto* buttons = new QHBoxLayout(detailButtons_);
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->addWidget(detailWithdraw_);
    buttons->addWidget(detailRestock_);
    buttons->addWidget(detailEdit_);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    layout->addWidget(detailTitle_);
    layout->addLayout(pills);
    layout->addWidget(details_);
    layout->addWidget(detailResistor_);
    layout->addStretch();
    layout->addWidget(detailButtons_);
    return card;
}

void MainWindow::createCentralWidget() {
    table_ = new QTableView;
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(false);
    table_->setWordWrap(false);
    table_->setIconSize(QSize(12, 12));
    table_->verticalHeader()->hide();
    table_->verticalHeader()->setDefaultSectionSize(32);
    QHeaderView* header = table_->horizontalHeader();
    header->setHighlightSections(false);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ComponentModel::ColName, QHeaderView::Stretch);
    header->setSectionResizeMode(ComponentModel::ColLocation, QHeaderView::Stretch);
    header->setSortIndicator(ComponentModel::ColCategory, Qt::AscendingOrder);
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    header->setToolTip(QStringLiteral("Clic per ordinare, tasto destro per scegliere le colonne"));
    connect(header, &QHeaderView::customContextMenuRequested, this, &MainWindow::showColumnMenu);
    table_->setSortingEnabled(true);
    restoreHiddenColumns();

    auto* content = new QHBoxLayout;
    content->setSpacing(12);
    content->addWidget(table_, 1);
    content->addWidget(createDetailPanel());

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);
    layout->addWidget(createSummaryBar());
    layout->addWidget(createFilterBar());
    layout->addLayout(content, 1);
    setCentralWidget(central);

    // --- Collegamenti ---
    connect(search_, &QLineEdit::textChanged, this, &MainWindow::refresh);
    connect(categoryFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        updateRangeFilter();
        refresh();
    });
    connect(minValue_, &QLineEdit::textChanged, this, &MainWindow::refresh);
    connect(maxValue_, &QLineEdit::textChanged, this, &MainWindow::refresh);
    connect(lowStockOnly_, &QCheckBox::toggled, this, &MainWindow::refresh);

    connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex& current) {
        const Component* c = model_->componentAt(current.row());
        currentId_ = c ? c->id : 0;
        updateDetails();
        updateActions();
    });
    // Dopo ogni aggiornamento della tabella (filtri, ordinamento) si riseleziona lo stesso componente.
    connect(model_, &QAbstractItemModel::modelReset, this, [this] {
        const int id = currentId_;
        selectComponent(id);
        if (model_->rowOfId(id) < 0) {
            currentId_ = 0;
            updateDetails();
            updateActions();
        }
    });
    connect(table_, &QTableView::activated, this, &MainWindow::editComponent);

    updateRangeFilter();
}

bool MainWindow::openFile(const QString& path) {
    Inventory loaded;
    std::vector<std::string> warnings;
    try {
        warnings = loaded.load(toPath(path));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, QStringLiteral("Apertura non riuscita"),
                              QStringLiteral("Impossibile aprire l'archivio:\n%1").arg(toQ(e.what())));
        return false;
    }

    model_->setComponents({});  // la tabella non deve più puntare ai dati vecchi
    inventory_ = std::move(loaded);
    filePath_ = QFileInfo(path).absoluteFilePath();
    currentId_ = 0;
    QSettings().setValue(kLastFileKey, filePath_);
    clearFilters();
    updateTitle();

    if (!warnings.empty()) {
        QStringList lines;
        for (const std::string& w : warnings) lines << toQ(w);
        QMessageBox box(QMessageBox::Warning, QStringLiteral("Avvisi durante l'apertura"),
                        QStringLiteral("Alcune righe del file contenevano dati non validi e sono state corrette o "
                                       "ignorate."),
                        QMessageBox::Ok, this);
        box.setDetailedText(lines.join(QLatin1Char('\n')));
        box.exec();
    }
    return true;
}

void MainWindow::newArchive() {
    const QString dir = filePath_.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                            : QFileInfo(filePath_).absolutePath();
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Nuovo archivio"), dir + "/componenti.csv",
                                                QStringLiteral("Archivio CSV (*.csv)"));
    if (path.isEmpty()) return;
    path = ensureCsvSuffix(path);

    model_->setComponents({});
    inventory_ = Inventory();
    filePath_ = QFileInfo(path).absoluteFilePath();
    currentId_ = 0;
    if (!persist()) return;
    QSettings().setValue(kLastFileKey, filePath_);
    clearFilters();
    updateTitle();
}

void MainWindow::openArchive() {
    const QString dir = filePath_.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                            : QFileInfo(filePath_).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Apri archivio"), dir,
                                                      QStringLiteral("Archivio CSV (*.csv);;Tutti i file (*)"));
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::saveArchiveAs() {
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Salva una copia dell'archivio"),
                                                filePath_.isEmpty() ? QString() : filePath_,
                                                QStringLiteral("Archivio CSV (*.csv)"));
    if (path.isEmpty()) return;
    path = ensureCsvSuffix(path);
    try {
        inventory_.save(toPath(path));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, QStringLiteral("Salvataggio non riuscito"), toQ(e.what()));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Copia salvata"),
                              QStringLiteral("Copia salvata in:\n%1\n\nVuoi continuare a lavorare su questo nuovo file?")
                                  .arg(QDir::toNativeSeparators(path))) == QMessageBox::Yes) {
        filePath_ = QFileInfo(path).absoluteFilePath();
        QSettings().setValue(kLastFileKey, filePath_);
        updateTitle();
        updateSummary();
    }
}

bool MainWindow::persist() {
    if (filePath_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Nessun archivio"),
                             QStringLiteral("Apri o crea un archivio dal menu File per salvare i dati."));
        return false;
    }
    try {
        inventory_.save(toPath(filePath_));
        return true;
    } catch (const std::exception& e) {
        QMessageBox::critical(this, QStringLiteral("Salvataggio non riuscito"),
                              QStringLiteral("Le modifiche NON sono state salvate:\n%1").arg(toQ(e.what())));
        return false;
    }
}

QStringList MainWindow::distinctValues(std::string Component::*field) const {
    std::set<QString> values;
    for (const Component* c : inventory_.all()) {
        const std::string& v = c->*field;
        if (!text::trim(v).empty()) values.insert(toQ(v));
    }
    QStringList list(values.begin(), values.end());
    list.sort(Qt::CaseInsensitive);
    return list;
}

const Component* MainWindow::selectedComponent() const {
    return currentId_ > 0 ? inventory_.findById(currentId_) : nullptr;
}

void MainWindow::selectComponent(int id) {
    const int row = model_->rowOfId(id);
    if (row < 0) return;
    const QModelIndex index = model_->index(row, ComponentModel::ColName);
    table_->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    table_->scrollTo(index);
}

void MainWindow::addComponent() {
    Component initial;
    if (const int idx = categoryFilter_->currentData().toInt(); idx >= 0) {
        initial.category = allCategories()[static_cast<std::size_t>(idx)];
    }
    ComponentDialog dialog(QStringLiteral("Nuovo componente"), initial, distinctValues(&Component::location),
                           distinctValues(&Component::supplier), this);
    if (dialog.exec() != QDialog::Accepted) return;
    Component c = dialog.component();
    c.id = 0;

    if (const Component* existing = inventory_.findDuplicate(c)) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Componente già presente"),
            QStringLiteral("In archivio c'è già questo componente (#%1, %2 pezzi).\n\n"
                           "Vuoi sommare %3 pezzi a quello esistente invece di creare un doppione?")
                .arg(existing->id)
                .arg(existing->quantity)
                .arg(c.quantity),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (answer == QMessageBox::Cancel) return;
        if (answer == QMessageBox::Yes) {
            const int id = existing->id;
            inventory_.findById(id)->quantity += c.quantity;
            currentId_ = id;
            refresh();
            persist();
            return;
        }
    }

    currentId_ = inventory_.add(std::move(c)).id;
    refresh();
    persist();
}

void MainWindow::editComponent() {
    const Component* selected = selectedComponent();
    if (!selected) return;
    ComponentDialog dialog(QStringLiteral("Modifica componente #%1").arg(selected->id), *selected,
                           distinctValues(&Component::location), distinctValues(&Component::supplier), this);
    if (dialog.exec() != QDialog::Accepted) return;

    *inventory_.findById(selected->id) = dialog.component();
    refresh();
    persist();
}

void MainWindow::duplicateComponent() {
    const Component* selected = selectedComponent();
    if (!selected) return;
    Component copy = *selected;
    copy.id = 0;
    copy.quantity = 0;
    ComponentDialog dialog(QStringLiteral("Nuovo componente (copia di #%1)").arg(selected->id), copy,
                           distinctValues(&Component::location), distinctValues(&Component::supplier), this);
    if (dialog.exec() != QDialog::Accepted) return;
    Component c = dialog.component();
    c.id = 0;
    if (inventory_.findDuplicate(c)) {
        if (QMessageBox::question(this, QStringLiteral("Componente già presente"),
                                  QStringLiteral("Esiste già un componente con gli stessi dati. Crearlo comunque?")) !=
            QMessageBox::Yes) {
            return;
        }
    }
    currentId_ = inventory_.add(std::move(c)).id;
    refresh();
    persist();
}

void MainWindow::deleteComponent() {
    const Component* selected = selectedComponent();
    if (!selected) return;
    const int id = selected->id;
    const QString description = QStringLiteral("#%1 %2 %3").arg(id).arg(toQ(selected->name), toQ(selected->displayValue()));
    QMessageBox box(QMessageBox::Question, QStringLiteral("Elimina componente"),
                    QStringLiteral("Eliminare definitivamente il componente\n%1?").arg(description.trimmed()),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setDefaultButton(QMessageBox::No);
    box.button(QMessageBox::Yes)->setText(QStringLiteral("Elimina"));
    box.button(QMessageBox::No)->setText(QStringLiteral("Annulla"));
    if (box.exec() != QMessageBox::Yes) return;

    // Prima si seleziona il componente successivo, poi si elimina.
    const int row = model_->rowOfId(id);
    const Component* next = model_->componentAt(row + 1) ? model_->componentAt(row + 1) : model_->componentAt(row - 1);
    currentId_ = next ? next->id : 0;
    inventory_.remove(id);
    refresh();
    persist();
}

void MainWindow::changeQuantity(bool withdraw) {
    const Component* selected = selectedComponent();
    if (!selected) return;
    const int id = selected->id;
    const int available = selected->quantity;
    const QString what = QStringLiteral("%1 %2").arg(toQ(selected->name), toQ(selected->displayValue())).trimmed();

    if (withdraw && available == 0) {
        QMessageBox::information(this, QStringLiteral("Preleva"), QStringLiteral("%1 è esaurito.").arg(what));
        return;
    }

    bool ok = false;
    const int amount =
        withdraw ? QInputDialog::getInt(this, QStringLiteral("Preleva"),
                                        QStringLiteral("%1\nDisponibili: %2\n\nQuanti pezzi prelevi?").arg(what).arg(available),
                                        1, 1, available, 1, &ok)
                 : QInputDialog::getInt(this, QStringLiteral("Carica"),
                                        QStringLiteral("%1\nDisponibili: %2\n\nQuanti pezzi aggiungi?").arg(what).arg(available),
                                        1, 1, INT_MAX - available, 1, &ok);
    if (!ok) return;

    Component* c = inventory_.findById(id);
    c->quantity += withdraw ? -amount : amount;
    const bool low = c->isLowStock();
    const int quantity = c->quantity;
    const int minimum = c->minQuantity;
    refresh();
    if (persist() && withdraw && low) {
        QMessageBox::warning(this, QStringLiteral("Scorta in esaurimento"),
                             QStringLiteral("%1: restano %2 pezzi (scorta minima %3).\nRicordati di riordinarlo.")
                                 .arg(what)
                                 .arg(quantity)
                                 .arg(minimum));
    }
}

void MainWindow::showColorCode() {
    ColorCodeDialog dialog(inventory_, this);
    dialog.exec();
}

void MainWindow::showStatistics() {
    showStatisticsDialog(inventory_, this);
}

void MainWindow::showLowStock() {
    clearFilters();
    lowStockOnly_->setChecked(true);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, QStringLiteral("Informazioni"),
                       QStringLiteral("<h3>Catalogo componenti elettronici</h3>"
                                      "<p>Archivio dei componenti del laboratorio, con ricerca per valore, "
                                      "gestione delle scorte e codice colori dei resistori.</p>"
                                      "<p>I dati sono salvati automaticamente in un file CSV, apribile anche con "
                                      "Excel o con la versione da terminale del programma.</p>"));
}

void MainWindow::clearFilters() {
    // I segnali non vengono bloccati (servono alle caselle per aggiornarsi): si sospende solo l'aggiornamento.
    clearingFilters_ = true;
    search_->clear();
    categoryFilter_->setCurrentIndex(0);
    minValue_->clear();
    maxValue_->clear();
    lowStockOnly_->setChecked(false);
    clearingFilters_ = false;
    updateRangeFilter();
    refresh();
}

void MainWindow::updateRangeFilter() {
    const int idx = categoryFilter_->currentData().toInt();
    const std::string unit = idx >= 0 ? categoryUnit(allCategories()[static_cast<std::size_t>(idx)]) : "";
    const bool enabled = !unit.empty();
    rangeLabel_->setEnabled(enabled);
    minValue_->setEnabled(enabled);
    maxValue_->setEnabled(enabled);
    const QString tip = enabled ? QStringLiteral("Valori in %1, es. 1k, 100n, 4u7").arg(toQ(unit))
                                : QStringLiteral("Scegli Resistore, Condensatore o Induttore per filtrare per valore");
    minValue_->setToolTip(tip);
    maxValue_->setToolTip(tip);
}

void MainWindow::refresh() {
    if (clearingFilters_) return;
    SearchFilter filter;
    filter.query = toStd(search_->text());
    if (const int idx = categoryFilter_->currentData().toInt(); idx >= 0) {
        filter.category = allCategories()[static_cast<std::size_t>(idx)];
    }
    filter.onlyLowStock = lowStockOnly_->isChecked();

    auto readBound = [](QLineEdit* edit) -> std::optional<double> {
        const QString t = edit->text().trimmed();
        if (!edit->isEnabled() || t.isEmpty()) {
            edit->setStyleSheet(QString());
            return std::nullopt;
        }
        const auto v = units::parseValue(toStd(t));
        edit->setStyleSheet(v ? QString() : QStringLiteral("color: %1;").arg(theme().dangerFg.name()));
        return v;
    };
    filter.minValue = readBound(minValue_);
    filter.maxValue = readBound(maxValue_);

    model_->setComponents(inventory_.search(filter));
    updateDetails();
    updateActions();
    updateSummary();
}

void MainWindow::updateDetails() {
    const ThemeColors& t = theme();
    const Component* c = selectedComponent();
    if (!c) {
        detailTitle_->setText(inventory_.empty() ? QStringLiteral("Archivio vuoto")
                                                 : QStringLiteral("Nessun componente selezionato"));
        detailCategory_->hide();
        detailStatus_->hide();
        details_->setText(
            QStringLiteral("<p style='color:%1'>%2</p>")
                .arg(t.muted.name(),
                     inventory_.empty() ? QStringLiteral("Premi <b>Nuovo</b> per inserire il primo componente.")
                                        : QStringLiteral("Seleziona un componente nella tabella per vederne la "
                                                         "scheda.")));
        detailResistor_->hide();
        detailButtons_->hide();
        return;
    }

    detailTitle_->setText(toQ(c->name));

    const QColor catColor = categoryColor(c->category);
    detailCategory_->setText(toQ(categoryName(c->category)) + QStringLiteral("  #") + QString::number(c->id));
    detailCategory_->setStyleSheet(pillStyle(t.dark ? catColor.darker(160) : catColor.lighter(175),
                                             t.dark ? catColor.lighter(150) : catColor.darker(150)));
    detailCategory_->show();

    if (c->quantity == 0) {
        detailStatus_->setText(QStringLiteral("Esaurito"));
        detailStatus_->setStyleSheet(pillStyle(t.dangerBg, t.dangerFg));
    } else if (c->isLowStock()) {
        detailStatus_->setText(QStringLiteral("Da riordinare"));
        detailStatus_->setStyleSheet(pillStyle(t.dangerBg, t.dangerFg));
    } else {
        detailStatus_->setText(QStringLiteral("Disponibile"));
        detailStatus_->setStyleSheet(pillStyle(t.successBg, t.successFg));
    }
    detailStatus_->show();

    const QString labelStyle = QStringLiteral("color:%1; padding: 3px 14px 3px 0").arg(t.muted.name());
    auto row = [&labelStyle](const QString& label, const QString& value) {
        return QStringLiteral("<tr><td style='%1'>%2</td><td style='padding: 3px 0'>%3</td></tr>")
            .arg(labelStyle, label, value);
    };
    QString value = QStringLiteral("<b>%1</b>").arg(toQ(c->displayValue()).toHtmlEscaped());
    if (!c->value.empty() && c->displayValue() != c->value) {
        value += QStringLiteral(" <span style='color:%1'>(%2)</span>").arg(t.muted.name(), toQ(c->value).toHtmlEscaped());
    }
    const QString quantityColor = c->isLowStock() ? t.dangerFg.name() : t.text.name();

    QString html = QStringLiteral("<table cellspacing='0'>");
    html += row(QStringLiteral("Valore"), value);
    html += row(QStringLiteral("Package"), toQ(c->package).toHtmlEscaped());
    html += row(QStringLiteral("Quantità"),
                QStringLiteral("<b style='color:%1'>%2</b> <span style='color:%3'>(minimo %4)</span>")
                    .arg(quantityColor)
                    .arg(c->quantity)
                    .arg(t.muted.name())
                    .arg(c->minQuantity));
    html += row(QStringLiteral("Posizione"), toQ(c->location).toHtmlEscaped());
    if (!c->supplier.empty()) html += row(QStringLiteral("Fornitore"), toQ(c->supplier).toHtmlEscaped());
    if (c->unitPrice > 0) {
        html += row(QStringLiteral("Prezzo"), toQ(text::formatDecimalTrim(c->unitPrice, 4) + " €") +
                                                  QStringLiteral(" <span style='color:%1'>(scorta %2)</span>")
                                                      .arg(t.muted.name(), toQ(text::formatMoney(c->stockValue()))));
    }
    if (!c->notes.empty()) html += row(QStringLiteral("Note"), linkify(c->notes));

    const auto bands = resistorBands(*c);
    if (!bands.empty()) {
        QStringList names;
        for (const std::string& b : bands) names << toQ(b);
        html += row(QStringLiteral("Colori"), names.join(QStringLiteral(" – ")));
    }
    html += QStringLiteral("</table>");
    details_->setText(html);

    if (bands.empty()) {
        detailResistor_->clear();
        detailResistor_->hide();
    } else {
        detailResistor_->setBands(bands);
        detailResistor_->show();
    }
    detailButtons_->show();
}

void MainWindow::updateActions() {
    const Component* c = selectedComponent();
    const bool hasFile = !filePath_.isEmpty();
    actAdd_->setEnabled(hasFile);
    actEdit_->setEnabled(hasFile && c);
    actDuplicate_->setEnabled(hasFile && c);
    actDelete_->setEnabled(hasFile && c);
    actWithdraw_->setEnabled(hasFile && c && c->quantity > 0);
    actRestock_->setEnabled(hasFile && c);
    actSaveAs_->setEnabled(hasFile);
    detailWithdraw_->setEnabled(actWithdraw_->isEnabled());
    detailRestock_->setEnabled(actRestock_->isEnabled());
    detailEdit_->setEnabled(actEdit_->isEnabled());
}

void MainWindow::updateSummary() {
    const Statistics s = inventory_.statistics();
    kpiItems_->setText(QString::number(s.items));
    kpiPieces_->setText(QString::number(s.pieces));
    kpiLow_->setText(QString::number(s.lowStock));
    kpiLow_->setStyleSheet(s.lowStock > 0 ? QStringLiteral("color: %1;").arg(theme().dangerFg.name()) : QString());
    kpiValue_->setText(toQ(text::formatMoney(s.value)));

    QString status;
    if (model_->rowCount() != s.items) {
        status = QStringLiteral("Visualizzati %1 di %2 componenti").arg(model_->rowCount()).arg(s.items);
    } else {
        status = QStringLiteral("%1 componenti").arg(s.items);
    }
    if (!filePath_.isEmpty()) status += QStringLiteral("   ·   Archivio: %1").arg(QDir::toNativeSeparators(filePath_));
    statusLabel_->setText(status);
}

void MainWindow::updateTitle() {
    const QString name = filePath_.isEmpty() ? QStringLiteral("nessun archivio") : QFileInfo(filePath_).fileName();
    setWindowTitle(QStringLiteral("Catalogo componenti – %1").arg(name));
}
