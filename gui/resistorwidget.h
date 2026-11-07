// Disegno di un resistore con le sue bande colorate.
#pragma once

#include <QColor>
#include <QIcon>
#include <QWidget>

#include <string>
#include <vector>

class ResistorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ResistorWidget(QWidget* parent = nullptr);

    // Bande del valore (cifre + moltiplicatore) ed eventuale banda di tolleranza.
    void setBands(const std::vector<std::string>& valueBands, const std::string& toleranceBand = "");
    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<std::string> bands_;
    std::string tolerance_;
};

// Colore corrispondente al nome italiano di una banda ("rosso", "oro"...).
QColor bandColor(const std::string& name);

// Piccolo quadratino colorato da usare negli elenchi a discesa.
QIcon bandIcon(const std::string& name);
