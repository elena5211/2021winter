#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <fstream>
#include "FTL.h"
#include <string.h>
#define total_line_number 41943040

int SSD::GetVictimBlockIndex() {
	int victim_count = 0;
	int victim_phyblock_num = 0;
	for (int i = 0; i < TOTAL_PHY_BLOCK_NUMBER; i++) {
		if (my_dead_ratio_table[i] > victim_count) {
			victim_count = my_dead_ratio_table[i];
			victim_phyblock_num = i;
		}
	}
	return victim_phyblock_num;
}

int SSD::MigratePage(int now_PPA) {
	int LPA = my_OOB_table[now_PPA];
	WritePage(LPA);
	return now_PPA;
}

int SSD::EraseBlock(int block) {
	for (int i = 0; i < PPB; i++) {
		int now_test_page_index = block * PPB + i;
		if (my_ssd[now_test_page_index] == VALID) {
			migrate_valid_count++;
			current_migrate_count++;
			int old_PPA = MigratePage(now_test_page_index);
			my_ssd[old_PPA] = ERASED;
		}
		else if (my_ssd[now_test_page_index] == INVALID) { my_ssd[now_test_page_index] = ERASED; }
	}
	my_dead_ratio_table[block] = 0;
	free_block_queue.push(block);
	return 0;
}

int SSD::WritePage(int LPA) {
	int old_PPA = my_LPAtable[LPA];
	my_LPAtable[LPA] = free_page_index;
	my_ssd[free_page_index] = VALID;
	my_OOB_table[free_page_index] = LPA;
	if (old_PPA > 0) {
		my_ssd[old_PPA] = INVALID;
		my_dead_ratio_table[old_PPA / PPB] += 1;

	}

	free_page_index++;
	if (free_page_index % PPB == 0) {
		free_page_index = PPB * free_block_queue.front();
		free_block_queue.pop();
		if (free_block_queue.size() <= 1) {
			int result = GarbageCollection();
			if (result < 0) {
				cout << "GarbageCollection Fail!!\n" << endl;
				return -1;
			}
		}
	}
	return 0;
}

int SSD::GarbageCollection() {
	static int hundred_count = 0;
	GC_count++;
	int victim_block = GetVictimBlockIndex();
	int result = EraseBlock(victim_block);

	if (result < 0) {
		cout << "EraseBlock Fail!!\n" << endl;
		return -1;
	}
	hundred_count++;
	if (hundred_count % 1000 == 0) {
		char buffer[100];
		sprintf(buffer, "%d, %d\n", hundred_count/1000, current_migrate_count);
		fputs(buffer, outfile);
	}
	current_migrate_count = 0;
	return 0;
}

int SSD::GetInvalidCount() {
	return migrate_valid_count;
}

int getTotalLine(const char* name) {
	FILE* fp;
	int line = 0;
	char c;
	fp = fopen(name, "r");
	while ((c = fgetc(fp)) != EOF)
		if (c == '\n') line++;
	fclose(fp);
	return(line);
}

float GetWAF(int current_invalid_count, int current_workload_count) {
	float result = float(current_workload_count + current_invalid_count) / float(current_workload_count);
	return result;
}

int main() {
	printf("make SSD\n");
	SSD SSD;
	FILE* readFile;
	printf("file read start!\n");
	readFile = fopen("C:/Users/eun32/Downloads/1_zip_11", "rb");
	int now_line_number = 0;
	//int total_line_number = getTotalLine("C:/Users/eun32/Downloads/1_zip_11");
	int printindex = 5;
	if (readFile == NULL) { printf("nononono"); return 0; }
	printf("file read end\n");

	while (!feof(readFile)) {
		char buffer[50];
		char* workptr;
		int LPA = 0;
		fgets(buffer, sizeof(buffer), readFile);
		workptr = buffer+3;
		workptr = strtok(workptr, " ");
		LPA = atoi(workptr) / 8;
		SSD.WritePage(LPA);
		now_line_number += 1;
		if (now_line_number == total_line_number / 100 * printindex) {
			printf("%3d / 100 success :", printindex);
			printf("%.4f\n", GetWAF(SSD.GetInvalidCount(), now_line_number));
			printindex+=5;
		}
	}
	printf("\n\nSSD.GC_count: %d", SSD.GC_count);
	return 0;
}