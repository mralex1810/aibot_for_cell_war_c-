#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <math.h>
#include <random>
#include <ctime>

using namespace std;

int CELL_COUNTER;
vector<vector<unsigned char>> cell_action(10, vector<unsigned char>(64));
vector<vector<unsigned char>> tentacle_action(10, vector<unsigned char>(64));
const vector<int> lvlscore = {0, 10, 25, 100, 250, 500, 990};
int lvl_to_score(int score)
{
    return upper_bound(lvlscore.begin(), lvlscore.end(), score) - lvlscore.begin() - 1;
}
int distance(pair<int, int> pos1, pair<int, int> pos2)
{
    return ((int)sqrt((pos1.first - pos2.first) * (pos1.first - pos2.first) +
                      (pos1.second - pos2.second) * (pos1.second - pos2.second))) /
           10;
}
struct Tentacle;
struct Cell
{
    string owner;
    int score, counter, lvl, id;
    pair<int, int> pos;
    vector<Tentacle *> tentacles;

    Cell(string owner, int lvl, pair<int, int> pos)
    {
        this->owner = owner;
        this->lvl = lvl;
        this->pos = pos;
        score = lvlscore[lvl];
        tentacles = {};
        id = CELL_COUNTER++;
        counter = 0;
    }

    void Cell::delete_tentacle(Tentacle *tentacle)
    {
        for (int i = 0; i < tentacles.size(); i++)
        {
            if (tentacles[i] == tentacle)
            {
                swap(tentacles[i], tentacles[tentacles.size() - 1]);
                tentacles.pop_back();
            }
        }
    }

    void attack(string attacker, int damage);
    void update();
    bool add_tentacle(Tentacle *tentacle);
    void delete_tentacle(Tentacle *tentacle);
};

struct Tentacle
{
    Cell *attacker;
    Cell *attacked;
    int len;
    bool need_attack;
    Tentacle(Cell *attacker, Cell *attacked)
    {
        this->attacker = attacker;
        this->attacked = attacked;
        len = distance(attacker->pos, attacked->pos);
        need_attack = false;
    }
    void destroy()
    {
        attacker->attack(attacker->owner, len / 2);
        attacked->attack(attacker->owner, len / 2);
        attacker->delete_tentacle(this);
    }
    void destroy_in_attacked()
    {
        attacked->attack(attacker->owner, len);
    }
    void update()
    {
        if (need_attack)
        {
            need_attack = false;
            attacked->attack(attacker->owner, 1);
        }
    }
};
bool Cell::add_tentacle(Tentacle *tentacle)
{
    if (tentacle->len <= score && tentacles.size() < lvl)
    {
        tentacles.push_back(tentacle);
        score -= tentacle->len;
    }
}
void Cell::update()
{
    if (owner == "gray")
    {
        return;
    }
    counter += lvl + 1;
    if (counter > 30)
    {
        counter = 0;
        if (score < 1000)
            score++;
        else
            counter += 10;
        lvl = lvl_to_score(score);
        for (auto tentacle : tentacles)
            tentacle->need_attack = true;
    }
}
void Cell::attack(string attacker, int damage)
{
    if (owner == attacker)
    {
        if (score >= 1000)
        {
            counter += 20;
        }
        else
        {
            score += damage;
        }
        lvl = lvl_to_score(score);
    }
    else if (damage <= score)
    {
        score -= damage;
        if (owner != "gray")
            lvl = lvl_to_score(score);
    }
    else
    {
        for (auto tentacle : tentacles)
        {
            tentacle->destroy_in_attacked();
        }
        tentacles.clear();
        if (owner == "gray")
            score = lvlscore[lvl];
        else
            score = damage - score;
        owner = attacker;
        lvl = lvl_to_score(score);
    }
}

struct AI
{
    vector<double> gen;
    string owner;

    AI(string owner, vector<double> gen)
    {
        this->owner = owner;
        this->gen = gen;
    }
};

int main()
{
    mt19937 gen32;
    gen32.seed(time(nullptr));
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 24; j++)
        {
            cell_action[i][j] = (char)gen32();
        }
    }
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 24; j++)
        {
            tentacle_action[i][j] = (char)gen32();
        }
    }
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 24; j++)
        {
            cout << (int)cell_action[i][j] << ' ';
        }
        cout << endl;
    }
}