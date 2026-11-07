#include "componentmodel.h"

#include <QBrush>
#include <QColor>
#include <QFont>

#include <algorithm>

#include "inventory.h"
#include "qtutil.h"
#include "textutil.h"

ComponentModel::ComponentModel(QObject* parent) : QAbstractTableModel(parent) {}

void ComponentModel::setComponents(std::vector<const Component*> components) {
    beginResetModel();
    items_ = std::move(components);
    applySort();
    endResetModel();
}

const Component* ComponentModel::componentAt(int row) const {
    if (row < 0 || row >= static_cast<int>(items_.size())) return nullptr;
    return items_[static_cast<std::size_t>(row)];
}

int ComponentModel::rowOfId(int id) const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i]->id == id) return static_cast<int>(i);
    }
    return -1;
}

int ComponentModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

int ComponentModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ComponentModel::data(const QModelIndex& index, int role) const {
    const Component* c = componentAt(index.row());
    if (!c) return {};
    const int column = index.column();

    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case ColId: return c->id;
                case ColCategory: return toQ(categoryName(c->category));
                case ColName: return toQ(c->name);
                case ColValue: return toQ(c->displayValue());
                case ColPackage: return toQ(c->package);
                case ColQuantity: return c->quantity;
                case ColMinQuantity: return c->minQuantity;
                case ColLocation: return toQ(c->location);
                case ColSupplier: return toQ(c->supplier);
                case ColPrice:
                    return c->unitPrice > 0 ? toQ(text::formatDecimalTrim(c->unitPrice, 4) + " €") : QString();
            }
            break;

        case Qt::TextAlignmentRole:
            if (column == ColId || column == ColQuantity || column == ColMinQuantity || column == ColPrice) {
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            }
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);

        case Qt::BackgroundRole:
            if (c->isLowStock()) return QBrush(QColor(255, 226, 222));
            break;

        case Qt::ForegroundRole:
            if (c->isLowStock()) return QBrush(QColor(140, 20, 20));
            break;

        case Qt::FontRole:
            if (column == ColQuantity && c->isLowStock()) {
                QFont bold;
                bold.setBold(true);
                return bold;
            }
            break;

        case Qt::ToolTipRole:
            if (column == ColQuantity && c->isLowStock()) {
                return QStringLiteral("Sotto la scorta minima (%1): da riordinare").arg(c->minQuantity);
            }
            if (column == ColValue && !c->value.empty() && c->displayValue() != c->value) {
                return QStringLiteral("Inserito come \"%1\"").arg(toQ(c->value));
            }
            if (column == ColName && !c->notes.empty()) return toQ(c->notes);
            break;
    }
    return {};
}

QVariant ComponentModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
        case ColId: return QStringLiteral("ID");
        case ColCategory: return QStringLiteral("Categoria");
        case ColName: return QStringLiteral("Sigla / descrizione");
        case ColValue: return QStringLiteral("Valore");
        case ColPackage: return QStringLiteral("Package");
        case ColQuantity: return QStringLiteral("Quantità");
        case ColMinQuantity: return QStringLiteral("Scorta min.");
        case ColLocation: return QStringLiteral("Posizione");
        case ColSupplier: return QStringLiteral("Fornitore");
        case ColPrice: return QStringLiteral("Prezzo unit.");
    }
    return {};
}

void ComponentModel::sort(int column, Qt::SortOrder order) {
    beginResetModel();
    sortColumn_ = column;
    sortOrder_ = order;
    applySort();
    endResetModel();
}

void ComponentModel::applySort() {
    auto byText = [this](std::string Component::*field) {
        sortComponents(items_, SortKey::CategoryAndValue);
        std::stable_sort(items_.begin(), items_.end(), [field](const Component* a, const Component* b) {
            return text::toLower(a->*field) < text::toLower(b->*field);
        });
    };

    switch (sortColumn_) {
        case ColId: sortComponents(items_, SortKey::Id); break;
        case ColName: sortComponents(items_, SortKey::Name); break;
        case ColQuantity: sortComponents(items_, SortKey::Quantity); break;
        case ColLocation: sortComponents(items_, SortKey::Location); break;
        case ColPackage: byText(&Component::package); break;
        case ColSupplier: byText(&Component::supplier); break;
        case ColMinQuantity:
            sortComponents(items_, SortKey::CategoryAndValue);
            std::stable_sort(items_.begin(), items_.end(),
                             [](const Component* a, const Component* b) { return a->minQuantity < b->minQuantity; });
            break;
        case ColPrice:
            sortComponents(items_, SortKey::CategoryAndValue);
            std::stable_sort(items_.begin(), items_.end(),
                             [](const Component* a, const Component* b) { return a->unitPrice < b->unitPrice; });
            break;
        default: sortComponents(items_, SortKey::CategoryAndValue); break;
    }
    if (sortOrder_ == Qt::DescendingOrder) std::reverse(items_.begin(), items_.end());
}
