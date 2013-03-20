#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDebug>
#include <opencv/highgui.h>

#include "mesh.h"
#include "surfaceprocessor.h"
#include "linalg/delaunay.h"
#include "linalg/common.h"
#include "linalg/procrustes.h"

Mesh::Mesh(const QString &filename, bool centralizeLoadedMesh)
{
    QString modelPath = filename;
    modelPath.append(".abs");
    QFile f(modelPath);
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);

    int mapHeight;
    in >> mapHeight;
    in.readLine();
    int mapwidth;
    in >> mapwidth;

    /*minMapX = mapwidth;
    maxMapX = 0;
    minMapY = mapHeight;
    maxMapY = 0;*/

    in.readLine();
    in.readLine();

    int total = mapwidth*mapHeight;
    qDebug() << "total points" << total;

    int flags[total];
    double *xPoints = new double[total];
    double *yPoints = new double[total];
    double *zPoints = new double[total];

    for (int i = 0; i < total; i++)
    {
        in >> (flags[i]);
    }
    qDebug() << "flags loaded";

    minx = 1e300;
    maxx = -1e300;
    for (int i = 0; i < total; i++)
    {
        in >> (xPoints[i]);
        if (flags[i])
        {
            double x = xPoints[i];
            if (x > maxx)
            {
                //maxMapX = indexToXCoord(i);
                maxx = x;
            }
            if (x < minx)
            {
                //minMapX = indexToXCoord(i);
                minx = x;
            }
        }
    }
    qDebug() << "x points loaded";

    miny = 1e300;
    maxy = -1e300;
    for (int i = 0; i < total; i++)
    {
        in >> (yPoints[i]);
        if (flags[i])
        {
            double y = yPoints[i];
            //qDebug() << y << miny << maxy;
            if (y > maxy)
            {
                //maxMapY = indexToYCoord(i);
                maxy = y;
            }
            if (y < miny)
            {
                //minMapY = indexToYCoord(i);
                miny = y;
            }
        }
    }
    qDebug() << "y points loaded";
    /*double tmp = maxMapY;
    maxMapY = minMapY;
    minMapY = tmp;*/

    minz = 1e300;
    maxz = -1e300;
    for (int i = 0; i < total; i++)
    {
        in >> (zPoints[i]);
        if (flags[i])
        {
            double z = zPoints[i];
            if (z > maxz)
            {
                maxz = z;
            }
            if (z < minz)
            {
                minz = z;
            }
        }
    }
    qDebug() << "z points loaded";

    for (int i = 0; i < total; i++)
    {
        if (flags[i])
        {
            cv::Point3d p;
            p.x = xPoints[i];
            p.y = yPoints[i];
            p.z = zPoints[i];
            points.append(p);
        }
    }

    delete [] xPoints;
    delete [] yPoints;
    delete [] zPoints;

    if (centralizeLoadedMesh)
        centralize();

    calculateTriangles();

    // Texture
    //QString texturePath(filename);
    //texturePath.append(".ppm");
    //texture = cvLoadImage(texturePath.toStdString());
}

void Mesh::recalculateMinMax()
{
    minx = 1e300;
    maxx = -1e300;
    miny = 1e300;
    maxy = -1e300;
    minz = 1e300;
    maxz = -1e300;

    int n = points.size();
    for (int i = 0; i < n; i++)
    {
        cv::Point3d &p = points[i];
        if (p.x > maxx) maxx = p.x;
        if (p.x < minx) minx = p.x;
        if (p.y > maxy) maxy = p.y;
        if (p.y < miny) miny = p.y;
        if (p.z > maxz) maxz = p.z;
        if (p.z < minz) minz = p.z;
    }
}

void Mesh::centralize()
{
    qDebug() << "Centering";
    double sumx = 0;
    double sumy = 0;
    double sumz = 0;
    double count = points.count();
    foreach(cv::Point3d p, points)
    {
        sumx += p.x;
        sumy += p.y;
        sumz += p.z;
    }

    for (int i = 0; i < points.count(); i++)
    {
        points[i].x -= sumx/count;
        points[i].y -= sumy/count;
        points[i].z -= sumz/count;
    }

    minx -= sumx/count;
    miny -= sumy/count;
    maxx -= sumx/count;
    maxy -= sumy/count;
    minz -= sumz/count;
    maxz -= sumz/count;

    qDebug() << "..done";
}

void Mesh::move(cv::Point3d translationVector)
{
    Procrustes3D::translate(points, translationVector);
    minx += translationVector.x;
    miny += translationVector.y;
    minz += translationVector.z;
    maxx += translationVector.x;
    maxy += translationVector.y;
    maxz += translationVector.z;
}

