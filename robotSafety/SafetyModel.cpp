#include "SafetyModel.h"
#include <QtMath>

static double clamp(double value, double lo, double hi)
{
    return qMax(lo, qMin(hi, value));
}

SafetyResult SafetyModel::evaluate(const SafetyInput &input)
{
    SafetyResult result;

    result.brakeDistance = (input.robotSpeed * input.robotSpeed) / qMax(0.1, 2.0 * input.deceleration);
    result.nearestDistance = qMin(input.humanDistance, input.obstacleDistance);
    result.safetyMargin = result.nearestDistance - result.brakeDistance;
    const double contactDistance = 0.1;

    double humanRisk = clamp((input.warningDistance - input.humanDistance) / qMax(0.1, input.warningDistance), 0.0, 1.0);
    double obstacleRisk = clamp((input.warningDistance - input.obstacleDistance) / qMax(0.1, input.warningDistance), 0.0, 1.0);
    double doorRisk = clamp(input.doorOpenRate / 100.0, 0.0, 1.0);
    double brakeRisk = result.safetyMargin < 0.0 ? 1.0 : clamp((input.stopDistance - result.safetyMargin) / qMax(0.1, input.stopDistance), 0.0, 1.0);
    double emergencyRisk = input.emergency ? 1.0 : 0.0;

    result.riskScore = clamp(100.0 * qMax(qMax(humanRisk, obstacleRisk), qMax(qMax(doorRisk, brakeRisk), emergencyRisk)), 0.0, 100.0);

    bool stop = input.emergency
             || input.doorOpenRate >= 70.0
             || input.humanDistance <= contactDistance
             || input.obstacleDistance <= contactDistance;

    bool danger = input.humanDistance <= input.stopDistance
               || input.obstacleDistance <= input.stopDistance;

    bool warning = input.humanDistance <= input.warningDistance
                || input.obstacleDistance <= input.warningDistance
                || input.doorOpenRate >= 25.0
                || result.riskScore >= 45.0;

    if (stop) {
        result.state = "STOP";
        result.recommendedSpeed = 0.0;
        if (input.emergency) result.reason = "Emergency signal is on.";
        else if (input.doorOpenRate >= 70.0) result.reason = "The door is open too much.";
        else result.reason = "A human or obstacle touched the robot.";
    } else if (danger) {
        result.state = "DANGER";
        result.recommendedSpeed = qMax(0.1, input.robotSpeed * 0.2);
        result.reason = "An object is inside the stop area. Prepare to stop.";
    } else if (warning) {
        result.state = "WARNING";
        result.recommendedSpeed = qMax(0.2, input.robotSpeed * 0.45);
        result.reason = "Something is near the warning area. Slow down and monitor.";
    } else {
        result.state = "MOVE";
        result.recommendedSpeed = input.robotSpeed;
        result.reason = "No object is close enough to require warning or stop.";
    }

    return result;
}
