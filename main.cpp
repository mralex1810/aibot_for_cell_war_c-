#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <math.h>
#include <random>
#include <ctime>
#include <limits.h>
#include <thread>
#include <mutex>

using namespace std;

const int POPULATION = 18;
const int GENERATIONS = 100;
const int GENES = 64;
const int CHANCE_MUTATION = 20;
const int GEN_MUTATION = 60;
const int CELLS_COUNT = 45;
const int SIGNS_CELL = 10;
const int SIGNS_TENTACLE = 10;

//mutex score_lock;
mt19937 gen32;
vector<vector<char>> cell_action(10, vector<char>(GENES));
vector<vector<char>> tentacle_action(10, vector<char>(GENES));
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

class Cell
{
public:
    string owner;
    int score;
    int lvl;
    int id;
    int counter;
    pair<int, int> pos;
    vector<Cell *> cells;

    Cell(string owner, int lvl, pair<int, int> pos, int &CELL_COUNTER)
    {
        this->owner = owner;
        this->lvl = lvl;
        this->pos = pos;
        this->score = lvlscore[lvl];
        cells = {};
        counter = 0;
        id = CELL_COUNTER++;
    }

    void attack(string attacker, int damage, vector<pair<pair<int, int>, int>> &attack_next)
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
            for (auto cell : cells)
            {
                attack_next.push_back(make_pair(make_pair(id, cell->id), distance(pos, cell->pos)));
            }
            cells.clear();
            if (owner == "gray")
                score = lvlscore[lvl];
            else
                score = damage - score;
            owner = attacker;
            lvl = lvl_to_score(score);
        }
    }
    void update(vector<pair<pair<int, int>, int>> &attack_next)
    {
        if (owner == "gray")
        {
            return;
        }
        int tmp = counter;
        counter = tmp + lvl + 1;

        if (counter >= 30)
        {
            counter = 0;
            if (score < 1000)
            {
                score++;
            }
            else
            {
                counter += 20;
            }
            lvl = lvl_to_score(score);
            for (auto cell : cells)
            {
                attack_next.push_back(make_pair(make_pair(id, cell->id), 1));
            }
        }
    }
    bool add_tentacle(Cell *cell_end)
    {
        if (distance(pos, cell_end->pos) <= score && cells.size() < lvl)
        {
            cells.push_back(cell_end);
            score -= distance(pos, cell_end->pos);
            return true;
        }
        return false;
    }

    void delete_tentacle(Cell *cell_end)
    {
        for (int i = 0; i < cells.size(); i++)
        {
            if (cells[i] == cell_end)
            {
                swap(cells[i], cells[cells.size() - 1]);
                cells.pop_back();
            }
        }
    }
};

class Chromosome
{
public:
    vector<char> chromosome;
    Chromosome()
    {
        for (int i = 0; i < GENES; i++)
            chromosome.push_back((char)(gen32() % 255 - 128));
    }
    Chromosome(Chromosome *parent)
    {
        for (int i = 0; i < GENES; i++)
        {
            if (gen32() % 100 < CHANCE_MUTATION)
                chromosome.push_back((char)(parent->get_gen(i) + (gen32() % GEN_MUTATION) - GEN_MUTATION / 2));
            else
                chromosome.push_back(parent->get_gen(i));
        }
    }
    int get_gen(int index)
    {
        return chromosome[index];
    }
    void print()
    {
        cout << "[";
        for (int i = 0; i < GENES - 1; i++)
            cout << (int)chromosome[i] << ", ";
        cout << (int)chromosome[GENES - 1];
        cout << "]\n";
    }
};

class AI
{
public:
    Chromosome *gens;
    string owner;

    AI(string owner, Chromosome *gens)
    {
        this->owner = owner;
        this->gens = gens;
    }

