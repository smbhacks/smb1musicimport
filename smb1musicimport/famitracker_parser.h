#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <format>

struct FtMacro
{
	int purpose;
	int id;
	int loop;
	int release;
	int chip;
	std::vector<int> values;
};

struct FtInst
{
	int id;
	int volume;
	int arpeggio;
	int pitch;
	int hipitch;
	int duty;
	std::string name;
};

class FtTXT
{
private:
	int next_line(int pos);
	int go_to_track(int track_no);
	std::string get_string(int& pos);

	const char* InstrumentsLabel = "# Instruments";
	const char* MacrosLabel = "# Macros";
	
	std::string m_content;
	std::vector<FtInst> m_instruments;
	std::vector<FtMacro> m_macros;
	std::vector<std::vector<int>> m_orders;
	bool m_open = false;
	unsigned int m_interal_pos;
	int m_selected_track;
public:
	FtTXT(std::string path);
	bool is_open();
	void select_track(int track_no);
	bool go_to_pattern(int pattern_no);

	int num_of_tracks;
	int num_of_orders;
};