#include <iostream>
#include <format>
#include "famitracker_parser.h"

int main()
{
    FtTXT file("test/music.txt");
    if (!file.is_open())
    {
        std::cout << "ERROR: Couldn't open the file!";
        return -1;
    }
    file.select_track(2);
    file.go_to_pattern(11);
}
