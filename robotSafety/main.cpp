#include <QApplication>

#include "riskpage.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    RiskPage page;
    page.setWindowTitle("Robot Logic Circuit Simulator");
    page.setFixedSize(1360, 860);
    page.show();

    return app.exec();
}
