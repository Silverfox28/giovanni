// Finestra con le statistiche del magazzino.
#pragma once

class Inventory;
class QWidget;

void showStatisticsDialog(const Inventory& inventory, QWidget* parent);
