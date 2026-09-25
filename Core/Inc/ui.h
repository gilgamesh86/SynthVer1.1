#include "main.h"
#include <stdbool.h>

typedef enum { TITLE, BODY1, BODY2 } text;

typedef enum { PARAM_INT, PARAM_FLOAT } paramType;

void displayInit(void);
void displayText(text type, char *str, ...);
void displayClear(void);
bool parameterSet(TIM_HandleTypeDef *htim, void *parameter, paramType type);
