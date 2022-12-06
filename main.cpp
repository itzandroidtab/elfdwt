#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <vector>
#include <array>
#include <iomanip>

namespace elf {
    // size of the e_ident
    constexpr static uint32_t e_ident_size = 16;

    /**
     * @brief section type
     * 
     */
    enum class type: uint32_t {
        undefined = 0,
        progbits = 1,
    };  

    /**
     * @brief Elf header
     * 
     */
    struct header {
        uint8_t e_ident[e_ident_size];
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_version;
        uint32_t e_entry;
        uint32_t e_phoff;
        uint32_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
    };

    /**
     * @brief Elf program header
     * 
     */
    struct program_header {
        uint32_t	p_type;
        uint32_t	p_offset;
        uint32_t	p_vaddr;
        uint32_t	p_paddr;
        uint32_t	p_filesz;
        uint32_t	p_memsz;
        uint32_t	p_flags;
        uint32_t	p_align;
    };

    /**
     * @brief Elf sector header
     * 
     */
    struct sector_header {
        uint32_t sh_name;
        uint32_t sh_type;
        uint32_t sh_flags;
        uint32_t sh_addr;
        uint32_t sh_offset;
        uint32_t sh_size;
        uint32_t sh_link;
        uint32_t sh_info;
        uint32_t sh_addralign;
        uint32_t sh_entsize;  
    };
}

/**
 * @brief Get a vector with all the raw data in a file
 * 
 * @param file 
 * @return std::vector<uint8_t> 
 */
std::vector<uint8_t> get_raw_file(const std::string file) {
    // open the file we have as argument
    auto input = std::ifstream(file, std::ios::binary);

    // create a vector with all the data
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});

    // close the file
    input.close();

    // return the file buffer
    return buffer;
}

/**
 * @brief Calculate the crc from the first 7 words of memory
 * 
 * @param data 
 * @return uint32_t 
 */
uint32_t calculate_crc(const std::array<uint32_t, 7> data) {
    uint32_t ret = 0;

    for (const auto &d: data) {
        ret += d;
    }

    return static_cast<uint32_t>(0) - ret;
}

int main(int argc, char **argv) {
    // give the user some feedback when we start the application
    std::cout << "ELFdwt for little endian" << std::endl;

    // check if we have any files as parameters
    if (argc <= 1) {
        std::cout << "Error: argument expected" << std::endl;

        return -1;
    }

    // get the raw data of the file
    auto raw = get_raw_file(argv[1]);

    // check if we have data
    if (!raw.size()) {
        std::cout << "Error: could not open file" << std::endl;

        return -1;
    }

    // elf signature for comparing to the raw data
    const std::array<uint8_t, 4> elf_signature = {0x7f, 'E', 'L', 'F'};

    // check if we have a valid elf signature
    if (!std::equal(elf_signature.begin(), elf_signature.end(), raw.begin())) {
        std::cout << "Error: invalid elf file (no header)" << std::endl;

        return -1;
    }

    // check if we have enough data in the buffer for a valid elf header
    if (raw.size() < sizeof(elf::header)) {
        std::cout << "Error: invalid elf file (file to small, header)" << std::endl;

        return -1;
    }

    // get the elf header from the file
    const auto& header = (*reinterpret_cast<elf::header*>(raw.data()));

    // check if we have at least 2 sections in the header. 
    // One for the null entry and at least one for the data
    // section
    if (header.e_shnum < 2) {
        std::cout << "Error: invalid elf file (not enough sections)" << std::endl;

        return -1;
    }

    // check if all the sections fit into the raw file
    if (raw.size() < (header.e_shoff + (header.e_shnum * sizeof(elf::sector_header)))) {
        std::cout << "Error: invalid elf file (file to small, sections)" << std::endl;

        return -1;
    }

    // create a pointer to the section array in the file
    const auto *const sections = reinterpret_cast<elf::sector_header*>(raw.data() + header.e_shoff);

    // go to the first section. (section 0 is a null entry. All 
    // items are 0 so we should skip this)
    const auto& vectors = sections[1];

    // check if the vector section is valid
    if (static_cast<elf::type>(vectors.sh_type) != elf::type::progbits) {
        std::cout << "Error: first section does not have the progbits flag set" << std::endl;

        return -1;
    }

    // check if the section is big enough to fit the section data + 8 words
    if (raw.size() < vectors.sh_offset + (8 * sizeof(uint32_t))) {
        std::cout << "Error: invalid elf file (file to small, vectors)" << std::endl;

        return -1;
    }

    // array for calculating the checksum
    std::array<uint32_t, 7> crc_data = {};

    // get the data for the checksum
    for (uint32_t i = 0; i < crc_data.size(); i++) {
        // get the offset for the data
        const uint8_t *const ptr = raw.data() + vectors.sh_offset + (i * sizeof(uint32_t));

        // TODO: make this configurable. Currently only for little endian
        crc_data[i] = (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
    }

    // calculate the checksum 
    uint32_t crc = calculate_crc(crc_data);

    // set the data at the correct location
    std::cout << std::hex << "Signature over range: 0x" << std::setfill('0') << std::setw(8) 
              << 0x0 << " - " << std::setw(8) << ((crc_data.size() - 1) * sizeof(uint32_t)) 
              << ": " << std::setw(8) << (crc_data.size() * sizeof(uint32_t)) << " = " 
              << std::setw(8) << crc << std::endl;

    // set the crc in the raw data
    uint8_t *const location = raw.data() + vectors.sh_offset + (7 * sizeof(uint32_t));

    // TODO: make this configurable. Currently only for little endian
    location[0] = crc & 0xff;
    location[1] = (crc >> 8) & 0xff;
    location[2] = (crc >> 16) & 0xff;
    location[3] = (crc >> 24) & 0xff;
    
    // open the same file as the input
    std::ofstream output(argv[1], std::ios::binary);

    // copy the binary data to the file
    std::copy(
        raw.begin(), raw.end(),
        std::ostreambuf_iterator<char>(output)
    );

    // close the file
    output.close();

    // give the user a signal we are done
    std::cout << "Processing completed, success" << std::endl;
}
