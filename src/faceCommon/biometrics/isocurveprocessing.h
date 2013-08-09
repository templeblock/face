#ifndef ISOCURVEPROCESSING_H
#define ISOCURVEPROCESSING_H

#include "facelib/mesh.h"
#include "biometrics/template.h"

typedef QVector<VectorOfPoints> VectorOfCurves;

class SubjectIsoCurves
{
public:
    int subjectID;
    VectorOfCurves vectorOfIsocurves;
};

class IsoCurveProcessing
{
public:
    IsoCurveProcessing();

    static QVector<SubjectIsoCurves> readDirectory(const QString &path, const QString &separator, const QString &fileNameFilter = QString());

    static void sampleIsoCurves(QVector<SubjectIsoCurves> &data, int modulo);

    static void sampleIsoCurvePoints(QVector<SubjectIsoCurves> &data, int modulo);

    static void sampleIsoCurvePoints(VectorOfCurves &isocurves, int modulo);

    static void sampleIsoCurvePoints(VectorOfPoints &isocurve, int modulo);

    static void selectIsoCurves(QVector<SubjectIsoCurves> &data, int start, int end);

    static void stats(QVector<SubjectIsoCurves> &data);

    static QVector<Template> generateTemplates(QVector<SubjectIsoCurves> &data, bool onlyZ);

    static Template generateTemplate(const SubjectIsoCurves &subj, bool onlyZ);

    static Vector generateFeatureVector(const VectorOfCurves &isocurves, bool onlyZ);

    static QVector<Template> generateEuclDistanceTemplates(QVector<SubjectIsoCurves> &data);
};

#endif // ISOCURVEPROCESSING_H
