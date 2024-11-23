#include "icb_gui.h"
#include <math.h>
#define SP_NUMBER 5
#define TABLE_CENTER_X 362         
#define TABLE_CENTER_Y 250         
#define TABLE_RADIUS 150           
#define PHILOSOPHER_RADIUS 35      
#define STICK_LENGTH 40            
#define STICK_HEIGHT 3            
#define PI 3.14

int FRM;
ICBYTES screen;

enum PhilosopherState {
    Thinking,
    Hungry,
    Eating
};

enum StickState {
    Available,
    InUse
};

void EatingAnimation() {
}

void DrawTable(int philosopherStates[SP_NUMBER], int stickStates[SP_NUMBER]) {
    FillCircle(screen, TABLE_CENTER_X, TABLE_CENTER_Y, TABLE_RADIUS, 0xff4e3b);
    float angleIncrement = 2 * PI / SP_NUMBER;

    for (int i = 0; i < SP_NUMBER; i++) {
        float angle = i * angleIncrement;
        int currX = TABLE_CENTER_X + (int)(TABLE_RADIUS * cos(angle));
        int currY = TABLE_CENTER_Y + (int)(TABLE_RADIUS * sin(angle));

        int color; 
        if (philosopherStates[i] == Thinking) color = 0x00ff00; //Green - Thinking
        else if (philosopherStates[i] == Eating) color = 0xeb0909; //Red - Eating
        else if (philosopherStates[i] == Hungry) color = 0xffff00; //Yellow - Hungry

        FillCircle(screen, currX, currY, PHILOSOPHER_RADIUS, color);

        angle = ((i + 1) % SP_NUMBER) * angleIncrement;
        int nextX = TABLE_CENTER_X + (int)(TABLE_RADIUS * cos(angle));
        int nextY = TABLE_CENTER_Y + (int)(TABLE_RADIUS * sin(angle));

        int stickCenterX = (currX + nextX) / 2;
        int stickCenterY = (currY + nextY) / 2;

        int stickColor = stickStates[i] == InUse ? 0xff0000 : 0x40ff00; //Red InUse, Green Available
        FillRect(screen, stickCenterX - STICK_LENGTH / 2, stickCenterY - STICK_HEIGHT / 2, STICK_LENGTH, STICK_HEIGHT, stickColor);
    }
    DisplayImage(FRM, screen);
}

class Sticks {
public:
    Sticks();
    void take(int stickId);
    void release(int stickId);
    bool isAvailable(int stickId);

private:
    StickState stickStates[SP_NUMBER];
};

class Philosophers {
public:
    Philosophers(int id, Sticks& sticks);
    void think();
    void eat();
    void pickSticks();
    void putSticks();
    PhilosopherState getState() { return state; } // Accessor for philosopher state

private:
    int id;
    Sticks& sticks;
    PhilosopherState state;
};

Sticks::Sticks() {
    for (int i = 0; i < SP_NUMBER; i++) {
        stickStates[i] = Available;
    }
}
void Sticks::take(int stickId) { stickStates[stickId] = InUse; }
void Sticks::release(int stickId) { stickStates[stickId] = Available; }
bool Sticks::isAvailable(int stickId) { return stickStates[stickId] == Available; }

Philosophers::Philosophers(int id, Sticks& sticks) : id(id), sticks(sticks), state(Thinking) {}
void Philosophers::think() { state = Thinking; }
void Philosophers::eat() {
    state = Eating;
    pickSticks();
    EatingAnimation();
    Sleep(0);
    putSticks();
}
void Philosophers::pickSticks() {
    if (sticks.isAvailable(id) && sticks.isAvailable((id + 1) % SP_NUMBER)) {
        sticks.take(id);
        sticks.take((id + 1) % SP_NUMBER);
        state = Eating;
    }
    else {
        state = Hungry;
    }
}
void Philosophers::putSticks() {
    sticks.release(id);
    sticks.release((id + 1) % SP_NUMBER);
}

void ICGUI_Create() { ICG_MWTitle("Dining Philosophers"); ICG_MWSize(790, 610); }

void Table() {
    CreateImage(screen, 560, 620, ICB_UINT);
}

void Start(void* simFunction) {
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)simFunction, NULL, 0, NULL);
    CreateImage(screen, 740, 520, ICB_UINT);
}

void Deadlock() {
    int philosopherStates[SP_NUMBER] = { Eating, Available, Hungry, Eating, Thinking };
    int stickStates[SP_NUMBER] = { InUse, Available, InUse, Available, InUse };
    DrawTable(philosopherStates, stickStates);
}

void Semaphore() {
    int philosopherStates[SP_NUMBER] = { Thinking, Hungry, Eating, Thinking, Eating };
    int stickStates[SP_NUMBER] = { Available, Available, InUse, Available, Available };
    DrawTable(philosopherStates, stickStates);
}

void ICGUI_main() {
    ICG_Button(10, 5, 120, 25, "Deadlock", Start, (void*)Deadlock);
    ICG_Button(135, 5, 120, 25, "Semaphore", Start, (void*)Semaphore);
    FRM = ICG_FrameMedium(10, 30, 750, 520);
}
