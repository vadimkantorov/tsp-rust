#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdint>
#include <functional>
#include <vector>
#include <bitset>
#include <stack>
#include <queue>
#include <utility>
using namespace std;

const size_t N = 50;
const uint32_t INF = uint32_t(1e+6);
typedef pair < size_t, size_t > Edge;
Edge NullEdge = make_pair(N, N);

struct PartialSolution
{
	enum class ReductionType
	{
		Row,
		Column,
	};

	size_t n = 0;
	uint32_t Cost = INF;
	uint32_t LowerBoundTimesTwo = 0;
	uint32_t Reduced[N][N];
	int8_t Constraints[N][N];
	vector<size_t> Path;

	PartialSolution WithEdge(Edge pivot, uint32_t D[N][N])
	{
		auto i = pivot.first, j = pivot.second;
		
		PartialSolution child = *this;
		child.Cost += D[i][j];
		for (size_t k = 0; k < n; k++)
		{
			child.Constraints[i][k] = child.Constraints[k][j] = -1;
			child.Reduced[i][k] = child.Reduced[k][j] = INF;
		}

		child.Constraints[i][j] = 1;
		child.Constraints[j][i] = -1;

		auto subpathTo = child.TraverseSubPath(i, 1);
		auto subpathFrom = child.TraverseSubPath(i, N);
		if (subpathTo.size() + subpathFrom.size() - 1 != n)
		{
			child.Constraints[subpathTo.back()][subpathFrom.back()] = -1;
			child.Reduced[subpathTo.back()][subpathFrom.back()] = INF;
		}

		child.Reduce();
		return child;
	}

	PartialSolution WithoutEdge(Edge pivot, uint32_t D[N][N])
	{
		auto i = pivot.first, j = pivot.second;
		
		PartialSolution child = *this;
		child.Constraints[i][j] = -1;
		child.Reduced[i][j] = INF;
		child.Reduce(ReductionType::Row, i);
		child.Reduce(ReductionType::Column, j);

		return child;
	}

	bool operator>(const PartialSolution& other) const
	{
		return LowerBoundTimesTwo > other.LowerBoundTimesTwo;
	}

	Edge ChoosePivotEdge()
	{
		auto minStride = [&](size_t k, size_t kStride) {uint32_t m = INF; for (size_t i = 0; i < n; i++) m = min(m, *(&Reduced[0][0] + IK(i, k, kStride))); return m;  };
		auto rowMin = [&](size_t k) {return minStride(k, N); };
		auto columnMin = [&](size_t k) {return minStride(k, 1); };
		
		uint32_t bestIncrease = 0;
		Edge bestPivot = NullEdge;
		for (size_t i = 0; i < n; i++)
		{
			for (size_t j = 0; j < n; j++)
			{
				if (Constraints[i][j] == 0 && Reduced[i][j] == 0)
				{
					Reduced[i][j] = INF;
					auto increase = rowMin(i) + columnMin(j);
					if (increase > bestIncrease)
					{
						bestIncrease = increase;
						bestPivot = make_pair(i, j);
					}
					Reduced[i][j] = 0;
				}
			}
		}

		return bestPivot;
	}

	void Print()
	{
		printf("cost: %d [0", Cost);
		for (size_t i = 1; i < n; i++)
			printf("->%d", Path[i]);
		puts("]");
	}

	vector<size_t> TraverseSubPath(size_t cur, int stride)
	{
		bitset<N> visited;
		visited.set(cur);
		vector<size_t> subpath{ cur };
		bool ok = true;
		for (size_t k = 0; ok && k < n; k++)
		{
			ok = true;
			size_t next = N;
			for (size_t i = 0; ok && i < n; i++)
			{
				if (*(&Constraints[0][0] + IK(cur, i, stride)) == 1)
				{
					if (visited[i] || next != N)
					{
						ok = false;
						break;
					}

					next = i;
				}
			}

			if (next == N)
				break;

			subpath.push_back(next);
			visited.set(next);
			cur = next;
		}
		return subpath;
	}

	bool IsComplete()
	{
		Path = TraverseSubPath(0, 1);
		//return Path.back() == n - 1 && Path.size() == n;
		return Path.size() == n + 1;
	}

	void Reduce()
	{
		for (size_t i = 0; i < n; i++)
			Reduce(ReductionType::Row, i);

		for (size_t j = 0; j < n; j++)
			Reduce(ReductionType::Column, j);
	}

	void Reduce(PartialSolution::ReductionType reductionType, size_t i)
	{
		auto kStride = reductionType == ReductionType::Row ? 1 : N;
		
		uint32_t m = INF;
		for (size_t k = 0; k < n; k++)
			if (*(&Constraints[0][0] + IK(i, k, kStride)) != -1)
				m = min(m, *(&Reduced[0][0] + IK(i, k, kStride)));
		
		if (m != INF)
		{
			for (size_t k = 0; k < n; k++)
				*(&Reduced[0][0] + IK(i, k, kStride)) -= m;
			LowerBoundTimesTwo += m;
		}
	}

	int IK(size_t i, size_t k, size_t kStride)
	{
		return (N + 1 - kStride)*i + kStride*k;
	}

	PartialSolution(size_t n, uint32_t D[N][N]) : n(n)
	{
		memcpy(Reduced, D, sizeof(Reduced[0][0]) * N * N);
		memset(Constraints, 0, sizeof(Constraints[0][0]) * N * N);
		for (size_t i = 0; i < n; i++)
		{
			Reduced[i][i] = INF;
			Constraints[i][i] = -1;
		}
		Cost = 0;

		Reduce();
	}

	PartialSolution()
	{

	}
};

void branch_and_bound(size_t n, uint32_t D[N][N])
{
	PartialSolution bestCompleteSolution;
	//PartialSolution root = PartialSolution(n, D).WithEdge(make_pair(n - 1, 0), D);
	PartialSolution root = PartialSolution(n, D);

	priority_queue<PartialSolution, vector<PartialSolution>, greater<PartialSolution> > Q;
	//stack<PartialSolution> Q;
	Q.push(root);

	while (!Q.empty())
	{
		auto currentSolution = Q.top();
		Q.pop();

		if (currentSolution.IsComplete())
		{
			if (currentSolution.Cost < bestCompleteSolution.Cost)
			{
				bestCompleteSolution = currentSolution;
				bestCompleteSolution.Print();
			}
		}
		else if (currentSolution.LowerBoundTimesTwo < 2 * bestCompleteSolution.Cost)
		{
			auto pivot = currentSolution.ChoosePivotEdge();
			if (pivot != NullEdge)
			{
				auto withPivot = currentSolution.WithEdge(pivot, D);
				auto withoutPivot = currentSolution.WithoutEdge(pivot, D);

				if (withPivot.LowerBoundTimesTwo < 2 * bestCompleteSolution.Cost)
					Q.push(withPivot);

				if (withoutPivot.LowerBoundTimesTwo < 2 * bestCompleteSolution.Cost)
					Q.push(withoutPivot);
			}
		}
	}
}

int main()
{
	freopen("tests/asanov/input.txt", "r", stdin);

	size_t n;
	uint32_t D[N][N];

	scanf("%u", &n);
	for (size_t i = 0; i < n; i++)
		for (size_t j = 0; j < n; j++)
			scanf("%u", &D[i][j]);

	branch_and_bound(n, D);
}