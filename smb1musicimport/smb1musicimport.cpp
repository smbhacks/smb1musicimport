#include <iostream>
#include <format>
#include "famitracker_parser.h"

#define SP(speed) (speed)*1, (speed)*2, (speed)*3, (speed)*4, (speed)*6, (speed)*8, (speed)*12, (speed)*16
#define tenSP(of) SP(of), SP(of+1), SP(of+2), SP(of+3), SP(of+4), SP(of+5), SP(of+6), SP(of+7), SP(of+8), SP(of+9)

void write_to_file(std::ofstream& file, uint8_t array[], int size, int mod = 8)
{
    file << "\t.db ";
    for (int i = 0; i < size; i++)
        file << std::format("${:02x}{}", array[i], (i + 1) == size ? "" : (i + 1) % mod == 0 ? "\n\t.db " : ", ");
}

uint8_t handle_sq2_data()
{
    return 0;
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
        tenSP(1), SP(11), SP(12), SP(13), SP(14), SP(15)
    };
    write_to_file(ofile, MusicLengthLookupTbl, 120);

    file.select_track(2);
    std::vector<std::vector<uint8_t>> music_data(4);
    std::fill(music_data.begin(), music_data.end(), std::vector<uint8_t>(256));

    //parse SQ2 and TRI
    const int SQ1_CH = 0;
    const int SQ2_CH = 1;
    const int TRI_CH = 2;
    const int NOI_CH = 3;
    for (int ch = 0; ch < 4; ch++)
    {
        for (int order_no = 0; order_no < file.num_of_orders; order_no++)
        {
            if (file.already_did_pattern(ch, order_no)) continue;
            file.go_to_pattern(ch, order_no);

            int cur_row_length = -1;
            bool d00_effect = false;

            while (!file.end_of_pattern() && !d00_effect)
            {
                int next_note_distance = 0;
                std::string cur_note = file.get_note(0, 0);
                int t = file.current_row();
                std::string check_note;
                do
                {
                    std::vector<std::string> effects = file.get_effects(1, 0);
                    check_note = file.get_note(0, 0);
                    file.current_row();
                    d00_effect = std::find(effects.begin(), effects.end(), "D00") != effects.end();
                    next_note_distance++;
                } while (check_note == "..." && !d00_effect);
                if (ch == SQ2_CH || ch == TRI_CH)
                {
                    if (d00_effect) next_note_distance++;
                    std::cout << cur_note << " " << next_note_distance << std::endl;
                    //music_data[ch].push_back(handle_sq2_data());
                }
            }

            file.pattern_done(ch, order_no);
        }
        std::cout << std::endl;
    }
}
