#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
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

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    model_ = new ComponentModel(this);
    createCentralWidget();
    createActions();
    statusLabel_ = new QLabel;
    statusBar()->addWidget(statusLabel_, 1);
    resize(1250, 720);
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
    actAdd_ = compMenu->addAction(QStringLiteral("&Nuovo componente..."), this, &MainWindow::addComponent);
    actAdd_->setShortcut(QKeySequence::New);
    actEdit_ = compMenu->addAction(QStringLiteral("&Modifica..."), this, &MainWindow::editComponent);
    actEdit_->setShortcut(Qt::Key_F2);
    actDuplicate_ = compMenu->addAction(QStringLiteral("&Duplica..."), this, &MainWindow::duplicateComponent);
    actDuplicate_->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    actDuplicate_->setToolTip(QStringLiteral("Crea un nuovo componente partendo da quello selezionato"));
    compMenu->addSeparator();
    actWithdraw_ = compMenu->addAction(QStringLiteral("&Preleva..."), this, [this] { changeQuantity(true); });
    actWithdraw_->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));
    actRestock_ = compMenu->addAction(QStringLiteral("&Carica..."), this, [this] { changeQuantity(false); });
    actRestock_->setShortcut(QKeySequence(QStringLiteral("Ctrl++")));
    compMenu->addSeparator();
    actDelete_ = compMenu->addAction(QStringLiteral("&Elimina"), this, &MainWindow::deleteComponent);
    actDelete_->setShortcut(QKeySequence::Delete);
    compMenu->addSeparator();
    QAction* actFind = compMenu->addAction(QStringLiteral("&Cerca"), this, [this] {
        search_->setFocus();
        search_->selectAll();
    });
    actFind->setShortcut(QKeySequence::Find);

    // --- Menu Strumenti ---
    QMenu* toolsMenu = menuBar()->addMenu(QStringLiteral("&Strumenti"));
    QAction* actLow = toolsMenu->addAction(QStringLiteral("Componenti da &riordinare"), this, &MainWindow::showLowStock);
    QAction* actStats = toolsMenu->addAction(QStringLiteral("&Statistiche..."), this, &MainWindow::showStatistics);
    QAction* actColors = toolsMenu->addAction(QStringLiteral("&Codice colori resistori..."), this, &MainWindow::showColorCode);

    QMenu* helpMenu = menuBar()->addMenu(QStringLiteral("&?"));
    helpMenu->addAction(QStringLiteral("&Informazioni"), this, &MainWindow::showAbout);

    // --- Barra degli strumenti ---
    QToolBar* toolbar = addToolBar(QStringLiteral("Principale"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->addAction(actAdd_);
    toolbar->addAction(actEdit_);
    toolbar->addAction(actDuplicate_);
    toolbar->addAction(actDelete_);
    toolbar->addSeparator();
    toolbar->addAction(actWithdraw_);
    toolbar->addAction(actRestock_);
    toolbar->addSeparator();
    toolbar->addAction(actLow);
    toolbar->addAction(actStats);
    toolbar->addAction(actColors);

    // Menu contestuale della tabella (tasto destro).
    table_->setContextMenuPolicy(Qt::ActionsContextMenu);
    table_->addAction(actEdit_);
    table_->addAction(actDuplicate_);
    table_->addAction(actWithdraw_);
    table_->addAction(actRestock_);
    table_->addAction(actDelete_);
}

void MainWindow::createCentralWidget() {
    // --- Filtri ---
    search_ = new QLineEdit;
    search_->setPlaceholderText(QStringLiteral("Cerca: sigla, valore (es. 4k7), package, posizione, note..."));
    search_->setClearButtonEnabled(true);

    categoryFilter_ = new QComboBox;
    categoryFilter_->addItem(QStringLiteral("Tutte le categorie"), -1);
    const auto& categories = allCategories();
    for (std::size_t i = 0; i < categories.size(); ++i) {
        categoryFilter_->addItem(toQ(categoryName(categories[i])), static_cast<int>(i));
    }

    rangeLabel_ = new QLabel(QStringLiteral("Valore da"));
    minValue_ = new QLineEdit;
    maxValue_ = new QLineEdit;
    for (QLineEdit* e : {minValue_, maxValue_}) {
        e->setMaximumWidth(80);
        e->setClearButtonEnabled(true);
    }
    minValue_->setPlaceholderText(QStringLiteral("min"));
    maxValue_->setPlaceholderText(QStringLiteral("max"));
    lowStockOnly_ = new QCheckBox(QStringLiteral("Solo da riordinare"));

    auto* filters = new QHBoxLayout;
    filters->addWidget(search_, 1);
    filters->addWidget(categoryFilter_);
    filters->addWidget(rangeLabel_);
    filters->addWidget(minValue_);
    filters->addWidget(new QLabel(QStringLiteral("a")));
    filters->addWidget(maxValue_);
    filters->addWidget(lowStockOnly_);

    // --- Tabella ---
    table_ = new QTableView;
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->hide();
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSortIndicator(ComponentModel::ColCategory, Qt::AscendingOrder);
    table_->setSortingEnabled(true);
    table_->setWordWrap(false);

    // --- Scheda del componente selezionato ---
    details_ = new QLabel;
    details_->setWordWrap(true);
    details_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    details_->setTextFormat(Qt::RichText);
    details_->setOpenExternalLinks(true);
    details_->setTextInteractionFlags(Qt::TextBrowserInteraction);
    detailResistor_ = new ResistorWidget;

    auto* detailPanel = new QWidget;
    auto* detailLayout = new QVBoxLayout(detailPanel);
    detailLayout->addWidget(details_);
    detailLayout->addWidget(detailResistor_);
    detailLayout->addStretch();
    auto* detailScroll = new QScrollArea;
    detailScroll->setWidget(detailPanel);
    detailScroll->setWidgetResizable(true);
    detailScroll->setMinimumWidth(280);

    auto* splitter = new QSplitter;
    splitter->addWidget(table_);
    splitter->addWidget(detailScroll);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({900, 340});

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    layout->addLayout(filters);
    layout->addWidget(splitter, 1);
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
    refresh();
    table_->resizeColumnsToContents();
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
    refresh();
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
        updateStatusBar();
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
    const QSignalBlocker b1(search_), b2(categoryFilter_), b3(minValue_), b4(maxValue_), b5(lowStockOnly_);
    search_->clear();
    categoryFilter_->setCurrentIndex(0);
    minValue_->clear();
    maxValue_->clear();
    lowStockOnly_->setChecked(false);
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
        edit->setStyleSheet(v ? QString() : QStringLiteral("color: #c0392b;"));
        return v;
    };
    filter.minValue = readBound(minValue_);
    filter.maxValue = readBound(maxValue_);

    model_->setComponents(inventory_.search(filter));
    updateDetails();
    updateActions();
    updateStatusBar();
}

