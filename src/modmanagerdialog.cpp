#include "modmanagerdialog.h"
#include "modmanager.h"
#include "modwidget.h"
#include "ui_modmanagerdialog.h"

ModManagerDialog::ModManagerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ModManagerDialog)
{
    ui->setupUi(this);

    // Disable resizing and remove maximize button
    setFixedSize(size());
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    // Get mods
    ModManager *manager = new ModManager();
    QList<Mod *> mods = manager->modsAfterBase;

    // Add the mods
    if (mods.isEmpty() == false) {
        for (auto mod : std::as_const(mods)) {
            // Create the widget
            ModWidget *modWidget = new ModWidget(mod, this);

            //mod->

            ui->scrollAreaWidgetContents->layout()->addWidget(modWidget);
        }
    }
}

ModManagerDialog::~ModManagerDialog()
{
    delete ui;
}

void ModManagerDialog::on_closeButton_clicked()
{
    this->close();
}
