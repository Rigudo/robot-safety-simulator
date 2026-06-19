QT += widgets
CONFIG += c++17

TARGET = RobotRiskSimulator
TEMPLATE = app

SOURCES += \
    main.cpp \
    riskpage.cpp \
    SafetyModel.cpp

HEADERS += \
    riskpage.h \
    SafetyModel.h
