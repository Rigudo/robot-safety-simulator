#ifndef RISKPAGE_H
#define RISKPAGE_H

#include <QCheckBox>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QTableWidget>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "SafetyModel.h"

class SimulationView : public QWidget
{
public:
    explicit SimulationView(QWidget *parent = nullptr);
    void setData(const SafetyInput &input, const SafetyResult &result, const QVector<double> &riskHistory, double robotTravel);
    void setRiskHistoryVisible(bool visible);
    void setLightMode(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SafetyInput input;
    SafetyResult result;
    QVector<double> history;
    double travel = 0.0;
    bool riskHistoryVisible = false;
    bool lightMode = false;
};

class RiskPage : public QWidget
{
    Q_OBJECT

public:
    explicit RiskPage(QWidget *parent = nullptr);

signals:
    void backRequested();

private slots:
    void evaluateManual();
    void stepSimulation();
    void toggleSimulation();
    void resetSimulation();
    void runPresetNormal();
    void runPresetHumanApproach();
    void runPresetObstacleApproach();
    void exportLogCsv();

private:
    void buildUi();
    void styleApp();
    void toggleTheme();
    QWidget *buildControlPanel();
    QWidget *buildResultPanel();
    QWidget *buildOutputPanel();
    QLabel *makeValueLabel(const QString &caption);
    QSlider *makeSlider(int min, int max, int value, QLabel *valueLabel, const QString &suffix = QString());

    SafetyInput readInput() const;
    void writeInput(const SafetyInput &input);
    void evaluateAndRender(const QString &source);
    void updateResultPanel(const SafetyResult &result);
    void appendLog(const QString &source, const SafetyInput &input, const SafetyResult &result);
    void updateSliderLabels();

    QSlider *humanSlider = nullptr;
    QSlider *obstacleSlider = nullptr;
    QSlider *doorSlider = nullptr;
    QSlider *speedSlider = nullptr;
    QSlider *decelSlider = nullptr;
    QSlider *warningSlider = nullptr;
    QSlider *stopSlider = nullptr;
    QCheckBox *emergencyCheckBox = nullptr;

    QLabel *humanValue = nullptr;
    QLabel *obstacleValue = nullptr;
    QLabel *doorValue = nullptr;
    QLabel *speedValue = nullptr;
    QLabel *decelValue = nullptr;
    QLabel *warningValue = nullptr;
    QLabel *stopValue = nullptr;

    QLabel *stateLabel = nullptr;
    QLabel *riskLabel = nullptr;
    QLabel *brakeLabel = nullptr;
    QLabel *marginLabel = nullptr;
    QLabel *speedRecommendLabel = nullptr;
    QLabel *decisionRuleLabel = nullptr;
    QLabel *reasonLabel = nullptr;
    QLabel *currentValuesLabel = nullptr;
    QLabel *ledDotLabel = nullptr;

    QPushButton *simulateButton = nullptr;
    QPushButton *themeToggleButton = nullptr;
    QPushButton *historyToggleButton = nullptr;
    QPushButton *outputToggleButton = nullptr;
    SimulationView *simulationView = nullptr;
    QWidget *outputPanel = nullptr;
    QTableWidget *logTable = nullptr;
    QTableWidget *caseTable = nullptr;

    QTimer timer;
    QVector<double> riskHistory;
    double robotTravel = 0.0;
    int tick = 0;
    int logIndex = 0;
    int scenarioMode = 0;
    bool lightMode = false;
};

#endif
