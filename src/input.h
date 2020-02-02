#ifndef INPUT_H
#define INPUT_H

typedef enum key {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_TOGGLE_BLOOM, KEY_BACK, KEY_ENTER, NUMKEYS} Key;

int GetKey(unsigned n);
int GetKeyDown(unsigned n);
int GetKeyUp(unsigned n);

void InputUpdate();

#endif