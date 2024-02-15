#include "write_file.h"

void write_to_file(std::ofstream& file, uint8_t array[], int size, int mod, std::string datatype)
{
    if (size == 0) return;
    file << "\t" << datatype;
    for (int i = 0; i < size; i++)
        file << std::format("${:02X}{}", array[i], (i + 1) == size ? "" : (i + 1) % mod == 0 ? "\n\t" + datatype : ", ");
}

void write_to_file_string(std::ofstream& file, std::string array[], int size, int mod, std::string datatype)
{
    if (size == 0) return;
    file << "\t" << datatype;
    for (int i = 0; i < size; i++)
        file << std::format("{}{}", array[i], (i + 1) == size ? "" : (i + 1) % mod == 0 ? "\n\t" + datatype : ", ");
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
    for (int size = 1; (size - 1) < test.size() / 2; size++)
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

const char* channel_names[] = { "SQ1_CH", "SQ2_CH", "TRI_CH", "NOI_CH" };
void export_to_studsbase(std::ofstream& ofile, std::vector<std::vector<std::vector<uint8_t>>> music_data, FtTXT ftfile, int track_number)
{
    //now optimize noise data per pattern
    ftfile.select_track(track_number);
    for (int i = 0; i < music_data[NOI_CH].size(); i++)
    {
        if (music_data[NOI_CH][i].empty()) continue;
        music_data[NOI_CH][i] = optimize_noi(music_data[NOI_CH][i]);
        music_data[NOI_CH][i].push_back(0);
    }

    std::vector<std::string> addresses;
    for (int order_no = 0; order_no < ftfile.num_of_orders; order_no++)
    {
        for (int ch = 0; ch < 4; ch++)
        {
            addresses.push_back(std::format("{}_{}_p{}", ftfile.track_name, channel_names[ch], ftfile.order_to_pattern(ch, order_no)));
        }
    }
    
    addresses.push_back("0");
    ofile << std::format("\n\n{}_PatternsHigh:\n", ftfile.track_name);
    write_to_file_string(ofile, addresses.data(), addresses.size(), 4, ".dh ");

    addresses.pop_back();
    ofile << std::format("\n\n{}_PatternsLow:\n", ftfile.track_name);
    write_to_file_string(ofile, addresses.data(), addresses.size(), 4, ".dl ");

    for (int ch = 0; ch < 4; ch++)
    {
        for (int order_no = 0; order_no < ftfile.num_of_orders; order_no++)
        {
            int pattern_number = ftfile.order_to_pattern(ch, order_no);
            if (ftfile.already_did_pattern(ch, order_no) || music_data[ch][pattern_number].empty()) continue;
            ofile << std::format("\n\n{}_{}_p{}:\n", ftfile.track_name, channel_names[ch], pattern_number);
            write_to_file(ofile, music_data[ch][pattern_number].data(), music_data[ch][pattern_number].size(), 16);
            ftfile.pattern_done(ch, order_no);
        }
    }
}
