#include "riskpage.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtMath>

static QColor stateColor(const QString &state)
{
    if (state == "STOP") return QColor("#dc2626");
    if (state == "DANGER") return QColor("#f97316");
    if (state == "WARNING") return QColor("#f59e0b");
    return QColor("#16a34a");
}

SimulationView::SimulationView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(620, 360);
}

void SimulationView::setData(const SafetyInput &newInput, const SafetyResult &newResult, const QVector<double> &riskHistory, double robotTravel)
{
    input = newInput;
    result = newResult;
    history = riskHistory;
    travel = robotTravel;
    update();
}

void SimulationView::setRiskHistoryVisible(bool visible)
{
    riskHistoryVisible = visible;
    setMinimumSize(620, riskHistoryVisible ? 540 : 360);
    updateGeometry();
    update();
}

void SimulationView::setLightMode(bool enabled)
{
    lightMode = enabled;
    update();
}

void SimulationView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor pageBg = lightMode ? QColor("#f3f4f6") : QColor("#1e1e1e");
    const QColor worldBg = lightMode ? QColor("#f8fafc") : QColor("#19192b");
    const QColor panelBg = lightMode ? QColor("#ffffff") : QColor("#242428");
    const QColor chartBg = lightMode ? QColor("#ffffff") : QColor("#202024");
    const QColor borderColor = lightMode ? QColor("#9ca3af") : QColor("#8d8d8d");
    const QColor primaryText = lightMode ? QColor("#111827") : QColor("#ffffff");
    const QColor secondaryText = lightMode ? QColor("#4b5563") : QColor("#b8c2d6");
    const QColor roadColor = lightMode ? QColor("#e7eef8") : QColor("#17243d");
    const QColor tagBg = lightMode ? QColor(255, 255, 255, 225) : QColor(25, 25, 43, 210);
    p.fillRect(rect(), pageBg);

    const int gap = 16;
    QRect world(24, 8, width() - 48, riskHistoryVisible ? qMin(315, height() - 210) : height() - 16);
    if (world.height() < 260) {
        world.setHeight(260);
    }
    p.setPen(QPen(borderColor, 1));
    p.setBrush(worldBg);
    p.drawRect(world);

    p.setPen(primaryText);
    QFont title("Segoe UI", 13, QFont::Bold);
    p.setFont(title);
    p.drawText(world.adjusted(18, 18, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Robot Movement Field");
    p.setFont(QFont("Segoe UI", 9));
    p.setPen(secondaryText);
    p.drawText(world.adjusted(18, 40, -18, -18), Qt::AlignTop | Qt::AlignLeft,
               "The robot moves forward. Distance to fixed objects becomes smaller.");

    const int sceneTop = world.top() + 108;
    const int sceneBottom = world.bottom() - (riskHistoryVisible ? 96 : 118);
    const int sceneCenterY = sceneTop + (sceneBottom - sceneTop) / 2 + (riskHistoryVisible ? 0 : 8);
    const int trackBaseY = sceneCenterY - (riskHistoryVisible ? 0 : 2);
    const int robotBaseY = trackBaseY - 6;
    const int roadLeft = world.left() + 22;
    const int trackRight = world.right() - 42;
    const double pxPerMeter = qMax(1.0, double(trackRight - roadLeft) / 25.0);
    const int robotHalfWidth = 74;
    const int maxRobotTravel = qMax(0, world.right() - roadLeft - robotHalfWidth * 2 - 18);
    const int robotX = roadLeft + robotHalfWidth + qMin(int(travel * pxPerMeter), maxRobotTravel);
    const int trackLeft = robotX + robotHalfWidth;
    const int baseY = trackBaseY;
    const int zoneTop = baseY - 34;
    const int zoneHeight = 68;
    const int humanY = baseY - 42;
    const int obstacleY = zoneTop + zoneHeight + 24;
    auto mapDist = [&](double d) { return trackLeft + int(d * pxPerMeter); };
    auto mapFixedObject = [&](double d) {
        return roadLeft + robotHalfWidth * 2 + int((travel + d) * pxPerMeter);
    };
    auto keepInWorld = [&](int x, int sidePadding) {
        return qBound(world.left() + sidePadding, x, world.right() - sidePadding);
    };

    const int stopX = mapDist(input.stopDistance);
    const int warningX = mapDist(input.warningDistance);
    const int brakeX = mapDist(result.brakeDistance);
    const int zoneLeft = world.left();
    const int zoneRight = world.right();
    const int zoneStopX = qBound(zoneLeft, stopX, zoneRight);
    const int zoneWarningX = qBound(zoneLeft, warningX, zoneRight);

    auto zoneNameForDistance = [&](double d) {
        if (d <= input.stopDistance) return QString("STOP zone");
        if (d <= input.warningDistance) return QString("WARNING zone");
        return QString("SAFE zone");
    };
    auto zoneColorForDistance = [&](double d) {
        if (d <= input.stopDistance) return QColor("#ef4e42");
        if (d <= input.warningDistance) return QColor("#f59e0b");
        return QColor("#26a47e");
    };

    p.setPen(Qt::NoPen);
    const int roadTop = robotBaseY + 74;
    p.setBrush(roadColor);
    p.drawRect(QRect(world.left(), roadTop, world.width(), 48));
    p.setPen(QPen(QColor("#915e57"), 2, Qt::DashLine));
    p.drawLine(world.left(), roadTop, world.right(), roadTop);

    p.setBrush(QColor(239, 78, 66, 62));
    p.drawRect(QRect(zoneLeft, zoneTop, qMax(0, zoneStopX - zoneLeft), zoneHeight));
    p.setBrush(QColor(245, 158, 11, 58));
    p.drawRect(QRect(zoneStopX, zoneTop, qMax(0, zoneWarningX - zoneStopX), zoneHeight));
    p.setBrush(QColor(38, 164, 126, 42));
    p.drawRect(QRect(zoneWarningX, zoneTop, qMax(0, zoneRight - zoneWarningX), zoneHeight));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.setPen(QColor("#ff8a80"));
    if (zoneStopX - world.left() > 76) {
        p.drawText(QRect(world.left() + 10, zoneTop + 8, zoneStopX - world.left() - 14, 18),
                   Qt::AlignLeft | Qt::AlignVCenter, "STOP AREA");
    }
    p.setPen(QColor("#fbbf24"));
    if (zoneWarningX - zoneStopX > 102) {
        p.drawText(QRect(zoneStopX + 8, zoneTop + 8, zoneWarningX - zoneStopX - 14, 18),
                   Qt::AlignLeft | Qt::AlignVCenter, "WARNING AREA");
    }

    QRect legend(world.left() + 18, world.top() + 54, 392, 28);
    p.setPen(QPen(QColor("#55555f"), 1));
    p.setBrush(panelBg);
    p.drawRoundedRect(legend, 4, 4);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.setPen(primaryText);
    p.drawText(legend.left() + 12, legend.top() + 19, "Zone:");
    p.setBrush(QColor("#ef4e42"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRect(legend.left() + 54, legend.top() + 8, 14, 12), 3, 3);
    p.setPen(QColor("#f2f2f2"));
    p.drawText(legend.left() + 74, legend.top() + 19, "STOP");
    p.setBrush(QColor("#f97316"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRect(legend.left() + 124, legend.top() + 8, 14, 12), 3, 3);
    p.setPen(QColor("#f2f2f2"));
    p.drawText(legend.left() + 144, legend.top() + 19, "DANGER");
    p.setBrush(QColor("#f59e0b"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRect(legend.left() + 214, legend.top() + 8, 14, 12), 3, 3);
    p.setPen(QColor("#f2f2f2"));
    p.drawText(legend.left() + 234, legend.top() + 19, "WARNING");
    p.setBrush(QColor("#26a47e"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRect(legend.left() + 326, legend.top() + 8, 14, 12), 3, 3);
    p.setPen(QColor("#f2f2f2"));
    p.drawText(legend.left() + 346, legend.top() + 19, "SAFE");

    p.setPen(QPen(QColor("#6b7280"), 2));
    p.drawLine(world.left(), baseY, world.right(), baseY);
    p.setPen(QPen(QColor("#b5655c"), 2, Qt::DashLine));
    p.drawLine(trackLeft, baseY, trackRight, baseY);
    p.setPen(QPen(QColor("#5aa7ff"), 2));
    const int movementY = roadTop + 34;
    p.drawLine(world.left(), movementY, world.right(), movementY);
    p.setBrush(QColor("#5aa7ff"));
    p.setPen(Qt::NoPen);
    p.drawPolygon(QPolygon() << QPoint(robotX + 74, movementY) << QPoint(robotX + 62, movementY - 7) << QPoint(robotX + 62, movementY + 7));
    p.setFont(QFont("Segoe UI", 8));
    p.setPen(QColor("#aeb6c7"));
    p.drawLine(trackLeft, baseY - 6, trackLeft, baseY + 6);
    p.drawText(QRect(trackLeft + 4, baseY + 12, 42, 18), Qt::AlignLeft | Qt::AlignVCenter, "0m");
    for (int m = 5; m <= 20; m += 5) {
        int x = trackLeft + int(m * pxPerMeter);
        if (x > world.right() - 20) continue;
        p.drawLine(x, baseY - 6, x, baseY + 6);
        const int labelX = qBound(world.left() + 4, x - 20, world.right() - 44);
        p.drawText(QRect(labelX, baseY + 14, 40, 18), Qt::AlignCenter, QString::number(m) + "m");
    }

    p.setPen(QPen(QColor("#ef4e42"), 4));
    p.drawLine(stopX, zoneTop - 18, stopX, zoneTop + zoneHeight + 18);
    p.setPen(QPen(QColor("#f59e0b"), 4));
    p.drawLine(warningX, zoneTop - 18, warningX, zoneTop + zoneHeight + 18);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    const bool labelsTooClose = qAbs(warningX - stopX) < 132;
    p.setPen(QColor("#ef4e42"));
    const QString stopLineText = labelsTooClose ? "STOP" : "STOP line";
    const int stopLabelWidth = labelsTooClose ? 54 : 104;
    const int lineLabelY = qMax(legend.bottom() + 6, zoneTop - 30);
    int stopLabelX = labelsTooClose
        ? qBound(world.left() + 4, stopX - stopLabelWidth - 8, world.right() - stopLabelWidth - 4)
        : qBound(world.left() + 4, stopX - stopLabelWidth / 2, world.right() - stopLabelWidth - 4);
    QRect stopLabelRect(stopLabelX, lineLabelY, stopLabelWidth, 18);
    p.setPen(Qt::NoPen);
    p.setBrush(tagBg);
    p.drawRoundedRect(stopLabelRect.adjusted(1, 1, -1, -1), 4, 4);
    p.setPen(QColor("#ef4e42"));
    p.drawText(stopLabelRect, Qt::AlignCenter, stopLineText);
    p.setPen(QColor("#f59e0b"));
    const QString warningLineText = labelsTooClose ? "WARN" : "WARNING line";
    const int warningLabelWidth = labelsTooClose ? 58 : 124;
    int warningLabelX = labelsTooClose
        ? qBound(world.left() + 4, warningX + 8, world.right() - warningLabelWidth - 4)
        : qBound(world.left() + 4, warningX - warningLabelWidth / 2, world.right() - warningLabelWidth - 4);
    QRect warningLabelRect(warningLabelX, lineLabelY + (labelsTooClose ? 18 : 0), warningLabelWidth, 18);
    p.setPen(Qt::NoPen);
    p.setBrush(tagBg);
    p.drawRoundedRect(warningLabelRect.adjusted(1, 1, -1, -1), 4, 4);
    p.setPen(QColor("#f59e0b"));
    p.drawText(warningLabelRect, Qt::AlignCenter, warningLineText);

    QColor robotColor = result.state == "STOP" ? QColor("#ef4e42")
                       : (result.state == "DANGER" ? QColor("#f97316") : QColor("#26a47e"));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#167965"));
    p.drawRoundedRect(QRect(robotX - 34, robotBaseY - 55, 68, 42), 11, 11);
    p.setBrush(robotColor);
    p.drawRoundedRect(QRect(robotX - 51, robotBaseY - 16, 102, 68), 17, 17);
    p.drawRect(QRect(robotX - 69, robotBaseY - 4, 26, 38));
    p.drawRect(QRect(robotX + 43, robotBaseY - 4, 26, 38));
    p.setBrush(QColor("#116653"));
    p.drawEllipse(QPoint(robotX - 32, robotBaseY + 52), 18, 18);
    p.drawEllipse(QPoint(robotX + 32, robotBaseY + 52), 18, 18);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPoint(robotX - 16, robotBaseY - 36), 7, 7);
    p.drawEllipse(QPoint(robotX + 16, robotBaseY - 36), 7, 7);
    p.setBrush(QColor("#173233"));
    p.drawEllipse(QPoint(robotX - 16, robotBaseY - 36), 3, 3);
    p.drawEllipse(QPoint(robotX + 16, robotBaseY - 36), 3, 3);
    p.setBrush(QColor("#6cd1b7"));
    p.drawEllipse(QPoint(robotX, robotBaseY + 5), 9, 9);

    const int humanEdgeX = keepInWorld(mapFixedObject(input.humanDistance) - (input.humanDistance <= 0.1 ? 26 : 0), 48);
    const int obsEdgeX = keepInWorld(mapFixedObject(input.obstacleDistance), 42);
    int humanX = humanEdgeX + 16;
    int obsX = obsEdgeX + 10;

    const QColor humanColor = zoneColorForDistance(input.humanDistance);
    p.setPen(Qt::NoPen);
    p.setBrush(humanColor);
    p.drawEllipse(QPoint(humanX, humanY - 16), 10, 10);
    p.setBrush(humanColor);
    p.drawRoundedRect(QRect(humanX - 7, humanY - 3, 14, 25), 6, 6);
    p.setPen(QPen(humanColor, 5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(humanX - 7, humanY + 3, humanX - 18, humanY + 14);
    p.drawLine(humanX + 7, humanY + 3, humanX + 18, humanY + 14);
    p.drawLine(humanX - 4, humanY + 20, humanX - 15, humanY + 34);
    p.drawLine(humanX + 4, humanY + 20, humanX + 16, humanY + 33);
    p.setPen(QPen(QColor("#19192b"), 2));
    p.drawLine(humanX - 4, humanY - 16, humanX + 4, humanY - 16);
    p.setPen(QColor("#ffffff"));
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    const int humanNameY = humanY - 47;
    const int humanDistanceY = humanY - 67;
    const int humanNameX = qBound(world.left() + 8, humanX - 34, world.right() - 76);
    p.drawText(QRect(humanNameX, humanNameY, 68, 18), Qt::AlignCenter, "Human");
    p.setPen(QPen(QColor("#6b7280"), 1));
    p.drawLine(humanEdgeX, humanY + 34, humanEdgeX, baseY - 8);
    p.setPen(zoneColorForDistance(input.humanDistance));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    const int humanDistanceX = qBound(world.left() + 8, humanX - 66, world.right() - 140);
    p.drawText(QRect(humanDistanceX, humanDistanceY, 132, 18), Qt::AlignCenter,
               QString("Human %1m").arg(input.humanDistance, 0, 'f', 1));

    const QColor obstacleColor = zoneColorForDistance(input.obstacleDistance);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2f3647"));
    p.drawRoundedRect(QRect(obsX - 23, obstacleY - 13, 46, 30), 7, 7);
    p.setBrush(obstacleColor);
    QPolygon cone;
    cone << QPoint(obsX, obstacleY - 31)
         << QPoint(obsX - 24, obstacleY + 12)
         << QPoint(obsX + 24, obstacleY + 12);
    p.drawPolygon(cone);
    p.setBrush(QColor(255, 255, 255, 180));
    p.drawRect(QRect(obsX - 14, obstacleY - 4, 28, 6));
    p.setBrush(QColor("#65728a"));
    p.drawRoundedRect(QRect(obsX - 20, obstacleY + 13, 40, 10), 3, 3);
    p.setPen(QColor("#ffffff"));
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    const int obstacleNameX = qBound(world.left() + 8, obsX + 24, world.right() - 92);
    p.drawText(QRect(obstacleNameX, obstacleY + 10, 84, 18), Qt::AlignLeft | Qt::AlignVCenter, "Obstacle");
    p.setPen(QPen(QColor("#6b7280"), 1));
    p.drawLine(obsEdgeX, baseY + 8, obsEdgeX, obstacleY - 20);
    p.setPen(zoneColorForDistance(input.obstacleDistance));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    const QString obstacleText = QString("Obstacle %1m -> %2")
        .arg(input.obstacleDistance, 0, 'f', 1)
        .arg(zoneNameForDistance(input.obstacleDistance));
    const int obstacleLabelWidth = qMin(190, world.width() - 24);
    const int obstacleLabelX = qBound(world.left() + 8, obsX - obstacleLabelWidth / 2, world.right() - obstacleLabelWidth - 8);
    p.drawText(QRect(obstacleLabelX, movementY + 18, obstacleLabelWidth, 18), Qt::AlignCenter, obstacleText);

    const int brakeY = baseY + 34;
    const QColor brakeColor("#38bdf8");
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(56, 189, 248, 54));
    p.drawRoundedRect(QRect(trackLeft, brakeY - 5, qMax(2, brakeX - trackLeft), 10), 4, 4);
    p.setPen(QPen(brakeColor, 2, Qt::DashLine));
    p.drawLine(trackLeft, brakeY, brakeX, brakeY);
    p.setPen(QPen(brakeColor, 2));
    p.drawLine(trackLeft, brakeY - 8, trackLeft, brakeY + 8);
    p.drawLine(brakeX, brakeY - 8, brakeX, brakeY + 8);
    p.setPen(brakeColor);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    const QString brakeText = QString("Brake distance = %1 m").arg(result.brakeDistance, 0, 'f', 2);
    const int brakeLabelWidth = 166;
    const int brakeLabelX = qBound(world.left() + 8, trackLeft, world.right() - brakeLabelWidth - 8);
    QRect brakeLabelRect(brakeLabelX, brakeY - 24, brakeLabelWidth, 18);
    p.setPen(Qt::NoPen);
    p.setBrush(tagBg);
    p.drawRoundedRect(brakeLabelRect.adjusted(1, 1, -1, -1), 4, 4);
    p.setPen(brakeColor);
    p.drawText(brakeLabelRect.adjusted(7, 0, -7, 0), Qt::AlignVCenter | Qt::AlignLeft, brakeText);

    const QColor doorColor = input.doorOpenRate >= 70.0 ? QColor("#ef4e42")
                         : (input.doorOpenRate >= 25.0 ? QColor("#f59e0b") : QColor("#26a47e"));
    const int infoBoxWidth = qMin(390, world.width() - 36);
    QRect doorBox(world.left() + 18, world.bottom() - 76, infoBoxWidth, 26);
    p.setPen(QPen(doorColor, 1));
    p.setBrush(panelBg);
    p.drawRoundedRect(doorBox, 4, 4);
    p.setPen(doorColor);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(doorBox.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
               QString("Door: %1% -> %2").arg(input.doorOpenRate, 0, 'f', 0)
                   .arg(input.doorOpenRate >= 70.0 ? "STOP" : (input.doorOpenRate >= 25.0 ? "WARNING" : "Normal")));

    const double nearest = qMin(input.humanDistance, input.obstacleDistance);
    const QString objectName = input.humanDistance <= input.obstacleDistance ? "Human" : "Obstacle";
    const QString statusText = result.state == "STOP"
        ? QString("%1 touched the robot.").arg(objectName)
        : (result.state == "DANGER"
            ? QString("%1 is inside the STOP area. DANGER until contact.").arg(objectName)
            : (nearest <= input.warningDistance
                ? QString("%1 entered the WARNING area.").arg(objectName)
                : QString("All objects are outside the warning line.")));
    QRect statusBox(world.left() + 18, world.bottom() - 40, infoBoxWidth, 28);
    p.setPen(QPen(stateColor(result.state), 1));
    p.setBrush(panelBg);
    p.drawRoundedRect(statusBox, 4, 4);
    p.setPen(primaryText);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(statusBox.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft,
               statusText + QString(" Nearest object: %1 m").arg(nearest, 0, 'f', 1));

    if (!riskHistoryVisible) return;

    QRect chart(24, world.bottom() + gap, width() - 48, height() - world.bottom() - gap - 24);
    if (chart.height() < 150) return;
    p.setPen(QPen(borderColor, 1));
    p.setBrush(chartBg);
    p.drawRect(chart);
    p.setPen(primaryText);
    p.setFont(title);
    p.drawText(chart.adjusted(18, 12, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Risk History");
    p.setFont(QFont("Segoe UI", 9));
    p.setPen(secondaryText);
    p.drawText(chart.adjusted(18, 34, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Higher line means the robot is closer to WARNING or STOP.");

    QRect plot = chart.adjusted(50, 50, -24, -26);
    const int plotPad = 4;
    p.setPen(QPen(QColor("#6b7280"), 1));
    p.drawRect(plot);
    p.drawText(plot.left() - 36, plot.top() + 8, "100");
    p.drawText(plot.left() - 26, plot.bottom(), "0");
    p.setPen(QPen(QColor("#343a46"), 1));
    for (int i = 1; i < 4; ++i) {
        int y = plot.top() + i * plot.height() / 4;
        p.drawLine(plot.left(), y, plot.right(), y);
    }

    if (history.size() == 1) {
        double y = plot.bottom() - plotPad - history.first() / 100.0 * (plot.height() - plotPad * 2);
        p.setPen(Qt::NoPen);
        p.setBrush(stateColor(result.state));
        p.drawEllipse(QPointF(plot.left(), y), 4, 4);
    } else if (history.size() > 1) {
        QPainterPath path;
        for (int i = 0; i < history.size(); ++i) {
            double x = plot.left() + double(i) / qMax(1, history.size() - 1) * plot.width();
            double y = plot.bottom() - plotPad - history[i] / 100.0 * (plot.height() - plotPad * 2);
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        p.setPen(QPen(stateColor(result.state), 3));
        p.drawPath(path);
    }
}

RiskPage::RiskPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("RiskPage");
    styleApp();
    buildUi();
    connect(&timer, &QTimer::timeout, this, &RiskPage::stepSimulation);
    resetSimulation();
}

void RiskPage::styleApp()
{
    const QString pageBg = lightMode ? "#f3f4f6" : "#1e1e1e";
    const QString panelBg = lightMode ? "#ffffff" : "#1e1e1e";
    const QString boxBg = lightMode ? "#ffffff" : "#242428";
    const QString boxBg2 = lightMode ? "#f8fafc" : "#202024";
    const QString text = lightMode ? "#111827" : "#ffffff";
    const QString subText = lightMode ? "#4b5563" : "#b8b8b8";
    const QString border = lightMode ? "#9ca3af" : "#565656";
    const QString buttonBg = lightMode ? "#ffffff" : "#2b2b2b";
    const QString buttonHover = lightMode ? "#eef2f7" : "#353535";
    const QString tableBg = lightMode ? "#ffffff" : "#202020";
    const QString tableHead = lightMode ? "#e5e7eb" : "#292929";
    const QString tabBg = lightMode ? "#f3f4f6" : "#252525";

    QString sheet =
        "QWidget#RiskPage { background:@PAGE@; color:@TEXT@; font-family:'Segoe UI'; }"
        "QLabel { color:@TEXT@; }"
        "QLabel#Title { color:@TEXT@; font-size:24px; font-weight:800; }"
        "QLabel#SubTitle { color:@SUB@; font-size:12px; }"
        "QLabel#Guide { background:@BOX@; color:@TEXT@; border:1px solid @BORDER@; border-radius:4px; padding:9px 14px; font-size:13px; }"
        "QLabel#MetricLine { color:@TEXT@; font-size:12px; padding:2px 0; }"
        "QLabel#ReasonBox { background:@BOX@; border:1px solid @BORDER@; border-radius:4px; padding:7px; color:@TEXT@; font-size:12px; }"
        "QLabel#CurrentValuesBox { background:@BOX2@; border:1px solid @BORDER@; border-radius:4px; padding:7px; color:@TEXT@; font-size:11px; }"
        "QLabel#RuleBox { background:@RULEBG@; border:1px solid #ef4e42; border-radius:4px; padding:7px; color:@RULETEXT@; font-size:12px; font-weight:700; }"
        "QFrame#Header { background:@PAGE@; border:0; }"
        "QGroupBox { background:@PANEL@; border:1px solid @BORDER@; border-radius:5px; margin-top:14px; font-weight:400; color:@TEXT@; }"
        "QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 4px; background:@PANEL@; color:@TEXT@; }"
        "QPushButton { background:@BUTTON@; color:@TEXT@; border:1px solid @BORDER@; border-radius:5px; padding:8px 13px; }"
        "QPushButton:hover { background:@HOVER@; border-color:@BORDER@; }"
        "QPushButton:checked { background:#374151; color:#ffffff; border-color:#6b7280; }"
        "QPushButton#Primary { background:#ef4e42; color:white; border:1px solid #ef4e42; font-weight:700; }"
        "QPushButton#Primary:hover { background:#ff594d; }"
        "QPushButton#Danger { background:@BUTTON@; color:@TEXT@; border:1px solid @BORDER@; font-weight:700; }"
        "QSlider::groove:horizontal { height:4px; background:#9ca3af; border-radius:2px; }"
        "QSlider::sub-page:horizontal { background:#ef4e42; border-radius:2px; }"
        "QSlider::handle:horizontal { background:#f2f2f2; border:1px solid #9a9a9a; width:13px; height:13px; margin:-5px 0; border-radius:7px; }"
        "QCheckBox { color:@TEXT@; spacing:9px; }"
        "QCheckBox::indicator { width:18px; height:18px; border:1px solid @BORDER@; border-radius:4px; background:@BOX@; }"
        "QCheckBox::indicator:checked { background:#ef4e42; border:1px solid #ef4e42; }"
        "QTabWidget::pane { border:1px solid @BORDER@; background:@PANEL@; }"
        "QTabBar::tab { background:@TAB@; color:@TEXT@; padding:8px 16px; border:1px solid @BORDER@; border-bottom:0; }"
        "QTabBar::tab:selected { background:@PANEL@; color:@TEXT@; }"
        "QTableWidget { background:@TABLE@; color:@TEXT@; gridline-color:@BORDER@; border:1px solid @BORDER@; selection-background-color:#ef4e42; }"
        "QHeaderView::section { background:@HEAD@; color:@TEXT@; padding:7px; border:0px; border-bottom:1px solid @BORDER@; font-weight:700; }";
    sheet.replace("@PAGE@", pageBg);
    sheet.replace("@PANEL@", panelBg);
    sheet.replace("@BOX@", boxBg);
    sheet.replace("@BOX2@", boxBg2);
    sheet.replace("@TEXT@", text);
    sheet.replace("@SUB@", subText);
    sheet.replace("@BORDER@", border);
    sheet.replace("@BUTTON@", buttonBg);
    sheet.replace("@HOVER@", buttonHover);
    sheet.replace("@RULEBG@", lightMode ? "#fff7ed" : "#30251d");
    sheet.replace("@RULETEXT@", lightMode ? "#b91c1c" : "#ffb4a9");
    sheet.replace("@TAB@", tabBg);
    sheet.replace("@TABLE@", tableBg);
    sheet.replace("@HEAD@", tableHead);
    setStyleSheet(sheet);
}

void RiskPage::toggleTheme()
{
    lightMode = !lightMode;
    styleApp();
    if (themeToggleButton) {
        themeToggleButton->setText(lightMode ? "Dark Mode" : "White Mode");
    }
    if (simulationView) {
        simulationView->setLightMode(lightMode);
    }
}

QLabel *RiskPage::makeValueLabel(const QString &caption)
{
    QLabel *label = new QLabel(caption);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setMinimumWidth(78);
    return label;
}

QSlider *RiskPage::makeSlider(int min, int max, int value, QLabel *, const QString &)
{
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    connect(slider, &QSlider::valueChanged, this, [this]() { updateSliderLabels(); evaluateAndRender("Input Change"); });
    return slider;
}

void RiskPage::buildUi()
{
    QWidget *central = new QWidget;
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    QFrame *header = new QFrame;
    header->setObjectName("Header");
    QVBoxLayout *head = new QVBoxLayout(header);
    head->setContentsMargins(16, 14, 16, 8);
    head->setSpacing(8);
    QVBoxLayout *titles = new QVBoxLayout;
    titles->setSpacing(4);
    QLabel *title = new QLabel("Robot Logic Circuit Simulator");
    title->setObjectName("Title");
    title->setAlignment(Qt::AlignCenter);
    QLabel *sub = new QLabel("Safety risk module: adjust sensor values and check the robot state.");
    sub->setObjectName("SubTitle");
    sub->setAlignment(Qt::AlignCenter);
    titles->addWidget(title);
    titles->addWidget(sub);
    head->addLayout(titles);

    QHBoxLayout *presetRow = new QHBoxLayout;
    presetRow->setSpacing(8);
    presetRow->addStretch();
    themeToggleButton = new QPushButton("White Mode");
    QPushButton *normal = new QPushButton("Normal");
    QPushButton *human = new QPushButton("Human Approach");
    QPushButton *obstacle = new QPushButton("Obstacle Approach");
    simulateButton = new QPushButton("Start Robot");
    simulateButton->setObjectName("Primary");
    QPushButton *reset = new QPushButton("Reset");
    reset->setObjectName("Danger");
    presetRow->addWidget(themeToggleButton);
    presetRow->addWidget(normal);
    presetRow->addWidget(human);
    presetRow->addWidget(obstacle);
    presetRow->addWidget(simulateButton);
    presetRow->addWidget(reset);
    head->addLayout(presetRow);

    root->addWidget(header);


    QLabel *guide = new QLabel("Read left to right: set sensor values -> run the robot forward -> check why the robot slows down or stops.");
    guide->setObjectName("Guide");
    guide->setAlignment(Qt::AlignCenter);
    root->addWidget(guide);

    QHBoxLayout *main = new QHBoxLayout;
    main->setContentsMargins(10, 10, 10, 2);
    main->setSpacing(12);
    main->addWidget(buildControlPanel());

    QGroupBox *viewBox = new QGroupBox("Logic Gate Diagram");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewBox);
    viewLayout->setContentsMargins(12, 18, 12, 8);
    simulationView = new SimulationView;
    viewLayout->addWidget(simulationView);
    main->addWidget(viewBox, 1);

    main->addWidget(buildResultPanel());
    root->addLayout(main, 1);

    QHBoxLayout *outputTools = new QHBoxLayout;
    outputTools->setContentsMargins(10, 0, 10, 8);
    outputTools->setSpacing(8);
    outputTools->addStretch();
    historyToggleButton = new QPushButton("Risk History");
    historyToggleButton->setCheckable(true);
    historyToggleButton->setFixedWidth(136);
    outputTools->addWidget(historyToggleButton);
    outputToggleButton = new QPushButton("Log / Summary");
    outputToggleButton->setCheckable(true);
    outputToggleButton->setFixedWidth(136);
    outputTools->addWidget(outputToggleButton);
    root->addLayout(outputTools);
    outputPanel = buildOutputPanel();
    outputPanel->hide();
    root->addWidget(outputPanel);
    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(central);

    connect(normal, &QPushButton::clicked, this, &RiskPage::runPresetNormal);
    connect(themeToggleButton, &QPushButton::clicked, this, &RiskPage::toggleTheme);
    connect(human, &QPushButton::clicked, this, &RiskPage::runPresetHumanApproach);
    connect(obstacle, &QPushButton::clicked, this, &RiskPage::runPresetObstacleApproach);
    connect(simulateButton, &QPushButton::clicked, this, &RiskPage::toggleSimulation);
    connect(reset, &QPushButton::clicked, this, &RiskPage::resetSimulation);
    connect(historyToggleButton, &QPushButton::clicked, this, [this]() {
        bool show = historyToggleButton->isChecked();
        if (show && outputPanel->isVisible()) {
            outputPanel->hide();
            outputToggleButton->setChecked(false);
        }
        simulationView->setRiskHistoryVisible(show);
    });
    connect(outputToggleButton, &QPushButton::clicked, this, [this]() {
        bool show = outputToggleButton->isChecked();
        if (show) {
            simulationView->setRiskHistoryVisible(false);
            historyToggleButton->setChecked(false);
        }
        outputPanel->setVisible(show);
    });
}

QWidget *RiskPage::buildControlPanel()
{
    QGroupBox *box = new QGroupBox("Sensor Input Panel");
    box->setFixedWidth(310);
    QGridLayout *grid = new QGridLayout(box);
    grid->setContentsMargins(14, 24, 14, 14);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(14);

    humanValue = makeValueLabel("8.0 m");
    obstacleValue = makeValueLabel("10.0 m");
    doorValue = makeValueLabel("0 %");
    speedValue = makeValueLabel("1.0 m/s");
    decelValue = makeValueLabel("1.5 m/s2");
    warningValue = makeValueLabel("6.0 m");
    stopValue = makeValueLabel("2.5 m");

    humanSlider = makeSlider(0, 200, 80, humanValue, "m");
    obstacleSlider = makeSlider(0, 200, 100, obstacleValue, "m");
    doorSlider = makeSlider(0, 100, 0, doorValue, "%");
    speedSlider = makeSlider(0, 50, 10, speedValue, "m/s");
    decelSlider = makeSlider(5, 50, 15, decelValue, "m/s2");
    warningSlider = makeSlider(10, 120, 60, warningValue, "m");
    stopSlider = makeSlider(5, 80, 25, stopValue, "m");
    emergencyCheckBox = new QCheckBox("Emergency Signal");
    connect(emergencyCheckBox, &QCheckBox::toggled, this, [this]() { evaluateAndRender("Input Change"); });

    QStringList names = {"Human", "Obstacle", "Door", "Target Speed", "Braking Power", "Warning Line", "Stop Line"};
    QList<QSlider*> sliders = {humanSlider, obstacleSlider, doorSlider, speedSlider, decelSlider, warningSlider, stopSlider};
    QList<QLabel*> labels = {humanValue, obstacleValue, doorValue, speedValue, decelValue, warningValue, stopValue};
    for (int i = 0; i < names.size(); ++i) {
        grid->addWidget(new QLabel(names[i]), i, 0);
        grid->addWidget(sliders[i], i, 1);
        grid->addWidget(labels[i], i, 2);
    }
    grid->addWidget(emergencyCheckBox, names.size(), 0, 1, 3);

    QPushButton *run = new QPushButton("RUN");
    run->setObjectName("Primary");
    QPushButton *step = new QPushButton("Step");
    QPushButton *exportButton = new QPushButton("Export");
    grid->addWidget(run, names.size() + 1, 0, 1, 3);
    grid->addWidget(step, names.size() + 2, 0, 1, 3);
    grid->addWidget(exportButton, names.size() + 3, 0, 1, 3);

    connect(run, &QPushButton::clicked, this, &RiskPage::evaluateManual);
    connect(step, &QPushButton::clicked, this, &RiskPage::stepSimulation);
    connect(exportButton, &QPushButton::clicked, this, &RiskPage::exportLogCsv);

    return box;
}

QWidget *RiskPage::buildResultPanel()
{
    QGroupBox *box = new QGroupBox("Robot State");
    box->setFixedWidth(320);
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(12, 17, 12, 6);
    layout->setSpacing(3);

    stateLabel = new QLabel("MOVE");
    stateLabel->setAlignment(Qt::AlignCenter);
    stateLabel->setMinimumHeight(52);
    stateLabel->setMaximumHeight(58);
    riskLabel = new QLabel("Risk score: 0 / 100");
    brakeLabel = new QLabel("Brake distance: 0 m");
    marginLabel = new QLabel("Safety margin: 0 m");
    speedRecommendLabel = new QLabel("Robot speed: 0 m/s");
    decisionRuleLabel = new QLabel("Rule: -");
    reasonLabel = new QLabel("Why: -");
    currentValuesLabel = new QLabel(
        "Current Values\n"
        "Human        0.0 m\n"
        "Obstacle     0.0 m\n"
        "Door          0 %\n"
        "Brake         0.0 m");
    for (QLabel *label : {riskLabel, brakeLabel, marginLabel, speedRecommendLabel}) {
        label->setObjectName("MetricLine");
    }
    decisionRuleLabel->setObjectName("RuleBox");
    decisionRuleLabel->setWordWrap(true);
    reasonLabel->setObjectName("ReasonBox");
    reasonLabel->setWordWrap(true);
    currentValuesLabel->setObjectName("CurrentValuesBox");
    currentValuesLabel->setTextFormat(Qt::PlainText);
    currentValuesLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    currentValuesLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    currentValuesLabel->setMinimumHeight(80);
    currentValuesLabel->setMaximumHeight(86);

    layout->addWidget(stateLabel);
    layout->addSpacing(2);
    QHBoxLayout *ledRow = new QHBoxLayout;
    QLabel *ledText = new QLabel("LED:");
    ledDotLabel = new QLabel("●");
    ledDotLabel->setObjectName("LedDot");
    ledRow->addStretch();
    ledRow->addWidget(ledText);
    ledRow->addWidget(ledDotLabel);
    ledRow->addStretch();
    layout->addLayout(ledRow);
    layout->addSpacing(4);
    layout->addWidget(riskLabel);
    layout->addWidget(brakeLabel);
    layout->addWidget(marginLabel);
    layout->addWidget(speedRecommendLabel);
    layout->addSpacing(4);
    layout->addWidget(decisionRuleLabel);
    layout->addWidget(reasonLabel);
    layout->addWidget(currentValuesLabel);
    layout->addStretch();

    return box;
}
QWidget *RiskPage::buildOutputPanel()
{
    QTabWidget *tabs = new QTabWidget;
    tabs->setMinimumHeight(210);
    tabs->setMaximumHeight(250);

    logTable = new QTableWidget(0, 10);
    logTable->setHorizontalHeaderLabels({"#", "Time", "Source", "Human(m)", "Obstacle(m)", "Door(%)", "Speed", "Risk", "Margin", "State"});
    logTable->horizontalHeader()->setStretchLastSection(true);
    logTable->verticalHeader()->setVisible(false);
    tabs->addTab(logTable, "Simulation Log");

    caseTable = new QTableWidget(0, 5);
    caseTable->setHorizontalHeaderLabels({"Parameter", "Current", "Meaning", "Warning", "Stop"});
    caseTable->horizontalHeader()->setStretchLastSection(true);
    caseTable->verticalHeader()->setVisible(false);
    tabs->addTab(caseTable, "Model Summary");

    return tabs;
}

SafetyInput RiskPage::readInput() const
{
    SafetyInput input;
    input.humanDistance = humanSlider->value() / 10.0;
    input.obstacleDistance = obstacleSlider->value() / 10.0;
    input.doorOpenRate = doorSlider->value();
    input.robotSpeed = speedSlider->value() / 10.0;
    input.deceleration = decelSlider->value() / 10.0;
    input.warningDistance = warningSlider->value() / 10.0;
    input.stopDistance = stopSlider->value() / 10.0;
    input.emergency = emergencyCheckBox->isChecked();
    return input;
}

void RiskPage::writeInput(const SafetyInput &input)
{
    humanSlider->setValue(int(input.humanDistance * 10));
    obstacleSlider->setValue(int(input.obstacleDistance * 10));
    doorSlider->setValue(int(input.doorOpenRate));
    speedSlider->setValue(int(input.robotSpeed * 10));
    decelSlider->setValue(int(input.deceleration * 10));
    warningSlider->setValue(int(input.warningDistance * 10));
    stopSlider->setValue(int(input.stopDistance * 10));
    emergencyCheckBox->setChecked(input.emergency);
    updateSliderLabels();
}

void RiskPage::updateSliderLabels()
{
    int minWarning = stopSlider->value() + 10;
    if (warningSlider->value() < minWarning) {
        warningSlider->blockSignals(true);
        warningSlider->setValue(qMin(warningSlider->maximum(), minWarning));
        warningSlider->blockSignals(false);
    }

    humanValue->setText(QString::number(humanSlider->value() / 10.0, 'f', 1) + " m");
    obstacleValue->setText(QString::number(obstacleSlider->value() / 10.0, 'f', 1) + " m");
    doorValue->setText(QString::number(doorSlider->value()) + " %");
    speedValue->setText(QString::number(speedSlider->value() / 10.0, 'f', 1) + " m/s");
    decelValue->setText(QString::number(decelSlider->value() / 10.0, 'f', 1) + " m/s2");
    warningValue->setText(QString::number(warningSlider->value() / 10.0, 'f', 1) + " m");
    stopValue->setText(QString::number(stopSlider->value() / 10.0, 'f', 1) + " m");
}

void RiskPage::evaluateAndRender(const QString &source)
{
    updateSliderLabels();
    SafetyInput input = readInput();
    SafetyResult result = SafetyModel::evaluate(input);
    riskHistory.append(result.riskScore);
    if (riskHistory.size() > 80) riskHistory.removeFirst();
    simulationView->setData(input, result, riskHistory, robotTravel);
    updateResultPanel(result);
    appendLog(source, input, result);

    if (result.state == "STOP" && timer.isActive()) {
        timer.stop();
        if (simulateButton) simulateButton->setText("Start Robot");
    }
}

void RiskPage::updateResultPanel(const SafetyResult &result)
{
    SafetyInput input = readInput();
    QColor color = stateColor(result.state);
    const double contactDistance = 0.1;
    stateLabel->setText(result.state);
    stateLabel->setStyleSheet(QString("font-size:32px; font-weight:700; color:%1; background:transparent; padding:4px;").arg(color.name()));
    if (ledDotLabel) {
        ledDotLabel->setStyleSheet(QString("font-size:18px; color:%1;").arg(color.name()));
    }
    riskLabel->setText(QString("Risk score: %1 / 100").arg(result.riskScore, 0, 'f', 1));
    brakeLabel->setText(QString("Brake distance: %1 m").arg(result.brakeDistance, 0, 'f', 2));
    marginLabel->setText(QString("Safety margin: %1 m").arg(result.safetyMargin, 0, 'f', 2));
    speedRecommendLabel->setText(QString("Robot speed: %1 m/s").arg(result.recommendedSpeed, 0, 'f', 1));
    if (currentValuesLabel) {
        currentValuesLabel->setText(QString(
            "Current Values\n"
            "Human        %1 m\n"
            "Obstacle     %2 m\n"
            "Door         %3 %\n"
            "Brake        %4 m")
            .arg(input.humanDistance, 0, 'f', 1)
            .arg(input.obstacleDistance, 0, 'f', 1)
            .arg(input.doorOpenRate, 0, 'f', 0)
            .arg(result.brakeDistance, 0, 'f', 1));
    }

    QString rule;
    if (input.emergency) {
        rule = "Rule: Emergency signal ON -> STOP";
    } else if (input.doorOpenRate >= 70.0) {
        rule = "Rule: Door >= 70% open -> STOP";
    } else if (input.humanDistance <= contactDistance || input.obstacleDistance <= contactDistance) {
        rule = "Rule: Object touched robot -> STOP";
    } else if (input.humanDistance <= input.stopDistance || input.obstacleDistance <= input.stopDistance) {
        rule = "Rule: Object inside STOP area -> DANGER until contact";
    } else if (result.safetyMargin <= 0.0) {
        rule = "Rule: Brake distance is longer than free space -> WARNING";
    } else if (input.humanDistance <= input.warningDistance || input.obstacleDistance <= input.warningDistance) {
        rule = "Rule: Object inside WARNING line -> WARNING";
    } else if (input.doorOpenRate >= 25.0) {
        rule = "Rule: Door >= 25% open -> WARNING";
    } else if (result.riskScore >= 45.0) {
        rule = "Rule: Risk score is high -> WARNING";
    } else {
        rule = "Rule: All values are outside danger zones -> MOVE";
    }
    decisionRuleLabel->setText(rule);
    reasonLabel->setText("Why: " + result.reason);
}

void RiskPage::appendLog(const QString &source, const SafetyInput &input, const SafetyResult &result)
{
    int row = logTable->rowCount();
    logTable->insertRow(row);
    QStringList values = {
        QString::number(++logIndex),
        QDateTime::currentDateTime().toString("HH:mm:ss"),
        source,
        QString::number(input.humanDistance, 'f', 1),
        QString::number(input.obstacleDistance, 'f', 1),
        QString::number(input.doorOpenRate, 'f', 0),
        QString::number(input.robotSpeed, 'f', 1),
        QString::number(result.riskScore, 'f', 1),
        QString::number(result.safetyMargin, 'f', 2),
        result.state
    };
    for (int i = 0; i < values.size(); ++i) logTable->setItem(row, i, new QTableWidgetItem(values[i]));
    logTable->scrollToBottom();
}

void RiskPage::evaluateManual()
{
    evaluateAndRender("Manual");
}

void RiskPage::stepSimulation()
{
    SafetyInput input = readInput();
    if (input.emergency || input.doorOpenRate >= 70.0 || qMin(input.humanDistance, input.obstacleDistance) <= 0.1) {
        evaluateAndRender("Stop Hold");
        return;
    }

    double moveStep = qMax(0.2, input.robotSpeed * 0.35);
    robotTravel += moveStep;

    if (scenarioMode == 1) {
        input.humanDistance = qMax(0.0, input.humanDistance - moveStep);
    } else if (scenarioMode == 2) {
        input.obstacleDistance = qMax(0.0, input.obstacleDistance - moveStep);
    } else {
        input.humanDistance = qMax(0.0, input.humanDistance - moveStep);
        input.obstacleDistance = qMax(0.0, input.obstacleDistance - moveStep);
    }
    writeInput(input);
    evaluateAndRender("Step");
    ++tick;
}

void RiskPage::toggleSimulation()
{
    if (timer.isActive()) {
        timer.stop();
        simulateButton->setText("Start Robot");
    } else {
        timer.start(650);
        simulateButton->setText("Stop Robot");
    }
}

void RiskPage::resetSimulation()
{
    timer.stop();
    if (simulateButton) simulateButton->setText("Start Robot");
    tick = 0;
    robotTravel = 0.0;
    logIndex = 0;
    riskHistory.clear();
    if (logTable) logTable->setRowCount(0);
    SafetyInput input;
    writeInput(input);
    evaluateAndRender("Reset");

    if (caseTable) {
        caseTable->setRowCount(0);
        QStringList rows[] = {
            {"Human Distance", humanValue->text(), "person distance from robot", "<= warning", "<= stop"},
            {"Obstacle Distance", obstacleValue->text(), "object distance from robot", "<= warning", "<= stop"},
            {"Door Open", doorValue->text(), "open rate of door sensor", ">= 25%", ">= 70%"},
            {"Brake Distance", brakeLabel->text().section(':', 1).trimmed(), "speed^2 / 2a", "near margin", "margin <= 0"},
            {"Risk Score", riskLabel->text().section(':', 1).trimmed(), "combined risk index", ">= 45", "critical"}
        };
        for (const QStringList &rowData : rows) {
            int row = caseTable->rowCount();
            caseTable->insertRow(row);
            for (int c = 0; c < rowData.size(); ++c) caseTable->setItem(row, c, new QTableWidgetItem(rowData[c]));
        }
    }
}

void RiskPage::runPresetNormal()
{
    scenarioMode = 0;
    robotTravel = 0.0;
    SafetyInput input;
    writeInput(input);
    evaluateAndRender("Preset Normal");
}

void RiskPage::runPresetHumanApproach()
{
    scenarioMode = 1;
    robotTravel = 0.0;
    SafetyInput input;
    input.humanDistance = 9.0;
    input.obstacleDistance = 14.0;
    input.robotSpeed = 1.6;
    writeInput(input);
    evaluateAndRender("Preset Human");
}

void RiskPage::runPresetObstacleApproach()
{
    scenarioMode = 2;
    robotTravel = 0.0;
    SafetyInput input;
    input.humanDistance = 14.0;
    input.obstacleDistance = 10.0;
    input.robotSpeed = 1.8;
    writeInput(input);
    evaluateAndRender("Preset Obstacle");
}

void RiskPage::exportLogCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "Export Simulation Log", "risk_log.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    for (int c = 0; c < logTable->columnCount(); ++c) {
        if (c) out << ',';
        out << logTable->horizontalHeaderItem(c)->text();
    }
    out << '\n';
    for (int r = 0; r < logTable->rowCount(); ++r) {
        for (int c = 0; c < logTable->columnCount(); ++c) {
            if (c) out << ',';
            QTableWidgetItem *item = logTable->item(r, c);
            out << (item ? item->text() : "");
        }
        out << '\n';
    }
}

