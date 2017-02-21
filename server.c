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
    bool shoot, left, right, up, pulsing;
    float X, Y, D, Vx, Vy, pulse_time;
    int r, g, b;
    string name;

    Ship(string n, int R, int G, int B, int width, int height)
    {
        name = n;
        r = R;
        g = G;
        b = B;
        X = width / 2;
        Y = height / 2;
        Vx = 0;
        Vy = 0;
        D = M_PI / 2;
    };

    void move(float r_s, float a, float f, int width, int height)
    {
        if (left)
        {
            D = fmod(D + r_s, 2 * M_PI);
        };
        if (right)
        {
            D = fmod(D - r_s, 2 * M_PI);
        };
        if (up)
        {
            Vx += a / 2 * cos(D);
            Vy -= a / 2 * sin(D);
        };
        Vx *= f;
        Vy *= f;
        X = fmod(X + Vx, width);
        Y = fmod(Y + Vy, height);
        if (up)
        {
            Vx += a / 2 * cos(D);
            Vy -= a / 2 * sin(D);
        };
        if (time(&timer) - pulse_time > 3)
        {
            pulsing = 0;
        };
    };
};


class Projectile
{
public:
    bool unused;
    float X, Y, D, Vx, Vy, birth;
    int r, g, b;

    void shoot(Ship *ship, float p_s)
    {
        D = ship->D;
        X = ship->X + 10 * sin(D);
        Y = ship->Y + 10 * cos(D);
        Vx = p_s * sin(D) + ship->Vx;
        Vy = -p_s * cos(D) + ship->Vy;
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

    void move(int width, int height)
    {
        if (!dead)
        {
            X = fmod(X + Vx, width);
            Y = fmod(Y + Vy, height);
        };
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

    void check_for_shoots()
    {
        Ship *s;
        Projectile *p;
        for (int i = 0; i < players.size(); i++)
        {
            s = players[i].ship;
            if (s->shoot)
            {
                for (int j = 0; j < Ps.size(); j++)
                {
                    p = &Ps[j];
                    if (p->unused)
                    {
                        p->shoot(s, projectile_speed);
                        points -= 1;
                    };
                };
            };
        };
    };

    void check_for_collisions()
    {
        Asteroid *a, *son1, *son2;
        Projectile *p;
        Ship *s;
        float alpha, sin_alpha, cos_alpha;
        // asteroids-projectile collisions
        for (int i = 0; i < As.size(); i++)
        {
            a = &As[i];
            if (!a->dead)
            {
                for (int j = 0; j < Ps.size(); j++)
                {
                    p = &Ps[i];
                    if (!p->unused)
                    {
                        if (pow(a->X - p->X, 2) + pow(a->Y - p->Y, 2) < pow(a->radius[a->type], 2))
                        {
                            a->dead = 1;
                            p->unused = 0;
                            if (a->type < 3)
                            {
                                son1 = &As[a->list_position + 1];
                                son2 = &As[a->list_position + 4 - 2 * a->type];
                                son1->generate(a->list_position + 1,
                                    window_width, window_height,
                                    asteroids_speed, a->type + 1);
                                son2->generate(a->list_position + 4
                                    - 2 * a->type, window_width, window_height,
                                    asteroids_speed, a->type + 1);
                                points += 20 + 30 * a->type;
                                son1->X = a->X;
                                son1->Y = a->Y;
                                son2->X = a->X;
                                son2->Y = a->Y;
                                son1->Vx = a->Vx;
                                son1->Vy = a->Vy;
                                son2->Vx = a->Vx;
                                son2->Vy = a->Vy;
                                alpha = (rand() % 1000) / 1000;
                                sin_alpha = sin(alpha);
                                cos_alpha = cos(alpha);
                                son1->Vx = a->Vx * cos_alpha - a->Vy * sin_alpha;
                                son1->Vy = a->Vx * sin_alpha + a->Vy * cos_alpha;
                                sin_alpha = sin(-alpha);
                                cos_alpha = cos(-alpha);
                                son2->Vx = a->Vx * cos_alpha - a->Vy * sin_alpha;
                                son2->Vy = a->Vx * sin_alpha + a->Vy * cos_alpha;
                            }
                            else
                            {
                                points += 100;
                            };
                            if (points > highscore)
                            {
                                highscore = points;
                            };
                        };
                    };
                };
            };
        };
        // ship-asteroid/projectile collisions
        for (int i = 0; i < players.size(); i++)
        {
            s = players[i].ship;
            if (!s->pulsing)
            {
                for (int j = 0; j < As.size(); j++)
                {
                    a = &As[j];
                    if (!a->dead)
                    {
                        if (pow(s->X - a->X, 2) + pow(s->Y - a->Y, 2) <= pow(a->radius[a->type] + 5, 2))
                        {
                            s->pulsing = 1;
                            s->pulse_time = time(&timer);
                            s->X = window_width/2;
                            s->Y = window_height/2;
                            s->D = M_PI/2;
                            s->Vx = 0;
                            s->Vy = 0;
                            lifes -= 1;
                            break;
                        };
                    };
                };
                for (int j = 0; j < Ps.size(); j++)
                {
                    p = &Ps[j];
                    if (!p->unused)
                    {
                        if (pow(s->X - p->X, 2) + pow(s->Y - p->Y, 2) <= 8)
                        {
                            s->pulsing = 1;
                            s->pulse_time = time(&timer);
                            s->X = window_width/2;
                            s->Y = window_height/2;
                            s->D = M_PI/2;
                            s->Vx = 0;
                            s->Vy = 0;
                            lifes -= 1;
                            p->unused = 0;
                            break;
                        };
                    };
                };
            };
        };
    };

    void move()
    {
        check_for_level();
        check_for_shoots();
        for (int i = 0; i < players.size(); i++)
        {
            players[i].ship->move(rotation_speed, acceleration, friction,
                window_width, window_height);
        };
        for (int i = 0; i < Ps.size(); i++)
        {
            if (!Ps[i].unused)
            {
                Ps[i].remove();
                Ps[i].move(window_width, window_height);
            };
        };
        for (int i = 0; i < As.size(); i++)
        {
            if (!As[i].dead)
            {
                As[i].move(window_width, window_height);
            };
        };
        check_for_collisions();
    };
};


int main()
{
    srand(time(&timer));
    double secs = time(&timer);
    cout << secs << endl;
    return 0;
};
