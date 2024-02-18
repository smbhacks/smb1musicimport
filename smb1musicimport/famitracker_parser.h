#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <format>

class FtTXT
{
public:
	struct FtMacro
	{
		int purpose = -1; //blank macro
		int id;
		int loop;
		int release;
		int chip;
		std::vector<int> values;
	};

	struct FtInst
	{
		int id = -1;
		int volume;
		int arpeggio;
		int pitch;
		int hipitch;
		int duty;
		std::string name = "__NOT_EXISTING_INSTRUMENT";
	};

private:
	int next_line(int pos);
	int go_to_nth_element(int nth_element, std::string find_str, int pos = 0);
	bool advance_row(int row_advance);
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
	int current_row();
	int get_instrument(int row_advance_before_note = 0, int row_advance_after_note = 1);
	std::string get_note(int row_advance_before_note = 0, int row_advance_after_note = 1);
	std::vector<std::string> get_effects(int row_advance_before_note = 0, int row_advance_after_note = 1);
	FtInst get_FtInst(int instrument_number);
	FtMacro get_FtMacro(int purpose, int id);

	int num_of_tracks;
	int num_of_orders;
	int num_of_instruments;
	int track_speed;
	int track_tempo;
	std::string track_name;
};