// Finestra per il codice colori dei resistori (dai colori al valore e viceversa).
#pragma once

#include <QDialog>

#include <vector>

class Inventory;
class QComboBox;
class QLabel;
class QLineEdit;
class ResistorWidget;

class ColorCodeDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColorCodeDialog(const Inventory& inventory, QWidget* parent = nullptr);

private:
    QWidget* createDecodeTab();
    QWidget* createEncodeTab();
    void setBandCount(int count);
    void updateDecode();
    void updateEncode();
    QString stockText(double ohms) const;

    const Inventory& inventory_;

    QComboBox* bandCount_ = nullptr;
    std::vector<QLabel*> bandLabels_;
    std::vector<QComboBox*> bandCombos_;
    ResistorWidget* decodeResistor_ = nullptr;
    QLabel* decodeResult_ = nullptr;
    QLabel* decodeStock_ = nullptr;

    QLineEdit* encodeInput_ = nullptr;
    ResistorWidget* encode4_ = nullptr;
    ResistorWidget* encode5_ = nullptr;
    QLabel* encode4Text_ = nullptr;
    QLabel* encode5Text_ = nullptr;
    QLabel* encodeStock_ = nullptr;
};
