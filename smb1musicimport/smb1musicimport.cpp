#include <iostream>
#include "famitracker_parser.h"

int main()
{
    FtTXT file("test/music.txt");
    if (!file.is_open())
    {
        std::cout << "ERROR: Couldn't open the file!";
        return -1;
    }
}
