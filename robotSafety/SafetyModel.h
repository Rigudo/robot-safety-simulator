// Header guard start: prevents this header file from being included multiple times.
#ifndef SAFETYMODEL_H
#define SAFETYMODEL_H

// QString is Qt's string class used to store text values.
#include <QString>

// SafetyInput stores all sensor and parameter values used for safety judgment.
struct SafetyInput
{
    // Distance from the robot to the nearest human, measured in meters.
    double humanDistance = 15.0;

    // Distance from the robot to the nearest obstacle, measured in meters.
    double obstacleDistance = 18.0;

    // Door open rate, measured from 0 percent to 100 percent.
    double doorOpenRate = 0.0;

    // Current or target robot speed, measured in meters per second.
    double robotSpeed = 1.0;

    // Robot deceleration ability, measured in meters per second squared.
    double deceleration = 1.5;

    // Emergency signal flag; true means emergency stop condition is active.
    bool emergency = false;

    // Distance threshold where the robot should enter WARNING state.
    double warningDistance = 6.0;

    // Distance threshold where the robot should enter DANGER state.
    double stopDistance = 2.5;
};

// SafetyResult stores the calculated safety judgment result.
struct SafetyResult
{
    // Final robot state text: MOVE, WARNING, DANGER, or STOP.
    QString state;

    // Human-readable explanation of why the state was selected.
    QString reason;

    // Combined risk value from 0 to 100.
    double riskScore = 0.0;

    // Calculated braking distance based on speed and deceleration.
    double brakeDistance = 0.0;

    // Shorter distance between humanDistance and obstacleDistance.
    double nearestDistance = 0.0;

    // Remaining safety margin after subtracting braking distance from nearest distance.
    double safetyMargin = 0.0;

    // Recommended robot speed after safety judgment.
    double recommendedSpeed = 0.0;
};

// SafetyModel contains the safety decision logic separated from the UI code.
class SafetyModel
{
public:
    // Evaluate one input set and return the calculated safety result.
    static SafetyResult evaluate(const SafetyInput &input);
};

// Header guard end.
#endif
