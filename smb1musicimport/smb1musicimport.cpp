#include <iostream>
#include <format>
#include "famitracker_parser.h"

#define SP(speed) speed*1, speed*2, speed*3, speed*4, speed*6, speed*8, speed*12, speed*16
#define tenSP(of) SP(of), SP(of+1), SP(of+2), SP(of+3), SP(of+4), SP(of+5), SP(of+6), SP(of+7), SP(of+8), SP(of+9)

void write_to_file(std::ofstream& file, uint8_t array[], int size, int mod = 7)
{
    file << "\t.db ";
    for (int i = 0; i < size; i++)
        file << std::format("${:02x}{}", array[i], (i + 1) == size ? "" : (i + 1) % mod == 0 ? "\n\t.db " : ", ");
}

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

    std::ofstream ofile("test/music_data.asm");
    //create MusicLengthLookupTbl
    ofile << "MusicLengthLookupTbl:\n";
    uint8_t MusicLengthLookupTbl[] =
    {
        tenSP(1), tenSP(11), tenSP(21), SP(31), SP(32)
    };
    write_to_file(ofile, MusicLengthLookupTbl, 256);

    file.select_track(2);
    std::vector<std::vector<uint8_t>> music_data(4);
    std::fill(music_data.begin(), music_data.end(), std::vector<uint8_t>(256));

    //parse SQ2 and TRI
    for (int ch = 1; ch < 3; ch++)
    {
        for (int order_no = 0; order_no < file.num_of_orders; order_no++)
        {
            if (file.already_did_pattern(ch, order_no)) continue;
            file.go_to_pattern(ch, order_no);
            file.pattern_done(ch, order_no);
        }
        std::cout << std::endl;
    }
}
