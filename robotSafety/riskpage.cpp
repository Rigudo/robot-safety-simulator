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

    // Select theme colors.
    const QColor bg = lightMode ? QColor("#f3f4f6") : QColor("#1e1e1e");
    const QColor panel = lightMode ? QColor("#ffffff") : QColor("#19192b");
    const QColor text = lightMode ? QColor("#111827") : QColor("#ffffff");
    const QColor sub = lightMode ? QColor("#4b5563") : QColor("#b8c2d6");
    const QColor border = lightMode ? QColor("#9ca3af") : QColor("#8d8d8d");

    // Draw background.
    p.fillRect(rect(), bg);

    // Define main field rectangle.
    QRect field(24, 8, width() - 48, riskHistoryVisible ? 315 : height() - 16);
    p.setPen(QPen(border, 1));
    p.setBrush(panel);
    p.drawRect(field);

    // Draw field title.
    p.setPen(text);
    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.drawText(field.adjusted(18, 16, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Robot Movement Field");
    p.setPen(sub);
    p.setFont(QFont("Segoe UI", 9));
    p.drawText(field.adjusted(18, 42, -18, -18), Qt::AlignTop | Qt::AlignLeft, "Robot, human, obstacle, warning line, stop line, and brake distance.");

    // Convert meter values to screen coordinates.
    const int baseY = field.center().y() + 28;
    const int robotX = field.left() + 80 + qMin(int(travel * 8.0), 100);
    const double pxPerMeter = qMax(1.0, double(field.right() - robotX - 120) / 20.0);
    auto mapDistance = [&](double d) { return qBound(field.left() + 20, robotX + 70 + int(d * pxPerMeter), field.right() - 20); };

    // Calculate threshold and object positions.
    const int stopX = mapDistance(input.stopDistance);
    const int warningX = mapDistance(input.warningDistance);
    const int brakeX = mapDistance(result.brakeDistance);
    const int humanX = mapDistance(input.humanDistance);
    const int obstacleX = mapDistance(input.obstacleDistance);

    // Draw road and zones.
    p.setPen(QPen(QColor("#6b7280"), 2));
    p.drawLine(field.left() + 20, baseY, field.right() - 20, baseY);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(239, 78, 66, 55));
    p.drawRect(QRect(robotX + 70, baseY - 38, qMax(0, stopX - robotX - 70), 76));
    p.setBrush(QColor(245, 158, 11, 50));
    p.drawRect(QRect(stopX, baseY - 38, qMax(0, warningX - stopX), 76));
    p.setBrush(QColor(38, 164, 126, 35));
    p.drawRect(QRect(warningX, baseY - 38, qMax(0, field.right() - warningX), 76));

    // Draw threshold lines.
    p.setPen(QPen(QColor("#ef4e42"), 4));
    p.drawLine(stopX, baseY - 58, stopX, baseY + 58);
    p.setPen(QPen(QColor("#f59e0b"), 4));
    p.drawLine(warningX, baseY - 58, warningX, baseY + 58);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.setPen(QColor("#ef4e42"));
    p.drawText(QRect(stopX - 45, baseY - 82, 90, 20), Qt::AlignCenter, "STOP line");
    p.setPen(QColor("#f59e0b"));
    p.drawText(QRect(warningX - 55, baseY - 82, 110, 20), Qt::AlignCenter, "WARNING line");

    // Draw brake distance.
    p.setPen(QPen(QColor("#38bdf8"), 3, Qt::DashLine));
    p.drawLine(robotX + 70, baseY + 58, brakeX, baseY + 58);
    p.setPen(QColor("#38bdf8"));
    p.drawText(QRect(robotX + 70, baseY + 64, 190, 20), Qt::AlignLeft, QString("Brake: %1 m").arg(result.brakeDistance, 0, 'f', 2));

    // Draw robot body.
    p.setPen(Qt::NoPen);
    p.setBrush(stateColor(result.state));
    p.drawRoundedRect(QRect(robotX - 42, baseY - 36, 84, 62), 14, 14);
    p.setBrush(QColor("#111827"));
    p.drawEllipse(QPoint(robotX - 26, baseY + 28), 15, 15);
    p.drawEllipse(QPoint(robotX + 26, baseY + 28), 15, 15);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPoint(robotX - 14, baseY - 14), 5, 5);
    p.drawEllipse(QPoint(robotX + 14, baseY - 14), 5, 5);

    // Draw human marker.
    QColor humanColor = input.humanDistance <= input.stopDistance ? QColor("#ef4e42") : input.humanDistance <= input.warningDistance ? QColor("#f59e0b") : QColor("#26a47e");
    p.setBrush(humanColor);
    p.drawEllipse(QPoint(humanX, baseY - 62), 10, 10);
    p.drawRoundedRect(QRect(humanX - 7, baseY - 48, 14, 30), 6, 6);
    p.setPen(humanColor);
    p.drawText(QRect(humanX - 70, baseY - 94, 140, 20), Qt::AlignCenter, QString("Human %1 m").arg(input.humanDistance, 0, 'f', 1));

    // Draw obstacle marker.
    QColor obstacleColor = input.obstacleDistance <= input.stopDistance ? QColor("#ef4e42") : input.obstacleDistance <= input.warningDistance ? QColor("#f59e0b") : QColor("#26a47e");
    p.setPen(Qt::NoPen);
    p.setBrush(obstacleColor);
    QPolygon cone;
    cone << QPoint(obstacleX, baseY + 8) << QPoint(obstacleX - 24, baseY + 58) << QPoint(obstacleX + 24, baseY + 58);
    p.drawPolygon(cone);
    p.setPen(obstacleColor);
    p.drawText(QRect(obstacleX - 80, baseY + 62, 160, 20), Qt::AlignCenter, QString("Obstacle %1 m").arg(input.obstacleDistance, 0, 'f', 1));

    // Draw state summary.
    p.setPen(QPen(stateColor(result.state), 1));
    p.setBrush(panel);
    QRect stateBox(field.left() + 18, field.bottom() - 44, field.width() - 36, 28);
    p.drawRoundedRect(stateBox, 4, 4);
    p.setPen(text);
    p.drawText(stateBox.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, QString("State: %1 | Risk: %2 | Margin: %3 m | Door: %4%")
                   .arg(result.state).arg(result.riskScore, 0, 'f', 1).arg(result.safetyMargin, 0, 'f', 2).arg(input.doorOpenRate, 0, 'f', 0));

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
    QRect plot = chart.adjusted(50, 52, -24, -24);
    p.setPen(QPen(QColor("#6b7280"), 1));
    p.drawRect(plot);
    if (history.size() > 1) {
        QPainterPath path;
        for (int i = 0; i < history.size(); ++i) {
            double x = plot.left() + double(i) / qMax(1, history.size() - 1) * plot.width();
            double y = plot.bottom() - history[i] / 100.0 * plot.height();
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
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
    sheet += "QPushButton#Primary { background:#ef4e42; color:white; border:1px solid #ef4e42; font-weight:700; }";
    sheet += "QTableWidget { background:" + panelBg + "; color:" + text + "; gridline-color:" + border + "; border:1px solid " + border + "; }";
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
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);
    QFrame *header = new QFrame;
    QVBoxLayout *head = new QVBoxLayout(header);
    QLabel *title = new QLabel("Robot Risk Safety Simulator");
    title->setObjectName("Title");
    title->setAlignment(Qt::AlignCenter);
    QLabel *sub = new QLabel("Adjust sensor values and check robot safety state in real time.");
    sub->setObjectName("SubTitle");
    sub->setAlignment(Qt::AlignCenter);
    head->addWidget(title);
    head->addWidget(sub);
    QHBoxLayout *presetRow = new QHBoxLayout;
    presetRow->addStretch();
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
    head->addLayout(presetRow);
    root->addWidget(header);
    QLabel *guide = new QLabel("Left: sensor input | Center: simulation diagram | Right: decision result | Bottom: history and log");
    guide->setObjectName("Guide");
    guide->setAlignment(Qt::AlignCenter);
    root->addWidget(guide);
    QHBoxLayout *main = new QHBoxLayout;
    main->setContentsMargins(10, 10, 10, 2);
    main->setSpacing(12);
    main->addWidget(buildControlPanel());
    QGroupBox *viewBox = new QGroupBox("Simulation Diagram");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewBox);
    simulationView = new SimulationView;
    viewLayout->addWidget(simulationView);
    main->addWidget(viewBox, 1);
    main->addWidget(buildResultPanel());
    root->addLayout(main, 1);
    QHBoxLayout *outputTools = new QHBoxLayout;
    outputTools->setContentsMargins(10, 0, 10, 8);
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
    connect(historyToggleButton, &QPushButton::clicked, this, [this]() { simulationView->setRiskHistoryVisible(historyToggleButton->isChecked()); });
    connect(outputToggleButton, &QPushButton::clicked, this, [this]() { outputPanel->setVisible(outputToggleButton->isChecked()); });
}

