#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class SettingsDialog : public QDialog {
    Q_OBJECT
private:
    QComboBox* languageCombo;
    QComboBox* editorCombo;
    QComboBox* devApiCombo;

public:
    SettingsDialog(QWidget* parent = nullptr);
    QString getEditor() const;
    QString getDevApi() const;
};

#endif // SETTINGSDIALOG_H