//부처의 마음으로 다시 시작

#include <queue>
#include <iostream>

#define LBA_RANGE 32
#define OPSIZE 0.1

#define PAGE_SIZE 4 //KB
#define BLOCK_SIZE 2 //MB

#define PPB 512

#define TOTAL_PHY_BLOCK_NUMBER 18023
#define TOTAL_PHY_PAGE_NUMBER 18023 * PPB
//8388608
#define TOTAL_LOG_BLOCK_NUMBER LBA_RANGE / BLOCK_SIZE * 1024
#define TOTAL_LOG_PAGE_NUMBER TOTAL_LOG_BLOCK_NUMBER * PPB

#define VALID 1
#define ERASED 0
#define INVALID -1

#define GC_BOUND 3

using namespace std;

int migrate = 0;

class page {
public:
    int status;
    int OOB; //LOG PAGE 저장
};

class block {
public:
    bool used_flag; //다 쓴 block 판별
    bool cleaning_flag; //쓰고있는 block 판별 victim 안되도록
    bool writeonce_flag; //이 블록은 write once가 들어가는가?
    int invalid_count; //invalid가 몇개일까용?
    int first_free; //first free page는 어디일까용?
    int group_free;
    page* pages;

    block() {
        used_flag = false;
        cleaning_flag = false;
        writeonce_flag = false;
        invalid_count = 0;
        first_free = 0;
        group_free = 0;
        pages = new page[PPB];
    }
};

class SSD {
public:
    block* blocks;
    int mig_block;
    int V_block;
    int iV_block;
    int* my_LPA_table;
    int* my_dead_ratio_table;
    queue<int> free_b_que;

    SSD() {
        blocks = new block[TOTAL_PHY_BLOCK_NUMBER];
        for(int i = 0; i < TOTAL_PHY_BLOCK_NUMBER; i++) {
            free_b_que.push(i);
        }
        my_LPA_table = new int[TOTAL_LOG_PAGE_NUMBER];
        for(int i = 0; i <TOTAL_LOG_PAGE_NUMBER; i++) {
            my_LPA_table[i] = -1;
        }
        my_dead_ratio_table = new int[TOTAL_PHY_BLOCK_NUMBER];
        for(int i = 0; i <TOTAL_PHY_BLOCK_NUMBER; i++) {
            my_dead_ratio_table[i] = 0;
        }

        mig_block = free_b_que.front();
        free_b_que.pop();
        V_block = free_b_que.front();
        free_b_que.pop();
        blocks[V_block].writeonce_flag = true;
        blocks[V_block].cleaning_flag = false;
        iV_block = free_b_que.front();
        blocks[iV_block].cleaning_flag = false;
        free_b_que.pop();
    }
    void GC();
    void Write(int LPA, int block, bool user);
    int SelectVictim();
    void Erase(int block);
};

void SSD::GC(){
    //printf("GC");
    int victim = SelectVictim();
    Erase(victim);
}

int SSD::SelectVictim(){
    int victim = -1;
    int dead_ratio = 0;
    for(int i = 0; i < TOTAL_PHY_BLOCK_NUMBER; i++) {
        if((blocks[i].cleaning_flag == true)) {
            if(my_dead_ratio_table[i] >= dead_ratio) {
                //printf("dead ratio : %d, i : %d, old victim : %d", my_dead_ratio_table[i], i, victim);
                dead_ratio = my_dead_ratio_table[i];
                victim = i;
            }
        }
    }
    //printf("victim %d \n", victim);
    return victim;
}

void SSD::Write(int LPA, int block, bool user) {
    int old_PPA = my_LPA_table[LPA];
    if(old_PPA >= 0) {
        blocks[old_PPA/PPB].pages[old_PPA%PPB].status = INVALID;
        blocks[old_PPA/PPB].invalid_count++;
        my_dead_ratio_table[old_PPA/PPB]++;
        //printf("my_dead_ratio talbe[%d]: %d", old_PPA/PPB, my_dead_ratio_table[old_PPA/PPB]);
        //cout<<"writeonce?"<<blocks[old_PPA/PPB].writeonce_flag<<endl;
        if(my_dead_ratio_table[old_PPA/PPB] == PPB) {
            //reset
            blocks[old_PPA/PPB].cleaning_flag = false;
            blocks[old_PPA/PPB].first_free = 0;
            blocks[old_PPA/PPB].group_free = 0;
            blocks[old_PPA/PPB].invalid_count = 0;
            blocks[old_PPA/PPB].used_flag = false;
            blocks[old_PPA/PPB].writeonce_flag = false;
            my_dead_ratio_table[old_PPA/PPB] = 0;
            free_b_que.push(old_PPA/PPB);
            //printf("push %d, user? %d\n", old_PPA/PPB, user);
            //printf("free size: %d\n\n", free_b_que.size());
        }
    }

    blocks[block].pages[blocks[block].first_free].status = VALID;
    blocks[block].pages[blocks[block].first_free].OOB = LPA;
    my_LPA_table[LPA] = block*PPB + blocks[block].first_free;
    blocks[block].first_free++;

    if(blocks[block].first_free == PPB) {
        blocks[block].used_flag = true;
        blocks[block].cleaning_flag = true;
        if(!user) {
            mig_block = free_b_que.front();
            free_b_que.pop();
            //printf("mig_block pop %d \n", V_block);
            //printf("free size: %d\n\n", free_b_que.size());
            blocks[mig_block].cleaning_flag = false;
        }
        //printf("mig pop %d\n", mig_block);
    }
}

void SSD::Erase(int block) {
    for (int i = 0; i < PPB; i++) {
        if(blocks[block].pages[i].status == VALID) {
            migrate++;
            //printf("erase mig_block %d\n", block);
            Write(blocks[block].pages[i].OOB, mig_block, false);
        }
        else {
            continue;
        }
    }
    return;
}