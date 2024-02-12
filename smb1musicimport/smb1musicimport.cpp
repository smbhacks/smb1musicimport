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
uint8_t RowsBase2[8] = { SP_2(1) };
uint8_t RowsBase3[8] = { SP_3(1) };
uint8_t UsedRowSizes[8] = { SP_2(1) };
uint8_t get_note_value(std::string pitch_table, std::string note)
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

bool handle_sq2_rhythm(std::vector<uint8_t>& data, int remaining_rows, std::string pitch_table, std::string cur_note, int& cur_row_length)
{
    bool put_down_note = false;
    while (remaining_rows > 0)
    {
        for (int x = 7; x >= 0; x--)
        {
            if (remaining_rows - UsedRowSizes[x] >= 0)
            {
                remaining_rows -= UsedRowSizes[x];
                if(cur_row_length != UsedRowSizes[x]) data.push_back(0x80 + x);
                cur_row_length = UsedRowSizes[x];
                if (put_down_note) data.push_back(get_note_value(pitch_table, "---"));
                else
                {
                    data.push_back(get_note_value(pitch_table, cur_note));
                }
                put_down_note = true;
                break;
            }
        }
    }
    return put_down_note;
}

void handle_sq1_note(std::vector<uint8_t>& data, int remaining_rows, std::string pitch_table, std::string cur_note, int& cur_row_length)
{
    bool put_down_note = false;
    uint8_t note_value = get_note_value(pitch_table, cur_note);
    while (remaining_rows > 0)
    {
        for (int x = 7; x >= 0; x--)
        {
            if (remaining_rows - UsedRowSizes[x] >= 0)
            {
                remaining_rows -= UsedRowSizes[x];
                cur_row_length = UsedRowSizes[x];
                uint8_t note_value; 
                uint8_t rhythm_value = (x >> 2) + (x << 6);
                if(!put_down_note) note_value = get_note_value(pitch_table, cur_note);
                else note_value = get_note_value(pitch_table, "---");
                data.push_back(note_value | rhythm_value);
                put_down_note = true;
                break;
            }
        }
    }
}

void handle_noi_note(std::vector<uint8_t>& data, int remaining_rows, int note_value, int& cur_row_length)
{
    bool put_down_note = false;
    while (remaining_rows > 0)
    {
        for (int x = 7; x >= 0; x--)
        {
            if (remaining_rows - UsedRowSizes[x] >= 0)
            {
                remaining_rows -= UsedRowSizes[x];
                cur_row_length = UsedRowSizes[x];
                uint8_t rhythm_value = (x >> 2) + (x << 6);
                if (put_down_note) note_value = 0x04;
                data.push_back(note_value | rhythm_value);
                put_down_note = true;
                break;
            }
        }
    }
}

const int primes[] =
{
  1, // not actually a prime :)
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
  59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
  127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
  191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251
};
std::vector<uint8_t> optimize_noi(std::vector<uint8_t> test)
{
    if (std::find(std::begin(primes), std::end(primes), test.size()) != std::end(primes)) return test; //if the array size is a prime, then it cant be optimized!
    for (int size = 1; (size - 1) < test.size()/2; size++)
    {
        if (test.size() % size != 0) continue;
        int window_start = size;
        bool correct = true;
        while (window_start < test.size() && correct)
        {
            for (int x = 0; x < size; x++) 
                if (test[x] != test[x + window_start])
                {
                    correct = false;
                    break;
                }
            window_start += size;
        }
        if (correct)
        {
            std::vector<uint8_t> optimized;
            for (int x = 0; x < size; x++) optimized.push_back(test[x]);
            return optimized;
        }
    }
    //couldnt find optimization
    return test;
}

