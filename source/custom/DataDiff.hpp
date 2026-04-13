//
// Created by Liam on 23/02/2026.
//

#ifndef FILEDIFFERENCEGRABBER_DATADIFF_HPP
#define FILEDIFFERENCEGRABBER_DATADIFF_HPP
#include <fstream>

#endif //FILEDIFFERENCEGRABBER_DATADIFF_HPP

#include <iostream>
#include "RingBuffer.hpp"

namespace Nes
{
    namespace Custom
    {
        // we need this so that we can compare files of any vector type
        /*!
         * The `DataDiff` class provides functionality to compare two files represented as vectors of bytes and generate a list of differences between them. It also allows applying those differences to a file to revert it back to its original state. The class includes methods for reading files, comparing them, and applying differences, making it a useful tool for tasks such as patching files or creating binary diffs.
         */
        template <typename T>
        class DataDiff
        {
        public:

            /*!
             * The `difference` struct represents a single difference between two files, containing the byte index where
             * the difference occurs and the old byte value from the first file. This struct is used to store the
             * differences found when comparing two files, and it can be used to apply those differences to a file to
             * revert it back to its original state.
             */
            struct difference
            {
                long byteIndex;
                T oldFileByte;

                difference(long byteIndex, T oldFileByte) : byteIndex(byteIndex), oldFileByte(oldFileByte) {}
            };

            /*!
             * This function compares two files and creates a vector of `difference` structs which contain the offset
             * as a 32-bit byte and the old byte value from the first file. The function should be able to handle files
             * of different sizes, and it should always treat the smaller file as the "old" file and the larger file as
             * the "new" file, regardless of the order in which they are passed in.
             * This means that if the files are of different sizes, the function will compare the bytes of the smaller
             * file to the corresponding bytes of the larger file, and any remaining bytes in the larger file will be
             * considered differences.
             * @param oldFile The old file, which should be the smaller file if the files are of different sizes
             * @param newFile The new file, which should be the larger file if the files are of different sizes
             * @return std::vector<difference> a vector of differences between the two files, where each difference contains the byte index and the old byte value from the old file regardless of which file is larger.
             */
            static std::vector<difference> CompareFiles(const std::vector<T>& oldFile, const std::vector<T>& newFile)
            {
                std::vector<difference> differences;

                //std::cout << "old file size: " << oldFile.size() << std::endl;
                //std::cout << "new file size: " << newFile.size() << std::endl;

                bool newIsLarger = true;
                if (oldFile.size() != newFile.size())
                    if (oldFile.size() > newFile.size())
                        newIsLarger = false;

                if (newIsLarger) // Here, the new file is larger
                {
                    for (size_t i = 0; i < oldFile.size(); ++i)
                    {
                        //std::cout << "Comparing byte index " << i << std::endl;
                        if (oldFile[i] != newFile[i])
                        {
                            //std::cout << "Found difference at byte index " << i << std::endl;
                            // create a difference and push it back
                            difference diff = {static_cast<long>(i), oldFile[i]};
                            differences.push_back(diff);
                        }
                    }

                    // TODO: Make sure that any leftover bytes are added if files are not equal and there are differences left in the old file

                } else // Here, the old file is larger
                {
                    for (size_t i = 0; i < newFile.size(); ++i)
                    {
                        if (oldFile[i] != newFile[i])
                        {
                            // we still want the old file since that's what we want to save
                            difference diff = {static_cast<long>(i), oldFile[i]};
                            differences.push_back(diff);
                        }
                    }

                    // TODO: Make sure that any leftover bytes are added if files are not equal and there are differences left in the old file

                }

                return differences;
            }

            /*!
             * This function takes in a file and a vector of differences, and applies the differences to the file by changing the bytes at the specified indices to the old byte values from the differences. This should return the old file if the differences were generated by comparing the old file to a new file.
             * @param file The file we want to apply the differences to
             * @param differences The differences we want to apply, as generated by `CompareFiles`
             * @return std::vector<T> the file with the differences applied, which should be the old file if the differences were generated by comparing the old file to a new file
             */
            static std::vector<T> applyDifferences(std::vector<T>& file, const std::vector<difference>& differences)
            {
                for (const auto& diff : differences)
                {
                    file[diff.byteIndex] = diff.oldFileByte;
                }

                return file;
            }

