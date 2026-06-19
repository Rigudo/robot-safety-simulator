// Risk page implementation file.
#include "riskpage.h"

// Qt utility and UI headers.
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
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtMath>

// Convert robot state text into a color used by labels and drawings.
static QColor stateColor(const QString &state)
{
    if (state == "STOP") return QColor("#dc2626");
    if (state == "DANGER") return QColor("#f97316");
    if (state == "WARNING") return QColor("#f59e0b");
    return QColor("#16a34a");
}

// Constructor for the custom drawing widget.
SimulationView::SimulationView(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(620, 360);
}

// Store new simulation data and request repaint.
void SimulationView::setData(const SafetyInput &newInput, const SafetyResult &newResult, const QVector<double> &riskHistory, double robotTravel)
{
    input = newInput;
    result = newResult;
    history = riskHistory;
    travel = robotTravel;
    update();
}

// Show or hide the risk history graph.
void SimulationView::setRiskHistoryVisible(bool visible)
{
    riskHistoryVisible = visible;
    setMinimumSize(620, riskHistoryVisible ? 540 : 360);
    updateGeometry();
    update();
}

// Switch drawing colors between dark and light mode.
void SimulationView::setLightMode(bool enabled)
{
    lightMode = enabled;
    update();
}

// Draw the robot field, objects, threshold lines, and optional graph.
void SimulationView::paintEvent(QPaintEvent *)
{
    // Prepare painter.
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Theme colors are kept close to the existing Logic Page design.
    const QColor bg = lightMode ? QColor("#f3f4f6") : QColor("#1e1e1e");
    const QColor panel = lightMode ? QColor("#ffffff") : QColor("#19192b");
    const QColor text = lightMode ? QColor("#111827") : QColor("#ffffff");
    const QColor sub = lightMode ? QColor("#374151") : QColor("#b8c2d6");
    const QColor border = lightMode ? QColor("#9ca3af") : QColor("#8d8d8d");
    const QColor road = lightMode ? QColor("#dbeafe") : QColor("#17243d");

    // Draw background.
    p.fillRect(rect(), bg);

    // Main diagram area. When risk history is enabled, the diagram becomes shorter.
    QRect field(24, 8, width() - 48, riskHistoryVisible ? 315 : height() - 16);
    p.setPen(QPen(border, 1));
    p.setBrush(panel);
    p.drawRect(field);

    // Title and short explanation inside the diagram.
    p.setPen(text);
    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.drawText(field.adjusted(18, 16, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Robot Movement Field");
    p.setPen(sub);
    p.setFont(QFont("Segoe UI", 9));
    p.drawText(field.adjusted(18, 42, -18, -18), Qt::AlignTop | Qt::AlignLeft,
               "The robot moves forward. Distance to fixed objects becomes smaller.");

    // Basic geometry: the robot stays on a ground line like the Logic Page diagram.
    const int baseY = field.center().y() + 40;
    const int robotX = field.left() + 100 + qMin(int(travel * 8.0), 95);
    const int roadLeft = field.left() + 18;
    const int roadRight = field.right() - 18;
    const double pxPerMeter = qMax(1.0, double(roadRight - robotX - 120) / 20.0);

    // Convert distance in meters to x coordinate.
    auto mapDistance = [&](double d) {
        return qBound(roadLeft, robotX + 82 + int(d * pxPerMeter), roadRight);
    };

    // Calculate threshold and object positions.
    const int stopX = mapDistance(input.stopDistance);
    const int warningX = mapDistance(input.warningDistance);
    const int brakeX = mapDistance(result.brakeDistance);
    const int humanX = mapDistance(input.humanDistance);
    const int obstacleX = mapDistance(input.obstacleDistance);

    // Draw the full road area first so SAFE zone visually reaches the end.
    QRect roadRect(roadLeft, baseY - 42, roadRight - roadLeft, 84);
    p.setPen(Qt::NoPen);
    p.setBrush(road);
    p.drawRect(roadRect);

    // Draw stop, warning, and safe zones as transparent overlays.
    p.setBrush(QColor(239, 78, 66, 55));
    p.drawRect(QRect(robotX + 72, roadRect.top(), qMax(0, stopX - robotX - 72), roadRect.height()));
    p.setBrush(QColor(245, 158, 11, 50));
    p.drawRect(QRect(stopX, roadRect.top(), qMax(0, warningX - stopX), roadRect.height()));
    p.setBrush(QColor(38, 164, 126, 45));
    p.drawRect(QRect(warningX, roadRect.top(), qMax(0, roadRight - warningX), roadRect.height()));

    // Draw dashed road boundary and solid ground line. This restores the robot-on-ground feeling.
    p.setPen(QPen(QColor("#a16255"), 1, Qt::DashLine));
    p.drawLine(roadLeft, roadRect.top(), roadRight, roadRect.top());
    p.drawLine(roadLeft, roadRect.bottom(), roadRight, roadRect.bottom());
    p.setPen(QPen(lightMode ? QColor("#2563eb") : QColor("#3b82f6"), 3));
    p.drawLine(roadLeft, baseY + 42, roadRight, baseY + 42);

    // Draw threshold lines and labels.
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.setPen(QPen(QColor("#ef4e42"), 4));
    p.drawLine(stopX, baseY - 58, stopX, baseY + 58);
    p.setPen(QPen(QColor("#f59e0b"), 4));
    p.drawLine(warningX, baseY - 58, warningX, baseY + 58);
    p.setPen(QColor("#ef4e42"));
    p.drawText(QRect(stopX - 45, baseY - 82, 90, 20), Qt::AlignCenter, "STOP");
    p.setPen(QColor("#f59e0b"));
    p.drawText(QRect(warningX - 55, baseY - 82, 110, 20), Qt::AlignCenter, "WARN");

    // Draw brake distance as a thin auxiliary line, not as a dominant design element.
    p.setPen(QPen(QColor("#38bdf8"), 3, Qt::DashLine));
    p.drawLine(robotX + 72, baseY + 24, brakeX, baseY + 24);
    p.setPen(QColor("#38bdf8"));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(QRect(robotX + 72, baseY + 28, 190, 20), Qt::AlignLeft,
               QString("Brake distance = %1 m").arg(result.brakeDistance, 0, 'f', 2));

    // Draw scale marks, similar to a simple simulator axis.
    p.setFont(QFont("Segoe UI", 8));
    p.setPen(QColor("#9ca3af"));
    for (int m : {5, 10, 15}) {
        int x = mapDistance(m);
        p.drawLine(x, baseY - 8, x, baseY + 8);
        p.drawText(QRect(x - 20, baseY + 12, 40, 18), Qt::AlignCenter, QString::number(m) + "m");
    }

    // Draw robot body in the same simple icon style as the Logic Page.
    const QColor robotBody = stateColor(result.state);
    p.setPen(Qt::NoPen);
    p.setBrush(robotBody);
    p.drawRoundedRect(QRect(robotX - 45, baseY - 48, 90, 58), 12, 12);
    p.drawRoundedRect(QRect(robotX - 30, baseY - 86, 60, 36), 10, 10);
    p.drawRect(QRect(robotX - 66, baseY - 28, 22, 34));
    p.drawRect(QRect(robotX + 44, baseY - 28, 22, 34));
    p.setBrush(QColor("#0f766e"));
    p.drawEllipse(QPoint(robotX - 28, baseY + 22), 17, 17);
    p.drawEllipse(QPoint(robotX + 28, baseY + 22), 17, 17);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPoint(robotX - 14, baseY - 66), 6, 6);
    p.drawEllipse(QPoint(robotX + 14, baseY - 66), 6, 6);
    p.setBrush(QColor("#6ee7b7"));
    p.drawEllipse(QPoint(robotX, baseY - 22), 9, 9);

    // Draw human marker. It remains simple and icon-like.
    QColor humanColor = input.humanDistance <= input.stopDistance ? QColor("#ef4e42")
                       : input.humanDistance <= input.warningDistance ? QColor("#f59e0b")
                       : QColor("#26a47e");
    p.setPen(QPen(humanColor, 5, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(humanColor);
    p.drawEllipse(QPoint(humanX, baseY - 75), 10, 10);
    p.drawLine(humanX, baseY - 64, humanX, baseY - 30);
    p.drawLine(humanX, baseY - 50, humanX - 18, baseY - 36);
    p.drawLine(humanX, baseY - 50, humanX + 18, baseY - 36);
    p.drawLine(humanX, baseY - 30, humanX - 15, baseY - 10);
    p.drawLine(humanX, baseY - 30, humanX + 15, baseY - 10);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.setPen(humanColor);
    p.drawText(QRect(humanX - 70, baseY - 112, 140, 18), Qt::AlignCenter,
               QString("Human %1 m").arg(input.humanDistance, 0, 'f', 1));

    // Draw obstacle marker. Keep the cone style and avoid extra label boxes.
    QColor obstacleColor = input.obstacleDistance <= input.stopDistance ? QColor("#ef4e42")
                          : input.obstacleDistance <= input.warningDistance ? QColor("#f59e0b")
                          : QColor("#26a47e");
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#475569"));
    p.drawRoundedRect(QRect(obstacleX - 25, baseY + 10, 50, 34), 6, 6);
    p.setBrush(obstacleColor);
    QPolygon cone;
    cone << QPoint(obstacleX, baseY - 8)
         << QPoint(obstacleX - 24, baseY + 36)
         << QPoint(obstacleX + 24, baseY + 36);
    p.drawPolygon(cone);
    p.setPen(obstacleColor);
    p.drawText(QRect(obstacleX - 80, baseY + 48, 160, 18), Qt::AlignCenter,
               QString("Obstacle %1 m").arg(input.obstacleDistance, 0, 'f', 1));

    // Draw compact status lines at the bottom of the diagram.
    p.setPen(QPen(QColor("#26a47e"), 1));
    p.setBrush(panel);
    QRect doorBox(field.left() + 18, field.bottom() - 58, field.width() - 36, 24);
    p.drawRoundedRect(doorBox, 4, 4);
    p.setPen(QColor("#10b981"));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(doorBox.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
               QString("Door: %1% -> %2").arg(input.doorOpenRate, 0, 'f', 0).arg(input.doorOpenRate >= 70.0 ? "STOP" : input.doorOpenRate >= 25.0 ? "WARNING" : "Normal"));

    QRect summaryBox(field.left() + 18, field.bottom() - 30, field.width() - 36, 24);
    p.drawRoundedRect(summaryBox, 4, 4);
    p.setPen(text);
    p.drawText(summaryBox.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
               QString("State %1 | Risk %2 | Safety margin %3 m | Nearest object %4 m")
                   .arg(result.state)
                   .arg(result.riskScore, 0, 'f', 1)
                   .arg(result.safetyMargin, 0, 'f', 2)
                   .arg(result.nearestDistance, 0, 'f', 1));

    // Draw graph only when visible.
    if (!riskHistoryVisible) return;
    QRect chart(24, field.bottom() + 16, width() - 48, height() - field.bottom() - 40);
    if (chart.height() < 140) return;
    p.setPen(QPen(border, 1));
    p.setBrush(panel);
    p.drawRect(chart);
    p.setPen(text);
    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.drawText(chart.adjusted(18, 12, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Risk History");
    p.setFont(QFont("Segoe UI", 9));
    p.setPen(sub);
    p.drawText(chart.adjusted(18, 36, -18, -18), Qt::AlignTop | Qt::AlignLeft,
               "Higher line means the robot is closer to WARNING or STOP.");
    QRect plot = chart.adjusted(50, 58, -24, -24);
    p.setPen(QPen(QColor("#6b7280"), 1));
    p.drawRect(plot);
    p.setPen(QPen(QColor("#374151"), 1));
    for (int i = 1; i < 4; ++i) {
        int y = plot.top() + i * plot.height() / 4;
        p.drawLine(plot.left(), y, plot.right(), y);
    }
    p.setPen(sub);
    p.drawText(plot.left() - 34, plot.top() + 8, "100");
    p.drawText(plot.left() - 24, plot.bottom(), "0");
    if (history.size() > 1) {
        QPainterPath path;
        for (int i = 0; i < history.size(); ++i) {
            double x = plot.left() + double(i) / qMax(1, history.size() - 1) * plot.width();
            double y = plot.bottom() - history[i] / 100.0 * plot.height();
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        p.setPen(QPen(stateColor(result.state), 3));
        p.drawPath(path);
    }
}

// Create the main page and initialize timer connection.
RiskPage::RiskPage(QWidget *parent) : QWidget(parent)
{
    setObjectName("RiskPage");
    styleApp();
    buildUi();
    connect(&timer, &QTimer::timeout, this, &RiskPage::stepSimulation);
    resetSimulation();
}

// Apply stylesheet for dark or light mode.
void RiskPage::styleApp()
{
    const QString pageBg = lightMode ? "#f3f4f6" : "#1e1e1e";
    const QString panelBg = lightMode ? "#ffffff" : "#242428";
    const QString text = lightMode ? "#111827" : "#ffffff";
    const QString subText = lightMode ? "#4b5563" : "#b8b8b8";
    const QString border = lightMode ? "#9ca3af" : "#565656";
    QString sheet;
    sheet += "QWidget#RiskPage { background:" + pageBg + "; color:" + text + "; font-family:'Segoe UI'; }";
    sheet += "QLabel { color:" + text + "; }";
    sheet += "QLabel#Title { color:" + text + "; font-size:24px; font-weight:800; }";
    sheet += "QLabel#SubTitle { color:" + subText + "; font-size:12px; }";
    sheet += "QLabel#Guide { background:" + panelBg + "; color:" + text + "; border:1px solid " + border + "; border-radius:4px; padding:9px; }";
    sheet += "QLabel#MetricLine { color:" + text + "; font-size:12px; padding:2px 0; }";
    sheet += "QLabel#ReasonBox, QLabel#CurrentValuesBox { background:" + panelBg + "; border:1px solid " + border + "; border-radius:4px; padding:7px; }";
    sheet += "QLabel#RuleBox { background:#30251d; border:1px solid #ef4e42; border-radius:4px; padding:7px; color:#ffb4a9; font-weight:700; }";
    sheet += "QGroupBox { background:" + panelBg + "; border:1px solid " + border + "; border-radius:5px; margin-top:14px; color:" + text + "; }";
    sheet += "QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 4px; background:" + panelBg + "; color:" + text + "; }";
    sheet += "QPushButton { background:" + panelBg + "; color:" + text + "; border:1px solid " + border + "; border-radius:5px; padding:8px 13px; }";
    sheet += "QPushButton:hover { background:" + (lightMode ? "#eef2f7" : "#353535") + "; }";
    sheet += "QPushButton#Primary { background:#ef4e42; color:white; border:1px solid #ef4e42; font-weight:700; }";
    sheet += "QTableWidget { background:" + panelBg + "; color:" + text + "; gridline-color:" + border + "; border:1px solid " + border + "; }";
    sheet += "QHeaderView::section { background:" + (lightMode ? "#e5e7eb" : "#292929") + "; color:" + text + "; padding:7px; border:0px; font-weight:700; }";
    setStyleSheet(sheet);
}

// Toggle between dark and light mode.
void RiskPage::toggleTheme()
{
    lightMode = !lightMode;
    styleApp();
    if (themeToggleButton) themeToggleButton->setText(lightMode ? "Dark Mode" : "White Mode");
    if (simulationView) simulationView->setLightMode(lightMode);
}

// Create a value label next to a slider.
QLabel *RiskPage::makeValueLabel(const QString &caption)
{
    QLabel *label = new QLabel(caption);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setMinimumWidth(78);
    return label;
}

// Create a slider and connect value change to re-evaluation.
QSlider *RiskPage::makeSlider(int min, int max, int value, QLabel *, const QString &)
{
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    connect(slider, &QSlider::valueChanged, this, [this]() { updateSliderLabels(); evaluateAndRender("Input Change"); });
    return slider;
}

// Build the full UI layout.
void RiskPage::buildUi()
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    // Header row follows the existing project page style: back button, centered title, right controls.
    QHBoxLayout *topRow = new QHBoxLayout;
    QPushButton *backButton = new QPushButton("◀ Back to Introduction");
    backButton->setFixedWidth(245);
    connect(backButton, &QPushButton::clicked, this, &RiskPage::backRequested);

    QLabel *title = new QLabel("Robot Risk Safety Simulator");
    title->setObjectName("Title");
    title->setAlignment(Qt::AlignCenter);

    QWidget *rightControls = new QWidget;
    QHBoxLayout *presetRow = new QHBoxLayout(rightControls);
    presetRow->setContentsMargins(0, 0, 0, 0);
    themeToggleButton = new QPushButton("White Mode");
    QPushButton *normal = new QPushButton("Normal");
    QPushButton *human = new QPushButton("Human Approach");
    QPushButton *obstacle = new QPushButton("Obstacle Approach");
    simulateButton = new QPushButton("Start Robot");
    simulateButton->setObjectName("Primary");
    QPushButton *reset = new QPushButton("Reset");
    presetRow->addWidget(themeToggleButton);
    presetRow->addWidget(normal);
    presetRow->addWidget(human);
    presetRow->addWidget(obstacle);
    presetRow->addWidget(simulateButton);
    presetRow->addWidget(reset);

    topRow->addWidget(backButton);
    topRow->addWidget(title, 1);
    topRow->addWidget(rightControls);
    root->addLayout(topRow);

    QLabel *guide = new QLabel("Set sensor values on the left, check robot movement in the center, and confirm the state on the right.");
    guide->setObjectName("Guide");
    guide->setAlignment(Qt::AlignCenter);
    root->addWidget(guide);

    QHBoxLayout *main = new QHBoxLayout;
    main->setContentsMargins(0, 10, 0, 0);
    main->setSpacing(12);
    main->addWidget(buildControlPanel());

    QGroupBox *viewBox = new QGroupBox("Risk Simulation Diagram");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewBox);
    simulationView = new SimulationView;
    viewLayout->addWidget(simulationView);
    main->addWidget(viewBox, 1);
    main->addWidget(buildResultPanel());
    root->addLayout(main, 1);

    QHBoxLayout *outputTools = new QHBoxLayout;
    outputTools->addStretch();
    historyToggleButton = new QPushButton("Risk History");
    historyToggleButton->setCheckable(true);
    outputToggleButton = new QPushButton("Log / Summary");
    outputToggleButton->setCheckable(true);
    outputTools->addWidget(historyToggleButton);
    outputTools->addWidget(outputToggleButton);
    root->addLayout(outputTools);

    outputPanel = buildOutputPanel();
    outputPanel->hide();
    root->addWidget(outputPanel);

    connect(normal, &QPushButton::clicked, this, &RiskPage::runPresetNormal);
    connect(themeToggleButton, &QPushButton::clicked, this, &RiskPage::toggleTheme);
    connect(human, &QPushButton::clicked, this, &RiskPage::runPresetHumanApproach);
    connect(obstacle, &QPushButton::clicked, this, &RiskPage::runPresetObstacleApproach);
    connect(simulateButton, &QPushButton::clicked, this, &RiskPage::toggleSimulation);
    connect(reset, &QPushButton::clicked, this, &RiskPage::resetSimulation);

    // Risk History and Log/Summary are exclusive panels.
    // Both panels can be turned off, but only one panel can be visible at a time.
    connect(historyToggleButton, &QPushButton::clicked, this, [this]() {
        const bool showHistory = historyToggleButton->isChecked();
        if (showHistory) {
            outputToggleButton->setChecked(false);
            outputPanel->hide();
        }
        simulationView->setRiskHistoryVisible(showHistory);
    });

    // Turning on Log/Summary automatically turns off Risk History.
    connect(outputToggleButton, &QPushButton::clicked, this, [this]() {
        const bool showOutput = outputToggleButton->isChecked();
        if (showOutput) {
            historyToggleButton->setChecked(false);
            simulationView->setRiskHistoryVisible(false);
        }
        outputPanel->setVisible(showOutput);
    });
}

// Build the left input panel.
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

// Build the right result panel.
QWidget *RiskPage::buildResultPanel()
{
    QGroupBox *box = new QGroupBox("Robot State");
    box->setFixedWidth(320);
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(12, 17, 12, 8);
    layout->setSpacing(6);

    stateLabel = new QLabel("MOVE");
    riskLabel = new QLabel("Risk score: 0 / 100");
    brakeLabel = new QLabel("Brake distance: 0 m");
    marginLabel = new QLabel("Safety margin: 0 m");
    speedRecommendLabel = new QLabel("Robot speed: 0 m/s");
    decisionRuleLabel = new QLabel("Rule: -");
    reasonLabel = new QLabel("Why: -");
    currentValuesLabel = new QLabel("Current Values\nHuman 0.0 m\nObstacle 0.0 m\nDoor 0 %\nBrake 0.0 m");
    ledDotLabel = new QLabel("LED: ●");

    stateLabel->setAlignment(Qt::AlignCenter);
    stateLabel->setMinimumHeight(80);
    ledDotLabel->setAlignment(Qt::AlignCenter);
    for (QLabel *label : {riskLabel, brakeLabel, marginLabel, speedRecommendLabel}) label->setObjectName("MetricLine");
    decisionRuleLabel->setObjectName("RuleBox");
    reasonLabel->setObjectName("ReasonBox");
    currentValuesLabel->setObjectName("CurrentValuesBox");
    decisionRuleLabel->setWordWrap(true);
    reasonLabel->setWordWrap(true);
    currentValuesLabel->setTextFormat(Qt::PlainText);

    layout->addWidget(stateLabel);
    layout->addWidget(ledDotLabel);
    layout->addSpacing(8);
    layout->addWidget(riskLabel);
    layout->addWidget(brakeLabel);
    layout->addWidget(marginLabel);
    layout->addWidget(speedRecommendLabel);
    layout->addWidget(decisionRuleLabel);
    layout->addWidget(reasonLabel);
    layout->addWidget(currentValuesLabel);
    layout->addStretch();
    return box;
}

// Build the bottom output panel.
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

// Read current UI values.
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

// Write input values to UI.
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

// Update slider text labels.
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

// Evaluate model and update UI.
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

// Update result labels.
void RiskPage::updateResultPanel(const SafetyResult &result)
{
    SafetyInput input = readInput();
    QColor color = stateColor(result.state);
    const double contactDistance = 0.1;
    stateLabel->setText(result.state);
    stateLabel->setStyleSheet(QString("font-size:34px; font-weight:800; color:%1; background:transparent; padding:4px;").arg(color.name()));
    if (ledDotLabel) ledDotLabel->setStyleSheet(QString("font-size:18px; color:%1;").arg(color.name()));
    riskLabel->setText(QString("Risk score: %1 / 100").arg(result.riskScore, 0, 'f', 1));
    brakeLabel->setText(QString("Brake distance: %1 m").arg(result.brakeDistance, 0, 'f', 2));
    marginLabel->setText(QString("Safety margin: %1 m").arg(result.safetyMargin, 0, 'f', 2));
    speedRecommendLabel->setText(QString("Robot speed: %1 m/s").arg(result.recommendedSpeed, 0, 'f', 1));
    currentValuesLabel->setText(QString("Current Values\nHuman        %1 m\nObstacle     %2 m\nDoor         %3 %\nBrake        %4 m")
        .arg(input.humanDistance, 0, 'f', 1)
        .arg(input.obstacleDistance, 0, 'f', 1)
        .arg(input.doorOpenRate, 0, 'f', 0)
        .arg(result.brakeDistance, 0, 'f', 1));

    QString rule;
    if (input.emergency) rule = "Rule: Emergency signal ON -> STOP";
    else if (input.doorOpenRate >= 70.0) rule = "Rule: Door >= 70% open -> STOP";
    else if (input.humanDistance <= contactDistance || input.obstacleDistance <= contactDistance) rule = "Rule: Object touched robot -> STOP";
    else if (input.humanDistance <= input.stopDistance || input.obstacleDistance <= input.stopDistance) rule = "Rule: Object inside STOP area -> DANGER";
    else if (result.safetyMargin <= 0.0) rule = "Rule: Brake distance longer than free space -> WARNING";
    else if (input.humanDistance <= input.warningDistance || input.obstacleDistance <= input.warningDistance) rule = "Rule: Object inside WARNING line -> WARNING";
    else if (input.doorOpenRate >= 25.0) rule = "Rule: Door >= 25% open -> WARNING";
    else if (result.riskScore >= 45.0) rule = "Rule: Risk score is high -> WARNING";
    else rule = "Rule: All values are outside danger zones -> MOVE";

    decisionRuleLabel->setText(rule);
    reasonLabel->setText("Why: " + result.reason);
}

// Append one simulation log row.
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

// Manual evaluation button handler.
void RiskPage::evaluateManual()
{
    evaluateAndRender("Manual");
}

// Move the simulation forward by one step.
void RiskPage::stepSimulation()
{
    SafetyInput input = readInput();
    if (input.emergency || input.doorOpenRate >= 70.0 || qMin(input.humanDistance, input.obstacleDistance) <= 0.1) {
        evaluateAndRender("Stop Hold");
        return;
    }
    double moveStep = qMax(0.2, input.robotSpeed * 0.35);
    robotTravel += moveStep;
    if (scenarioMode == 1) input.humanDistance = qMax(0.0, input.humanDistance - moveStep);
    else if (scenarioMode == 2) input.obstacleDistance = qMax(0.0, input.obstacleDistance - moveStep);
    else {
        input.humanDistance = qMax(0.0, input.humanDistance - moveStep);
        input.obstacleDistance = qMax(0.0, input.obstacleDistance - moveStep);
    }
    writeInput(input);
    evaluateAndRender("Step");
    ++tick;
}

// Start or stop timer simulation.
void RiskPage::toggleSimulation()
{
    if (timer.isActive()) {
        timer.stop();
        simulateButton->setText("Start Robot");
    }
    else {
        timer.start(650);
        simulateButton->setText("Stop Robot");
    }
}

// Reset all values and tables.
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

// Load normal preset.
void RiskPage::runPresetNormal()
{
    scenarioMode = 0;
    robotTravel = 0.0;
    SafetyInput input;
    writeInput(input);
    evaluateAndRender("Preset Normal");
}

// Load human approach preset.
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

// Load obstacle approach preset.
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

// Export log table to CSV.
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
