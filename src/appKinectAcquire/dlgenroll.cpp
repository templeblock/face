#include "dlgenroll.h"
#include "ui_dlgenroll.h"

#include "kinect.h"
#include "dlgscanface.h"

DlgEnroll::DlgEnroll(QMap<int, QString> &mapIdToName, QMap<QString, int> mapNameToId,
                     QHash<int, FaceTemplate *> database, const FaceClassifier &classifier,
                     const QString &pathToAlignReference, const QString &pathToHaarFaceDetect, QWidget *parent) :
    mapIdToName(mapIdToName), mapNameToId(mapNameToId), database(database), classifier(classifier),
    pathToAlignReference(pathToAlignReference), pathToHaarFaceDetect(pathToHaarFaceDetect),
    QDialog(parent), ui(new Ui::DlgEnroll)
{
    ui->setupUi(this);
}

DlgEnroll::~DlgEnroll()
{
    delete ui;
}

void DlgEnroll::on_btnAdd_clicked()
{
    DlgScanFace dlgScan(pathToAlignReference, pathToHaarFaceDetect, this);
    if (dlgScan.exec() != QDialog::Accepted) return;
    Mesh *face = dlgScan.result;
    scans << face;

    ui->listScans->addItem(QString::number(ui->listScans->count()+1));
    ui->listScans->item(ui->listScans->count()-1)->setSelected(true);
}

void DlgEnroll::on_btnRemove_clicked()
{
    QModelIndexList selection = ui->listScans->selectionModel()->selectedIndexes();
    if (selection.count() < 1) return;
    int index = selection[0].row();

    qDeleteAll(ui->listScans->selectedItems());
    delete scans[index];
    scans.removeAt(index);
}

void DlgEnroll::on_listScans_itemSelectionChanged()
{
    QList<QListWidgetItem*> items = ui->listScans->selectedItems();
    bool enabled = items.count() > 0;
    ui->btnRemove->setEnabled(enabled);
    ui->btnExport->setEnabled(enabled);

    ui->glWidget->clearAll();
    if (items.count() > 0)
    {
        int index = ui->listScans->selectionModel()->selectedIndexes()[0].row();
        ui->glWidget->addFace(scans[index]);
    }
    ui->glWidget->updateGL();
}

void DlgEnroll::on_buttonBox_accepted()
{
    // check name
    QString name = ui->leName->text();
    if (name.isEmpty() || name.isNull())
    {
        QMessageBox msg(QMessageBox::Critical, "Error", "Please enter username", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if (mapNameToId.contains(name))
    {
        QMessageBox msg(QMessageBox::Critical, "Error", "Username " + name + " already taken", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // check templates
    if (scans.count() < 2)
    {
        QMessageBox msg(QMessageBox::Critical, "Error", "Please scan more reference data", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // extract templates
    int id = 1;
    int count = mapIdToName.keys().count();
    if (count > 0)
    {
        id = mapIdToName.keys()[count - 1] + 1;
    }

    QProgressDialog progDlg("Extracting templates", QString(), 0, scans.count(), this);
    progDlg.setWindowModality(Qt::WindowModal);
    progDlg.setMinimumDuration(100);
    QList<FaceTemplate*> templates;
    int index = 0;
    foreach (const Mesh *m, scans)
    {
        progDlg.setValue(index);
        templates << new FaceTemplate(id, *m, classifier);
        index++;
    }
    progDlg.setValue(scans.count());

    // check quality
    int templateNum = 1;
    foreach (const FaceTemplate *t, templates)
    {
        double d = classifier.compare(templates, t);
        qDebug() << "template:" << templateNum << "distance:" << d;

        if (d >= -1.0)
        {
            QMessageBox msg(QMessageBox::Question, "Question",
                            "Template " + QString::number(templateNum) + " does not fulfil quality requirements. Proceed anyway?",
                            QMessageBox::Yes | QMessageBox::No, this);
            if (msg.exec() != QMessageBox::Yes)
            {
                qDeleteAll(templates);
                return;
            }
        }

        templateNum++;
    }

    // insert to database
    mapIdToName[id] = name;
    mapNameToId[name] = id;

    foreach (FaceTemplate *t, templates)
    {
        database.insertMulti(id, t);
    }

    qDebug() << "Added" << id << name << "scans: " << database.values(id).count();

    accept();
}

void DlgEnroll::on_btnExport_clicked()
{
    QModelIndexList selection = ui->listScans->selectionModel()->selectedIndexes();
    if (selection.count() < 1) return;
    int index = selection[0].row();

    QString path = QFileDialog::getSaveFileName(this, QString(), QString(), "*.obj");
    if (path.isEmpty() || path.isNull()) return;

    scans[index]->writeOBJ(path, '.');
}