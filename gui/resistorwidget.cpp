#include "resistorwidget.h"

#include <QPainter>
#include <QPixmap>

#include <algorithm>

QColor bandColor(const std::string& name) {
    struct Entry {
        const char* name;
        QColor color;
    };
    static const Entry kColors[] = {
        {"nero", QColor(20, 20, 20)},       {"marrone", QColor(125, 70, 30)},  {"rosso", QColor(215, 30, 30)},
        {"arancione", QColor(245, 135, 20)}, {"giallo", QColor(250, 215, 30)}, {"verde", QColor(30, 150, 60)},
        {"blu", QColor(30, 80, 220)},       {"viola", QColor(140, 60, 200)},   {"grigio", QColor(130, 130, 130)},
        {"bianco", QColor(250, 250, 250)},  {"oro", QColor(205, 165, 50)},     {"argento", QColor(190, 195, 200)},
    };
    for (const Entry& e : kColors) {
        if (name == e.name) return e.color;
    }
    return QColor(0, 0, 0, 0);
}

QIcon bandIcon(const std::string& name) {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setPen(QColor(60, 60, 60));
    p.setBrush(bandColor(name));
    p.drawRect(1, 1, 13, 13);
    return QIcon(pixmap);
}

ResistorWidget::ResistorWidget(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void ResistorWidget::setBands(const std::vector<std::string>& valueBands, const std::string& toleranceBand) {
    bands_ = valueBands;
    tolerance_ = toleranceBand;
    update();
}

void ResistorWidget::clear() {
    setBands({}, "");
}

QSize ResistorWidget::sizeHint() const {
    return {260, 64};
}

QSize ResistorWidget::minimumSizeHint() const {
    return {160, 48};
}

void ResistorWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF area = QRectF(rect()).adjusted(4, 4, -4, -4);
    const double bodyH = std::min(area.height(), area.width() / 3.6);
    const double bodyW = bodyH * 2.9;
    const QRectF body(area.center().x() - bodyW / 2, area.center().y() - bodyH / 2, bodyW, bodyH);

    // Reofori.
    p.setPen(QPen(QColor(150, 150, 150), std::max(2.0, bodyH / 9), Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(area.left(), area.center().y()), QPointF(area.right(), area.center().y()));

    // Corpo.
    p.setPen(QPen(QColor(140, 110, 70), 1.2));
    p.setBrush(QColor(228, 205, 160));
    p.drawRoundedRect(body, bodyH * 0.38, bodyH * 0.38);

    // Bande.
    const double bandW = bodyW * 0.07;
    const QPen bandPen(QColor(60, 60, 60, 110), 0.8);
    auto drawBand = [&](double x, const std::string& name) {
        p.setPen(bandPen);
        p.setBrush(bandColor(name));
        p.drawRect(QRectF(x, body.top() + 0.6, bandW, body.height() - 1.2));
    };
    for (std::size_t i = 0; i < bands_.size(); ++i) {
        drawBand(body.left() + bodyW * 0.17 + static_cast<double>(i) * bandW * 1.8, bands_[i]);
    }
    if (!tolerance_.empty()) drawBand(body.right() - bodyW * 0.16 - bandW, tolerance_);
}
