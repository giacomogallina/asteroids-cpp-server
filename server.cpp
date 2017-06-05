#include <iostream>
#include <ctime>
#include <string.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <math.h>
#include <cstdlib>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

time_t timer;
clockid_t clk_id;
struct timespec tp;

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
    float X, Y, D, Vx, Vy;
    time_t pulse_time;
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
        time(&pulse_time);
        pulsing = 1;
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
            // cout << "moving!" << endl;
            Vx += a / 2.0 * cos(D);
            Vy -= a / 2.0 * sin(D);
        };
        Vx *= f;
        Vy *= f;
        X = fmod(X + Vx, width);
        Y = fmod(Y + Vy, height);
        while(X < 0)
        {
            X += width;
        };
        while(Y < 0)
        {
            Y += height;
        };
        if (up)
        {
            Vx += a / 2 * cos(D);
            Vy -= a / 2 * sin(D);
        };
        if (difftime(timer, pulse_time) > 3)
        {
            pulsing = 0;
            // cout << "pulsing: " << pulsing<< endl;
        };
    };
};


class Projectile
{
public:
    bool unused;
    float X, Y, D, Vx, Vy;
    time_t birth;
    int r, g, b;

    void shoot(Ship *ship, float p_s)
    {
        D = ship->D;
        X = ship->X + 10 * cos(D);
        Y = ship->Y - 10 * sin(D);
        Vx = p_s * cos(D) + ship->Vx;
        Vy = -p_s * sin(D) + ship->Vy;
        time(&birth);
        r = ship->r;
        g = ship->g;
        b = ship->b;
        unused = 0;
    };

    void remove()
    {
        if (difftime(timer, birth) > 2)
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
        while(X < 0)
        {
            X += width;
        };
        while(Y < 0)
        {
            Y += height;
        };
    };
};


class Asteroid
{
public:
    bool dead = 1;
    float X, Y, D, Vx, Vy;
    int list_position, type;
    int radius[3] = {50, 25, 15};

