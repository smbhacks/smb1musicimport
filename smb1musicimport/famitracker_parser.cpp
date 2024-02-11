#include "famitracker_parser.h"

int FtTXT::next_line(int pos)
{
	return m_content.find('\n', pos) + 1;
}

std::string FtTXT::get_string(int& pos)
{
	while (m_content[pos] == ' ' || m_content[pos] == '\n') ++pos;
	int pos_end;
	if (m_content[pos] == '"')
	{
		pos_end = m_content.find('"', pos+1);
		std::string text = m_content.substr(pos+1, pos_end - pos - 1);
		pos = pos_end + 1;
		return text;
	}
	else
	{
		int pos_newline = m_content.find('\n', pos);
		int pos_space = m_content.find(' ', pos);
		pos_end = std::min(pos_space, pos_newline);
		std::string text = m_content.substr(pos, pos_end - pos);
		pos = pos_end;
		return text;
	}
}

bool FtTXT::is_open()
{
	return m_open;
}

FtTXT::FtTXT(std::string path)
{
	int pos;
	std::ifstream file(path);
	if (file.is_open())
	{
		//load file
		m_content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	
		//load instruments
		pos = next_line(m_content.find(InstrumentsLabel));
		while (get_string(pos) == "INST2A03")
		{
			FtInst instrument;
			instrument.id = stoi(get_string(pos));
			instrument.volume = stoi(get_string(pos));
			instrument.arpeggio = stoi(get_string(pos));
			instrument.pitch = stoi(get_string(pos));
			instrument.hipitch = stoi(get_string(pos));
			instrument.duty = stoi(get_string(pos));
			instrument.name = get_string(pos);
			m_instruments.push_back(instrument);
		}

		//load macros
		pos = next_line(m_content.find(MacrosLabel));
		while (get_string(pos) == "MACRO")
		{
			FtMacro macro;
			macro.purpose = stoi(get_string(pos));
			macro.id = stoi(get_string(pos));
			macro.loop = stoi(get_string(pos));
			macro.release = stoi(get_string(pos));
			macro.chip = stoi(get_string(pos));
			int pos_end = m_content.find('\n', pos);
			get_string(pos); // dummy read to skip :
			while (pos < pos_end)
			{
				macro.values.push_back(stoi(get_string(pos)));
			}

			m_macros.push_back(macro);
		}
		m_open = true;

		//count tracks
		pos = 0;
		num_of_tracks = 0;
		while (pos < m_content.size())
		{
			pos = m_content.find("TRACK", pos + 1);
			++num_of_tracks;
		}
		--num_of_tracks;
	}
	else m_open = false;
}

int FtTXT::go_to_nth_element(int nth_element, std::string find_str, int pos = 0)
{
	while (nth_element >= 0)
	{
		pos = m_content.find(find_str, pos + 1);
		if (pos > m_content.size()) return m_content.size();
		nth_element--;
	}
	return pos;
}

// Tracks are indexed from 1
void FtTXT::select_track(int track_no)
{
	m_selected_track = track_no;
	int pos = go_to_nth_element(track_no, "TRACK");
	pos = m_content.find("ORDER", pos);
	while (get_string(pos) == "ORDER")
	{
		// 2 dummy reads for ORDER number and :
		get_string(pos);
		get_string(pos);
		std::vector<int> values;
		int pos_end = m_content.find('\n', pos);
		while (pos < pos_end)
		{
			values.push_back(stoi(get_string(pos), nullptr, 16));
		}
		m_orders.push_back(values);
	}
	num_of_orders = m_orders.size();
}

bool FtTXT::go_to_pattern(int ch, int order_no)
{
	int pattern_no = m_orders[order_no][ch];
	m_interal_pos = go_to_nth_element(m_selected_track, "TRACK");
	int out_of_bounds = go_to_nth_element(m_selected_track + 1, "TRACK");
	m_interal_pos = m_content.find(std::format("PATTERN {:02X}", pattern_no), m_interal_pos);
	if (m_interal_pos < out_of_bounds)
	{
		m_selected_ch = ch;
		m_interal_pos = m_content.find("ROW", m_interal_pos);
		return true;
	}
	else return false; //couldnt find pattern!
}

std::string FtTXT::get_note(int row_advance)
{
	int pos = m_interal_pos;
	pos = go_to_nth_element(m_selected_ch, ":", pos);
	return m_content.substr(pos + 2, 3);
}