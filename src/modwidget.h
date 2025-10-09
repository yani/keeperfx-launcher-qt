#pragma once

#include "mod.h"

#include <QWidget>

namespace Ui { class ModWidget; }

class ModWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModWidget(Mod *mod, QWidget *parent = nullptr);
    ~ModWidget();

private:
    Ui::ModWidget *ui;

    Mod *mod;
};
