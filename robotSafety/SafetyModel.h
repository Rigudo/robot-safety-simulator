#ifndef SAFETYMODEL_H
#define SAFETYMODEL_H

#include <QString>

struct SafetyInput
{
    double humanDistance = 15.0;
    double obstacleDistance = 18.0;
    double doorOpenRate = 0.0;
    double robotSpeed = 1.0;
    double deceleration = 1.5;
    bool emergency = false;

    double warningDistance = 6.0;
    double stopDistance = 2.5;
};

struct SafetyResult
{
    QString state;
    QString reason;
    double riskScore = 0.0;
    double brakeDistance = 0.0;
    double nearestDistance = 0.0;
    double safetyMargin = 0.0;
    double recommendedSpeed = 0.0;
};

class SafetyModel
{
public:
    static SafetyResult evaluate(const SafetyInput &input);
};

#endif
