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
	int go_to_nth_element(int nth_element, std::string find_str, int pos = 0);
	std::string get_string(int& pos);

	const char* InstrumentsLabel = "# Instruments";
	const char* MacrosLabel = "# Macros";
	
	std::string m_content;
	std::vector<FtInst> m_instruments;
	std::vector<FtMacro> m_macros;
	std::vector<std::vector<int>> m_orders; //order, ch
	std::vector<std::vector<int>> m_pattern_processed_counter; //ch, pattern
	bool m_open = false;
	unsigned int m_internal_pos; //always at the start of a "ROW"
	int m_selected_track;
	int m_selected_ch;
	int m_pattern_length;
	int m_current_row;
public:
	FtTXT(std::string path);
	void select_track(int track_no);
	void pattern_done(int ch, int order_no);
	bool is_open();
	bool go_to_pattern(int ch, int order_no);
	bool end_of_pattern();
	bool already_did_pattern(int ch, int order_no);
	int order_to_pattern(int ch, int order_no);
	std::string get_note(int row_advance = 1);

	int num_of_tracks;
	int num_of_orders;
};