            /*!
             * This function reads all bytes from a file and returns them as a vector of chars. It is used to read the old and
             * new files that we want to compare.
             * @param filename the name of the file to read
             * @return std::vector<char> the bytes of the file
             */
            static std::vector<char> ReadAllBytes(char const* filename)
            {
                std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
                const std::ifstream::pos_type pos = ifs.tellg();

                std::vector<char>  result(pos);

                ifs.seekg(0, std::ios::beg);
                ifs.read(reinterpret_cast<std::istream::char_type*>(&result[0]), pos);

                return result;
            }

            /*!
             * @param differences the differences between the two files, as generated by `CompareFiles`
             *
             * The diff file will be formatted like this:
             * 1. `filetype=DIF\n` this starts out the file and tells us it is a DIF file
             * 2. the format of changes will be
             *     1. binary format of where the change occurs in the file:
             *         1. 00000001 - the first place
             *     2. If it's a range of contiguous changes, the first bit will be flipped
             *         1. 10000001 - a range from the first place
             *     3. And the size of the change will be added in terms of bytes
             *         1. 00000010 - 2 contiguous bytes
             *     4. And finally, the actual changes that happened
             *         1. 00100100 - the previous state was 00100100
             * 3. All together an example of a file is as follows:
             *
             * "filetype=DIF
             * 00000001 00100100 10000010 00000011 00001011 01001011 10110011"
             *
             * 00000001 - a change at the first byte
             * 00100100 - what change took place
             *
             * 10000010 - a contiguous change from the second byte
             * 00000011 - for three bytes
             * 00001011 - the first change
             * 01001011 - the second change
             * 10110011 - the third change
             */
            static std::string GetDiff(const std::vector<difference> &differences)
            {
                std::string outputString; // this will store the output string that we will write to the file at the end, we use a string to avoid the overhead of writing to the file multiple times

                long lastByte = differences.back().byteIndex;
                std::size_t encodingSize = 8;

                //std::cout << "Last byte index: " << lastByte << std::endl;

                if (lastByte > 0xFF)
                {
                    //std::cout << "Bigger than 0xFF" << std::endl;
                    encodingSize = 16;
                    if (lastByte > 0xFFFF)
                    {
                        //std::cout << "Bigger than 0xFFFF" << std::endl;
                        encodingSize = 32;
                        // if (lastByte > 0xFFFFFFFF)
                        // {
                        //     std::cout << "Bigger than 0xFFFFFFFF" << std::endl;
                        //     encodingSize = 64;
                        // }
                    }
                }

                //std::cout << "Encoding size: " << encodingSize << " bits" << std::endl;

                outputString += "filetype=DIF";

                switch (encodingSize)
                {
                case 8:
                    outputString += "0001"; // the first 4 bits of the file will be the encoding size, which tells us how many bits are used to represent the byte index of the change
                    break;
                case 16:
                    outputString += "0010";
                    break;
                case 32:
                    outputString += "0100";
                default:
                    break;
                }

                // Iterate through the differences
                for (int i = 0; i < differences.size(); i++)
                {
                    //std::cout << "Current byte index: " << differences[i].byteIndex;
                    // if (i-1 >= 0) std::cout << " Previous byte index: " << differences[i - 1].byteIndex << std::endl;
                    // else std::cout << std::endl;
                    // check if the next difference is contiguous with the current one
                    if (i + 1 < differences.size() - 1 && differences[i + 1].byteIndex == differences[i].byteIndex + 1)
                    {
                        // if so, we need to find out how long the contiguous change is and write that out as well
                        int j = 1; // we start at 0 because we haven't checked the next byte yet
                        std::vector<char> changes; // store the changes

                        // store the first change
                        changes.push_back(differences[i].oldFileByte);

                        while (differences[i+j].byteIndex == differences[i+j - 1].byteIndex + 1)
                        {
                            changes.push_back(differences[i+j].oldFileByte);

                            j++;
                        }

                        switch (encodingSize)
                        {
                        default:
                            outputString += std::bitset<8>(differences[i].byteIndex | 0b10000000).to_string(); // the place where the first contiguous change occurs, with the first bit flipped to indicate that it is a contiguous change
                            outputString += std::bitset<8>(j).to_string(); // the length of the contiguous change
                            break;
                        case 16:
                            outputString += std::bitset<16>(differences[i].byteIndex | 0b1000000000000000).to_string(); // the place where the first contiguous change occurs, with the first bit flipped to indicate that it is a contiguous change
                            outputString += std::bitset<16>(j).to_string(); // the length of the contiguous change
                            break;
                        case 32:
                            outputString += std::bitset<32>(differences[i].byteIndex | 0b10000000000000000000000000000000).to_string(); // the place where the first contiguous change occurs, with the first bit flipped to indicate that it is a contiguous change
                            outputString += std::bitset<32>(j).to_string(); // the length of the contiguous change
                            break;
                        }

                        for (char change : changes)
                        {
                            outputString += std::bitset<8>(change).to_string(); // the rest of the changes that took place
                        }

                        //std::cout << "Contiguous change at byte index " << differences[i].byteIndex << " for " << j << " bytes" << std::endl;

                        i += j - 1; // we need to move the index past the contiguous change
                    } else
                    {
                        switch (encodingSize)
                        {
                        default:
                            outputString += std::bitset<8>(differences[i].byteIndex).to_string(); // the place where the first contiguous change occurs, with the first bit flipped to indicate that it is a contiguous change
                            break;
                        case 16:
                            outputString += std::bitset<16>(differences[i].byteIndex).to_string(); // the place where the first contiguous change occurs, with the first bit flipped to indicate that it is a contiguous change
                            break;
                        case 32:
                            outputString += std::bitset<32>(differences[i].byteIndex).to_string(); // the place where the first contiguous change occurs, with the first bit flipped to indicate that it is a contiguous change
                            break;
                        }
                        // if not, we need to just add the difference to the output string
                        outputString += std::bitset<8>(differences[i].oldFileByte).to_string();
                    }
                }

                std::cout << outputString << std::endl;
                return outputString;
            }

