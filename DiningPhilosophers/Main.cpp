#include "icb_gui.h"
#include <math.h>
#include <random>

#define TABLE_CENTER_X 362         
#define TABLE_CENTER_Y 250         
#define TABLE_RADIUS 150           
#define PHILOSOPHER_RADIUS 35      
#define STICK_LENGTH 40            
#define STICK_HEIGHT 3            
#define PI 3.14
#define NUM 5
#define MAXTIME 100
#define MINTIME 0
#define ONTABLE -1

int FRM;
ICBYTES screen;

enum State { Thinking, Hungry, Eating };

int GetRandomTime()
{
    static std::mt19937 rnd(std::time(nullptr));
    return std::uniform_int_distribution<>(MINTIME, MAXTIME)(rnd);
}

class ChopStick
{
    int id;
    int owner;
    bool useSemaphore;
    HANDLE binarySemaphore;
public:

    ChopStick(int id, bool useSemaphore) : id(id), useSemaphore(useSemaphore), owner(ONTABLE)
    {
        if (useSemaphore)
        {
            binarySemaphore = CreateSemaphore(NULL, 1, 1, NULL);
        }
    }

    void Take(int philosopherId) {
        if (useSemaphore)
        {
            WaitForSingleObject(binarySemaphore, INFINITE);
        }
        else
        {
            while (owner != ONTABLE) { Sleep(1); } // Wait for the stick to be available
        }
        owner = philosopherId;
    }
    void Release()
    {
        owner = ONTABLE;
        if (useSemaphore)
        {
            ReleaseSemaphore(binarySemaphore, 1, NULL);
        }
    }
    bool IsAvailable() { return owner == ONTABLE; }

    // Drawing
    int getId() { return id; }
    int getOwner() { return owner; }


    ~ChopStick()
    {
        if (binarySemaphore) // If the semaphore is created
        {
            CloseHandle(binarySemaphore);
        }
    }
};

class Philosopher
{
    int id;
    State state;
    int eatCount;

    ChopStick& leftStick;
    ChopStick& rightStick;

    HANDLE mainThread;

    void Think()
    {
        Sleep(GetRandomTime());
    }

    void TakeSticks()
    {
        state = Hungry;

        leftStick.Take(id);
        rightStick.Take(id);

        state = Eating;
    }

    void Eat()
    {
        Sleep(GetRandomTime());
        eatCount++;
    }

    void PutSticks()
    {
        leftStick.Release();
        rightStick.Release();

        state = Thinking;
    }

    void* PhilosopherThread()
    {
        while (true)
        {
            Think();
            TakeSticks();
            Eat();
            PutSticks();
        }
        return 0;
    }

    static void* ThreadEntry(LPVOID param)
    {
        Philosopher* _this = (Philosopher*)param;
        return (_this->PhilosopherThread());
    }

public:
    Philosopher(int id, ChopStick& leftStick, ChopStick& rightStick) : id(id), leftStick(leftStick), rightStick(rightStick), state(Thinking), eatCount(0)
    {
        mainThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadEntry, this, 0, NULL);
    }

    ~Philosopher()
    {
        TerminateThread(mainThread, 0); //  Kill the thread
        CloseHandle(mainThread);
    }

    int getId() { return id; }
    State getState() { return state; }
    int getEatCount() { return eatCount; }
};

class Table
{
    bool empty;
    ChopStick* sticks[NUM];
    Philosopher* philosophers[NUM];

    bool IsDeadlock()
    {
        for (int i = 0; i < NUM; i++) {
            if (philosophers[i]->getState() != Hungry)
            {
                return false;
            }
        }
        return true;
    }

public:
    Table() { empty = true; }

    bool IsEmpty() { return empty; }

    void Create(bool useSemaphore)
    {
        if (!empty) return;

        for (int i = 0; i < NUM; i++) {
            sticks[i] = new ChopStick(i, useSemaphore);
        }

        for (int i = 0; i < NUM; i++) {
            philosophers[i] = new Philosopher(i, *sticks[i], *sticks[(i + 1) % NUM]);
        }
        empty = false;
    }

    void Destroy()
    {
        if (empty) return;

        for (int i = 0; i < NUM; i++)
        {
            delete philosophers[i];
        }
        for (int i = 0; i < NUM; i++)
        {
            delete sticks[i];
        }
        empty = true;
    }

    bool TryDraw()
    {
        //ChopStick c;
        //c.getOwner();

        //Philosopher p;
        //p.getState();


        if (empty) return false;

        FillCircle(screen, TABLE_CENTER_X, TABLE_CENTER_Y, TABLE_RADIUS, 0x663300);
        float angleIncrement = 2 * PI / NUM;

        bool deadlock = IsDeadlock();

        for (int i = 0; i < NUM; i++)
        {
            Philosopher& philosopher = *philosophers[i];
            int id = philosopher.getId();
            State state = philosopher.getState();

            float angle = id * angleIncrement;
            int currX = TABLE_CENTER_X + (int)(TABLE_RADIUS * cos(angle));
            int currY = TABLE_CENTER_Y + (int)(TABLE_RADIUS * sin(angle));

            int color = 0;
            if (deadlock)
            {
                color = 0xff0000;
            }
            else if (state == Eating)
            {
                color = 0x00ff00;
            }
            else if (state == Hungry)
            {
                color = 0xff6600;
            }
            else if (state == Thinking)
            {
                color = 0x0066ff;
            }

            FillCircle(screen, currX, currY, PHILOSOPHER_RADIUS, color);
            static char str[5];
            sprintf_s(str, "%d", philosopher.getEatCount());
            Impress12x20(screen, currX, currY, str, 0);

            angle = ((id + 1) % NUM) * angleIncrement;
            int nextX = TABLE_CENTER_X + (int)(TABLE_RADIUS * cos(angle));
            int nextY = TABLE_CENTER_Y + (int)(TABLE_RADIUS * sin(angle));

            int stickCenterX = (currX + nextX) / 2;
            int stickCenterY = (currY + nextY) / 2;

            ChopStick& stick = *sticks[i];

            int stickColor = stick.IsAvailable() ? 0x40ff00 : 0xff0000; //Red InUse, Green Available
            FillRect(screen, stickCenterX - STICK_LENGTH / 2, stickCenterY - STICK_HEIGHT / 2, STICK_LENGTH, STICK_HEIGHT, stickColor);
        }
        return true;
    }

    ~Table()
    {
        Destroy();
    }
};

Table table;


void ICGUI_Create()
{
    ICG_MWTitle("Dining Philosophers");
    ICG_MWSize(790, 610);
}

void Start()
{
    if (!table.IsEmpty())
    {
        table.Destroy();
    }

    table.Create(false);
}

void StartWithSemaphore()
{
    if (!table.IsEmpty())
    {
        table.Destroy();
    }

    table.Create(true);
}

void* ScreenController(void* param)
{
    while (true)
    {
        if (table.TryDraw())
        {
            DisplayImage(FRM, screen);
        }
        Sleep(30);
    }
    return 0;
}

void ICGUI_main() {
    ICG_Button(10, 5, 80, 25, "Start", Start);
    ICG_Button(95, 5, 160, 25, "Start With Semaphore", StartWithSemaphore);
    FRM = ICG_FrameMedium(10, 30, 750, 520);
    CreateImage(screen, 740, 520, ICB_UINT);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ScreenController, NULL, 0, NULL);
}