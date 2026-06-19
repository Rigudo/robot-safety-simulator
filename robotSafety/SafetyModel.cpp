// Include the SafetyModel declarations, SafetyInput, and SafetyResult structures.
#include "SafetyModel.h"

// Include Qt math helper functions such as qMax and qMin.
#include <QtMath>

// clamp() limits a value so that it always stays between lo and hi.
static double clamp(double value, double lo, double hi)
{
    // First limit the value to hi with qMin, then limit it to lo with qMax.
    return qMax(lo, qMin(hi, value));
}

// evaluate() receives sensor input values and calculates the robot safety state.
SafetyResult SafetyModel::evaluate(const SafetyInput &input)
{
    // Create an empty result object that will be filled step by step.
    SafetyResult result;

    // Calculate braking distance using speed^2 / (2 * deceleration).
    // qMax prevents division by a value that is too close to zero.
    result.brakeDistance = (input.robotSpeed * input.robotSpeed) / qMax(0.1, 2.0 * input.deceleration);

    // Select the smaller distance between the human and the obstacle.
    result.nearestDistance = qMin(input.humanDistance, input.obstacleDistance);

    // Calculate remaining safety margin after considering braking distance.
    result.safetyMargin = result.nearestDistance - result.brakeDistance;

    // Treat 0.1 m or less as physical contact with the robot.
    const double contactDistance = 0.1;

    // Calculate human risk based on how close the human is to the warning distance.
    double humanRisk = clamp((input.warningDistance - input.humanDistance) / qMax(0.1, input.warningDistance), 0.0, 1.0);

    // Calculate obstacle risk based on how close the obstacle is to the warning distance.
    double obstacleRisk = clamp((input.warningDistance - input.obstacleDistance) / qMax(0.1, input.warningDistance), 0.0, 1.0);

    // Convert door open rate from percent to a 0.0 to 1.0 risk value.
    double doorRisk = clamp(input.doorOpenRate / 100.0, 0.0, 1.0);

    // If safety margin is negative, braking risk is maximum; otherwise calculate risk from margin.
    double brakeRisk = result.safetyMargin < 0.0 ? 1.0 : clamp((input.stopDistance - result.safetyMargin) / qMax(0.1, input.stopDistance), 0.0, 1.0);

    // Emergency risk becomes maximum only when the emergency flag is true.
    double emergencyRisk = input.emergency ? 1.0 : 0.0;

    // Use the highest individual risk as the final risk score and scale it to 0 to 100.
    result.riskScore = clamp(100.0 * qMax(qMax(humanRisk, obstacleRisk), qMax(qMax(doorRisk, brakeRisk), emergencyRisk)), 0.0, 100.0);

    // STOP condition: emergency, door open too much, or physical contact.
    bool stop = input.emergency
             || input.doorOpenRate >= 70.0
             || input.humanDistance <= contactDistance
             || input.obstacleDistance <= contactDistance;

    // DANGER condition: human or obstacle entered the stop distance area.
    bool danger = input.humanDistance <= input.stopDistance
               || input.obstacleDistance <= input.stopDistance;

    // WARNING condition: near object, partially opened door, or high risk score.
    bool warning = input.humanDistance <= input.warningDistance
                || input.obstacleDistance <= input.warningDistance
                || input.doorOpenRate >= 25.0
                || result.riskScore >= 45.0;

    // STOP has the highest priority because the robot must not continue moving.
    if (stop) {
        // Save the final state text.
        result.state = "STOP";

        // Recommended speed is zero because the robot must stop.
        result.recommendedSpeed = 0.0;

        // Explain the exact reason for STOP.
        if (input.emergency) result.reason = "Emergency signal is on.";
        else if (input.doorOpenRate >= 70.0) result.reason = "The door is open too much.";
        else result.reason = "A human or obstacle touched the robot.";
    }
    // DANGER is used when an object is inside the stop area but not yet touching.
    else if (danger) {
        // Save the final state text.
        result.state = "DANGER";

        // Reduce robot speed strongly while keeping a small movement value for visualization.
        result.recommendedSpeed = qMax(0.1, input.robotSpeed * 0.2);

        // Explain why the robot is in DANGER state.
        result.reason = "An object is inside the stop area. Prepare to stop.";
    }
    // WARNING is used when the robot should slow down and monitor the environment.
    else if (warning) {
        // Save the final state text.
        result.state = "WARNING";

        // Reduce robot speed to less than half of the current speed.
        result.recommendedSpeed = qMax(0.2, input.robotSpeed * 0.45);

        // Explain why the robot is in WARNING state.
        result.reason = "Something is near the warning area. Slow down and monitor.";
    }
    // MOVE is used when all safety values are normal.
    else {
        // Save the final state text.
        result.state = "MOVE";

        // Keep the current speed because no safety risk is detected.
        result.recommendedSpeed = input.robotSpeed;

        // Explain why the robot can move normally.
        result.reason = "No object is close enough to require warning or stop.";
    }

    // Return all calculated result values to the UI.
    return result;
}
