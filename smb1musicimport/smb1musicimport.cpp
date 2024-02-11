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
    file.select_track(3);
    for (int ch = 0; ch < 4; ch++)
    {
        for (int order_no = 0; order_no < file.num_of_orders; order_no++)
        {
            file.go_to_pattern(ch, order_no);
            std::cout << file.get_note() << " ";
        }
        std::cout << std::endl;
    }
}
