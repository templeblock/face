#ifndef EVALUATEKINECT_H
#define EVALUATEKINECT_H

#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include <QApplication>

#include "gui/glwidget.h"
#include "facedata/mesh.h"
#include "facedata/surfaceprocessor.h"
#include "linalg/common.h"
#include "biometrics/featureextractor.h"
#include "biometrics/isocurveprocessing.h"
#include "biometrics/evaluation.h"
#include "biometrics/scorelevefusion.h"
#include "biometrics/histogramfeatures.h"
#include "facedata/landmarkdetector.h"
#include "facedata/facealigner.h"
#include "linalg/kernelgenerator.h"
#include "linalg/serialization.h"
#include "linalg/loader.h"
#include "linalg/matrixconverter.h"
#include "biometrics/facetemplate.h"
#include "biometrics/multibiomertricsautotuner.h"
#include "biometrics/biodataprocessing.h"

using namespace Face::Biometrics;

class EvaluateKinect
{
public:

    static void evaluateReferenceDistances()
    {
        Face::Biometrics::FaceClassifier faceClassifier("../../test/kinect/classifiers2/");

        QMap<int, Face::Biometrics::FaceTemplate *> references;

        QVector<QString> templateFiles = Face::LinAlg::Loader::listFiles("../../test/kinect/", "*.yml", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, templateFiles)
        {
            int id = QFileInfo(path).baseName().split("-")[0].toInt();
            Face::Biometrics::FaceTemplate *t = new Face::Biometrics::FaceTemplate(id, path, faceClassifier);
            references.insertMulti(id, t);
        }

        foreach (int id, references.uniqueKeys())
        {
            qDebug() << id;
            QList<Face::Biometrics::FaceTemplate *> ref = references.values(id);
            foreach (const Face::Biometrics::FaceTemplate *probe, ref)
            {
                qDebug() << "  " << faceClassifier.compare(ref, probe, Face::Biometrics::FaceClassifier::CompareMeanDistance);
            }
        }
    }

    static void evaluateRefeference()
    {
        Face::Biometrics::FaceClassifier faceClassifier("../../test/kinect/classifiers/");

        QHash<int, Face::Biometrics::FaceTemplate *> references;
        QVector<Face::Biometrics::FaceTemplate *> testTemplates;

        QVector<QString> templateFiles = Face::LinAlg::Loader::listFiles("../../test/kinect/", "*.yml", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, templateFiles)
        {
            int id = QFileInfo(path).baseName().split("-")[0].toInt();
            Face::Biometrics::FaceTemplate *t = new Face::Biometrics::FaceTemplate(id, path, faceClassifier);

            if (!references.contains(id) || references.values(id).count() < 2)
            {
                references.insertMulti(id, t);
            }
            else
            {
                testTemplates << t;
            }
        }

        Face::Biometrics::Evaluation e = faceClassifier.evaluate(references, testTemplates,
                                                                 Face::Biometrics::FaceClassifier::CompareMeanDistance);
        qDebug() << e.eer; // << e.fnmrAtFmr(0.01) << e.fnmrAtFmr(0.001) << e.fnmrAtFmr(0.0001);
        qDebug() << e.maxSameDistance << e.minDifferentDistance;
        //e.outputResults("kinect", 10);
    }

    static void evaluateSimple()
    {
        Face::Biometrics::FaceClassifier faceClassifier("../../test/kinect/classifiers/");
        QVector<Face::Biometrics::FaceTemplate*> templates;

        QVector<QString> templateFiles = Face::LinAlg::Loader::listFiles("../../test/kinect/", "*.yml", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, templateFiles)
        {
            int id = QFileInfo(path).baseName().split("-")[0].toInt();
            templates << new Face::Biometrics::FaceTemplate(id, path, faceClassifier);
        }

        Face::Biometrics::Evaluation e = faceClassifier.evaluate(templates);
        qDebug() << e.eer;

        foreach (Face::Biometrics::FaceTemplate *t, templates)
        {
            delete t;
        }
        //e.outputResults("kinect", 15);
    }