    vector<pair<int, int>> process(vector<Cell *> &cells, vector<vector<bool>> &tentacles_ways, int tick)
    {
        vector<pair<int, int>> ans;
        for (auto cell_begin : cells)
        {
            if (cell_begin->owner != owner)
                continue;
            for (auto cell_end : cells)
            {
                if (cell_begin->id == cell_end->id)
                    continue;
                bool is_enemy = 0;
                bool is_gray = 0;
                bool is_friend = 0;
                if (cell_end->owner == "gray")
                    is_gray = 1;
                else if (cell_end->owner != cell_begin->owner)
                    is_enemy = 1;
                else
                    is_friend = 1;
                int way = distance(cell_begin->pos, cell_end->pos);
                if (!tentacles_ways[cell_begin->id][cell_end->id])
                {
                    vector<int> signs_cell = {way / 20, 15000 / way, 100 * is_enemy, 100 * is_gray, 100 * is_friend,
                                              cell_end->score / 10, cell_begin->score / 10, (cell_end->score - cell_begin->score) / 10,
                                              tick / 50, (cell_begin->lvl - (int)cell_begin->cells.size()) * 100 / 7};
                    int battle = 0;
                    for (int i = 0; i < SIGNS_CELL; i++)
                    {
                        for (int j = 0; j < GENES; j++)
                            battle += cell_action[i][j] * signs_cell[i] * gens->get_gen(j);
                    }
                    if (battle > 0)
                    {
                        ans.push_back(make_pair(cell_begin->id, cell_end->id));
                        //cout << "attack\n";
                    }
                }
                else
                {
                    vector<int> signs_tentacle = {way / 20, 15000 / way, 100 * is_enemy, 100 * is_gray, 100 * is_friend,
                                                  cell_end->score / 10, cell_begin->score / 10, (cell_end->score - way / 2) / 8,
                                                  tick / 50, (cell_begin->lvl - (int)cell_begin->cells.size()) * 100 / 7};
                    int destroy = 0;
                    for (int i = 0; i < SIGNS_TENTACLE; i++)
                    {
                        for (int j = 0; j < GENES; j++)
                            destroy += tentacle_action[i][j] * signs_tentacle[i] * gens->get_gen(j);
                    }
                    if (destroy > 0)
                    {
                        ans.push_back(make_pair(cell_begin->id, cell_end->id));
                        //cout << "destroy\n";
                    }
                }
            }
        }
        return ans;
    }
};