const enum ExportMode
{
    Undefined,
    StudsBase,
    Pattern,
    SinglePattern
};
int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        std::cout << "\nUsage: [textexport.txt] [pitch_table.asm] [song number] [-studsbase]/[-pattern -o/-u]/[-singlepattern]";
        std::cout << "\n[textexport.txt]: Path to the .txt file exported by Famitracker.";
        std::cout << "\n[pitch_table.asm]: Path to the pitch lookup table file. \nThe parser gets note values from here based on the text row numbers!";
        std::cout << "\n[song number]: Song number in the module. 0 means every song";
        std::cout << "\n-studsbase: Export for the studsbase SMB1 base.";
        std::cout << "\n-pattern -o/-u: Export pattern music data. Takes in an extra argument for the noise patterns:";
        std::cout << "\n\t-o: Optimize noise pattern with the 00 music data at the end (repeat from start)";
        std::cout << "\n\t-u: Keep the noise data unoptimized.";
        std::cout << "\n-singlepattern: Try to export all of the song's music data in under 1 label.";
        std::cout << "\n";
        return -1;
    }
    int input_track_number = atoi(argv[3]) - 1;
    ExportMode export_mode = Undefined;
    bool export_optimized = false;
    if (std::strcmp(argv[4], "-studsbase"))
    {
        export_mode = StudsBase;
    }
    else if (std::strcmp(argv[4], "-pattern"))
    {
        if (argc < 6)
        {
            std::cout << "\nPlease specify whether you want the pattern export be optimized or not with the -o and -u parameters!\n";
            return -1;
        }
        if (std::strcmp(argv[5], "-o")) export_optimized = true;
        else if (std::strcmp(argv[5], "-u")) export_optimized = false;
        else
        {
            std::cout << "\nERROR: Invalid parameter: " << argv[5] << "\n";
        }
    }

    FtTXT file(argv[1]);
    if (!file.is_open())
    {
        std::cout << "ERROR: Couldn't open the music file!";
        return -1;
    }
    std::ifstream pfile(argv[2]);
    if (!pfile.is_open())
    {
        std::cout << "ERROR: Couldn't open the pitch_table.asm file!";
        return -1;
    }
    std::string pitch_table((std::istreambuf_iterator<char>(pfile)), std::istreambuf_iterator<char>());
    pfile.close();

    std::ofstream ofile("music_data.asm");
    //create MusicLengthLookupTbl
    ofile << "MusicLengthLookupTbl:\n";
    write_to_file(ofile, MusicLengthLookupTbl, 112);
    ofile << "\n";

    std::vector<std::vector<std::vector<uint8_t>>> music_data(4);
    std::fill(music_data.begin(), music_data.end(), std::vector<std::vector<uint8_t>>(256));

    const std::string noise_inst_names[] = { "HIHAT", "KICK", "SNARE" };
    const int noise_inst_values[] =  { 0x10,    0x20,    0x30 };
    const char* channel_names[] = { "SQ1_CH", "SQ2_CH", "TRI_CH", "NOI_CH" };
    const int SQ1_CH = 0;
    const int SQ2_CH = 1;
    const int TRI_CH = 2;
    const int NOI_CH = 3;

    //calculate whether to use 2 or 3 based music by
    //counting the row lengths
    int div2 = 0;
    int div3 = 0;
    file.select_track(input_track_number);
    for (int ch = 0; ch < 4; ch++)
    {
        for (int order_no = 0; order_no < file.num_of_orders; order_no++)
        {
            if (file.already_did_pattern(ch, order_no)) continue;
            file.go_to_pattern(ch, order_no);

            bool d00_effect = false;
            while (!file.end_of_pattern() && !d00_effect)
            {
                int next_note_distance = 0;
                std::string check_note;
                do
                {
                    std::vector<std::string> effects = file.get_effects(1, 0);
                    check_note = file.get_note(0, 0);
                    file.current_row();
                    d00_effect = std::find(effects.begin(), effects.end(), "D00") != effects.end();
                    next_note_distance++;
                } while (check_note == "..." && !d00_effect);
                if (next_note_distance % 2 == 0) div2++;
                if (next_note_distance % 3 == 0) div3++;
            }
            file.pattern_done(ch, order_no);
        }
    }
    //if 3 based song, overwrite the row sizes
    if (div3 > div2)
    {
        for (int i = 0; i < 8; i++) UsedRowSizes[i] = RowsBase3[i];
    }

    //handle actual parsing
    file.select_track(input_track_number);
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
                int cur_instrument = file.get_instrument(0, 0);
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
                if (d00_effect) next_note_distance++;
                if (ch == SQ2_CH || ch == TRI_CH)
                {
                    bool put_down_note = false;
                    if (next_note_distance != cur_row_length)
                    {
                        put_down_note = handle_sq2_rhythm(music_data[ch][pattern_number], next_note_distance, pitch_table, cur_note, cur_row_length);
                    }
                    if(!put_down_note) music_data[ch][pattern_number].push_back(get_note_value(pitch_table, cur_note));
                } 
                else if(ch == SQ1_CH)
                {
                    handle_sq1_note(music_data[ch][pattern_number], next_note_distance, pitch_table, cur_note, cur_row_length);
                }
                else
                {
                    uint8_t note_value = 0;
                    if (cur_instrument == -2)
                    {
                        note_value = 0x04; //silence note
                    }
                    else
                    {
                        FtTXT::FtInst instrument = file.get_FtInst(cur_instrument);
                        for (int i = 0; i < 3; i++)
                        {
                            if (noise_inst_names[i] == instrument.name) note_value = noise_inst_values[i];
                        }
                        if (note_value == 0)
                        {
                            std::cout << std::format("ERROR at ch:{}, pattern:{} | {:02X}:\"{}\" is not a valid noise instrument name!\n", ch, pattern_number, cur_instrument, instrument.name);
                            std::cout << "Make sure your noise instrument is named KICK, HIHAT or SNARE.";
                            return -1;
                        }
                    }
                    handle_noi_note(music_data[ch][pattern_number], next_note_distance, note_value, cur_row_length);
                }
            }
            if (ch == SQ2_CH) music_data[ch][pattern_number].push_back(0);
            file.pattern_done(ch, order_no);
        }
    }

    //now optimize noise data per pattern
    std::vector<std::vector<uint8_t>> optimized_noise_patterns;
    for (int i = 0; i < music_data[NOI_CH].size(); i++)
    {
        if (music_data[NOI_CH][i].empty()) continue;
        std::vector<uint8_t> optimized_data = optimize_noi(music_data[NOI_CH][i]);
        optimized_noise_patterns.push_back(optimized_data);
    }

}
