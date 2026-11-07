// Modello Qt che presenta un elenco di componenti come tabella.
#pragma once

#include <QAbstractTableModel>

#include <vector>

#include "component.h"

class ComponentModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColId,
        ColCategory,
        ColName,
        ColValue,
        ColPackage,
        ColQuantity,
        ColMinQuantity,
        ColLocation,
        ColSupplier,
        ColPrice,
        ColumnCount,
    };

    explicit ComponentModel(QObject* parent = nullptr);

    // Sostituisce l'elenco mostrato. I puntatori devono restare validi fino alla chiamata successiva.
    void setComponents(std::vector<const Component*> components);

    const Component* componentAt(int row) const;
    int rowOfId(int id) const;  // -1 se non presente

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void sort(int column, Qt::SortOrder order) override;

private:
    void applySort();

    std::vector<const Component*> items_;
    int sortColumn_ = ColCategory;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
};
