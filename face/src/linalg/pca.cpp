#include "pca.h"

#include <QDebug>

#include "matrixconverter.h"

PCA::PCA(QVector<Matrix> &vectors, int maxComponents, bool debug)
{
    learn(vectors, maxComponents, debug);
}

PCA::PCA(const QString &path)
{
    cv::FileStorage storage(path.toStdString(), cv::FileStorage::READ);
    storage["eigenvalues"] >> cvPca.eigenvalues;
    storage["eigenvectors"] >> cvPca.eigenvectors;
    storage["mean"] >> cvPca.mean;
}

void PCA::learn(QVector<Matrix> &vectors, int maxComponents, bool debug)
{
    if (debug) qDebug() << "PCA";
    if (debug) qDebug() << "Creating input data matrix";
    Matrix data = MatrixConverter::columnVectorsToDataMatrix(vectors);
    if (debug) qDebug() << "Calculating eigenvectors and eigenvalues";
    cvPca = cv::PCA(data, cv::Mat(), CV_PCA_DATA_AS_COL, maxComponents);
    if (debug) qDebug() << "PCA done";
}

void PCA::serialize(const QString &path)
{
    cv::FileStorage storage(path.toStdString(), cv::FileStorage::WRITE);
    storage << "eigenvalues" << cvPca.eigenvalues;
    storage << "eigenvectors" << cvPca.eigenvectors;
    storage << "mean" << cvPca.mean;
}

Matrix PCA::project(const Matrix &in)
{
    Matrix out = cvPca.project(in);
    return out;
}

QVector<Matrix> PCA::project(const QVector<Matrix> &vectors)
{
    QVector<Matrix> result;
    for (int i = 0; i < vectors.count(); i++)
    {
        Matrix out = project(vectors[i]);
        result.append(out);
    }
    return result;
}

Matrix PCA::scaledProject(const Matrix &vector)
{
    Matrix out = project(vector);
    for (int i = 0; i < cvPca.eigenvalues.rows; i++)
        out(i) = out(i) / cvPca.eigenvalues.at<double>(i);
    return out;
}

Matrix PCA::backProject(const Matrix &in)
{
    Matrix out = cvPca.backProject(in);
    return out;
}

void PCA::modesSelectionThreshold(double t)
{
	if (t >= 1) return; // nothing to do here

    double sum = 0.0;
    int r;
    for (r = 0; r < cvPca.eigenvalues.rows; r++)
    {
        sum += cvPca.eigenvalues.at<double>(r);
    }

    double actualSum = 0.0;
    for (r = 0; r < cvPca.eigenvalues.rows; r++)
    {
        actualSum += cvPca.eigenvalues.at<double>(r);

        if (actualSum/sum > t)
            break;
    }

    setModes(r+1);
}

double PCA::getVariation(int mode)
{
    double val = cvPca.eigenvalues.at<double>(mode);
    if (val != val) val = 0.0;
    if (val < 0) val = - val;
    return val;
}

Matrix PCA::getMean()
{
    return (Matrix)(cvPca.mean);
}

Matrix PCA::normalizeParams(const Matrix &params)
{
    int n = getModes();
    assert(params.rows == n);
    Matrix result = Matrix::zeros(n, 1);

    for (int i = 0; i < n; i++)
    {
        double p = params(i);
        double limit = 3*sqrt(getVariation(i));
        if (p > limit)
            p = limit;
        else if (p < -limit)
            p = -limit;
        result(i) = p;
    }
    return result;
}
