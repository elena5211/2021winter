//부처의 마음으로 처음부터 시작

//#define TOTAL_LINE 41943040
#define TOTAL_LINE 276824064

#define WriteOnce 0
#define WS 1000
#define GS 1000
#include <cstdio>
#include <functional>
#include "FTL.cpp"
#include <string.h>
#include "group.cpp"

using namespace std;


int main(int argc, char* argv[]) {
    SSD SSD;
    FILE* readFile;
    int** group;
    group = new int*[TOTAL_LOG_PAGE_NUMBER];
    int* group_count;
    group_count = new int[TOTAL_LOG_PAGE_NUMBER]{0};
    int* group_last;
    group_last = new int[TOTAL_LOG_PAGE_NUMBER]{0};
    int now_line_number = 1;
    //int WS = atoi(argv[1]);
    //int GS = atoi(argv[2]);
    const char* file_address = "/home/youngmin/OracleApproach/1TB_zip08";
    printf("file_address: %s\n", file_address);
	readFile = fopen(file_address, "rb");

    if (readFile == NULL) { printf("nononono"); return 0; }
	printf("file read end\n");
	//outFile = fopen("cat_ftl_waf_1TB_11.csv", "w");
	printf("size: %d\n", SSD.free_b_que.size());
    
    //size check
	while (!feof(readFile)) {
        
        char buffer[50];
        char* workptr;
        fgets(buffer, sizeof(buffer), readFile);
        workptr = buffer + 3;
		workptr = strtok(workptr, " ");
		int LPA = atoi(workptr) / 8;
		group_count[LPA] += 1;
	}
    //group allocation
    for (int i = 0 ; i < TOTAL_LOG_PAGE_NUMBER; i++) {
		group[i] = new int[group_count[i]];
	}
    rewind(readFile);
    int trace_now = 1;

    //grouping
    while (trace_now < TOTAL_LINE) {
        char buffer[50];
        char* workptr;
        fgets(buffer, sizeof(buffer), readFile);
        workptr = buffer + 3;
		workptr = strtok(workptr, " ");
		int LPA = atoi(workptr) / 8;
        InitGroup(group, group_last, LPA, trace_now);
        trace_now ++;
    }
    printf("group 끝이야 \n");
    rewind(readFile);
    trace_now = 1;

    delete[] group_last;
    group_last = new int[TOTAL_LOG_PAGE_NUMBER]{0};

    int write_size = WS*PPB;

    while(trace_now < TOTAL_LINE) {

        while(SSD.free_b_que.size() < GS+3) {
            SSD.GC();
        }
        printf("free b que size %d \n", SSD.free_b_que.size());
        if(TOTAL_LINE - trace_now < write_size) {
            write_size = TOTAL_LINE - trace_now + 1;
            printf("write size %d\n", write_size);
        }
        Node* sequence = (Node*)malloc(sizeof(Node)*write_size);
        for (int i = 0; i < write_size; i++) {
            char buffer[50];
            char* workptr;
            fgets(buffer, sizeof(buffer), readFile);
            workptr = buffer + 3;
		    workptr = strtok(workptr, " ");
		    int LPA = atoi(workptr) / 8;
            if(group_count[LPA] - group_last[LPA] == 1){
                //printf("once!! \n");
                sequence[i] = Node(LPA, trace_now, WriteOnce);
                group_last[LPA]++;
            }
            else {
                sequence[i] = Node(LPA, trace_now, group[LPA][group_last[LPA]+1]);
                group_last[LPA] ++;
            }
            trace_now++;
            //printf("trace now %d\n", trace_now);
        }
        SortTraceNext(group, sequence, write_size);
        int start;
        for (int i = 0 ; i< write_size; i++) {
            if(sequence[i].next > WriteOnce) {
                start = i;
                break;
            }
        }
        int end;
        for (int i = start; i < write_size; i++) {
            if(sequence[i].next - sequence[start].next > write_size) {
                end = i;
                break;
            }
        }

        printf("return block %f\n", float(end-start)/PPB);
        printf("write once ratio %f\n", float(start)/PPB);
        for(int i = 0; i<write_size; i++) {
            int now_valid = 512 - SSD.blocks[SSD.V_block].group_free;
            int now_invalid = 512 - SSD.blocks[SSD.iV_block].group_free;

            if(sequence[i].next == WriteOnce) {
                if(now_valid > 0) {
                    sequence[i].group = SSD.V_block;
                    SSD.blocks[SSD.V_block].group_free ++;
                }
                else {
                    SSD.V_block = SSD.free_b_que.front();
                    SSD.free_b_que.pop();
                    //printf("V_block pop %d \n", SSD.V_block);
                    //printf("free size: %d\n\n", SSD.free_b_que.size());
                    sequence[i].group = SSD.V_block;
                    SSD.blocks[SSD.V_block].writeonce_flag = true;
                    SSD.blocks[SSD.V_block].cleaning_flag = false;
                    SSD.blocks[SSD.V_block].group_free ++;
                }
            }
            else {
                if(now_invalid > 0) {
                    sequence[i].group = SSD.iV_block;
                    SSD.blocks[SSD.iV_block].group_free ++;
                }
                else {
                    SSD.iV_block = SSD.free_b_que.front();
                    SSD.free_b_que.pop();
                    //rintf("iV_block pop %d \n", SSD.iV_block);
                    //printf("free size: %d\n\n", SSD.free_b_que.size());
                    sequence[i].group = SSD.iV_block;
                    SSD.blocks[SSD.iV_block].group_free ++;
                    SSD.blocks[SSD.iV_block].cleaning_flag = false;
                }
            }
        }
        SortTraceLine(sequence, write_size);
        for(int i = 0; i < write_size; i++) {
            //printf(":%d: sorted trace LPA %d trace line %d next %d group %d\n", i, sequence[i].LPA, sequence[i].trace, sequence[i].next, sequence[i].group);
        }

        //Write
        for(int i = 0; i < write_size; i++) {
            //printf(":%d: sorted trace LPA %d trace line %d next %d group %d page loc%d \n", i, sequence[i].LPA, sequence[i].trace, sequence[i].next, sequence[i].group, SSD.blocks[sequence[i].group].first_free);
            SSD.Write(sequence[i].LPA, sequence[i].group, true);
            now_line_number++;
        }
        printf("migrate count %d now line %d\n\n", migrate, now_line_number);
    }
    printf("WAF comecome %4f\n", float(now_line_number + migrate - 1)/now_line_number);
    return 0;
}