// Build the left input panel.
QWidget *RiskPage::buildControlPanel()
{
    QGroupBox *box = new QGroupBox("Sensor Input Panel");
    box->setFixedWidth(310);
    QGridLayout *grid = new QGridLayout(box);
    grid->setContentsMargins(14, 24, 14, 14);
    humanValue = makeValueLabel("8.0 m"); obstacleValue = makeValueLabel("10.0 m"); doorValue = makeValueLabel("0 %");
    speedValue = makeValueLabel("1.0 m/s"); decelValue = makeValueLabel("1.5 m/s2"); warningValue = makeValueLabel("6.0 m"); stopValue = makeValueLabel("2.5 m");
    humanSlider = makeSlider(0, 200, 80, humanValue, "m"); obstacleSlider = makeSlider(0, 200, 100, obstacleValue, "m"); doorSlider = makeSlider(0, 100, 0, doorValue, "%");
    speedSlider = makeSlider(0, 50, 10, speedValue, "m/s"); decelSlider = makeSlider(5, 50, 15, decelValue, "m/s2"); warningSlider = makeSlider(10, 120, 60, warningValue, "m"); stopSlider = makeSlider(5, 80, 25, stopValue, "m");
    emergencyCheckBox = new QCheckBox("Emergency Signal");
    connect(emergencyCheckBox, &QCheckBox::toggled, this, [this]() { evaluateAndRender("Input Change"); });
    QStringList names = {"Human", "Obstacle", "Door", "Target Speed", "Braking Power", "Warning Line", "Stop Line"};
    QList<QSlider*> sliders = {humanSlider, obstacleSlider, doorSlider, speedSlider, decelSlider, warningSlider, stopSlider};
    QList<QLabel*> labels = {humanValue, obstacleValue, doorValue, speedValue, decelValue, warningValue, stopValue};
    for (int i = 0; i < names.size(); ++i) { grid->addWidget(new QLabel(names[i]), i, 0); grid->addWidget(sliders[i], i, 1); grid->addWidget(labels[i], i, 2); }
    grid->addWidget(emergencyCheckBox, names.size(), 0, 1, 3);
    QPushButton *run = new QPushButton("RUN"); run->setObjectName("Primary");
    QPushButton *step = new QPushButton("Step"); QPushButton *exportButton = new QPushButton("Export");
    grid->addWidget(run, names.size() + 1, 0, 1, 3); grid->addWidget(step, names.size() + 2, 0, 1, 3); grid->addWidget(exportButton, names.size() + 3, 0, 1, 3);
    connect(run, &QPushButton::clicked, this, &RiskPage::evaluateManual); connect(step, &QPushButton::clicked, this, &RiskPage::stepSimulation); connect(exportButton, &QPushButton::clicked, this, &RiskPage::exportLogCsv);
    return box;
}

