#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>
#include <random>
#include <ctime>
#include <mutex>

using namespace std;

const int POPULATION = 6;
const int GENERATIONS = 100;
const int CELLS_GENES = 26;
const int TENTACLE_GENES = 24;
const int CONTROLLER_GENES = 18;
const int GENES = CELLS_GENES + TENTACLE_GENES + CONTROLLER_GENES;
const int CHANCE_MUTATION = 20;
const int GEN_MUTATION = 50;
const int SIGNS_CELL = 10;
const int SIGNS_TENTACLE = 10;

mutex score_lock;
mt19937 gen32;
vector<vector<char>> cell_action(10, vector<char>(GENES));
vector<vector<char>> tentacle_action(10, vector<char>(GENES));
const vector<int> ScoreToLvl ={0, 10, 25, 100, 250, 500, 990, 1000};
const vector<int> ScoreToLvlGrayCell = {0, 5, 15, 50, 100, 200, 400, 410};

int lvl_to_score(int score)
{
    return upper_bound(ScoreToLvl.begin(), ScoreToLvl.end(), score) - ScoreToLvl.begin() - 1;
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
    int owner;
    int score;
    int lvl;
    int id;
    int counter;
    int maxLvl;
    pair<int, int> pos;
    vector<Cell *> cells;
    int tentaclesMax;
    const int CounterController = 80;

    Cell(int owner, int lvl, pair<int, int> pos, int &CELL_COUNTER)
    {
        this->owner = owner;
        this->lvl = lvl;
        this->pos = pos;
        this->score = ScoreToLvl[lvl];
        maxLvl = lvl + 2;
        cells = {};
        counter = 0;
        id = CELL_COUNTER++;
        tentaclesMax = lvl / 3 + 1;

    }

    void attack(int attacker, int damage, vector<pair<pair<int, int>, int>> &attack_next)
    {
        if (owner == attacker)
        {
            if (score >= ScoreToLvl[maxLvl])
            {
                counter += damage * CounterController / max(1, (int)cells.size());
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
            if (owner != 0)
                lvl = lvl_to_score(score);
            tentaclesMax = lvl / 3 + 1;
        }
        else
        {
            if (owner == 0)
                score = ScoreToLvl[lvl];
            else
                score = damage - score;
            owner = attacker;
            lvl = lvl_to_score(score);
            tentaclesMax = lvl / 3 + 1;

        }
    }
    void update(vector<pair<pair<int, int>, int>> &attack_next)
    {
        if (owner == 0)
        {
            return;
        }
        counter += lvl + 1;
        if (counter >= CounterController)
        {
            counter = 0;
            if (score < ScoreToLvl[maxLvl])
            {
                score++;
            }
            else
            {
                counter += maxLvl * (CounterController / 10);
                score = ScoreToLvl[maxLvl];
            }
            lvl = lvl_to_score(score);
            for (auto cell : cells)
            {
                attack_next.push_back(make_pair(make_pair(id, cell->id), 1)); //TODO delay attack
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
    Chromosome(Chromosome *parent_male, Chromosome *parent_female)
    {
        for (int i = 0; i < GENES; i++)
        {
            if (gen32() % 100 < 50)
                chromosome.push_back((char)(parent_male->get_gen(i) + (gen32() % GEN_MUTATION) - GEN_MUTATION / 2));
            else
                chromosome.push_back((char)(parent_female->get_gen(i) + (gen32() % GEN_MUTATION) - GEN_MUTATION / 2));
        }
    }

    void mutate()
    {
        for (int i = 0; i < GENES; i++)
        {
            if (gen32() % 100 < CHANCE_MUTATION)
            {
                chromosome[i] += (gen32() % GEN_MUTATION) - GEN_MUTATION / 2;
            }
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
    int owner;

    AI(int owner, Chromosome *gens)
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
                if (cell_end->owner == 0)
                    is_gray = 1;
                else if (cell_end->owner != cell_begin->owner)
                    is_enemy = 1;
                else
                    is_friend = 1;
                int way = distance(cell_begin->pos, cell_end->pos);
                if (!tentacles_ways[cell_begin->id][cell_end->id])
                {
                    if (way > cell_begin->score)
                        continue;
                    vector<int> signs_cell = {way / 20, 15000 / way, 100 * is_enemy, 100 * is_gray, 100 * is_friend,
                                              cell_end->score / 10, cell_begin->score / 10, (cell_end->score - cell_begin->score) / 10,
                                              tick / 50, (cell_begin->lvl - (int)cell_begin->cells.size()) * 100 / 7};
                    int battle = 0;
                    for (int i = 0; i < SIGNS_CELL; i++)
                    {
                        for (int j = 0; j < GENES / 2; j++)
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
                        for (int j = GENES / 2; j < GENES; j++)
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

class FieldProperties
{
    public:
    vector<int> CellOfOwners;
    vector<vector<int>> Lvl;
    pair<int, int> Pos;

    FieldProperties(pair<int, int> pos, vector<vector<int>> lvl, vector<int> cellOfOwners)
    {
        Pos = pos;
        Lvl = lvl;
        CellOfOwners = cellOfOwners;
    }
};

bool IsCellDistant(pair<int, int> cellPos, vector<pair<int, int>> posList){
    for(auto pos : posList){
        if (distance(cellPos, pos) < 10){
            return false;
        }
    }
    return true;
}

FieldProperties MakeField(){
    vector<int> maxCellsWithLvl = {0, 2, 6, 6, 5, 2, 1};
    pair<int, int> pos;
    vector<int> cellOfOwners(4);
    vector <vector<int>> lvl(6);
    vector <pair<int, int>> posList;
    while (true)
    {
        int cells = 0;
        lvl.clear();
        vector<int> cellsWithLvl(7);
        for (int i = 1; i < maxCellsWithLvl.size(); i++)
        {
            cellsWithLvl[i] = gen32() % maxCellsWithLvl[i] + 1;;
            for (int j = cells; j < cells + cellsWithLvl[i]; j++) lvl[i].push_back(j);
            cells += cellsWithLvl[i];
        }
        if (cells < 10) continue;
        posList.clear();
        int fails = 0;
        for (int i = 0; i < cells; i++)
        {
            pair<int, int> newPos = {(gen32() % 1900) + 100, (gen32() % 900) + 100};
            if (IsCellDistant(newPos, posList))
            {
                posList.push_back(newPos);
                fails = 0;
            }
            else
            {
                fails++;
                i--;
                if (fails > 100) break;
            }
        }

        if (fails > 100) continue;
        cellOfOwners[0] = lvl[1][0];
        cellOfOwners[1] = cellOfOwners[0] + cells;
        cellOfOwners[2] = cellOfOwners[1] + cells;
        cellOfOwners[3] = cellOfOwners[2] + cells;

        for (var i = 0; i < cells; i++) posList.Add(new Vector3(-posList[i].x, -posList[i].y));
        for (var i = 0; i < cells; i++) posList.Add(new Vector3(posList[i].x, -posList[i].y));
        for (var i = 0; i < cells; i++) posList.Add(new Vector3(-posList[i].x, posList[i].y));

        for (var i = 0; i < 6; i++)
        {
            var newLvlList = new List<int>();
            for (var j = 0; j < 4; j++)
                foreach (var id in lvl[i])
            newLvlList.Add(id + j * cells);

            lvl[i] = newLvlList.ToArray();
        }

        pos = posList.ToArray();
        break;
    }

    return FieldProperties(pos, lvl, cellOfOwners);
}

void print(int a, int b, int c, int d, int e)
{
    string sa = to_string(a), sb = to_string(b), sc = to_string(c), sd = to_string(d), se = to_string(e);
    string tmp;
    for (int i = 0; i < 5 - sa.size(); i++)
        tmp += ' ';
    sa += tmp;
    tmp = "";
    for (int i = 0; i < 5 - sb.size(); i++)
        tmp += ' ';
    sb += tmp;
    tmp = "";
    for (int i = 0; i < 5 - sc.size(); i++)
        tmp += ' ';
    sc += tmp;
    tmp = "";
    for (int i = 0; i < 5 - sd.size(); i++)
        tmp += ' ';
    sd += tmp;
    tmp = "";
    for (int i = 0; i < 5 - se.size(); i++)
        tmp += ' ';
    se += tmp;
    cout << sa << ' ' << sb << ' ' << sc << ' ' << sd << ' ' << se << '\n';
}

void game(Chromosome *green_gens, Chromosome *red_gens, int green_i, int red_i, vector<pair<int, int>> *score)
{
    int start_time = time(nullptr);
    int CELL_COUNTER = 0;
    vector<Cell *> cells;

    cells.push_back(new Cell(0, 1, make_pair(100, 150), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(100, 350), CELL_COUNTER));
    cells.push_back(new Cell(1, 2, make_pair(100, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(100, 650), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(100, 850), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(300, 100), CELL_COUNTER));
    cells.push_back(new Cell(0, 2, make_pair(300, 300), CELL_COUNTER));
    cells.push_back(new Cell(0, 2, make_pair(300, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 2, make_pair(300, 700), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(300, 900), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(500, 150), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(500, 350), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(500, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(500, 650), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(500, 850), CELL_COUNTER));
    cells.push_back(new Cell(0, 5, make_pair(700, 100), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(700, 300), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(700, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(700, 700), CELL_COUNTER));
    cells.push_back(new Cell(0, 5, make_pair(700, 900), CELL_COUNTER));
    cells.push_back(new Cell(0, 6, make_pair(950, 50), CELL_COUNTER));
    cells.push_back(new Cell(0, 5, make_pair(950, 250), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(950, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(950, 750), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(950, 950), CELL_COUNTER));
    cells.push_back(new Cell(0, 5, make_pair(1200, 100), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(1200, 300), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(1200, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(1200, 700), CELL_COUNTER));
    cells.push_back(new Cell(0, 5, make_pair(1200, 900), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(1400, 150), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(1400, 350), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(1400, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(1400, 650), CELL_COUNTER));
    cells.push_back(new Cell(0, 4, make_pair(1400, 850), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(1600, 100), CELL_COUNTER));
    cells.push_back(new Cell(0, 2, make_pair(1600, 300), CELL_COUNTER));
    cells.push_back(new Cell(0, 2, make_pair(1600, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 2, make_pair(1600, 700), CELL_COUNTER));
    cells.push_back(new Cell(0, 3, make_pair(1600, 900), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(1800, 150), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(1800, 350), CELL_COUNTER));
    cells.push_back(new Cell(2, 2, make_pair(1800, 500), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(1800, 650), CELL_COUNTER));
    cells.push_back(new Cell(0, 1, make_pair(1800, 850), CELL_COUNTER));

    AI *greenbot = new AI(1, green_gens);
    AI *redbot = new AI(2, red_gens);

    vector<pair<pair<int, int>, int>> attack;
    vector<pair<pair<int, int>, int>> attack_next;

    vector<vector<bool>> tentacles_ways(cells.size(), vector<bool>(cells.size(), false));
    for (int tick = 0; tick < 5000; tick++)
    {
        if (tick % 15 < 1)
        {

            vector<pair<int, int>> actions = greenbot->process(cells, tentacles_ways, tick);
            for (auto c : redbot->process(cells, tentacles_ways, tick))
                actions.push_back(c);
            for (auto action : actions)
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
        if (cell->owner == 1)
        {
            green_score += 500 + cell->score;
            //cout << cell->score << endl;
        }
        else if (cell->owner == 2)
        {
            red_score += 500 + cell->score;
            //cout << cell->score << endl;
        }
    }
    //score_lock.lock();
    print(green_score, red_score, time(nullptr) - start_time, green_i, red_i);
    score->at(green_i).first += green_score - red_score / 2;
    score->at(red_i).first += red_score - green_score / 2;
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

    //freopen("gen.txt", "w", stdout);
    gen32.seed(time(nullptr));
    for (int i = 0; i < SIGNS_CELL; i++)
    {
        for (int j = 0; j < GENES / 2; j++)
        {
            cell_action[i][j] = (char)(gen32() % 255 - 128);
        }
    }
    for (int i = 0; i < SIGNS_TENTACLE; i++)
    {
        for (int j = 0; j < GENES / 2; j++)
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
        for (int i = 0; i < POPULATION; i++)
        {
            for (int j = 0; j < POPULATION; j++)
                game(gens[i], gens[j], i, j, &score);
        }
        sort(score.begin(), score.end());
        reverse(score.begin(), score.end());
        for (auto c : score)
            cout << c.first << ' ' << c.second << " ||| ";
        cout << endl;
        for (auto c : gens)
            c->print();
        for (int i = 0; i < POPULATION; i++)
        {
            if (gen32() % 100 < CHANCE_MUTATION)
                gens[i]->mutate();
        }
        for (int i = POPULATION - POPULATION / 3; i < POPULATION; i++)
        {
            Chromosome *child = new Chromosome(gens[gen32() % POPULATION], gens[gen32() % POPULATION]);
            delete gens[score[i].second];
            gens[score[i].second] = child;
        }
        cout << endl;
    }
}
