#include "Application.h"
#include <vld.h>

// -Host is charmander
// Use mouse click and point to move pieces
// -Press 'r' to restart
// -There is a Macro called TESTING in GameState.cpp Line 7, Set it to 1 to only spawn 2 pieces for testing

int main(int argc, char* argv[])
{
    App app;
    if (app.Initialize())
        app.Run();
    app.Shutdown();

    return 0;
}