// Build the right result panel.
QWidget *RiskPage::buildResultPanel()
{
    QGroupBox *box = new QGroupBox("Robot State");
    box->setFixedWidth(320);
    QVBoxLayout *layout = new QVBoxLayout(box);
    stateLabel = new QLabel("MOVE"); riskLabel = new QLabel("Risk score: 0 / 100"); brakeLabel = new QLabel("Brake distance: 0 m"); marginLabel = new QLabel("Safety margin: 0 m"); speedRecommendLabel = new QLabel("Robot speed: 0 m/s");
    decisionRuleLabel = new QLabel("Rule: -"); reasonLabel = new QLabel("Why: -"); currentValuesLabel = new QLabel("Current Values\nHuman 0.0 m\nObstacle 0.0 m\nDoor 0 %\nBrake 0.0 m"); ledDotLabel = new QLabel("●");
    stateLabel->setAlignment(Qt::AlignCenter); stateLabel->setMinimumHeight(54);
    for (QLabel *label : {riskLabel, brakeLabel, marginLabel, speedRecommendLabel}) label->setObjectName("MetricLine");
    decisionRuleLabel->setObjectName("RuleBox"); reasonLabel->setObjectName("ReasonBox"); currentValuesLabel->setObjectName("CurrentValuesBox");
    decisionRuleLabel->setWordWrap(true); reasonLabel->setWordWrap(true); currentValuesLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(stateLabel); layout->addWidget(ledDotLabel, 0, Qt::AlignCenter); layout->addWidget(riskLabel); layout->addWidget(brakeLabel); layout->addWidget(marginLabel); layout->addWidget(speedRecommendLabel); layout->addWidget(decisionRuleLabel); layout->addWidget(reasonLabel); layout->addWidget(currentValuesLabel); layout->addStretch();
    return box;
}

