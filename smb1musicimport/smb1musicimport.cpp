#include <iostream>
#include <format>
#include <fstream>
#include <vector>
#include <algorithm>
#include "famitracker_parser.h"
#include "write_file.h"

#define SP_2(speed) (speed)*1, (speed)*2, (speed)*4, (speed)*6, (speed)*8, (speed)*12, (speed)*16, (speed)*32
#define SP_3(speed) (speed)*1, (speed)*2, (speed)*3, (speed)*4, (speed)*6, (speed)*12, (speed)*18, (speed)*24

uint8_t MusicLengthLookupTbl[] =
{
    SP_2(1), SP_2(2), SP_2(3), SP_2(4), SP_2(5), SP_2(6), SP_2(7),
    SP_3(1), SP_3(2), SP_3(3), SP_3(4), SP_3(5), SP_3(6), SP_3(7)
};
uint8_t RowsBase2[8] = { SP_2(1) };
uint8_t RowsBase3[8] = { SP_3(1) };
uint8_t UsedRowSizes[8] = { SP_2(1) };

int count_newlines(std::string pitch_table, int p)
{
    int newlines = 0;
    while (p >= 0)
    {
        if (pitch_table[p] == '\n') newlines++;
        p--;
    }
    return newlines;
}

int get_note_value(std::string pitch_table, std::string note, int ch = SQ2_CH)
{
    if (ch == TRI_CH && note == "---") return 0;
    int p = pitch_table.find(note);
    if (p > pitch_table.size())
    {
        std::cout << std::format("\nWarning! Couldn't find {} in the {} note range!", note, ch == SQ1_CH ? "SQ1" : "SQ2/TRI");
        return -1;
    }
    //count newlines before p
    int newlines = count_newlines(pitch_table, p);
    if (!(ch == SQ1_CH) && newlines == 0)
    {
        //Trying to access the 0th music note, which is the terminator for SQ2. Try to find it
        //from the back
        p = pitch_table.rfind(note);
        newlines = count_newlines(pitch_table, p);
        if (newlines == 0) //if still couldnt find it, print out a warning
        {
            std::cout << std::format("\nWarning! Note value {} is 0x00 in the SQ2/TRI in the pitch table which is the same as the terminator byte!", note);
        }
    }
    if(newlines>=64 || (ch == SQ1_CH && newlines >= 32))
    {
        std::cout << std::format("\nWarning! Couldn't find {} in the {} note range!", note, ch == SQ1_CH ? "SQ1" : "SQ2/TRI");
        return -1;
    }
    return newlines * 2;
}