            /*!
             * Writes the output string to the specified file. This is done at the end of the program to avoid the
             * overhead of writing to the file multiple times, as we can build up the entire output string in memory and
             * then write it all at once.
             * @param outputFilename the file to write to
             * @param outputString the string to write to the file
             */
            static void WriteFile(const char* outputFilename, const std::string& outputString)
            {
                std::ofstream output(outputFilename, std::ios::out | std::ios::trunc);
                if (!output.is_open())
                {
                    std::cerr << "Failed to open output file" << std::endl;
                    return;
                }

                output << outputString;
                output.close();
            }

            /*!
             * This function takes in a diff string, as generated by `GetDiff`, and parses it to extract the differences
             * between the two files. It returns a vector of `difference` structs, which contain the byte index and the
             * bytes from both files at that index.
             * @return std::vector<difference> a vector of differences extracted from the diff string, where each
             * difference contains the byte index and old byte of the change as represented in the diff string.
             */
            static std::vector<difference> ReverseDiff(const std::string& diff)
            {
                std::string outputString;

                // first we need to check that the file opens with "filetype=DIF0000", and if it doesn't, we should return an error
                if (diff.size() < 12 || std::string(diff.begin(), diff.begin() + 12) != "filetype=DIF")
                {
                    std::cout << std::string(diff.begin(), diff.begin() + 12) << std::endl;
                    std::cerr << "Invalid diff file format" << std::endl;
                    return {};
                }

                //std::cout << "Size of diff string: " << diff.size() << std::endl;
                std::vector<difference> differences;
                std::bitset<8> mask_8 = 0b10000000;
                std::bitset<8> antimask_8 = 0b01111111;
                std::bitset<16> mask_16 = 0b1000000000000000;
                std::bitset<16> antimask_16 = 0b0111111111111111111;
                std::bitset<32> mask_32 = 0b10000000000000000000000000000000;
                std::bitset<32> antimask_32 = 0b01111111111111111111111111111111;

                auto diffSize = std::string(diff.begin() + 12, diff.begin() + 16);
                if (diffSize == "0001")
                {
                    // Start at index 15 because the first 16 characters are the file type and padding
                    for (size_t i = 16; i < diff.size(); i++) // Iterate over the diff
                    {
                        // first, check the first 32 characters to get the index of the change
                        std::bitset<8> index = std::bitset<8>(diff.substr(i, i + 8));
                        //std::cout << "Index: " << index.to_string() << std::endl;

                        if ((mask_8 & index) == mask_8)
                        {
                            // find the length of the contiguous change by checking the next 32 characters
                            std::bitset<8> length = std::bitset<8>(diff.substr(i + 8, i + 16));
                            //std::cout << "Length: " << length.to_string() << std::endl;
                            index = antimask_8 & index; // we need to set the first bit back to 0 to get the actual byte index
                            //std::cout << "Contiguous change at byte index " << index.to_ullong() << " for " << length.to_ulong() <<
                            //    " bytes" << std::endl;

                            // finally, we read the next length * 8 characters to get the actual changes that took place
                            for (int j = 0; j < length.to_ulong(); j++)
                            {
                                //std::cout << "Change: " << std::bitset<8>(diff.substr(i + 16 + j * 8, i + 24 + j * 8)) << std::endl;
                                differences.emplace_back(static_cast<long>(index.to_ullong() + j),
                                                         static_cast<char>(std::bitset<8>(
                                                             diff.substr(i + 16 + j * 8, i + 24 + j * 8)).to_ulong()));
                            }
                            i += 16 + length.to_ulong() * 8 - 1; // we need to move the index past the contiguous change
                        }
                        else
                        {
                            //std::cout << "Non-contiguous change at byte index " << i << std::endl;
                            //std::cout << "Binary number: " << std::bitset<8>(diff.substr(i + 8, i + 8 + 8)) << std::endl;
                            differences.emplace_back(static_cast<long>(index.to_ullong()),
                                                     static_cast<char>(std::bitset<8>(diff.substr(i + 8, i + 8 + 8)).to_ulong()));

                            i += 8 + 7; // Move the index past the index length + the change
                        }
                        // turn it into a 32-bit binary number
                    }
                } else if (diffSize == "0010")
                {
                    // Start at index 15 because the first 16 characters are the file type and padding
                    for (size_t i = 16; i < diff.size(); i++) // Iterate over the diff
                    {
                        // first, check the first 32 characters to get the index of the change
                        std::bitset<16> index = std::bitset<16>(diff.substr(i, i + 16));
                        std::cout << "Index: " << index.to_string() << std::endl;

                        if ((mask_16 & index) == mask_16)
                        {
                            // find the length of the contiguous change by checking the next 32 characters
                            std::bitset<16> length = std::bitset<16>(diff.substr(i + 16, i + 32));
                            //std::cout << "Length: " << length.to_string() << std::endl;
                            index = antimask_16 & index; // we need to set the first bit back to 0 to get the actual byte index
                            //std::cout << "Contiguous change at byte index " << index.to_ullong() << " for " << length.to_ulong() <<
                            //    " bytes" << std::endl;

                            // finally, we read the next length * 8 characters to get the actual changes that took place
                            for (int j = 0; j < length.to_ulong(); j++)
                            {
                                //std::cout << "Change: " << std::bitset<8>(diff.substr(i + 32 + j * 8, i + 48 + j * 8)) << std::endl;
                                differences.emplace_back(static_cast<long>(index.to_ullong() + j),
                                                         static_cast<char>(std::bitset<8>(
                                                             diff.substr(i + 32 + j * 8, i + 48 + j * 8)).to_ulong()));
                            }
                            i += 32 + length.to_ulong() * 8 - 1; // we need to move the index past the contiguous change
                        }
                        else
                        {
                            //std::cout << "Non-contiguous change at byte index " << i << std::endl;
                            //std::cout << "Binary number: " << std::bitset<8>(diff.substr(i + 16, i + 16 + 8)) << std::endl;
                            differences.emplace_back(static_cast<long>(index.to_ullong()),
                                                     static_cast<char>(std::bitset<8>(diff.substr(i + 16, i + 16 + 8)).to_ulong()));

                            i += 16 + 7; // Move the index past the index length + the change
                        }
                        // turn it into a 32-bit binary number
                    }
                } else if (diffSize == "0100")
                {
                    // Start at index 15 because the first 16 characters are the file type and padding
                    for (size_t i = 16; i < diff.size(); i++) // Iterate over the diff
                    {
                        // first, check the first 32 characters to get the index of the change
                        std::bitset<32> index = std::bitset<32>(diff.substr(i, i + 32));
                        //std::cout << "Index: " << index.to_string() << std::endl;

                        if ((mask_32 & index) == mask_32)
                        {
                            // find the length of the contiguous change by checking the next 32 characters
                            std::bitset<32> length = std::bitset<32>(diff.substr(i + 32, i + 64));
                            //std::cout << "Length: " << length.to_string() << std::endl;
                            index = antimask_32 & index; // we need to set the first bit back to 0 to get the actual byte index
                            //std::cout << "Contiguous change at byte index " << index.to_ullong() << " for " << length.to_ulong() <<
                            //    " bytes" << std::endl;

                            // finally, we read the next length * 8 characters to get the actual changes that took place
                            for (int j = 0; j < length.to_ulong(); j++)
                            {
                                //std::cout << "Change: " << std::bitset<8>(diff.substr(i + 64 + j * 8, i + 72 + j * 8)) << std::endl;
                                differences.emplace_back(static_cast<long>(index.to_ullong() + j),
                                                         static_cast<char>(std::bitset<8>(
                                                             diff.substr(i + 64 + j * 8, i + 72 + j * 8)).to_ulong()));
                            }
                            i += 64 + length.to_ulong() * 8 - 1; // we need to move the index past the contiguous change
                        }
                        else
                        {
                            //std::cout << "Non-contiguous change at byte index " << i << std::endl;
                            //std::cout << "Binary number: " << std::bitset<8>(diff.substr(i + 32, i + 32 + 8)) << std::endl;
                            differences.emplace_back(static_cast<long>(index.to_ullong()),
                                                     static_cast<char>(std::bitset<8>(diff.substr(i + 32, i + 32 + 8)).to_ulong()));

                            i += 32 + 7; // Move the index past the index length + the change
                        }
                        // turn it into a 32-bit binary number
                    }
                } else
                {
                    std::cerr << "Invalid encoding size in diff file" << std::endl;
                    return {};
                }

                return differences;
            }

            /*!
             * This function takes in a file and a vector of differences, and applies the differences to the file by
             * changing the bytes at the indices specified in the differences
             * @param newFile The file we want to apply the differences to
             * @param outputFile The file we want to write the output to
             * @param differences The differences we want to apply, as generated by `ReverseDiff`
             */
            static void ReverseFile(const char* newFile, const char* outputFile, const std::vector<difference>& differences)
            {
                std::ifstream contents(newFile, std::ios::in);
                if (!contents.is_open())
                {
                    std::cerr << "Failed to open diff file" << std::endl;
                    return;
                }

                std::vector<char> bytes = ReadAllBytes(newFile);

                contents.close();

                std::ofstream output(outputFile, std::ios::out | std::ios::trunc);

                for (const auto& diff : differences)
                {
                    bytes[diff.byteIndex] = diff.oldFileByte;
                }

                output << std::string(bytes.begin(), bytes.end());

                output.close();
            }
        };
    }
}