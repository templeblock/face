#ifndef SCORELEVELFUSION_H
#define SCORELEVELFUSION_H

#include <QVector>

#include <opencv/ml.h>

#include "featureextractor.h"
#include "linalg/common.h"
#include "evaluation.h"
#include "linalg/lda.h"
#include "linalg/logisticregression.h"
#include "linalg/vector.h"

/*class ScoreLevelFusionComponent
{
public:
    //const QVector<Template> trainTemplates;
    //const Metrics *metrics;

    Evaluation evalResult;

    ScoreLevelFusionComponent() {}

    ScoreLevelFusionComponent(const QVector<Template> &trainTemplates,
                              const Metrics &metrics)
    {
        evalResult = Evaluation(trainTemplates, metrics);
    }

    ScoreLevelFusionComponent(const Evaluation &evalResult) : evalResult(evalResult) { }
};*/

class ScoreLevelFusionBase
{
protected:
    QList<Evaluation> components;

    bool learned;
    QVector<double> genuineMeans;
    QVector<double> impostorMeans;

    void prepareDataForClassification(QVector<Vector> &scores, QVector<int> &classes,
                                      int genuineLabel, int impostorLabel);

    Vector normalizeScore(QVector<double> &score) const;

    virtual void learnImplementation() = 0;

public:
    ScoreLevelFusionBase() { learned = false; }

    virtual ~ScoreLevelFusionBase() {}

    void learn();

    virtual double fuse(QVector<double> &scores) const = 0;

    ScoreLevelFusionBase & addComponent(const Evaluation &component);

    void popComponent();

    Evaluation evaluate(const QList<Templates> &templates, const QList<Metrics *> &, bool debugOutput = false);
    Evaluation evaluate(const QList<Evaluation> &evaluations, bool debugOutput = false);
};

class ScoreLDAFusion : public ScoreLevelFusionBase
{
private:
    double maxScore;
    double minScore;
    bool swapResultScore;

    LDA lda;

public:
    void learnImplementation();
    double fuse(QVector<double> &scores);
};

class ScoreLogisticRegressionFusion : public ScoreLevelFusionBase
{
private:
    LogisticRegression logR;

public:
    void learnImplementation();
    double fuse(QVector<double> &scores) const;
};

class ScoreWeightedSumFusion : public ScoreLevelFusionBase
{
private:
    QVector<double> eer;
    double weightDenominator;

public:
    void learnImplementation();
    double fuse(QVector<double> &scores) const;

    ScoreWeightedSumFusion() {}
    ScoreWeightedSumFusion(const QString &path);

    void serialize(const QString &path) const;
};

class ScoreProductFusion : public ScoreLevelFusionBase
{
public:
    void learnImplementation();
    double fuse(QVector<double> &scores) const;
};

class ScoreSVMFusion : public ScoreLevelFusionBase
{
private:
    cv::SVM svm;

    cv::Mat colVectorsToFPMatrix(QVector<Vector> &vectors) const;
    cv::Mat colVectorToColFPMatrix(QVector<int> &vector) const;
    cv::Mat colVectorToColFPMatrix(QVector<double> &vector) const;

public:
    void learnImplementation();
    double fuse(QVector<double> &scores) const;

    ScoreSVMFusion() {}
    ScoreSVMFusion(const QString &path);
    void serialize(const QString &path);
};

#endif // SCORELEVELFUSION_H
