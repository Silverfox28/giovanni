#include "statisticsdialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "inventory.h"
#include "qtutil.h"
#include "textutil.h"
#include "theme.h"

void showStatisticsDialog(const Inventory& inventory, QWidget* parent) {
    const Statistics s = inventory.statistics();

    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Statistiche del magazzino"));

    auto* form = new QFormLayout;
    auto addRow = [form](const QString& label, const QString& value) {
        auto* v = new QLabel(QStringLiteral("<b>%1</b>").arg(value.toHtmlEscaped()));
        form->addRow(label, v);
    };
    addRow(QStringLiteral("Componenti diversi:"), QString::number(s.items));
    addRow(QStringLiteral("Pezzi totali:"), QString::number(s.pieces));
    addRow(QStringLiteral("Valore stimato:"), toQ(text::formatMoney(s.value)));
    addRow(QStringLiteral("Sotto la scorta minima:"), QString::number(s.lowStock));
    addRow(QStringLiteral("Senza prezzo:"), QString::number(s.withoutPrice));
    addRow(QStringLiteral("Senza posizione:"), QString::number(s.withoutLocation));

    auto* table = new QTableWidget(static_cast<int>(s.byCategory.size()), 4);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("Categoria"), QStringLiteral("Articoli"), QStringLiteral("Pezzi"), QStringLiteral("Valore")});
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->verticalHeader()->setDefaultSectionSize(30);
    table->setAlternatingRowColors(true);
    for (int row = 0; row < static_cast<int>(s.byCategory.size()); ++row) {
        const CategoryStats& cs = s.byCategory[static_cast<std::size_t>(row)];
        const QString cells[] = {toQ(categoryName(cs.category)), QString::number(cs.items),
                                 QString::number(cs.pieces), toQ(text::formatMoney(cs.value))};
        for (int col = 0; col < 4; ++col) {
            auto* item = new QTableWidgetItem(cells[col]);
            if (col == 0) item->setIcon(categoryIcon(cs.category));
            if (col > 0) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(row, col, item);
        }
    }
    table->resizeColumnsToContents();
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("Chiudi"));
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto* layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    layout->addSpacing(8);
    layout->addWidget(new QLabel(QStringLiteral("Per categoria:")));
    layout->addWidget(table);
    layout->addWidget(buttons);
    dialog.resize(480, 600);
    dialog.exec();
}
