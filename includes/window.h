#include <string.h>
#include "lib.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>
#endif

typedef struct
{
    unsigned int width;
    unsigned int height;

    unsigned int startPointX;
    unsigned int startPointY;
} Box;

typedef struct
{
    unsigned int width;
    unsigned int height;

    unsigned int startPointX;
    unsigned int startPointY;

    const char *text;
    void (*onClick)(void);
} Button;

Button *screenButtons = NULL;
int numScreenButtons = 0;

void addButtonToScreen(Button button)
{
    screenButtons = (Button *)realloc(screenButtons, (numScreenButtons + 1) * sizeof(Button));

    if (screenButtons == NULL)
    {
        printf("Falha na alocação de memória.\n");
        return;
    }

    // Adiciona o novo botão
    screenButtons[numScreenButtons] = button;
    numScreenButtons++;
}

void configureConsole()
{
#ifdef _WIN32
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD fdwMode = ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hInput, fdwMode);

    INPUT_RECORD irInBuf[128];
    DWORD cNumRead;
#elif defined(__linux__)
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    timeout(0);
#endif
}

void handleButtonEvent(Button *button, int mouseX, int mouseY)
{
    if (mouseX >= button->startPointX && mouseX < (button->startPointX + button->width) &&
        mouseY >= button->startPointY && mouseY < (button->startPointY + button->height))
        if (button->onClick != NULL)
            button->onClick();
}

boolean handleEvents()
{
#ifdef _WIN32
    while (1)
    {
        ReadConsoleInput(hInput, irInBuf, 128, &cNumRead);
        for (DWORD i = 0; i < cNumRead; i++)
        {
            if (irInBuf[i].EventType == MOUSE_EVENT)
            {
                MOUSE_EVENT_RECORD mer = irInBuf[i].Event.MouseEvent;
                if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
                {
                    COORD pos = mer.dwMousePosition;
                    for (int i = 0; i < numScreenButtons; ++i)
                        handleButtonEvent(&screenButtons[i], pos.X, pos.Y);
                }
            }
        }

        if (kbhit() && getch() == 27)
            return FALSE;
    }
    return TRUE;

#elif defined(__linux__)
    int ch = getch();
    if (ch == KEY_MOUSE)
    {
        MEVENT event;
        if (getmouse(&event) == OK)
            if (event.bstate & BUTTON1_PRESSED)
                for (int i = 0; i < numScreenButtons; ++i)
                    handleButtonEvent(&screenButtons[i], event.x, event.y);
    }

    if (ch == 'q')
        return FALSE;

    refresh();
    return TRUE;
#endif
}

int *getWindowSize()
{
    int *windowSize = (int *)malloc(2 * sizeof(int));

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    windowSize[0] = csbi.dwSize.X;
    windowSize[1] = csbi.dwSize.Y;

#elif defined(__linux__)
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    windowSize[0] = w.ws_col;
    windowSize[1] = w.ws_row;

#endif

    return windowSize;
}

Box *createBox(unsigned int width, unsigned int height, unsigned int startPointX, unsigned int startPointY)
{
    Box *box = (Box *)malloc(sizeof(Box));

    box->width = width;
    box->height = height;
    box->startPointX = startPointX;
    box->startPointY = startPointY;

    return box;
}

void renderText(unsigned int posX, unsigned int posY, const char *text)
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD position = {posX, posY};
    SetConsoleCursorPosition(console, position);
    printf("%s", text);
#elif __linux__
    printf("\033[%d;%dH%s", posY, posX, text);
#endif
}

void createBorder(Box *box, unsigned int borderSize)
{
    if (box->width < 2 * borderSize || box->height < 2 * borderSize)
        error("Tamanho da borda muito grande para a caixa!\n");

    for (unsigned int y = 0; y < box->height; ++y)
        for (unsigned int x = 0; x < box->width; ++x)
            if (x < borderSize || x >= (box->width - borderSize) || y < borderSize || y >= (box->height - borderSize))
                renderText(x + box->startPointX, y + box->startPointY, "#");
}

int *getCenterPos(Box *boxRelative, unsigned short textLength, boolean horizontal, boolean vertical)
{
    int *positions = (int *)malloc(sizeof(int) * 2);

    positions[0] = horizontal ? (boxRelative->startPointX + (boxRelative->width - textLength) / 2) : 0;
    positions[1] = vertical ? (boxRelative->startPointY + (boxRelative->height - 1) / 2) : 0;

    return positions;
}