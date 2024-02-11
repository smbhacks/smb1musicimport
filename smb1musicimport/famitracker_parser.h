#pragma once
#include <string>
#include <vector>
#include <fstream>

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
	const char* InstrumentsLabel = "# Instruments";
	const char* MacrosLabel = "# Macros";
	int next_line(int pos);
	std::string get_string(int& pos);

	std::string m_content;
	bool m_open = false;
	std::vector<FtInst> m_instruments;
	std::vector<FtMacro> m_macros;
public:
	FtTXT(std::string path);
	bool is_open();
};