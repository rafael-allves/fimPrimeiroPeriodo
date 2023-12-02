#include "client/loadScreens.h"
#include "server/api/api.h"

void configureApp()
{
    setIsOpen(TRUE);

    #ifdef __linux__
        configureConsole();
    #endif

    initApi();

    initAppStateManager();
    loadScreens();
}

void finishApp()
{
    freeScreens(); freeScreenButtons(); freeScreenInputs(); freeApi();

    #ifdef __linux__
        endwin();
    #endif

}

int main(int argc, char **argv) {
    configureApp();

    useServerDb();
    createBinaryFile("teste");

    while(isOpen())
        runApp();
    
    finishApp();

    return 0;
}