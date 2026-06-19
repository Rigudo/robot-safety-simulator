// Header guard start: prevents this header file from being included multiple times.
#ifndef RISKPAGE_H
#define RISKPAGE_H

// QCheckBox is used for the emergency signal input.
#include <QCheckBox>

// QLabel is used to display text values and status output.
#include <QLabel>

// QWidget is the base class for Qt visual components.
#include <QWidget>

// QPushButton is used for RUN, Step, Reset, Export, and mode buttons.
#include <QPushButton>

// QSlider is used to adjust numerical input values.
#include <QSlider>

// QTableWidget is used to display simulation logs and model summaries.
#include <QTableWidget>

// QTimer is used to run the simulation repeatedly over time.
#include <QTimer>

// QVector is used to store risk score history values.
#include <QVector>

// Include the safety input, safety result, and safety model logic.
#include "SafetyModel.h"

// SimulationView is a custom widget that draws the robot simulation scene.
class SimulationView : public QWidget
{
public:
    // Constructor for creating the simulation drawing widget.
    explicit SimulationView(QWidget *parent = nullptr);

    // Receive the latest input, result, risk history, and robot travel distance for drawing.
    void setData(const SafetyInput &input, const SafetyResult &result, const QVector<double> &riskHistory, double robotTravel);

    // Turn the risk history graph area on or off.
    void setRiskHistoryVisible(bool visible);

    // Change the drawing colors according to light mode or dark mode.
    void setLightMode(bool enabled);

protected:
    // paintEvent is called automatically whenever Qt needs to redraw this widget.
    void paintEvent(QPaintEvent *event) override;

private:
    // Latest sensor input values used for drawing the scene.
    SafetyInput input;

    // Latest safety judgment result used for drawing state colors and text.
    SafetyResult result;

    // Risk score history used for drawing the graph.
    QVector<double> history;

    // Accumulated robot movement distance used for animation.
    double travel = 0.0;

    // True when the risk history graph should be displayed.
    bool riskHistoryVisible = false;

    // True when the UI should use light mode colors.
    bool lightMode = false;
};

// RiskPage is the main UI page that contains controls, results, logs, and simulation view.
class RiskPage : public QWidget
{
    // Q_OBJECT enables Qt signal and slot features for this class.
    Q_OBJECT

public:
    // Constructor for creating the whole risk simulation page.
    explicit RiskPage(QWidget *parent = nullptr);

signals:
    // Signal reserved for returning to another page if this page is used inside a multi-page app.
    void backRequested();

private slots:
    // Run one manual evaluation using the current slider values.
    void evaluateManual();

    // Advance the simulation by one time step.
    void stepSimulation();

    // Start or stop the timer-based simulation.
    void toggleSimulation();

    // Reset all simulation values to their default state.
    void resetSimulation();

    // Load the normal preset input values.
    void runPresetNormal();

    // Load a preset where a human approaches the robot.
    void runPresetHumanApproach();

    // Load a preset where an obstacle approaches the robot.
    void runPresetObstacleApproach();

    // Export the simulation log table to a CSV file.
    void exportLogCsv();

private:
    // Build the whole page layout.
    void buildUi();

    // Apply the stylesheet for the current theme.
    void styleApp();

    // Switch between dark mode and light mode.
    void toggleTheme();

    // Create the left input control panel.
    QWidget *buildControlPanel();

    // Create the right result output panel.
    QWidget *buildResultPanel();

    // Create the bottom log and summary output panel.
    QWidget *buildOutputPanel();

    // Create a value label used next to a slider.
    QLabel *makeValueLabel(const QString &caption);

    // Create a slider and connect it to automatic evaluation.
    QSlider *makeSlider(int min, int max, int value, QLabel *valueLabel, const QString &suffix = QString());

    // Read current UI widget values and convert them into a SafetyInput structure.
    SafetyInput readInput() const;

    // Write a SafetyInput structure back into the UI widgets.
    void writeInput(const SafetyInput &input);

