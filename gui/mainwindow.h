// Finestra principale della versione grafica del catalogo.
#pragma once

#include <QMainWindow>
#include <QString>

#include "inventory.h"
#include "theme.h"

class ComponentModel;
class QAction;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;
class ResistorWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Apre un archivio CSV (se il file non esiste ne viene creato uno vuoto al primo salvataggio).
    bool openFile(const QString& path);

private:
    void createActions();
    QWidget* createSummaryBar();
    QWidget* createFilterBar();
    QWidget* createDetailPanel();
    void createCentralWidget();
    void applyIcons();
    void showColumnMenu(const QPoint& position);
    void restoreHiddenColumns();
    void setThemeMode(ThemeMode mode);

    void newArchive();
    void openArchive();
    void saveArchiveAs();

    void addComponent();
    void editComponent();
    void duplicateComponent();
    void deleteComponent();
    void changeQuantity(bool withdraw);

    void showColorCode();
    void showStatistics();
    void showLowStock();
    void showAbout();

    void refresh();
    void clearFilters();
    void updateRangeFilter();
    void updateDetails();
    void updateActions();
    void updateSummary();
    void updateTitle();

    bool persist();
    const Component* selectedComponent() const;
    void selectComponent(int id);
    QStringList distinctValues(std::string Component::*field) const;

    Inventory inventory_;
    QString filePath_;
    int currentId_ = 0;  // ID del componente selezionato (0 = nessuno)
    bool clearingFilters_ = false;

    ComponentModel* model_ = nullptr;
    QTableView* table_ = nullptr;

    // Filtri
    QLineEdit* search_ = nullptr;
    QAction* searchIcon_ = nullptr;
    QComboBox* categoryFilter_ = nullptr;
    QLabel* rangeLabel_ = nullptr;
    QLineEdit* minValue_ = nullptr;
    QLineEdit* maxValue_ = nullptr;
    QCheckBox* lowStockOnly_ = nullptr;

    // Riquadri riassuntivi
    QLabel* kpiItems_ = nullptr;
    QLabel* kpiPieces_ = nullptr;
    QLabel* kpiLow_ = nullptr;
    QLabel* kpiValue_ = nullptr;

    // Scheda del componente selezionato
    QLabel* detailTitle_ = nullptr;
    QLabel* detailCategory_ = nullptr;
    QLabel* detailStatus_ = nullptr;
    QLabel* details_ = nullptr;
    ResistorWidget* detailResistor_ = nullptr;
    QWidget* detailButtons_ = nullptr;
    QPushButton* detailWithdraw_ = nullptr;
    QPushButton* detailRestock_ = nullptr;
    QPushButton* detailEdit_ = nullptr;

    QLabel* statusLabel_ = nullptr;

    QAction* actAdd_ = nullptr;
    QAction* actEdit_ = nullptr;
    QAction* actDuplicate_ = nullptr;
    QAction* actDelete_ = nullptr;
    QAction* actWithdraw_ = nullptr;
    QAction* actRestock_ = nullptr;
    QAction* actLowStock_ = nullptr;
    QAction* actStatistics_ = nullptr;
    QAction* actColorCode_ = nullptr;
    QAction* actSaveAs_ = nullptr;
};
