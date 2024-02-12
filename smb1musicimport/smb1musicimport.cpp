#include <iostream>
#include <format>
#include "famitracker_parser.h"

#define SP_2(speed) (speed)*1, (speed)*2, (speed)*3, (speed)*4, (speed)*8, (speed)*12, (speed)*16, (speed)*32
#define SP_3(speed) (speed)*1, (speed)*2, (speed)*3, (speed)*4, (speed)*6, (speed)*12, (speed)*18, (speed)*24

void write_to_file(std::ofstream& file, uint8_t array[], int size, int mod = 8)
{
    if (size == 0) return;
    file << "\t.db ";
    for (int i = 0; i < size; i++)
        file << std::format("${:02x}{}", array[i], (i + 1) == size ? "" : (i + 1) % mod == 0 ? "\n\t.db " : ", ");
}

uint8_t MusicLengthLookupTbl[] =
{
    SP_2(1), SP_2(2), SP_2(3), SP_2(4), SP_2(5), SP_2(6), SP_2(7),
    SP_3(1), SP_3(2), SP_3(3), SP_3(4), SP_3(5), SP_3(6), SP_3(7)
};

uint8_t handle_sq2_note(std::string pitch_table, std::string note)
{
    int p = pitch_table.find(note);
    //count newlines before p
    int newlines = 0;
    while (p != std::string::npos && p >= 0)
    {
        if (pitch_table[p] == '\n') newlines++;
        p--;
    }
    return newlines * 2;
}

bool handle_sq2_rhythm(std::vector<uint8_t>& data, int row_value, std::string pitch_table, std::string cur_note)
{
    bool put_down_note = false;
    while (row_value > 0)
    {
        for (int x = 7; x >= 0; x--)
        {
            if (row_value - MusicLengthLookupTbl[x] >= 0)
            {
                row_value -= MusicLengthLookupTbl[x];
                data.push_back(0x80 + x);
                if (put_down_note) data.push_back(handle_sq2_note(pitch_table, "..."));
                else if(row_value != 0)
                {
                    data.push_back(handle_sq2_note(pitch_table, cur_note));
                }
                put_down_note = true;
                break;
            }
        }
    }
    return put_down_note;
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
    write_to_file(ofile, MusicLengthLookupTbl, 112);

    file.select_track(2);
    std::vector<std::vector<std::vector<uint8_t>>> music_data(4);
    std::fill(music_data.begin(), music_data.end(), std::vector<std::vector<uint8_t>>(256));

    //parse SQ2 and TRI
    const char* channel_names[] = { "SQ1_CH", "SQ2_CH", "TRI_CH", "NOI_CH" };
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

            int pattern_number = file.order_to_pattern(ch, order_no);
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
                    bool put_down_note = false;
                    if (d00_effect) next_note_distance++;
                    if (next_note_distance != cur_row_length)
                    {
                        cur_row_length = next_note_distance;
                        put_down_note = handle_sq2_rhythm(music_data[ch][pattern_number], cur_row_length, pitch_table, cur_note);
                    }
                    if(!put_down_note) music_data[ch][pattern_number].push_back(handle_sq2_note(pitch_table, cur_note));
                }
            }
            ofile << std::format("\n\n{}_{}_{}:\n", file.track_name, channel_names[ch], pattern_number);
            write_to_file(ofile, music_data[ch][pattern_number].data(), music_data[ch][pattern_number].size(), 16);

            file.pattern_done(ch, order_no);
        }
        std::cout << std::endl;
    }
}
