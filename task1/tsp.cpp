#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <climits>
#include <iomanip>
using namespace std;

int n, K;
vector<vector<int>> d;
vector<int> cost;

void readData(string file)
{
    ifstream in(file);
    vector<int> x, y;
    string line;
    while (getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        for (int i = 0; i < line.size(); i++)
            if (line[i] == ';')
                line[i] = ' ';
        stringstream ss(line);
        int a, b, c;
        ss >> a >> b >> c;
        x.push_back(a);
        y.push_back(b);
        cost.push_back(c);
    }
    n = x.size();
    K = (n + 1) / 2;
    d.assign(n, vector<int>(n, 0));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            double dx = x[i] - x[j];
            double dy = y[i] - y[j];
            d[i][j] = (int)round(sqrt(dx * dx + dy * dy));
        }
}

long long objective(vector<int> &sol)
{
    long long sum = 0;
    int m = sol.size();
    for (int i = 0; i < m; i++)
    {
        sum += d[sol[i]][sol[(i + 1) % m]];
        sum += cost[sol[i]];
    }
    return sum;
}

bool correct(vector<int> &sol)
{
    if (sol.size() != K)
        return false;
    vector<bool> seen(n, false);
    for (int i = 0; i < sol.size(); i++)
    {
        if (sol[i] < 0 || sol[i] >= n || seen[sol[i]])
            return false;
        seen[sol[i]] = true;
    }
    return true;
}

vector<int> randomSolution(mt19937 &gen)
{
    vector<int> p(n);
    iota(p.begin(), p.end(), 0);
    shuffle(p.begin(), p.end(), gen);
    p.resize(K);
    return p;
}

vector<int> nnEnd(int start)
{
    vector<int> path;
    vector<bool> used(n, false);
    path.push_back(start);
    used[start] = true;
    while (path.size() < K)
    {
        int last = path.back();
        int bestNode = -1;
        int bestVal = INT_MAX;
        for (int v = 0; v < n; v++)
        {
            if (used[v])
                continue;
            int val = d[last][v] + cost[v];
            if (val < bestVal)
            {
                bestVal = val;
                bestNode = v;
            }
        }
        path.push_back(bestNode);
        used[bestNode] = true;
    }
    return path;
}

vector<int> nnAny(int start)
{
    vector<int> path;
    vector<bool> used(n, false);
    path.push_back(start);
    used[start] = true;
    while (path.size() < K)
    {
        int m = path.size();
        int bestNode = -1;
        int bestPos = -1;
        int bestVal = INT_MAX;
        for (int v = 0; v < n; v++)
        {
            if (used[v])
                continue;
            int val = d[v][path[0]] + cost[v];
            if (val < bestVal)
            {
                bestVal = val;
                bestNode = v;
                bestPos = 0;
            }
            for (int i = 0; i < m - 1; i++)
            {
                val = d[path[i]][v] + d[v][path[i + 1]] - d[path[i]][path[i + 1]] + cost[v];
                if (val < bestVal)
                {
                    bestVal = val;
                    bestNode = v;
                    bestPos = i + 1;
                }
            }
            val = d[path[m - 1]][v] + cost[v];
            if (val < bestVal)
            {
                bestVal = val;
                bestNode = v;
                bestPos = m;
            }
        }
        path.insert(path.begin() + bestPos, bestNode);
        used[bestNode] = true;
    }
    return path;
}

vector<int> greedyCycle(int start)
{
    vector<int> cycle;
    vector<bool> used(n, false);
    cycle.push_back(start);
    used[start] = true;
    while (cycle.size() < K)
    {
        int m = cycle.size();
        int bestNode = -1;
        int bestPos = -1;
        int bestVal = INT_MAX;
        for (int v = 0; v < n; v++)
        {
            if (used[v])
                continue;
            for (int i = 0; i < m; i++)
            {
                int a = cycle[i];
                int b = cycle[(i + 1) % m];
                int val = d[a][v] + d[v][b] - d[a][b] + cost[v];
                if (val < bestVal)
                {
                    bestVal = val;
                    bestNode = v;
                    bestPos = i + 1;
                }
            }
        }
        cycle.insert(cycle.begin() + bestPos, bestNode);
        used[bestNode] = true;
    }
    return cycle;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        cout << "usage: " << argv[0] << " data.csv name outputFolder" << endl;
        return 1;
    }
    string file = argv[1];
    string name = argv[2];
    string folder = argv[3];

    readData(file);
    cout << "Instance " << name << " (n=" << n << ", K=" << K << ")" << endl;

    string methods[4] = {"random", "nn_end", "nn_any", "greedy_cycle"};
    long long best[4], worst[4];
    double sum[4];
    vector<int> bestSol[4];
    for (int m = 0; m < 4; m++)
    {
        best[m] = LLONG_MAX;
        worst[m] = 0;
        sum[m] = 0;
    }

    mt19937 gen(42);

    for (int run = 0; run < 200; run++)
    {
        vector<int> sol[4];
        sol[0] = randomSolution(gen);
        sol[1] = nnEnd(run % n);
        sol[2] = nnAny(run % n);
        sol[3] = greedyCycle(run % n);
        for (int m = 0; m < 4; m++)
        {
            long long val = objective(sol[m]);
            if (val < best[m])
            {
                best[m] = val;
                bestSol[m] = sol[m];
            }
            if (val > worst[m])
                worst[m] = val;
            sum[m] += val;
        }
    }

    cout << left << setw(16) << "method" << right << setw(10) << "min" << setw(10) << "max" << setw(12) << "avg" << endl;
    for (int m = 0; m < 4; m++)
    {
        cout << left << setw(16) << methods[m] << right << setw(10) << best[m] << setw(10) << worst[m]
             << setw(12) << fixed << setprecision(1) << sum[m] / 200 << endl;
        if (!correct(bestSol[m]))
            cout << "solution " << methods[m] << " is not correct!" << endl;
    }

    ofstream stats(folder + "/" + name + "_stats.csv");
    stats << "method,min,max,avg" << endl;
    for (int m = 0; m < 4; m++)
    {
        stats << methods[m] << "," << best[m] << "," << worst[m] << "," << fixed << setprecision(2) << sum[m] / 200 << endl;
        ofstream out(folder + "/" + name + "_" + methods[m] + ".sol");
        out << best[m] << endl;
        for (int i = 0; i < bestSol[m].size(); i++)
        {
            if (i > 0)
                out << " ";
            out << bestSol[m][i];
        }
        out << endl;
    }
    return 0;
}
