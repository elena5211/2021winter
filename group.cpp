#include <algorithm>
#define init_seq 8388608
using namespace std;

class Node{
	public:
		int LPA;
		int trace;
		int next;
		int group;

	Node(int LPA, int trace, int next) {
		this->LPA = LPA;
		this->trace = trace;
		this->next = next;
		this->group = -1;
	}
};

void InitGroup(int** group, int* group_last, int LPA, int line) {
	group[LPA][group_last[LPA]] = line;
    group_last[LPA] ++;
}

bool SortNext(Node Node1, Node Node2) {
    return (Node1.next < Node2.next);
}

bool SortLine(Node Node1, Node Node2) {
	return (Node1.trace < Node2.trace);
}

//10%의 trace를 sort한다
void SortTraceNext(int** group, Node* next_update, int sort_size){
    sort(next_update, next_update + sort_size, SortNext);
}

void SortTraceLine(Node* next_update, int sort_size) {
	sort(next_update, next_update + sort_size, SortLine);
}

void SortValid(int** group, int* next_update, int sort_size) {
	sort(next_update, next_update+sort_size);
}

bool isLast(int** group, int* group_last, int* next_update){
    
}