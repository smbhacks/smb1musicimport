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
	int go_to_nth_element(int nth_element, std::string find_str, int defpos);
	std::string get_string(int& pos);

	const char* InstrumentsLabel = "# Instruments";
	const char* MacrosLabel = "# Macros";
	
	std::string m_content;
	std::vector<FtInst> m_instruments;
	std::vector<FtMacro> m_macros;
	std::vector<std::vector<int>> m_orders;
	bool m_open = false;
	unsigned int m_interal_pos; //always at the start of a "ROW"
	int m_selected_track;
	int m_selected_ch;
public:
	FtTXT(std::string path);
	bool is_open();
	void select_track(int track_no);
	bool go_to_pattern(int ch, int order_no);
	std::string get_note(int row_advance = 1);

	int num_of_tracks;
	int num_of_orders;
};