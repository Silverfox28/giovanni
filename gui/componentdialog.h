// Finestra per inserire o modificare un componente.
#pragma once

#include <QDialog>
#include <QStringList>

#include "component.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;

class ComponentDialog : public QDialog {
    Q_OBJECT

public:
    // locations e suppliers servono per suggerire i valori già usati.
    ComponentDialog(const QString& title, const Component& initial, const QStringList& locations,
                    const QStringList& suppliers, QWidget* parent = nullptr);

    Component component() const;

public slots:
    void accept() override;

private:
    Category selectedCategory() const;
    void updateValueHint();
    void updateValuePreview();

    Component base_;
    QComboBox* category_ = nullptr;
    QLineEdit* name_ = nullptr;
    QLineEdit* value_ = nullptr;
    QLabel* valuePreview_ = nullptr;
    QComboBox* package_ = nullptr;
    QSpinBox* quantity_ = nullptr;
    QSpinBox* minQuantity_ = nullptr;
    QComboBox* location_ = nullptr;
    QComboBox* supplier_ = nullptr;
    QDoubleSpinBox* price_ = nullptr;
    QLineEdit* notes_ = nullptr;
};
