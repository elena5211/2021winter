#pragma once

#include <queue>
#include <iostream>
#include <fstream>
using namespace std;

#define LBA_RANGE 32 //GB
#define OPSIZE 0.1
#define DEVICE_SIZE LBA_RANGE * (1 + OPSIZE)
#define PAGE_SIZE 4 //KB
#define BLOCK_SIZE 2 //MB
#define PPB 512

#define TOTAL_PHY_BLOCK_NUMBER DEVICE_SIZE / BLOCK_SIZE * 1024
#define TOTAL_PHY_PAGE_NUMBER DEVICE_SIZE / BLOCK_SIZE * 1024 * PPB
#define TOTAL_LOG_PAGE_NUMBER LBA_RANGE / BLOCK_SIZE * 1024 * PPB

#define VALID 1
#define ERASED 0
#define INVALID -1

queue<int> free_block_queue;

class SSD {
private:
	int* my_ssd;
	int* my_LPAtable;
	int* my_dead_ratio_table;
	int* my_OOB_table;
	//int now_written_block_ptr;
	int free_page_index;
	int migrate_valid_count;
	int current_migrate_count;

public:
	int GC_count;
	FILE* outfile;

	SSD() {
		my_ssd = new int[TOTAL_PHY_PAGE_NUMBER];
		my_OOB_table = new int[TOTAL_PHY_PAGE_NUMBER];
		for (int i = 0; i < TOTAL_PHY_PAGE_NUMBER; i++) {
			my_ssd[i] = ERASED;
			my_OOB_table[i] = -1;
		}
		//index는 log page, content는 phy page
		my_LPAtable = new int[TOTAL_LOG_PAGE_NUMBER];
		for (int i = 0; i < TOTAL_LOG_PAGE_NUMBER; i++) {
			my_LPAtable[i] = -1;																
		}
		//index는 phy block, content는 invalid count
		my_dead_ratio_table = new int[TOTAL_PHY_BLOCK_NUMBER];
		for (int i = 0; i < TOTAL_PHY_BLOCK_NUMBER; i++) {
			my_dead_ratio_table[i] = 0;
			free_block_queue.push(i);
		}
		free_page_index = 0;
		migrate_valid_count = 0;
		GC_count = 0;
		current_migrate_count = 0;
		//LBA, Ratio malloc

		outfile = fopen("C:/Users/eun32/UGRP_MLStudy/result11.txt", "w");
	}
	int GetVictimBlockIndex();
	int MigratePage(int now_PPA);
	int EraseBlock(int block);
	int WritePage(int LPA);
	int GarbageCollection();
	int GetInvalidCount();
};

