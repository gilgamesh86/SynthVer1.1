#include "main.h"

typedef enum { TITLE, BODY } text;

void displayInit(void);
void displayText(text type, char *str, ...);
void displayClear(void);
