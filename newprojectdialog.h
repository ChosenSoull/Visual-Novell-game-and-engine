#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QListWidget>

class NewProjectDialog : public QDialog {
    Q_OBJECT
private:
    QLineEdit* nameEdit;
    QLineEdit* pathEdit;
    QLineEdit* descEdit;
    QComboBox* renderApiCombo;
    QComboBox* typeCombo;
    QListWidget* standardLibsList;
    QListWidget* projectLibsList;

    void loadLibraries(QListWidget* list, const QString& dirPath, const QString& type);

public:
    NewProjectDialog(QWidget* parent = nullptr);
    ProjectInfo getProjectInfo() const;

private slots:
    void browsePath();
};

#endif // NEWPROJECTDIALOG_H