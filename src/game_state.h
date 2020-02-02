#ifndef GAMESTATE_H
#define GAMESTATE_H

typedef enum State {STATE_EXIT, STATE_MENU, STATE_GAME} State;

State GetState();
void SetNextState(State s);
void NextState();

#endif
