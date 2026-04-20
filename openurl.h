#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#elif __APPLE__
    #include <TargetConditionals.h>
#endif

void open_url(const char *url) {
    #ifdef _WIN32
        char command[512];
        snprintf(command, sizeof(command), "start %s", url);
        system(command);
    #elif __APPLE__
        char command[512];
        snprintf(command, sizeof(command), "open %s", url);
        system(command);
    #else
        char command[512];
        snprintf(command, sizeof(command), "xdg-open %s", url);
        system(command);
    #endif
}
