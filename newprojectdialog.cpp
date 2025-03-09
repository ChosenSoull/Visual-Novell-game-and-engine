#include "newprojectdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>

NewProjectDialog::NewProjectDialog(QWidget* parent) : QDialog(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Project Name:"));
    nameEdit = new QLineEdit(this);
    layout->addWidget(nameEdit);

    layout->addWidget(new QLabel("Project Path:"));
    pathEdit = new QLineEdit(this);
    QPushButton* browseButton = new QPushButton("Browse", this);
    connect(browseButton, &QPushButton::clicked, this, &NewProjectDialog::browsePath);
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseButton);
    layout->addLayout(pathLayout);

    layout->addWidget(new QLabel("Description:"));
    descEdit = new QLineEdit(this);
    layout->addWidget(descEdit);

    layout->addWidget(new QLabel("Render API:"));
    renderApiCombo = new QComboBox(this);
    renderApiCombo->addItems({"OpenGL", "Vulkan"});
    layout->addWidget(renderApiCombo);

    layout->addWidget(new QLabel("Game Type:"));
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"2D", "3D"});
    layout->addWidget(typeCombo);

    layout->addWidget(new QLabel("Standard Modules:"));
    standardLibsList = new QListWidget(this);
    loadLibraries(standardLibsList, "libs/standard", "module");
    layout->addWidget(standardLibsList);

    layout->addWidget(new QLabel("Project Modules:"));
    projectLibsList = new QListWidget(this);
    loadLibraries(projectLibsList, "libs/project", "module");
    layout->addWidget(projectLibsList);

    QPushButton* createButton = new QPushButton("Create", this);
    QPushButton* cancelButton = new QPushButton("Cancel", this);
    connect(createButton, &QPushButton::clicked, this, &NewProjectDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &NewProjectDialog::reject);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    setWindowTitle("Create New Project");
    resize(400, 700);
}

void NewProjectDialog::loadLibraries(QListWidget* list, const QString& dirPath, const QString& type) {
    QDir dir(dirPath);
    if (!dir.exists()) dir.mkpath(".");
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& libDir : dirs) {
        QFile infoFile(dir.absoluteFilePath(libDir + "/" + type + ".cfg"));
        if (infoFile.exists() && infoFile.open(QIODevice::ReadOnly)) {
            QSettings cfg(infoFile.fileName(), QSettings::IniFormat);
            QString name = cfg.value("Module/Name", libDir).toString();
            QString desc = cfg.value("Module/Description", "No description").toString();
            QListWidgetItem* item = new QListWidgetItem(name + " - " + desc);
            item->setData(Qt::UserRole, libDir);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            list->addItem(item);
            infoFile.close();
        }
    }
}

ProjectInfo NewProjectDialog::getProjectInfo() const {
    std::vector<std::string> standardLibs, projectLibs;
    for (int i = 0; i < standardLibsList->count(); ++i) {
        QListWidgetItem* item = standardLibsList->item(i);
        if (item->checkState() == Qt::Checked) {
            standardLibs.push_back(item->data(Qt::UserRole).toString().toStdString());
        }
    }
    for (int i = 0; i < projectLibsList->count(); ++i) {
        QListWidgetItem* item = projectLibsList->item(i);
        if (item->checkState() == Qt::Checked) {
            projectLibs.push_back(item->data(Qt::UserRole).toString().toStdString());
        }
    }
    return {
        nameEdit->text().toStdString(),
        pathEdit->text().toStdString(),
        descEdit->text().toStdString(),
        renderApiCombo->currentText().toLower().toStdString(),
        typeCombo->currentText().toLower().toStdString(),
        standardLibs,
        projectLibs
    };
}

void NewProjectDialog::browsePath() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Directory");
    if (!dir.isEmpty()) pathEdit->setText(dir);
}