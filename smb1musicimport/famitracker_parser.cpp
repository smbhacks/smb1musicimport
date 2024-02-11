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

FtTXT::FtTXT(std::string path)
{
	std::ifstream file(path);
	if (file.is_open())
	{
		//load file
		m_content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		
		//load instruments
		int pos = next_line(m_content.find(InstrumentsLabel));
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

		m_open = true;
	}
	else m_open = false;
}

bool FtTXT::is_open()
{
	return m_open;
}
