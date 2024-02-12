#pragma once
#include <format>
#include <fstream>
#include <vector>
#include "famitracker_parser.h"

const int SQ1_CH = 0;
const int SQ2_CH = 1;
const int TRI_CH = 2;
const int NOI_CH = 3;

void write_to_file(std::ofstream& file, uint8_t array[], int size, int mod = 8, std::string datatype = ".db ");
void write_to_file_string(std::ofstream& file, std::string array[], int size, int mod, std::string datatype = ".db ");
void export_to_studsbase(std::ofstream& file, std::vector<std::vector<std::vector<uint8_t>>> music_data, FtTXT ftfile, int track_number);