// Build the bottom output panel.
QWidget *RiskPage::buildOutputPanel()
{
    QTabWidget *tabs = new QTabWidget;
    tabs->setMinimumHeight(210); tabs->setMaximumHeight(250);
    logTable = new QTableWidget(0, 10);
    logTable->setHorizontalHeaderLabels({"#", "Time", "Source", "Human(m)", "Obstacle(m)", "Door(%)", "Speed", "Risk", "Margin", "State"});
    logTable->horizontalHeader()->setStretchLastSection(true); logTable->verticalHeader()->setVisible(false); tabs->addTab(logTable, "Simulation Log");
    caseTable = new QTableWidget(0, 5);
    caseTable->setHorizontalHeaderLabels({"Parameter", "Current", "Meaning", "Warning", "Stop"});
    caseTable->horizontalHeader()->setStretchLastSection(true); caseTable->verticalHeader()->setVisible(false); tabs->addTab(caseTable, "Model Summary");
    return tabs;
}

// Read current UI values.
SafetyInput RiskPage::readInput() const
{
    SafetyInput input;
    input.humanDistance = humanSlider->value() / 10.0; input.obstacleDistance = obstacleSlider->value() / 10.0; input.doorOpenRate = doorSlider->value(); input.robotSpeed = speedSlider->value() / 10.0;
    input.deceleration = decelSlider->value() / 10.0; input.warningDistance = warningSlider->value() / 10.0; input.stopDistance = stopSlider->value() / 10.0; input.emergency = emergencyCheckBox->isChecked();
    return input;
}

// Write input values to UI.
void RiskPage::writeInput(const SafetyInput &input)
{
    humanSlider->setValue(int(input.humanDistance * 10)); obstacleSlider->setValue(int(input.obstacleDistance * 10)); doorSlider->setValue(int(input.doorOpenRate)); speedSlider->setValue(int(input.robotSpeed * 10));
    decelSlider->setValue(int(input.deceleration * 10)); warningSlider->setValue(int(input.warningDistance * 10)); stopSlider->setValue(int(input.stopDistance * 10)); emergencyCheckBox->setChecked(input.emergency); updateSliderLabels();
}

// Update slider text labels.
void RiskPage::updateSliderLabels()
{
    int minWarning = stopSlider->value() + 10;
    if (warningSlider->value() < minWarning) { warningSlider->blockSignals(true); warningSlider->setValue(qMin(warningSlider->maximum(), minWarning)); warningSlider->blockSignals(false); }
    humanValue->setText(QString::number(humanSlider->value() / 10.0, 'f', 1) + " m"); obstacleValue->setText(QString::number(obstacleSlider->value() / 10.0, 'f', 1) + " m"); doorValue->setText(QString::number(doorSlider->value()) + " %");
    speedValue->setText(QString::number(speedSlider->value() / 10.0, 'f', 1) + " m/s"); decelValue->setText(QString::number(decelSlider->value() / 10.0, 'f', 1) + " m/s2"); warningValue->setText(QString::number(warningSlider->value() / 10.0, 'f', 1) + " m"); stopValue->setText(QString::number(stopSlider->value() / 10.0, 'f', 1) + " m");
}

