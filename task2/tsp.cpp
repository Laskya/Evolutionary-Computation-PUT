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

// ---------------------------------------------------------------- task 1

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

// ---------------------------------------------------------------- task 2
// Regret heuristics. For every free node v we look at all insertion positions
// and keep the best increase of the objective (best) and the second best
// one (second) - taken from a different position. regret = second - best.
// If v has only one possible position the regret is 0.
//
// wr = 1, wc = 0  -> pure 2-regret (ties are broken by the smaller best)
// wr, wc > 0      -> weighted sum  wr * regret - wc * best (we maximise it)

struct Choice
{
    int node;
    int pos;
    int best;
    int regret;
};

// Is candidate c better than the current best cur?  wr/wc as above.
bool better(const Choice &c, const Choice &cur, double wr, double wc)
{
    if (cur.node == -1)
        return true;
    if (wc == 0.0)
    {
        if (c.regret != cur.regret)
            return c.regret > cur.regret;
        return c.best < cur.best; // tie-break on the best change
    }
    double sc = wr * c.regret - wc * c.best;
    double sb = wr * cur.regret - wc * cur.best;
    return sc > sb; // strict: first found wins a tie
}

// positions: for the open path 0..m (insert index), for the cycle 1..m
vector<int> nnAnyRegret(int start, double wr, double wc)
{
    vector<int> path;
    vector<bool> used(n, false);
    path.push_back(start);
    used[start] = true;
    while (path.size() < K)
    {
        int m = path.size();
        Choice cur = {-1, -1, 0, 0};
        for (int v = 0; v < n; v++)
        {
            if (used[v])
                continue;
            int b1 = INT_MAX, b2 = INT_MAX, p1 = -1;
            for (int pos = 0; pos <= m; pos++)
            {
                int val;
                if (pos == 0)
                    val = d[v][path[0]] + cost[v];
                else if (pos == m)
                    val = d[path[m - 1]][v] + cost[v];
                else
                    val = d[path[pos - 1]][v] + d[v][path[pos]] - d[path[pos - 1]][path[pos]] + cost[v];
                if (val < b1)
                {
                    b2 = b1;
                    b1 = val;
                    p1 = pos;
                }
                else if (val < b2)
                    b2 = val;
            }
            Choice c = {v, p1, b1, b2 - b1};
            if (better(c, cur, wr, wc))
                cur = c;
        }
        path.insert(path.begin() + cur.pos, cur.node);
        used[cur.node] = true;
    }
    return path;
}

vector<int> greedyCycleRegret(int start, double wr, double wc)
{
    vector<int> cycle;
    vector<bool> used(n, false);
    cycle.push_back(start);
    used[start] = true;
    while (cycle.size() < K)
    {
        int m = cycle.size();
        Choice cur = {-1, -1, 0, 0};
        for (int v = 0; v < n; v++)
        {
            if (used[v])
                continue;
            int b1 = INT_MAX, b2 = INT_MAX, p1 = -1;
            for (int i = 0; i < m; i++)
            {
                int a = cycle[i];
                int b = cycle[(i + 1) % m];
                int val = d[a][v] + d[v][b] - d[a][b] + cost[v];
                if (val < b1)
                {
                    b2 = b1;
                    b1 = val;
                    p1 = i + 1;
                }
                else if (val < b2)
                    b2 = val;
            }
            int regret = (b2 == INT_MAX) ? 0 : b2 - b1; // one edge only
            Choice c = {v, p1, b1, regret};
            if (better(c, cur, wr, wc))
                cur = c;
        }
        cycle.insert(cycle.begin() + cur.pos, cur.node);
        used[cur.node] = true;
    }
    return cycle;
}

const int M = 8;
string methods[M] = {"random", "nn_end", "nn_any", "greedy_cycle",
                     "nn_any_regret", "nn_any_weighted", "greedy_cycle_regret", "greedy_cycle_weighted"};

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

    const double WR = 0.5, WC = 0.5; // default: equal weights

    long long best[M], worst[M];
    double sum[M];
    vector<int> bestSol[M];
    for (int m = 0; m < M; m++)
    {
        best[m] = LLONG_MAX;
        worst[m] = 0;
        sum[m] = 0;
    }

    mt19937 gen(42);

    for (int run = 0; run < 200; run++)
    {
        int s = run % n;
        vector<int> sol[M];
        sol[0] = randomSolution(gen);
        sol[1] = nnEnd(s);
        sol[2] = nnAny(s);
        sol[3] = greedyCycle(s);
        sol[4] = nnAnyRegret(s, 1.0, 0.0);
        sol[5] = nnAnyRegret(s, WR, WC);
        sol[6] = greedyCycleRegret(s, 1.0, 0.0);
        sol[7] = greedyCycleRegret(s, WR, WC);
        for (int m = 0; m < M; m++)
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

    cout << left << setw(24) << "method" << right << setw(10) << "min" << setw(10) << "max" << setw(12) << "avg" << endl;
    for (int m = 0; m < M; m++)
    {
        cout << left << setw(24) << methods[m] << right << setw(10) << best[m] << setw(10) << worst[m]
             << setw(12) << fixed << setprecision(1) << sum[m] / 200 << endl;
        if (!correct(bestSol[m]))
            cout << "solution " << methods[m] << " is not correct!" << endl;
    }

    ofstream stats(folder + "/" + name + "_stats.csv");
    stats << "method,min,max,avg" << endl;
    for (int m = 0; m < M; m++)
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

    // experiment with the weights: wr = w, wc = 1 - w
    ofstream wf(folder + "/" + name + "_weights.csv");
    wf << "method,w_regret,w_cost,min,max,avg" << endl;
    cout << endl << "weight sweep (w_regret / w_cost)" << endl;
    for (int a = 0; a < 2; a++)
    {
        string mname = a == 0 ? "nn_any_weighted" : "greedy_cycle_weighted";
        for (int w = 0; w <= 10; w++)
        {
            double wr = w / 10.0, wc = 1.0 - wr;
            long long mn = LLONG_MAX, mx = 0;
            double sm = 0;
            for (int run = 0; run < 200; run++)
            {
                vector<int> s = a == 0 ? nnAnyRegret(run % n, wr, wc) : greedyCycleRegret(run % n, wr, wc);
                if (!correct(s))
                    cout << "solution is not correct!" << endl;
                long long val = objective(s);
                mn = min(mn, val);
                mx = max(mx, val);
                sm += val;
            }
            wf << mname << "," << fixed << setprecision(1) << wr << "," << wc << "," << mn << "," << mx << ","
               << setprecision(2) << sm / 200 << endl;
            cout << left << setw(24) << mname << fixed << setprecision(1) << wr << " / " << wc
                 << right << setw(10) << mn << setw(10) << mx << setw(12) << sm / 200 << endl;
        }
    }
    return 0;
}