void Mesh::scale(cv::Point3d scaleParam)
{
    Procrustes3D::scale(points, scaleParam);
    minx *= scaleParam.x;
    miny *= scaleParam.y;
    minz *= scaleParam.z;
    maxx *= scaleParam.x;
    maxy *= scaleParam.y;
    maxz *= scaleParam.z;
}

void Mesh::rotate(cv::Vec3d xyz)
{
    rotate(xyz(0), xyz(1), xyz(2));
}

void Mesh::rotate(double x, double y, double z)
{
    Matrix Rx = (Matrix(3,3) <<
                 1, 0, 0,
                 0, cos(x), -sin(x),
                 0, sin(x), cos(x));
    Matrix Ry = (Matrix(3,3) <<
                 cos(y), 0, sin(y),
                 0, 1, 0,
                 -sin(y), 0, cos(y));
    Matrix Rz = (Matrix(3,3) <<
                 cos(z), -sin(z), 0,
                 sin(z), cos(z), 0,
                 0, 0, 1);
    Matrix R = Rx*Ry*Rz;

    int n = points.count();
    for (int i = 0; i < n; i++)
    {
        cv::Point3d &p = points[i];

        Matrix v = (Matrix(3,1) << p.x, p.y, p.z);
        Matrix newV = R*v;

        p.x = newV(0);
        p.y = newV(1);
        p.z = newV(2);
    }

    recalculateMinMax();
}

void Mesh::transform(Matrix &m)
{
    Procrustes3D::transform(points, m);

    recalculateMinMax();
}

void Mesh::calculateTriangles()
{
    qDebug() << "Calculating triangles";

    QVector<cv::Point2d> points2d;
    foreach(cv::Point3d p3d, points)
    {
        cv::Point2d p; p.x = p3d.x; p.y = p3d.y;
        points2d.append(p);
    }

    triangles = Delaunay::process(points2d);
    int c = triangles.count();

    QList<int> toRemove;
    for (int i = 0; i < c; i++)
    {
        double maxd = 0.0;

        cv::Point3d &p1 = points[triangles[i][0]];
        cv::Point3d &p2 = points[triangles[i][1]];
        cv::Point3d &p3 = points[triangles[i][2]];

        double d = euclideanDistance(p1, p2);
        if (d > maxd) maxd = d;
        d = euclideanDistance(p1, p3);
        if (d > maxd) maxd = d;
        d = euclideanDistance(p2, p3);
        if (d > maxd) maxd = d;

        //if (maxd > 30.0)
        //    toRemove.append(i);
    }

    for (int i = toRemove.count()-1; i >= 0; i--)
    {
        triangles.remove(toRemove.at(i));
    }

    qDebug() << "Triangles done, |triangles| =" << triangles.count();
}

Mesh Mesh::fromXYZFile(const QString &filename, bool centralizeLoadedMesh)
{
    qDebug() << "loading" << filename;
    QFile f(filename);
    bool exists = f.exists();
    assert(exists);
    bool opened = f.open(QIODevice::ReadOnly);
    assert(opened);
    QTextStream in(&f);

    Mesh mesh;
    double x,y,z;
    while (!in.atEnd())
    {
        in >> x; in >> y; in >> z;

        if (in.status() == QTextStream::ReadPastEnd)
            break;

        if (x > mesh.maxx) mesh.maxx = x;
        if (x < mesh.minx) mesh.minx = x;
        if (y > mesh.maxy) mesh.maxy = y;
        if (y < mesh.miny) mesh.miny = y;
        if (z > mesh.maxz) mesh.maxz = z;
        if (z < mesh.minz) mesh.minz = z;

        cv::Point3d p;
        p.x = x; p.y = y; p.z = z;
        mesh.points.append(p);
    }
    f.close();

    if (centralizeLoadedMesh)
        mesh.centralize();

    mesh.calculateTriangles();
    return mesh;
}

Mesh Mesh::fromPointcloud(VectorOfPoints &pointcloud, bool centralizeLoadedMesh)
{
    Mesh m;
    m.points = VectorOfPoints(pointcloud);
    m.calculateTriangles();
    m.recalculateMinMax();

    if (centralizeLoadedMesh)
        m.centralize();

    return m;
}

