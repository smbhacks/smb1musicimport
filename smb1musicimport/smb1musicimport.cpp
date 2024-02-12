#include <iostream>
#include <format>
#include <fstream>
#include <vector>
#include "famitracker_parser.h"
#include "write_file.h"

#define SP_2(speed) (speed)*1, (speed)*2, (speed)*3, (speed)*4, (speed)*8, (speed)*12, (speed)*16, (speed)*32
#define SP_3(speed) (speed)*1, (speed)*2, (speed)*3, (speed)*4, (speed)*6, (speed)*12, (speed)*18, (speed)*24

uint8_t MusicLengthLookupTbl[] =
{
    SP_2(1), SP_2(2), SP_2(3), SP_2(4), SP_2(5), SP_2(6), SP_2(7),
    SP_3(1), SP_3(2), SP_3(3), SP_3(4), SP_3(5), SP_3(6), SP_3(7)
};
uint8_t RowsBase2[8] = { SP_2(1) };
uint8_t RowsBase3[8] = { SP_3(1) };
uint8_t UsedRowSizes[8] = { SP_2(1) };
int get_note_value(std::string pitch_table, std::string note, bool sq1 = false)
{
    int p = pitch_table.find(note);
    if (p > pitch_table.size())
    {
        std::cout << std::format("\nWarning! Couldn't find {} in the {} note range!", note, sq1 ? "SQ1" : "SQ2/TRI");
        return -1;
    }
    //count newlines before p
    int newlines = 0;
    while (p >= 0)
    {
        if (pitch_table[p] == '\n') newlines++;
        p--;
    }
    if(newlines>=64 || (sq1 && newlines >= 32))
    {
        std::cout << std::format("\nWarning! Couldn't find {} in the {} note range!", note, sq1 ? "SQ1" : "SQ2/TRI");
        return -1;
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
    uint8_t note_value = get_note_value(pitch_table, cur_note, true);
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
                if(!put_down_note) note_value = get_note_value(pitch_table, cur_note, true);
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

const enum ExportMode
{
    Undefined,
    StudsBase,
    Pattern,
    SinglePattern
};
int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        std::cout << "\nUsage: [textexport.txt] [pitch_table.asm] [output.asm] [song number] [-studsbase]/[-pattern -o/-u]/[-singlepattern]";
        std::cout << "\n[textexport.txt]: Path to the .txt file exported by Famitracker.";
        std::cout << "\n[pitch_table.asm]: Path to the pitch lookup table file. \nThe parser gets note values from here based on the text row numbers!";
        std::cout << "\n[song number]: Song number in the module. 0 means every song";
        std::cout << "\n[output.asm]: Path where the result should be saved.";
        std::cout << "\n-studsbase: Export for the studsbase SMB1 base.";
        std::cout << "\n-pattern -o/-u: Export pattern music data. Takes in an extra argument for the noise patterns:";
        std::cout << "\n\t-o: Optimize noise pattern with the 00 music data at the end (repeat from start)";
        std::cout << "\n\t-u: Keep the noise data unoptimized.";
        std::cout << "\n-singlepattern: Try to export all of the song's music data in under 1 label.";
        std::cout << "\n";
        return -1;
    }
    int input_track_number = atoi(argv[4]) - 1;
    bool every_song = false;
    if (input_track_number == -1)
    {
        input_track_number++;
        every_song = true;
    }
    ExportMode export_mode = Undefined;
    bool export_optimized = false;
    if (std::strcmp(argv[5], "-studsbase") == 0)
    {
        export_mode = StudsBase;
    }
    else if (std::strcmp(argv[5], "-pattern") == 0)
    {
        export_mode = Pattern;
        if (argc < 6)
        {
            std::cout << "Please specify whether you want the pattern export be optimized or not with the -o and -u parameters!";
            return -1;
        }
        if (std::strcmp(argv[6], "-o") == 0) export_optimized = true;
        else if (std::strcmp(argv[6], "-u") == 0) export_optimized = false;
        else
        {
            std::cout << "ERROR: Invalid parameter: " << argv[6];
            return -1;
        }
    }
    else if (std::strcmp(argv[5], "-singlepattern"))
    {
        export_mode = SinglePattern;
    }
    else
    {
        std::cout << "ERROR: Invalid parameter: " << argv[5];
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

    std::ofstream ofile(argv[3]);
    //create MusicLengthLookupTbl
    ofile << "MusicLengthLookupTbl:\n";
    write_to_file(ofile, MusicLengthLookupTbl, 112);

    const std::string noise_inst_names[] = { "HIHAT", "KICK", "SNARE" };
    const int noise_inst_values[] = { 0x10,    0x20,    0x30 };
    std::vector<int> speed_values;
    do
    {
        std::vector<std::vector<std::vector<uint8_t>>> music_data(4);
        std::fill(music_data.begin(), music_data.end(), std::vector<std::vector<uint8_t>>(256));

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
            speed_values.push_back(8 * (file.track_speed - 1 + 7));
            for (int i = 0; i < 8; i++) UsedRowSizes[i] = RowsBase3[i];
        } else speed_values.push_back(8 * (file.track_speed - 1));

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
                        if (!put_down_note) music_data[ch][pattern_number].push_back(get_note_value(pitch_table, cur_note));
                    }
                    else if (ch == SQ1_CH)
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
        if (export_mode == StudsBase) export_to_studsbase(ofile, music_data, file, input_track_number);
        input_track_number++;
    } while (every_song && input_track_number < file.num_of_tracks);
    if (export_mode == StudsBase)
    {
        std::vector<std::string> track_names;
        for (int i = 0; i < file.num_of_tracks; i++)
        {
            file.select_track(i);
            track_names.push_back(file.track_name);
        }
        //export
        ofile << "\n\nMusicHeaderData:\n";
        std::vector<std::string> values;
        for (int i = 0; i < file.num_of_tracks; i++)
        {
            values.push_back(std::format("{}Hdr-MHD", track_names[i]));
        }
        write_to_file_string(ofile, values.data(), values.size(), 1);
        ofile << "\n";
        for (int i = 0; i < file.num_of_tracks; i++)
        {
            ofile << std::format("\n{}Hdr: .db ${:02X}, <{}_PatternsHigh, >{}_PatternsHigh, <{}_PatternsLow, >{}_PatternsLow", track_names[i], speed_values[i], track_names[i], track_names[i], track_names[i], track_names[i]);
        }
    }
}