    // Evaluate current input and update all visual output areas.
    void evaluateAndRender(const QString &source);

    // Update the state, risk, braking, margin, and reason labels.
    void updateResultPanel(const SafetyResult &result);

    // Add one row to the simulation log table.
    void appendLog(const QString &source, const SafetyInput &input, const SafetyResult &result);

    // Update all slider value text labels.
    void updateSliderLabels();

    // Slider controlling the distance between the robot and the human.
    QSlider *humanSlider = nullptr;

    // Slider controlling the distance between the robot and the obstacle.
    QSlider *obstacleSlider = nullptr;

    // Slider controlling how much the door is open.
    QSlider *doorSlider = nullptr;

    // Slider controlling the robot target speed.
    QSlider *speedSlider = nullptr;

    // Slider controlling the robot deceleration value.
    QSlider *decelSlider = nullptr;

    // Slider controlling the warning distance threshold.
    QSlider *warningSlider = nullptr;

    // Slider controlling the stop distance threshold.
    QSlider *stopSlider = nullptr;

    // Checkbox controlling the emergency stop input.
    QCheckBox *emergencyCheckBox = nullptr;

    // Label displaying the human distance value.
    QLabel *humanValue = nullptr;

    // Label displaying the obstacle distance value.
    QLabel *obstacleValue = nullptr;

    // Label displaying the door open value.
    QLabel *doorValue = nullptr;

    // Label displaying the robot speed value.
    QLabel *speedValue = nullptr;

    // Label displaying the deceleration value.
    QLabel *decelValue = nullptr;

    // Label displaying the warning distance value.
    QLabel *warningValue = nullptr;

    // Label displaying the stop distance value.
    QLabel *stopValue = nullptr;

    // Label displaying the final robot state.
    QLabel *stateLabel = nullptr;

    // Label displaying the calculated risk score.
    QLabel *riskLabel = nullptr;

    // Label displaying the calculated brake distance.
    QLabel *brakeLabel = nullptr;

    // Label displaying the calculated safety margin.
    QLabel *marginLabel = nullptr;

    // Label displaying the recommended robot speed.
    QLabel *speedRecommendLabel = nullptr;

    // Label displaying the rule that caused the current state.
    QLabel *decisionRuleLabel = nullptr;

    // Label displaying the reason text returned by SafetyModel.
    QLabel *reasonLabel = nullptr;

    // Label displaying the current input values in text form.
    QLabel *currentValuesLabel = nullptr;

    // Label displaying the colored LED dot.
    QLabel *ledDotLabel = nullptr;

    // Button used to start or stop timer-based simulation.
    QPushButton *simulateButton = nullptr;

    // Button used to switch between dark and light mode.
    QPushButton *themeToggleButton = nullptr;

    // Button used to show or hide risk history.
    QPushButton *historyToggleButton = nullptr;

    // Button used to show or hide log and summary output.
    QPushButton *outputToggleButton = nullptr;

    // Custom drawing widget for the robot simulation field.
    SimulationView *simulationView = nullptr;

    // Bottom panel containing log table and model summary tabs.
    QWidget *outputPanel = nullptr;

    // Table that stores simulation log rows.
    QTableWidget *logTable = nullptr;

    // Table that summarizes current model parameters.
    QTableWidget *caseTable = nullptr;

    // Timer used for repeated automatic simulation steps.
    QTimer timer;

    // Stores recent risk score values for the history graph.
    QVector<double> riskHistory;

    // Stores accumulated robot travel distance for animation.
    double robotTravel = 0.0;

    // Counts how many simulation steps have passed.
    int tick = 0;

    // Counts log row numbers.
    int logIndex = 0;

    // Stores selected scenario mode: 0 normal, 1 human approach, 2 obstacle approach.
    int scenarioMode = 0;

    // Stores whether the UI is currently in light mode.
    bool lightMode = false;
};

// Header guard end.
#endif