Mesh Mesh::fromMap(Map &map, bool centralizeLoadedMesh)
{
    QMap<int, int> coordToIndex;

    Mesh mesh;
    int index = 0;
    for (int y = 0; y < map.h; y++)
    {
        for (int x = 0; x < map.w; x++)
        {
            if (map.isSet(x,y))
            {
                mesh.points << cv::Point3d(x, map.h-y-1, map.get(x,y));
                int coord = map.coordToIndex(x,y);
                coordToIndex[coord] = index;
                index++;
            }
        }
    }

    if (centralizeLoadedMesh)
        mesh.centralize();

    mesh.recalculateMinMax();

    // triangles
    for (int y = 0; y < map.h; y++)
    {
        for (int x = 0; x < map.w; x++)
        {
            if (map.isSet(x,y) &&
                map.isValidCoord(x, y+1) && map.isSet(x, y+1) &&
                map.isValidCoord(x+1, y+1) && map.isSet(x+1, y+1))
            {
                mesh.triangles << cv::Vec3i(coordToIndex[map.coordToIndex(x,y)], coordToIndex[map.coordToIndex(x,y+1)], coordToIndex[map.coordToIndex(x+1,y+1)]);
                //mesh.triangles.append(cv::Vec3i(map.coordToIndex(x,y), map.coordToIndex(x,y+1), map.coordToIndex(x+1,y+1)));
            }

            if (map.isSet(x,y) &&
                map.isValidCoord(x+1, y+1) && map.isSet(x+1, y+1) &&
                map.isValidCoord(x+1, y) && map.isSet(x+1, y))
            {
                mesh.triangles << cv::Vec3i(coordToIndex[map.coordToIndex(x,y)], coordToIndex[map.coordToIndex(x+1,y+1)], coordToIndex[map.coordToIndex(x+1,y)]);
                //mesh.triangles.append(cv::Vec3i(map.coordToIndex(x,y), map.coordToIndex(x+1,y+1), map.coordToIndex(x+1,y)));
            }
        }
    }

    return mesh;
}

Mesh Mesh::fromOBJ(const QString &filename, bool centralizeLoadedMesh)
{
    qDebug() << "loading" << filename;
    QFile f(filename);
    bool fileExists = f.exists();
    assert(fileExists);
    bool fileOpened = f.open(QIODevice::ReadOnly);
    assert(fileOpened);
    QTextStream in(&f);

    Mesh result;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList items = line.split(QChar(' '));

        if (items[0].compare("v") == 0)
        {
            double x = items[1].toDouble();
            double y = items[2].toDouble();
            double z = items[3].toDouble();

            result.points << cv::Point3d(x,y,z);
        }
        else if (items[0].compare("f") == 0)
        {
            int t1 = items[1].toInt()-1;
            int t2 = items[2].toInt()-1;
            int t3 = items[3].toInt()-1;

            result.triangles << cv::Vec3i(t1, t2, t3);
        }
    }

    if (centralizeLoadedMesh)
        result.centralize();

    result.recalculateMinMax();

    return result;
}

Mesh::Mesh()
{
    minx = 1e300;
    maxx = -1e300;
    miny = 1e300;
    maxy = -1e300;
    minz = 1e300;
    maxz = -1e300;
}

Mesh::Mesh(Mesh *src)
{
    minx = src->minx;
    maxx = src->maxx;
    miny = src->miny;
    maxy = src->maxy;
    minz = src->minz;
    maxz = src->maxz;

    points = src->points;
    triangles = src->triangles;
}

Mesh::~Mesh()
{
    qDebug() << "deleting mesh";
}

QString formatNumber(double n, char decimalPoint)
{
    return QString::number(n).replace('.', decimalPoint);
}

void Mesh::writeOBJ(const QString &path, char decimalPoint)
{
    QFile outFile(path);
    outFile.open(QFile::WriteOnly);
    QTextStream outStream(&outFile);

    int pointCount = points.size();
    for (int i = 0; i < pointCount; i++)
    {
        cv::Point3d &p = points[i];
        outStream << "v " << formatNumber(p.x, decimalPoint) << " " << formatNumber(p.y, decimalPoint) << " " << formatNumber(p.z, decimalPoint) << endl;
    }

    int tCount = triangles.count();
    for (int i = 0; i < tCount; i++)
    {
        cv::Vec3i &t = triangles[i];
        outStream << "f " << (t[0]+1) << " " << (t[1]+1) << " " << (t[2]+1) << endl;
    }
}

void Mesh::writeOFF(const QString &path)
{
    QFile outFile(path);
    outFile.open(QFile::WriteOnly);
    QTextStream outStream(&outFile);

    outStream << "OFF" << endl;

    outStream << points.size() << " " << triangles.size() << " 0" << endl;

    int pointCount = points.size();
    for (int i = 0; i < pointCount; i++)
    {
        cv::Point3d &p = points[i];
        outStream << p.x << " " << p.y << " " << p.z << endl;
    }

    int tCount = triangles.count();
    for (int i = 0; i < tCount; i++)
    {
        cv::Vec3i &t = triangles[i];
        outStream << "3 " << t[0] << " " << t[1] << " " << t[2] << endl;
    }
}

void Mesh::printStats()
{
    qDebug() << "x-range: " << minx << maxx;
    qDebug() << "y-range: "<< miny << maxy;
    qDebug() << "z-range: "<< minz << maxz;
    qDebug() << "points: "<< points.count();
    qDebug() << "triangles: "<< triangles.count();
}