    static void createTemplates()
    {
        Face::FaceData::FaceAligner aligner(Face::FaceData::Mesh::fromOBJ("../../test/meanForAlign.obj", false));
        Face::Biometrics::FaceClassifier faceClassifier("../../test/kinect/classifiers2/");

        QVector<QString> binFiles = Face::LinAlg::Loader::listFiles("../../test/kinect/", "*.bin", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, binFiles)
        {
            QString baseName = QFileInfo(path).baseName();
            int id = baseName.split("-")[0].toInt();
            Face::FaceData::Mesh face = Face::FaceData::Mesh::fromBIN(path);
            aligner.icpAlign(face, 10, Face::FaceData::FaceAligner::NoseTipDetection);

            Face::Biometrics::FaceTemplate t(id, face, faceClassifier);
            t.serialize("../../test/kinect/" + baseName + ".yml.gz", faceClassifier);
        }
    }

    static void learnFromFrgc()
    {
        Face::FaceData::FaceAligner aligner(Face::FaceData::Mesh::fromOBJ("../../test/meanForAlign.obj", false));
        Face::Biometrics::FaceClassifier faceClassifier("../../test/frgc/classifiers/");
        QVector<Face::Biometrics::FaceTemplate*> templates;

        QVector<QString> binFiles = Face::LinAlg::Loader::listFiles("../../test/softKinetic/01/", "*.binz", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, binFiles)
        {
            int id = QFileInfo(path).baseName().split("-")[0].toInt();
            Face::FaceData::Mesh face = Face::FaceData::Mesh::fromBINZ(path);
            Face::FaceData::SurfaceProcessor::zsmooth(face, 0.5, 5);
            aligner.icpAlign(face, 10, Face::FaceData::FaceAligner::NoseTipDetection);
            templates << new Face::Biometrics::FaceTemplate(id, face, faceClassifier);
        }

        Face::Biometrics::FaceClassifier newClassifier;
        faceClassifier.relearnFinalFusion(templates, newClassifier, true);
        newClassifier.serialize("../../test/kinect/classifiers2");
    }

    static void evaluateKinect()
    {
        Face::FaceData::FaceAligner aligner(Face::FaceData::Mesh::fromOBJ("../../test/meanForAlign.obj", false));
        Face::Biometrics::FaceClassifier faceClassifier("../../test/kinect/classifiers2/");
        QVector<Face::Biometrics::FaceTemplate*> templates;

        QVector<QString> binFiles = Face::LinAlg::Loader::listFiles("../../test/kinect/", "*.bin", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, binFiles)
        {
            int id = QFileInfo(path).baseName().split("-")[0].toInt();
            Face::FaceData::Mesh face = Face::FaceData::Mesh::fromBIN(path, true);
            aligner.icpAlign(face, 10, Face::FaceData::FaceAligner::NoseTipDetection);
            templates << new Face::Biometrics::FaceTemplate(id, face, faceClassifier);
            templates.last()->serialize("../../test/kinect/" + QFileInfo(path).baseName() + ".xml", faceClassifier);
        }

        Face::Biometrics::Evaluation eval = faceClassifier.evaluate(templates);
        qDebug() << eval.eer;
        //eval.outputResults("kinect", 10);
    }

    static void evaluateSerializedKinect()
    {
        Face::Biometrics::FaceClassifier faceClassifier("../../test/kinect/classifiers2/");
        QVector<Face::Biometrics::FaceTemplate*> templates;

        QVector<QString> binFiles = Face::LinAlg::Loader::listFiles("../../test/kinect/", "*.xml", Face::LinAlg::Loader::AbsoluteFull);
        foreach(const QString &path, binFiles)
        {
            int id = QFileInfo(path).baseName().split("-")[0].toInt();
            templates << new Face::Biometrics::FaceTemplate(id, path, faceClassifier);
        }

        Face::Biometrics::Evaluation eval = faceClassifier.evaluate(templates);
        qDebug() << eval.eer;
    }

    static void evaluateMultiExtractor()
    {
        MultiBiomertricsAutoTuner::Settings settings(MultiBiomertricsAutoTuner::Settings::FCT_SVM, "../../test/allUnits");

        qDebug() << "loading frgc";
        MultiBiomertricsAutoTuner::Input frgc =
                MultiBiomertricsAutoTuner::Input::fromDirectoryWithExportedCurvatureImages("/media/data/frgc/spring2004/zbin-aligned2/", "d", 200);

        qDebug() << "loading kinect";
        MultiBiomertricsAutoTuner::Input kinect =
                MultiBiomertricsAutoTuner::Input::fromDirectoryWithAlignedMeshes("../../test/kinect", "-");

        qDebug() << "training";
        MultiExtractor extractor = MultiBiomertricsAutoTuner::train(frgc, kinect, settings);
        extractor.serialize("out");
    }
};
#endif // EVALUATEKINECT_H
