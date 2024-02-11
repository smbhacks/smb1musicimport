#include <iostream>
#include <format>
#include "famitracker_parser.h"

int main()
{
    FtTXT file("test/music.txt");
    if (!file.is_open())
    {
        std::cout << "ERROR: Couldn't open the music file!";
        return -1;
    }
    std::ifstream pfile("test/pitch_table.asm");
    if (!pfile.is_open())
    {
        std::cout << "ERROR: Couldn't open the pitch_table.asm file!";
        return -1;
    }
    std::string pitch_table((std::istreambuf_iterator<char>(pfile)), std::istreambuf_iterator<char>());
    pfile.close();
    file.select_track(0);

    //handle conversion
    for (int ch = 0; ch < 4; ch++)
    {
        for (int order_no = 0; order_no < file.num_of_orders; order_no++)
        {
            if (file.already_did_pattern(ch, order_no)) continue;
            file.go_to_pattern(ch, order_no);
            std::cout << "Channel " << ch << " Pattern " << file.order_to_pattern(ch, order_no) << std::endl;
            while (!file.end_of_pattern())
            {
                std::cout << file.get_note() << std::endl;
            }
            file.pattern_done(ch, order_no);
        }
        std::cout << std::endl;
    }
}