    void generate(int a, int width, int height, float speed, int t)
    {
        list_position = a;
        X = fmod((rand() % width)/2 - width/4, width);
        Y = fmod((rand() % height)/2 - height/4, height);
        Vx = ((rand() % 2000) / 1000.0 - 1) * speed;
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
            while(X < 0)
            {
                X += width;
            };
            while(Y < 0)
            {
                Y += height;
            };
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


class Engine
{
public:
    vector<Projectile> Ps;
    int highscore, window_width, window_height;
    float tick_rate;
    int tick_duration;
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
        tick_duration = 1000000000 / tick_rate;
        level_start_tick = -1;
        tick = 0;
        status = "null";
        level = 0;
        lifes = 5;
        points = 0;
        acceleration = 1000.0 / pow(tick_rate, 2);
        friction = pow(0.8, 1 / tick_rate);
        rotation_speed = 5.0 / tick_rate;
        projectile_speed = 400.0 / tick_rate;
        asteroids_speed = 200.0 / tick_rate;
        for (int i = 0; i < 8; i++)
        {
            Ps.push_back(Projectile());
        };
    };

    void wait_next_tick()
    {
        clock_gettime(clk_id, &tp);
        int time_to_sleep = fmod(next_tick_time - tp.tv_nsec, 1000000000);
        while (time_to_sleep < 0)
        {
            time_to_sleep += 1000000000;
        };
        // cout << time_to_sleep << endl;
        if (time_to_sleep < tick_duration)
        {
            usleep(time_to_sleep / 1000);
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
        // cout << "event_handle here!" << endl;
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
        cout << "starting level " << level << "!" << endl;
    };

    void check_for_level()
    {
        for (int i = 0; i < As.size(); i++)
        {
            if (!As[i].dead)
            {
                return;
            };
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

    void check_for_shoots()
    {
        Ship *s;
        Projectile *p;
        for (int i = 0; i < players.size(); i++)
        {
            s = players[i].ship;
            if (s->shoot)
            {
                s->shoot = 0;
                for (int j = 0; j < Ps.size(); j++)
                {
                    p = &Ps[j];
                    if (p->unused)
                    {
                        p->shoot(s, projectile_speed);
                        points -= 1;
                        break;
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
                    p = &Ps[j];
                    if (!p->unused)
                    {
                        if (pow(a->X - p->X, 2) + pow(a->Y - p->Y, 2) < pow(a->radius[a->type], 2))
                        {
                            a->dead = 1;
                            p->unused = 1;
                            if (a->type < 2)
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
                                // son1->Vx = a->Vx;
                                // son1->Vy = a->Vy;
                                // son2->Vx = a->Vx;
                                // son2->Vy = a->Vy;
                                alpha = (rand() % 1000) / 1000.0;
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
                    if (!(a->dead))
                    {
                        if (pow(s->X - a->X, 2) + pow(s->Y - a->Y, 2) <= pow(a->radius[a->type] + 5, 2))
                        {
                            s->pulsing = 1;
                            time(&(s->pulse_time));
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

    void move_stuff()
    {
        if (level > 0)
        {
            check_for_level();
        };
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

    void make_status()
    {
        string new_status, new_partial_status;
        Projectile *p;
        Ship *s;
        Asteroid *a;
        int types[7] = {0, 1, 2, 2, 1, 2, 2};
        int count = 0;
        new_status = "0";
        new_status += "," + to_string(level);
        // new_status += "," + to_string(Ps.size());
        for (int i = 0; i < Ps.size(); i++)
        {
            p = &Ps[i];
            if (!p->unused)
            {
                new_partial_status += "," + to_string(p->X);
                new_partial_status += "," + to_string(p->Y);
                new_partial_status += "," + to_string(p->D);
                new_partial_status += "," + to_string(p->r);
                new_partial_status += "," + to_string(p->g);
                new_partial_status += "," + to_string(p->b);
                count++;
            };
        };
        new_status += "," + to_string(count) + new_partial_status;
        new_partial_status = "";
        count = 0;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < As.size(); j++)
            {
                if (types[j%7] == i)
                {
                    a = &As[j];
                    if (!a->dead)
                    {
                        new_partial_status += "," + to_string(a->X);
                        new_partial_status += "," + to_string(a->Y);
                        new_partial_status += "," + to_string(M_PI / 2);
                        count++;
                    };
                };
            };
            // cout << i << endl;
            new_status += "," + to_string(count) + new_partial_status;
            count = 0;
            new_partial_status = "";
        };
        for (int i = 0; i < players.size(); i++)
        {
            s = players[i].ship;
            if ((!s->pulsing) || (fmod(difftime(timer, s->pulse_time), 0.5) <= 0.3))
            {
                new_partial_status += "," + s->name;
                new_partial_status += "," + to_string(s->X);
                new_partial_status += "," + to_string(s->Y);
                new_partial_status += "," + to_string(s->D);
                new_partial_status += "," + to_string(s->up);
                new_partial_status += "," + to_string(s->r);
                new_partial_status += "," + to_string(s->g);
                new_partial_status += "," + to_string(s->b);
                count++;
            };
        };
        new_status += "," + to_string(count) + new_partial_status;
        new_status += "," + to_string(points) + "," + to_string(highscore);
        status = new_status;
    };

    int make_new_id()
    {
        int new_id;
        bool ok;
        while(1)
        {
            new_id = rand();
            ok = 1;
            for (int i = 0; i < players.size(); i++)
            {
                if (players[i].id == new_id)
                {
                    ok = 0;
                };
            };
            if (ok)
            {
                return new_id;
            };
        };
    };

    void remove_player(Player *p)
    {
        for (int i = 0; i < players.size(); i++)
        {
            if (players[i].id == p->id)
            {
                players.erase(players.begin() + i);
            };
        };
    };

    void run();
};


void listener(int client, Engine *boss, vector<thread> *listeners, int position)
{
    char buffer[256];
    string temp;
    int check, id, n;
    vector<string> events_buffer;
    write(client, "Thank you for connecting", 24);
    bzero(buffer, 256);
    read(client, buffer, 255);
    for (int i = 0; i < 19; i++)
    {
        temp += buffer[i];
    };
    cout << temp << endl;
    temp = "";
    for (int i = 19; i < 256; i++)
    {
        if (buffer[i] == ',')
        {
            break;
        };
        temp += buffer[i];
    };
    // cout << "test 1" << endl;
    id = boss->make_new_id();
    // write(client, to_string(id).data(), to_string(id).size());
    send(client, to_string(id).data(), to_string(id).size(), MSG_NOSIGNAL);
    Ship s(temp, 255, 255, 255, boss->window_width, boss->window_height);
    Player p(id, &s);
    boss->players.push_back(p);
    cout << "new client (" << s.name << ") id: " << id << endl;
    if (boss->level == 0)
    {
        boss->level = 1;
        boss->start_level();
    };
    // cout << "test 2" << endl;
    while(1)
    {
        events_buffer.clear();
        // cout << "test 2.1" << endl;
        // bzero(buffer, 256);
        for (int i = 0; i < 256; i++)
        {
            buffer[i] = ' ';
        };
        // cout << "test 2.2" << endl;
        // if (read(client, buffer, 255) < 0)
        if (recv(client, buffer, 255, MSG_NOSIGNAL) < 0)
        {
            cout << s.name << " listener: cannot read events!" << endl;
            boss->remove_player(&p);
            listeners->erase(listeners->begin() + position);
            return;
        };
        // cout << "test 3" << endl;
        n = 0;
        temp = "";
        for (int i = 0; i < 256; i++)
        {
            // cout << buffer[i];
            if (buffer[i] == ',')
            {
                events_buffer.push_back(temp);
                // if (temp != "null")
                // {
                //     cout << temp << endl;
                // };
                temp = "";
            }
            else if (buffer[i] == ' ')
            {
                events_buffer.push_back(temp);
                // if (temp != "null")
                // {
                //     cout << temp << endl;
                // };
                break;
            }
            else
            {
                temp += buffer[i];
            };
        };
        // cout << endl;
        // cout << "test 4" << endl;
        for (int i = 1; i < events_buffer.size(); i += 2)
        {
            // cout << "test 4." << i << endl;
            // cout << id << ": " << events_buffer[i] << endl;
            boss->events.push_back(Event(id, events_buffer[i]));
        };
        // cout << "test 5" << endl;
        if (send(client, boss->status.data(), boss->status.size(), MSG_NOSIGNAL) < 0)
        {
            cout << s.name << " listener: cannot send status!" << endl;
            boss->remove_player(&p);
            cout << "test 1" << endl;
            // listeners->erase(listeners->begin() + position);
            // cout << "test 2" << endl;
            return;
        };
        // cout << "status sent!" << endl;
    };
};


void accept_connections(Engine *boss)
{
    cout << "accept_connections created!" << endl;
    int sockfd, new_client;
    vector<thread> listeners;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(12346);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    {
        cout << "cannot bind socket!" << endl;
        quick_exit(EXIT_FAILURE);
    };
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    cout << "socket initialized!" << endl;
    int position = 0;
    while(1)
    {
        new_client = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        cout << "found a new client!" << endl;
        thread t(listener, new_client, boss, &listeners, position);
        listeners.push_back(move(t));
        cout << "new listener thread created!" << endl;
        position ++;
    };
};


void Engine::run()
{
    // thread connections_accepter (accept_connections, this);
    clock_gettime(clk_id, &tp);
    next_tick_time = tp.tv_nsec + tick_duration;
    cout << next_tick_time << endl;
    // cout << points << ", " << highscore << endl;
    // start_level();
    while(1)
    {
        // cout << "tick " << tick << "!" << endl;
        event_handle();
        // cout << "event handled!" << endl;
        wait_next_tick();
        // cout << "next tick waited!" << endl;
        time(&timer);
        move_stuff();
        // cout << "stuff moved!" << endl;
        make_status();
        // cout << "status made!" << endl << endl;
        // if (tick % 1000 == 0)
        // {
        //     cout << status << endl;
        // };
    };
};

void do_some_stuff_just_to_waste_time()
{
    int n = 0;
    for (int i = 0; i < 100000; i++)
    {
        for (int j = 1; j < 10000; j++)
        {
            n += i % j;
            n = n % j;
        };
    };
    cout << "successfully did stuff just to waste some time, exiting!" << endl;
};


int main()
{
    srand(time(&timer));
    clk_id = CLOCK_REALTIME;
    clock_gettime(clk_id, &tp);
    Engine e;
    thread c(accept_connections, &e);
    // thread test(do_some_stuff_just_to_waste_time);
    e.run();
    return 0;
};