// Evaluate model and update UI.
void RiskPage::evaluateAndRender(const QString &source)
{
    updateSliderLabels();
    SafetyInput input = readInput(); SafetyResult result = SafetyModel::evaluate(input);
    riskHistory.append(result.riskScore); if (riskHistory.size() > 80) riskHistory.removeFirst();
    simulationView->setData(input, result, riskHistory, robotTravel); updateResultPanel(result); appendLog(source, input, result);
    if (result.state == "STOP" && timer.isActive()) { timer.stop(); if (simulateButton) simulateButton->setText("Start Robot"); }
}

// Update result labels.
void RiskPage::updateResultPanel(const SafetyResult &result)
{
    SafetyInput input = readInput(); QColor color = stateColor(result.state); const double contactDistance = 0.1;
    stateLabel->setText(result.state); stateLabel->setStyleSheet(QString("font-size:32px; font-weight:700; color:%1; background:transparent; padding:4px;").arg(color.name())); if (ledDotLabel) ledDotLabel->setStyleSheet(QString("font-size:22px; color:%1;").arg(color.name()));
    riskLabel->setText(QString("Risk score: %1 / 100").arg(result.riskScore, 0, 'f', 1)); brakeLabel->setText(QString("Brake distance: %1 m").arg(result.brakeDistance, 0, 'f', 2)); marginLabel->setText(QString("Safety margin: %1 m").arg(result.safetyMargin, 0, 'f', 2)); speedRecommendLabel->setText(QString("Robot speed: %1 m/s").arg(result.recommendedSpeed, 0, 'f', 1));
    currentValuesLabel->setText(QString("Current Values\nHuman        %1 m\nObstacle     %2 m\nDoor         %3 %\nBrake        %4 m").arg(input.humanDistance, 0, 'f', 1).arg(input.obstacleDistance, 0, 'f', 1).arg(input.doorOpenRate, 0, 'f', 0).arg(result.brakeDistance, 0, 'f', 1));
    QString rule;
    if (input.emergency) rule = "Rule: Emergency signal ON -> STOP"; else if (input.doorOpenRate >= 70.0) rule = "Rule: Door >= 70% open -> STOP"; else if (input.humanDistance <= contactDistance || input.obstacleDistance <= contactDistance) rule = "Rule: Object touched robot -> STOP"; else if (input.humanDistance <= input.stopDistance || input.obstacleDistance <= input.stopDistance) rule = "Rule: Object inside STOP area -> DANGER"; else if (result.safetyMargin <= 0.0) rule = "Rule: Brake distance longer than free space -> WARNING"; else if (input.humanDistance <= input.warningDistance || input.obstacleDistance <= input.warningDistance) rule = "Rule: Object inside WARNING line -> WARNING"; else if (input.doorOpenRate >= 25.0) rule = "Rule: Door >= 25% open -> WARNING"; else if (result.riskScore >= 45.0) rule = "Rule: Risk score is high -> WARNING"; else rule = "Rule: All values are outside danger zones -> MOVE";
    decisionRuleLabel->setText(rule); reasonLabel->setText("Why: " + result.reason);
}

// Append one simulation log row.
void RiskPage::appendLog(const QString &source, const SafetyInput &input, const SafetyResult &result)
{
    int row = logTable->rowCount(); logTable->insertRow(row);
    QStringList values = {QString::number(++logIndex), QDateTime::currentDateTime().toString("HH:mm:ss"), source, QString::number(input.humanDistance, 'f', 1), QString::number(input.obstacleDistance, 'f', 1), QString::number(input.doorOpenRate, 'f', 0), QString::number(input.robotSpeed, 'f', 1), QString::number(result.riskScore, 'f', 1), QString::number(result.safetyMargin, 'f', 2), result.state};
    for (int i = 0; i < values.size(); ++i) logTable->setItem(row, i, new QTableWidgetItem(values[i]));
    logTable->scrollToBottom();
}