bool handle_sq2_rhythm(std::vector<uint8_t>& data, int remaining_rows, std::string pitch_table, std::string cur_note, int& cur_row_length, int ch)
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
                if (put_down_note) data.push_back(get_note_value(pitch_table, "..."));
                else
                {
                    data.push_back(get_note_value(pitch_table, cur_note, ch));
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
    uint8_t note_value = get_note_value(pitch_table, cur_note, SQ1_CH);
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
                if (!put_down_note)
                {
                    note_value = get_note_value(pitch_table, cur_note, SQ1_CH);
                    if ((note_value | rhythm_value) == 0)
                    {
                        std::cout << std::format("Can't use note {} on SQ1 with row length {} as it produces note value $00, which is reserved for the sweep!\n", cur_note, cur_row_length);
                        note_value = get_note_value(pitch_table, "...", SQ1_CH);
                    }
                }
                else note_value = get_note_value(pitch_table, "...", SQ1_CH);
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
    std::vector<FtTXT::FtInst> square_instruments;
    const int SQ_INSTR_START = 5;
    for (int instr = SQ_INSTR_START; instr < file.num_of_instruments; instr++)
    {
        square_instruments.push_back(file.get_FtInst(instr));
    }
    const std::string mandatory_instruments[] = { "LongTriangle", "ShortTriangle", "SNARE", "HIHAT", "KICK" };
    bool failed_instrument_verification = false;
    for (int instr = 0; instr < SQ_INSTR_START; instr++)
    {
        if (file.get_FtInst(instr).name != mandatory_instruments[instr])
        {
            std::cout << std::format("\nInstrument {} with an id {:02X} is missing.", mandatory_instruments[instr], instr);
            failed_instrument_verification = true;
        }
    }
    if (failed_instrument_verification) return -1;

    std::vector<int> speed_values;
    std::vector<int> square_instrument_values;
    std::vector<int> triangle_instrument_values;
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
        } 
        else
        {
            speed_values.push_back(8 * (file.track_speed - 1));
            for (int i = 0; i < 8; i++) UsedRowSizes[i] = RowsBase2[i];
        }
        //handle actual parsing
        file.select_track(input_track_number);
        bool set_square_instrument = false;
        bool set_triangle_instrument = false;
        for (int ch = 0; ch < 4; ch++)
        {
            for (int order_no = 0; order_no < file.num_of_orders; order_no++)
            {
                if (file.already_did_pattern(ch, order_no)) continue;
                file.go_to_pattern(ch, order_no);

                int pattern_number = file.order_to_pattern(ch, order_no);
                int cur_row_length = -1;
                int cutoff_goto_value = -1; //this is set by a Bxx effect, -1 means it's just the end of the pattern
                bool cutoff_effect = false;
                bool sweep_effect_put_down = false;

                while (!file.end_of_pattern() && !cutoff_effect)
                {
                    int next_note_distance = 0;
                    std::string cur_note = file.get_note(0, 0);

                    int cur_instrument = file.get_instrument(0, 0);
                    if (cur_instrument != -2)
                    {
                        if (!set_square_instrument && (ch == SQ2_CH || ch == SQ1_CH))
                        {
                            square_instrument_values.push_back(cur_instrument);
                            set_square_instrument = true;
                        }
                        if (!set_triangle_instrument && ch == TRI_CH)
                        {
                            triangle_instrument_values.push_back(cur_instrument);
                            set_triangle_instrument = true;
                        }
                    }

                    std::vector<std::string> cur_effects = file.get_effects(0, 0);
                    for (auto& effect : cur_effects)
                    { 
                        if (effect[0] == 'I' && !sweep_effect_put_down)
                        {
                            if (effect != "I14") std::cout << "Warning. Sweep effect can only be I14!";
                            else if (ch == SQ1_CH)
                            {
                                music_data[ch][pattern_number].push_back(0);
                                sweep_effect_put_down = true;
                            }
                            else std::cout << "Warning. Sweep effect can only be used on SQ1.";
                        }
                    }
                    
                    int t = file.current_row();
                    std::string check_note;
                    do
                    {
                        std::vector<std::string> effects = file.get_effects(1, 0);
                        check_note = file.get_note(0, 0);
                        file.current_row();
                        //cutoff_effect = std::find(effects.begin(), effects.end(), "D00") != effects.end();
                        for (auto& effect : effects)
                        {
                            if (effect == "D00") cutoff_effect = true;
                            else if (effect[0] == 'B')
                            {
                                cutoff_effect = true;
                                cutoff_goto_value = stoi(effect.substr(1, 2), nullptr, 16);
                            }
                        }
                        next_note_distance++;
                    } while (check_note == "..." && !cutoff_effect);
                    if (cutoff_effect) next_note_distance++;
                    if (ch == SQ2_CH || ch == TRI_CH)
                    {
                        bool put_down_note = false;
                        if (next_note_distance != cur_row_length)
                        {
                            put_down_note = handle_sq2_rhythm(music_data[ch][pattern_number], next_note_distance, pitch_table, cur_note, cur_row_length, ch);
                        }
                        if (!put_down_note) music_data[ch][pattern_number].push_back(get_note_value(pitch_table, cur_note, ch));
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
                if (ch == SQ2_CH)
                {
                    music_data[ch][pattern_number].push_back(0);
                    music_data[ch][pattern_number].push_back(cutoff_goto_value);
                }
                file.pattern_done(ch, order_no);
            }
        }
        if (export_mode == StudsBase)
        {
            if (!set_square_instrument) square_instrument_values.push_back(SQ_INSTR_START);
            if (!set_triangle_instrument) triangle_instrument_values.push_back(0);
            export_to_studsbase(ofile, music_data, file, input_track_number);
        }
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
            ofile << std::format(", {}_id, {}", file.get_FtInst(square_instrument_values[i]).name, triangle_instrument_values[i] == 0 ? "LONGTRI" : "SHORTTRI");
        }
        ofile << "\n";
        for (int i = SQ_INSTR_START; i < file.num_of_instruments; i++)
        {
            ofile << std::format("\n{}_id = {}", file.get_FtInst(i).name, i - SQ_INSTR_START);
        }
        ofile << "\n\nInstrumentsDataHi:";
        for (int i = SQ_INSTR_START; i < file.num_of_instruments; i++) ofile << "\n\t.dh " << file.get_FtInst(i).name;
        ofile << "\n\nInstrumentsDataLo:";
        for (int i = SQ_INSTR_START; i < file.num_of_instruments; i++) ofile << "\n\t.dl " << file.get_FtInst(i).name;

        std::vector<uint8_t> instrument_lengths;
        for (int i = SQ_INSTR_START; i < file.num_of_instruments; i++)
        {
            FtTXT::FtInst instrument = file.get_FtInst(i);
            ofile << std::format("\n\n{}:\n", instrument.name);
            std::vector<uint8_t> values;
            FtTXT::FtMacro vol_macro = file.get_FtMacro(0, instrument.volume);
            FtTXT::FtMacro duty_macro = file.get_FtMacro(4, instrument.duty);
            if (vol_macro.values.size() == 0) vol_macro.values.push_back(15);
            if (duty_macro.values.size() == 0) duty_macro.values.push_back(0);
            if (vol_macro.values.size() > duty_macro.values.size()) values.resize(vol_macro.values.size());
            else values.resize(duty_macro.values.size());

            for (int i = 0; i < values.size(); i++)
            {
                int vol_value;
                int duty_value;
                vol_value = vol_macro.values[std::clamp(i, 0, (int)vol_macro.values.size() - 1)];
                duty_value = duty_macro.values[std::clamp(i, 0, (int)duty_macro.values.size() - 1)];
                values[i] = vol_value + (duty_value << 6) + 0x10;
            }

            std::reverse(values.begin(), values.end());
            instrument_lengths.push_back(values.size() - 1);
            write_to_file(ofile, values.data(), values.size(), 16);
        }

        ofile << "\n\nInstrumentsLengths:\n";
        write_to_file(ofile, instrument_lengths.data(), instrument_lengths.size(), 16);
    }
}
