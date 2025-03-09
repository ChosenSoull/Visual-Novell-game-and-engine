#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Language:"));
    languageCombo = new QComboBox(this);
    languageCombo->addItems({"English", "Russian"});
    layout->addWidget(languageCombo);

    layout->addWidget(new QLabel("Code Editor:"));
    editorCombo = new QComboBox(this);
    editorCombo->addItems({"VS Code", "Vim", "Nano"});
    layout->addWidget(editorCombo);

    layout->addWidget(new QLabel("Development Render API:"));
    devApiCombo = new QComboBox(this);
    devApiCombo->addItems({"OpenGL", "Vulkan"});
    QSettings settings("MyCompany", "PhantomGameEngine");
    devApiCombo->setCurrentText(settings.value("dev_api", "OpenGL").toString());
    layout->addWidget(devApiCombo);

    QPushButton* okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &SettingsDialog::accept);
    layout->addWidget(okButton);

    setWindowTitle("Settings");
}

QString SettingsDialog::getEditor() const { return editorCombo->currentText(); }
QString SettingsDialog::getDevApi() const { return devApiCombo->currentText(); }