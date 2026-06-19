// QApplication is the main Qt application class used to run a Qt Widgets program.
#include <QApplication>

// RiskPage is the main screen class of this simulator.
#include "riskpage.h"

// main() is the entry point where the C++ program starts running.
int main(int argc, char *argv[])
{
    // Create the Qt application object using command line arguments.
    QApplication app(argc, argv);

    // Create the main simulator page object.
    RiskPage page;

    // Set the title displayed on the top bar of the window.
    page.setWindowTitle("Robot Risk Safety Simulator");

    // Fix the window size so the UI layout stays stable during demonstration.
    page.setFixedSize(1360, 860);

    // Display the main page on the screen.
    page.show();

    // Start the Qt event loop and keep the program running until the window is closed.
    return app.exec();
}