void MainWindow::updateDetails() {
    const Component* c = selectedComponent();
    if (!c) {
        details_->setText(inventory_.empty()
                              ? QStringLiteral("<p>L'archivio è vuoto.</p><p>Usa <b>Nuovo componente</b> per "
                                               "inserire il primo componente.</p>")
                              : QStringLiteral("<p style='color:gray'>Seleziona un componente per vederne la "
                                               "scheda.</p>"));
        detailResistor_->clear();
        detailResistor_->hide();
        return;
    }

    auto row = [](const QString& label, const QString& value) {
        return QStringLiteral("<tr><td style='color:gray; padding-right:10px'>%1</td><td>%2</td></tr>").arg(label, value);
    };
    QString html = QStringLiteral("<h2 style='margin-bottom:0'>%1</h2>").arg(toQ(c->name).toHtmlEscaped());
    html += QStringLiteral("<p style='color:gray; margin-top:2px'>%1 · #%2</p>")
                .arg(toQ(categoryName(c->category)).toHtmlEscaped())
                .arg(c->id);
    if (c->isLowStock()) {
        html += QStringLiteral("<p style='color:#c0392b'><b>Da riordinare</b>: quantità pari o sotto la scorta "
                               "minima.</p>");
    }
    html += QStringLiteral("<table cellspacing='3'>");
    QString value = toQ(c->displayValue()).toHtmlEscaped();
    if (!c->value.empty() && c->displayValue() != c->value) {
        value += QStringLiteral(" <span style='color:gray'>(%1)</span>").arg(toQ(c->value).toHtmlEscaped());
    }
    html += row(QStringLiteral("Valore"), QStringLiteral("<b>%1</b>").arg(value));
    html += row(QStringLiteral("Package"), toQ(c->package).toHtmlEscaped());
    html += row(QStringLiteral("Quantità"), QStringLiteral("<b>%1</b>").arg(c->quantity));
    html += row(QStringLiteral("Scorta minima"), QString::number(c->minQuantity));
    html += row(QStringLiteral("Posizione"), toQ(c->location).toHtmlEscaped());
    html += row(QStringLiteral("Fornitore"), toQ(c->supplier).toHtmlEscaped());
    if (c->unitPrice > 0) {
        html += row(QStringLiteral("Prezzo unitario"), toQ(text::formatDecimalTrim(c->unitPrice, 4) + " €"));
        html += row(QStringLiteral("Valore scorta"), toQ(text::formatMoney(c->stockValue())));
    }
    if (!c->notes.empty()) html += row(QStringLiteral("Note"), linkify(c->notes));

    const auto bands = resistorBands(*c);
    if (!bands.empty()) {
        QStringList names;
        for (const std::string& b : bands) names << toQ(b);
        html += row(QStringLiteral("Codice colori"),
                    names.join(QStringLiteral(" – ")) + QStringLiteral(" + tolleranza"));
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
}

void MainWindow::updateStatusBar() {
    const Statistics s = inventory_.statistics();
    QString status;
    if (model_->rowCount() != s.items) {
        status = QStringLiteral("Visualizzati %1 di %2 componenti").arg(model_->rowCount()).arg(s.items);
    } else {
        status = QStringLiteral("%1 componenti").arg(s.items);
    }
    status += QStringLiteral("  ·  %1 pezzi  ·  %2 da riordinare  ·  valore %3")
                  .arg(s.pieces)
                  .arg(s.lowStock)
                  .arg(toQ(text::formatMoney(s.value)));
    if (!filePath_.isEmpty()) status += QStringLiteral("  ·  %1").arg(QDir::toNativeSeparators(filePath_));
    statusLabel_->setText(status);
}

void MainWindow::updateTitle() {
    const QString name = filePath_.isEmpty() ? QStringLiteral("nessun archivio") : QFileInfo(filePath_).fileName();
    setWindowTitle(QStringLiteral("Catalogo componenti – %1").arg(name));
}
