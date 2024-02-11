#pragma once
#include <string>
#include <vector>
#include <fstream>

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
	const std::string InstrumentsLabel = "# Instruments";
	int next_line(int pos);
	std::string get_string(int& pos);

	std::string m_content;
	bool m_open = false;
	std::vector<FtInst> m_instruments;
public:
	FtTXT(std::string path);
	bool is_open();
};