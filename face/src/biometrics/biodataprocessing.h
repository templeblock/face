#ifndef BIODATAPROCESSING_H
#define BIODATAPROCESSING_H

#include <QVector>
#include <QList>
#include <QSet>

#include "template.h"
#include "linalg/common.h"

class BioDataProcessing
{
public:
    static QList<QVector<Template> > divide(QVector<Template> &templates, int subjectsInOneCluster);

    static void divide(QVector<Matrix> &vectors, QVector<int> &classMembership, int subjectsInOneCluster,
                       QList<QVector<Matrix> > &resultVectors, QList<QVector<int> > &resultClasses);

    static void divideToNClusters(QVector<Matrix> &vectors, QVector<int> &classMembership, int numberOfClusters,
                       QList<QVector<Matrix> > &resultVectors, QList<QVector<int> > &resultClasses);

    static QList<QSet<int> > divideToNClusters(QVector<int> &classMembership, int numberOfClusters);

    static void divideAccordingToUniqueClasses(
    		QVector<Matrix> &vectors, QVector<int> &classMembership,
    		QList<QSet<int > > &uniqueClassesInClusters,
    		QList<QVector<Matrix> > &resultVectors, QList<QVector<int> > &resultClasses);
};

#endif // BIODATAPROCESSING_H