void game(Chromosome *green_gens, Chromosome *red_gens, int green_i, int red_i, vector<pair<int, int>> &score)
{
    int start_time = time(nullptr);
    int CELL_COUNTER = 0;
    vector<Cell *> cells;

    cells.push_back(new Cell("gray", 1, make_pair(100, 150), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(100, 350), CELL_COUNTER));
    cells.push_back(new Cell("green", 2, make_pair(100, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(100, 650), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(100, 850), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(300, 100), CELL_COUNTER));
    cells.push_back(new Cell("gray", 2, make_pair(300, 300), CELL_COUNTER));
    cells.push_back(new Cell("gray", 2, make_pair(300, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 2, make_pair(300, 700), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(300, 900), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(500, 150), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(500, 350), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(500, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(500, 650), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(500, 850), CELL_COUNTER));
    cells.push_back(new Cell("gray", 5, make_pair(700, 100), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(700, 300), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(700, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(700, 700), CELL_COUNTER));
    cells.push_back(new Cell("gray", 5, make_pair(700, 900), CELL_COUNTER));
    cells.push_back(new Cell("gray", 6, make_pair(950, 50), CELL_COUNTER));
    cells.push_back(new Cell("gray", 5, make_pair(950, 250), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(950, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(950, 750), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(950, 950), CELL_COUNTER));
    cells.push_back(new Cell("gray", 5, make_pair(1200, 100), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(1200, 300), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(1200, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(1200, 700), CELL_COUNTER));
    cells.push_back(new Cell("gray", 5, make_pair(1200, 900), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(1400, 150), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(1400, 350), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(1400, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(1400, 650), CELL_COUNTER));
    cells.push_back(new Cell("gray", 4, make_pair(1400, 850), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(1600, 100), CELL_COUNTER));
    cells.push_back(new Cell("gray", 2, make_pair(1600, 300), CELL_COUNTER));
    cells.push_back(new Cell("gray", 2, make_pair(1600, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 2, make_pair(1600, 700), CELL_COUNTER));
    cells.push_back(new Cell("gray", 3, make_pair(1600, 900), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(1800, 150), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(1800, 350), CELL_COUNTER));
    cells.push_back(new Cell("red", 2, make_pair(1800, 500), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(1800, 650), CELL_COUNTER));
    cells.push_back(new Cell("gray", 1, make_pair(1800, 850), CELL_COUNTER));
    
    AI *greenbot = new AI("green", green_gens);
    AI *redbot = new AI("red", red_gens);

    vector<pair<pair<int, int>, int>> attack;
    vector<pair<pair<int, int>, int>> attack_next;

    vector<vector<bool>> tentacles_ways(CELLS_COUNT, vector<bool>(CELLS_COUNT, false));
    for (int tick = 0; tick < 5000; tick++)
    {
        if (tick % 10 < 2)
        {
            vector<pair<int, int>> green_ans = greenbot->process(cells, tentacles_ways, tick);
            for (auto action : green_ans)
            {
                if (tentacles_ways[action.first][action.second])
                {
                    attack_next.push_back(make_pair(make_pair(action.first, action.second), distance(cells[action.first]->pos, cells[action.second]->pos) / 2));
                    attack_next.push_back(make_pair(make_pair(action.first, action.first), distance(cells[action.first]->pos, cells[action.second]->pos) / 2));
                    tentacles_ways[action.first][action.second] = 0;
                    cells[action.first]->delete_tentacle(cells[action.second]);
                }
                else
                {

                    if (cells[action.first]->add_tentacle(cells[action.second]))
                    {
                        tentacles_ways[action.first][action.second] = 1;
                    }
                }
            }
            vector<pair<int, int>> red_ans = redbot->process(cells, tentacles_ways, tick);
            for (auto action : red_ans)
            {
                if (tentacles_ways[action.first][action.second])
                {
                    attack_next.push_back(make_pair(make_pair(action.first, action.second), distance(cells[action.first]->pos, cells[action.second]->pos) / 2));
                    attack_next.push_back(make_pair(make_pair(action.first, action.first), distance(cells[action.first]->pos, cells[action.second]->pos) / 2));
                    tentacles_ways[action.first][action.second] = 0;
                    cells[action.first]->delete_tentacle(cells[action.second]);
                }
                else
                {

                    if (cells[action.first]->add_tentacle(cells[action.second]))
                    {
                        tentacles_ways[action.first][action.second] = 1;
                    }
                }
            }
        }
        for (auto cell : cells)
            cell->update(attack_next);
        attack = attack_next;
        attack_next.clear();
        for (auto action : attack)
        {
            cells[action.first.second]->attack(cells[action.first.first]->owner, action.second, attack_next);
        }
        attack.clear();
    }
    int green_score = 0;
    int red_score = 0;
    for (auto cell : cells)
    {
        if (cell->owner == "green")
        {
            green_score += 500 + cell->score;
            //cout << cell->score << endl;
        }
        else if (cell->owner == "red")
        {
            red_score += 500 + cell->score;
            //cout << cell->score << endl;
        }
    }
    cout << green_score << ' ' << red_score << ' ' << time(nullptr) - start_time << ' ' << green_i << ' ' << red_i << endl;
    //score_lock.lock();
    score[green_i].first += green_score - red_score / 2;
    score[red_i].first += red_score - green_score / 2;
    //score_lock.unlock();
}

void print(vector<vector<char>> &a)
{
    cout << "[[";
    for (int i = 0; i < a.size() - 1; i++)
    {
        for (int j = 0; j < a[i].size() - 1; j++)
            cout << (int)a[i][j] << ", ";
        cout << (int)a[i][a[i].size() - 1];
        cout << "], [";
    }
    int i = a.size() - 1;
    for (int j = 0; j < a[i].size() - 1; j++)
        cout << (int)a[i][j] << ", ";
    cout << (int)a[i][a[i].size() - 1];
    cout << "]]\n";
}

int main()
{

    freopen("D:/gen.txt", "w", stdout);
    gen32.seed(time(nullptr));
    for (int i = 0; i < SIGNS_CELL; i++)
    {
        for (int j = 0; j < GENES; j++)
        {
            cell_action[i][j] = (char)(gen32() % 255 - 128);
        }
    }
    for (int i = 0; i < SIGNS_TENTACLE; i++)
    {
        for (int j = 0; j < GENES; j++)
        {
            tentacle_action[i][j] = (char)(gen32() % 255 - 128);
        }
    }
    print(cell_action);
    print(tentacle_action);
    vector<Chromosome *> gens;
    for (int i = 0; i < POPULATION; i++)
        gens.push_back(new Chromosome());
    for (int generation = 0; generation < GENERATIONS; generation++)
    {
        cout << generation << endl;
        vector<pair<int, int>> score(POPULATION, make_pair(0, 0));
        for (int i = 0; i < POPULATION; i++)
            score[i].second = i;
        for (int i = 0; i < POPULATION - 1; i++)
        {
            for (int j = i + 1; j < POPULATION; j++)
                game(gens[i], gens[j], i, j, score);
        }
        sort(score.begin(), score.end());
        reverse(score.begin(), score.end());
        for (auto c : score)
            cout << c.first << ' ' << c.second << " ||| ";
        cout << endl;
        for (auto c : gens)
            c->print();
        for (int i = POPULATION - POPULATION / 3; i < POPULATION; i++)
        {
            delete gens[score[i].second];
            gens[score[i].second] = new Chromosome(gens[score[POPULATION - i - 1].second]);
        }
        cout << endl;
    }
}
