#include <iostream>
#include <ctime>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <math.h>
#include <cstdlib>

using namespace std;

time_t timer;

class Event
{
public:
    int id;
    string command;
    Event(int a, string b)
    {
        id = a;
        command = b;
    };
};


class Ship
{
public:
    bool shoot, left, right, up;
    float X, Y, D, Vx, Vy;
    int r, g, b;
};


class Projectile
{
public:
    bool unused;
    float X, Y, D, Vx, Vy, birth;
    int r, g, b;

    void shoot(Ship *ship, float g)
    {
        X = ship->X + 10 * sin(ship->D);
        Y = ship->Y + 10 * cos(ship->D);
        Vx = g * sin(ship->D) + ship->Vx;
        Vy = -g * cos(ship->D) + ship->Vy;
        birth = time(&timer);
        r = ship->r;
        g = ship->g;
        b = ship->b;
        unused = 0;
    };

    void remove()
    {
        if (time(&timer) - birth > 2)
        {
            X = -100;
            Y = -100;
            Vx = 0;
            Vy = 0;
            unused = 1;
        };
    };

    void move(int width, int height)
    {
        X = fmod(X + Vx, width);
        Y = fmod(Y + Vy, height);
    };
};


class Asteroid
{
public:
    bool dead;
    float X, Y, D, Vx, Vy;
    int list_position, type;
    int radius[3] = {50, 25, 15};

    void generate(int a, int width, int height, float speed, int t)
    {
        list_position = a;
        X = fmod((rand() % width)/2 - width/4, width);
        Y = fmod((rand() % height)/2 - height/4, height);
        Vx = ((rand() % 2000) / 1000 - 1) * speed;
        Vy = (rand() % 2 * 2 - 1) * pow(pow(speed, 2) - pow(Vx, 2), 0.5);
        dead = 0;
        type = t;
    };
};


class Player
{
public:
    int id;
    Ship *ship;
    Player(int a, Ship *b)
    {
        id = a;
        ship = b;
    };
};


class Connection_accepter
{};


class Listener
{};


class Engine
{
public:
    vector<Listener> listeners;
    Connection_accepter connection_accepter = Connection_accepter();
    vector<Projectile> Ps;
    int highscore, window_width, window_height;
    float tick_rate;
    float tick_duration;
    int level_start_tick;
    int tick = 0;
    vector<Event> events;
    string status;
    vector<Player> players;
    int level;
    vector<Asteroid> As;
    int lifes;
    int points;
    float acceleration;
    float friction;
    float rotation_speed;
    float projectile_speed;
    float asteroids_speed;
    float next_tick_time;

    void import_settings()
    {
        ifstream settings_file ("server_settings.txt", ios_base::in);
        settings_file >> highscore;
        settings_file >> window_width;
        settings_file >> window_height;
        settings_file >> tick_rate;
        settings_file.close();
    };

    Engine()
    {
        import_settings();
        tick_duration = 1 / tick_rate;
        level_start_tick = -1;
        tick = 0;
        status = "null";
        level = 0;
        lifes = 5;
        points = 0;
        acceleration = 1000.0 / pow(tick_rate, 2);
        friction = pow(0.8, tick_duration);
        rotation_speed = 5.0 / tick_rate;
        projectile_speed = 400.0 / tick_rate;
        asteroids_speed = 200.0 / tick_rate;
        for (int i = 0; i < 8; i++)
        {
            Ps.push_back(Projectile());
        };
    };

    void wait_next_frame()
    {
        float time_to_sleep = next_tick_time - time(&timer);
        if (time_to_sleep > 0)
        {
            usleep(time_to_sleep * 1000);
        };
        next_tick_time += tick_duration;
        tick += 1;
    };

    Ship *search(int id)
    {
        for (int i = 0; i < players.size(); i++)
        {
            if (players[i].id == id)
            {
                return players[i].ship;
            };
        };
    };

    void event_handle()
    {
        bool value;
        Ship *ship;
        for (int i = 0; i < events.size(); i++)
        {
            try
            {
                if (events[i].command[1] == 't')
                {
                    value = 1;
                }
                else if (events[i].command[1] == 'f')
                {
                    value = 0;
                };

                ship = search(events[i].id);
                if (events[i].command[0] == 's')
                {
                    ship->shoot = value;
                }
                else if (events[i].command[0] == 'l')
                {
                    ship->left = value;
                }
                else if (events[i].command[0] == 'r')
                {
                    ship->right = value;
                }
                else if (events[i].command[0] == 'u')
                {
                    ship->up = value;
                };
            }
            catch (int e)
            {
                cout << "found an error while processing events" << endl;
            };
        };
        events.clear();
    };

    void start_level()
    {
        As.clear();
        for (int i = 0; i < 14 * level; i++)
        {
            As.push_back(Asteroid());
        };
        for (int i = 0; i < 14 * level; i += 7)
        {
            As[i].generate(i, window_width, window_height, asteroids_speed, 0);
        };
        level_start_tick = -1;
    };

    void check_for_level()
    {
        for (int i = 0; i < As.size(); i++)
        {
            if (!As[i].dead)
            {
                return;
            };
            if (level_start_tick < tick)
            {
                level_start_tick = tick + tick_rate;
                level += 1;
            };
            if (tick == level_start_tick)
            {
                start_level();
            };
        };
    };

    void check_for_collisions()
    {
        Asteroid *a;
        Projectile *p;
        Ship *s;
        for (int i = 0; i < As.size(); i++)
        {
            a = &As[i];
            if (!a->dead)
            {
                for (int j = 0; j < Ps.size(); j++)
                {
                    p = &Ps[i];
                    if (pow(a->X - p->X, 2) + pow(a->Y - p->Y, 2) < pow(a->radius[a->type], 2))
                    {

                    };
                };
            };
        };
    };
};


int main()
{
    srand(time(&timer));
    double secs = time(&timer);
    cout << secs << endl;
    return 0;
};