// Manual evaluation button handler.
void RiskPage::evaluateManual() { evaluateAndRender("Manual"); }

// Move the simulation forward by one step.
void RiskPage::stepSimulation()
{
    SafetyInput input = readInput();
    if (input.emergency || input.doorOpenRate >= 70.0 || qMin(input.humanDistance, input.obstacleDistance) <= 0.1) { evaluateAndRender("Stop Hold"); return; }
    double moveStep = qMax(0.2, input.robotSpeed * 0.35); robotTravel += moveStep;
    if (scenarioMode == 1) input.humanDistance = qMax(0.0, input.humanDistance - moveStep); else if (scenarioMode == 2) input.obstacleDistance = qMax(0.0, input.obstacleDistance - moveStep); else { input.humanDistance = qMax(0.0, input.humanDistance - moveStep); input.obstacleDistance = qMax(0.0, input.obstacleDistance - moveStep); }
    writeInput(input); evaluateAndRender("Step"); ++tick;
}

// Start or stop timer simulation.
void RiskPage::toggleSimulation()
{
    if (timer.isActive()) { timer.stop(); simulateButton->setText("Start Robot"); } else { timer.start(650); simulateButton->setText("Stop Robot"); }
}

// Reset all values and tables.
void RiskPage::resetSimulation()
{
    timer.stop(); if (simulateButton) simulateButton->setText("Start Robot"); tick = 0; robotTravel = 0.0; logIndex = 0; riskHistory.clear(); if (logTable) logTable->setRowCount(0);
    SafetyInput input; writeInput(input); evaluateAndRender("Reset");
    if (caseTable) { caseTable->setRowCount(0); QStringList rows[] = {{"Human Distance", humanValue->text(), "person distance from robot", "<= warning", "<= stop"}, {"Obstacle Distance", obstacleValue->text(), "object distance from robot", "<= warning", "<= stop"}, {"Door Open", doorValue->text(), "open rate of door sensor", ">= 25%", ">= 70%"}, {"Brake Distance", brakeLabel->text().section(':', 1).trimmed(), "speed^2 / 2a", "near margin", "margin <= 0"}, {"Risk Score", riskLabel->text().section(':', 1).trimmed(), "combined risk index", ">= 45", "critical"}}; for (const QStringList &rowData : rows) { int row = caseTable->rowCount(); caseTable->insertRow(row); for (int c = 0; c < rowData.size(); ++c) caseTable->setItem(row, c, new QTableWidgetItem(rowData[c])); } }
}

// Load normal preset.
void RiskPage::runPresetNormal() { scenarioMode = 0; robotTravel = 0.0; SafetyInput input; writeInput(input); evaluateAndRender("Preset Normal"); }

// Load human approach preset.
void RiskPage::runPresetHumanApproach() { scenarioMode = 1; robotTravel = 0.0; SafetyInput input; input.humanDistance = 9.0; input.obstacleDistance = 14.0; input.robotSpeed = 1.6; writeInput(input); evaluateAndRender("Preset Human"); }

// Load obstacle approach preset.
void RiskPage::runPresetObstacleApproach() { scenarioMode = 2; robotTravel = 0.0; SafetyInput input; input.humanDistance = 14.0; input.obstacleDistance = 10.0; input.robotSpeed = 1.8; writeInput(input); evaluateAndRender("Preset Obstacle"); }

// Export log table to CSV.
void RiskPage::exportLogCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "Export Simulation Log", "risk_log.csv", "CSV (*.csv)"); if (path.isEmpty()) return;
    QFile file(path); if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return; QTextStream out(&file);
    for (int c = 0; c < logTable->columnCount(); ++c) { if (c) out << ','; out << logTable->horizontalHeaderItem(c)->text(); } out << '\n';
    for (int r = 0; r < logTable->rowCount(); ++r) { for (int c = 0; c < logTable->columnCount(); ++c) { if (c) out << ','; QTableWidgetItem *item = logTable->item(r, c); out << (item ? item->text() : ""); } out << '\n'; }
}
