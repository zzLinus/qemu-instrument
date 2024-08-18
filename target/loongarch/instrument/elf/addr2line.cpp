#include "addr2line.h"

#include <elf.h>
#include <algorithm>
#include <string>
#include <vector>

#define fatal(fmt, args...) fatal_impl(__FILE__, __LINE__, fmt, ##args)

void fatal_impl(const char* file, int line, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%d: ", file, line);
	vprintf(fmt, ap);
	printf("\n");
	exit(1);
}

class Elf {
	public:
		void load(uint8_t* data, int len);

		Elf64_Shdr* lookup_section(const char* target);

		uint8_t* data() const { return data_; }

	private:
		uint8_t* data_;
		int len_;
		Elf64_Ehdr* header_;
};

uint8_t* data;
Elf elf;

struct Stream {
	Stream(uint8_t* data, int len) : data_(data), len_(len), ofs_(0) {}

	bool read(void* out, int count) {
		if (ofs_ + count > len_) {
			ofs_ = len_;
			return false;
		}
		if (out)
			memcpy(out, data_ + ofs_, count);
		ofs_ += count;
		return true;
	}

	bool read_uint8(uint8_t* out) { return read(out, 1); }
	bool read_int8(int8_t* out) { return read(out, 1); }
	bool read_uint16(uint16_t* out) { return read(out, 2); }
	bool read_uint32(uint32_t* out) { return read(out, 4); }
	bool read_uint64(uint64_t* out) { return read(out, 8); }

	bool read_str(std::string* str) {
		uint8_t c;
		for (;;) {
			if (!read_uint8(&c))
				return false;
			if (c == 0)
				break;
			str->push_back(c);
		}
		return true;
	}

	bool read_uleb128(uint64_t* out) {
		uint64_t value = 0;
		int shift = 0;
		uint8_t b;
		for (;;) {
			if (!read_uint8(&b))
				return false;
			value |= ((uint64_t)b & 0x7F) << shift;
			if ((b & 0x80) == 0)
				break;
			shift += 7;
		}
		if (out)
			*out = value;
		return true;
	}

	bool read_sleb128(int64_t* out) {
		int64_t value = 0;
		int shift = 0;
		uint8_t b;
		for (;;) {
			if (!read_uint8(&b))
				return false;
			value |= ((uint64_t)b & 0x7F) << shift;
			shift += 7;
			if ((b & 0x80) == 0)
				break;
		}
		if (shift < 64 && (b & 0x40))
			value |= -(1 << shift);
		if (out)
			*out = value;
		return true;
	}

	bool read_initial_length(uint32_t* out) {
		if (!read_uint32(out))
			return false;
		if ((*out & 0xFFFFFFF0) == 0xFFFFFFF0)
			fatal("initial length extension (dwarf64?) unimplemented");
		return true;
	}

	bool align(int boundary) {
		int extra = ofs_ % boundary;
		if (extra) {
			int pad = boundary - extra;
			return read(NULL, pad);
		}
		return true;
	}

	uint8_t* data_;
	int len_;
	int ofs_;
};

class AddressMap {
	public:
		void load(uint8_t* data, int len);

		void dump();
		bool lookup(uint64_t address, std::string* file, int* line, int *column);

	private:
		struct Registers {
			uint64_t address;
			int file;
			int line;
			int column;
			bool is_stmt;
			bool basic_block;
			bool end_sequence;
			bool prologue_end;
			bool epilogue_begin;
			int isa;
			int discriminator;
			explicit Registers(bool default_is_stmt) :
				address(0),
				file(1),
				line(1),
				column(0),
				is_stmt(default_is_stmt),
				basic_block(false),
				end_sequence(false),
				prologue_end(false),
				epilogue_begin(false),
				isa(0),
				discriminator(0) {
				}
		};

		struct Row {
			uint64_t address;
			int file;
			int line;
			int column;
			Row(uint64_t a, int f, int l, int c) : address(a), file(f), line(l), column(c) {}
			bool operator<(const Row& other) const {
				return address < other.address;
			}
		};

		int load_one(uint8_t* data, int len);

		void emit(const Registers& regs) {
			if (regs.address) {
				int file = regs.file;
				if (file > 0)
					file += file_offset_ - 1;
				matrix_.push_back(Row(regs.address, file, regs.line, regs.column));
			}
		}

		int file_offset_;
		std::vector<std::string> files_;
		std::vector<Row> matrix_;
};

void Elf::load(uint8_t* data, int len) {
	if (len < (int)sizeof(Elf64_Ehdr))
		fatal("too short");

	data_ = data;
	len_ = len;
	header_ = (Elf64_Ehdr*)data;

	if (memcmp(header_->e_ident, ELFMAG, SELFMAG) != 0)
		fatal("bad magic");

	int eclass = header_->e_ident[EI_CLASS];
	if (eclass != ELFCLASS64)
		fatal("bad elf class %d", eclass);

	int edata = header_->e_ident[EI_DATA];
	if (edata != ELFDATA2LSB)
		fatal("bad elf data format %d", edata);

	int eversion = header_->e_ident[EI_VERSION];
	if (eversion != EV_CURRENT)
		fatal("bad elf version %d", eversion);

	/*int eabi = header_->e_ident[EI_OSABI];
		if (eabi != ELFOSABI_LINUX)
		fatal("%s: bad elf abi %d", filename, eabi);*/
}

Elf64_Shdr* Elf::lookup_section(const char* target) {
	Elf64_Shdr* sheaders = (Elf64_Shdr*)(data_ + header_->e_shoff);
	int sheader_count = header_->e_shnum;

	Elf64_Shdr* sheader_names = &sheaders[header_->e_shstrndx];
	char* names = (char*)&data_[sheader_names->sh_offset];

	for (int i = 0; i < sheader_count; i++) {
		Elf64_Shdr* shdr = &sheaders[i];
		char* name = names + shdr->sh_name;
		if (strcmp(name, target) == 0)
			return shdr;
	}
	return NULL;
}

void AddressMap::load(uint8_t* data, int len) {
	while (len > 0) {
		int one_len = load_one(data, len);
		data += one_len;
		len -= one_len;
	}

	std::sort(matrix_.begin(), matrix_.end());
}

int AddressMap::load_one(uint8_t* data, int len) {
	Stream in(data, len);

	uint32_t unit_length;
	check(in.read_initial_length(&unit_length));
	int end = in.ofs_ + unit_length;
	uint16_t version;
	check(in.read_uint16(&version));
	uint32_t header_length;
	check(in.read_uint32(&header_length));
	uint8_t minimum_instruction_length;
	check(in.read_uint8(&minimum_instruction_length));

	uint8_t default_is_stmt;
	check(in.read_uint8(&default_is_stmt));
	int8_t line_base;
	check(in.read_int8(&line_base));
	uint8_t line_range;
	check(in.read_uint8(&line_range));
	uint8_t opcode_base;
	check(in.read_uint8(&opcode_base));

	for (int i = 1; i < opcode_base; i++) {
		// In theory, we could record the opcode lengths here.
		// But we don't need them.
		check(in.read_uint8(NULL));
	}

	for (;;) {
		std::string path;
		check(in.read_str(&path));
		if (path.empty())
			break;
		// TODO: record path.
	}

	file_offset_ = files_.size();
	for (;;) {
		std::string file;
		check(in.read_str(&file));
		if (file.empty())
			break;
		files_.push_back(file);
		uint64_t dir_index = 0, mtime = 0, file_length = 0;
		check(in.read_uleb128(&dir_index));
		check(in.read_uleb128(&mtime));
		check(in.read_uleb128(&file_length));
	}

	Registers regs(default_is_stmt);

	while (in.ofs_ < end) {
		uint8_t op;
		if (!in.read_uint8(&op))
			break;

		switch (op) {
			case 0x0: { // extended
                check(in.read_uleb128(NULL));  // length
                check(in.read_uint8(&op));
                uint64_t addr;
                switch (op) {
                    case 0x01:  // DW_LNE_end_sequence
                        Log("end sequence\n");
                        regs.end_sequence = true;
                        emit(regs);
                        regs = Registers(default_is_stmt);
                        break;
                    case 0x02:  // DW_LNE_set_address
                        check(in.read_uint64(&addr));
                        Log("set addr 0x%llx\n", (unsigned long long)addr);
                        regs.address = addr;
                        break;
                    case 0x04:  // DW_LNE_set_discriminator
                        check(in.read_uleb128(&addr));
                        Log("set discriminator %d\n", (int)addr);
                        regs.discriminator = (int)addr;
                        break;
                    case 0x03:  // DW_LNE_define_file
                    case 0x80:  // DW_LNE_lo_user
                    case 0xff:  // DW_LNE_hi_user
                    default:
                        fatal("unhandled extended op 0x%x\n", op);
                }
                break;
            }

			case 0x1:  // DW_LNS_copy
                Log("copy\n");
                emit(regs);
                regs.basic_block = false;
                regs.prologue_end = false;
                regs.epilogue_begin = false;
                regs.discriminator = 0;
                break;

			case 0x2: {  // DW_LNS_advance_pc
                uint64_t delta;
                check(in.read_uleb128(&delta));
                delta *= minimum_instruction_length;
                regs.address += delta;
                Log("advance pc %d => %llx\n", (int)delta, (long long)regs.address);
                break;
            }

			case 0x3: {  // DW_LNS_advance_line
                int64_t delta;
                check(in.read_sleb128(&delta));
                regs.line += delta;
                Log("advance line %d => %d\n", (int)delta, (int)regs.line);
                break;
            }

			case 0x4: { // DW_LNS_set_file
                uint64_t file;
                check(in.read_uleb128(&file));
                const char* filename = "??";
                if (file > 0) {
                    filename = files_[file_offset_ + file - 1].c_str();
                }
                Log("file %s\n", filename);
                regs.file = file;
                break;
            }

			case 0x5: { // DW_LNS_set_column
                uint64_t column;
                check(in.read_uleb128(&column));
                regs.column = column;
                break;
            }

			case 0x6: { // DW_LNS_negate_stmt
                Log("negate stmt\n");
                regs.is_stmt = !!regs.is_stmt;
                break;
            }

			case 0x7: { // DW_LNS_set_basic_block
                fatal("unimpl\n");
                break;
            }

			case 0x8: { // DW_LNS_const_add_pc
                int adjusted_opcode = 255 - opcode_base;
                int address_increment =
                    (adjusted_opcode / line_range) * minimum_instruction_length;
                regs.address += address_increment;
                Log("add pc %d => %llx\n", address_increment, (long long)regs.address);
                break;
            }

			case 0x9: { // DW_LNS_fixed_advance_pc
                uint16_t delta;
                check(in.read_uint16(&delta));
                regs.address += delta;
                Log("advance pc %d => %llx\n", (int)delta, (long long)regs.address);
                break;
            }
            /* dwarf3
                DW_LNS_set_prologue_end ‡  0x0a
                DW_LNS_set_epilogue_begin ‡  0x0b
                DW_LNS_set_isa ‡  0x0c
            */

			default: {
                int adjusted_opcode = op - opcode_base;
                Log("special op 0x%x  ", adjusted_opcode);
                int address_increment =
                    (adjusted_opcode / line_range) * minimum_instruction_length;
                int line_increment = line_base + (adjusted_opcode % line_range);
                regs.address += address_increment;
                regs.line += line_increment;
                Log("addr += %d => %llx, line += %d => %d\n",
                        address_increment, (long long)regs.address,
                        line_increment, (int)regs.line);
                emit(regs);
                regs.basic_block = false;
                regs.prologue_end = false;
                regs.epilogue_begin = false;
                regs.discriminator = 0;
            }
		}
	}

	return in.ofs_;
}

void AddressMap::dump() {
	for (std::vector<Row>::const_iterator i = matrix_.begin();
			i != matrix_.end(); ++i) {
		Log("%llx %s:%d\n", (long long)i->address,
				files_[i->file].c_str(), i->line);
	}
}

bool AddressMap::lookup(uint64_t address, std::string* file, int* line, int *column) {
	Row query(address, 0, 0, 0);
	// Find the first address greater than the query, then back up by one.
	std::vector<Row>::const_iterator i =
		std::upper_bound(matrix_.begin(), matrix_.end(), query);
	if (i == matrix_.begin() || i == matrix_.end())
		return false;
	--i;
	*file = files_[i->file];
	*line = i->line;
	*column = i->column;
	return true;
}

class ArangesMap {
	public:
		void load(uint8_t* data, int len);

	private:
		int load_one(uint8_t* data, int len);

		std::vector<uint64_t> offsets_;
};

void ArangesMap::load(uint8_t* data, int len) {
	while (len > 0) {
		int consumed = load_one(data, len);
		data += consumed;
		len -= consumed;
	}
}

int ArangesMap::load_one(uint8_t* data, int len) {
	Stream in(data, len);

	uint32_t unit_length;
	check(in.read_initial_length(&unit_length));
	uint16_t version;
	check(in.read_uint16(&version));
	uint32_t debug_info_offset;
	check(in.read_uint32(&debug_info_offset));
	uint8_t address_size;
	check(in.read_uint8(&address_size));
	uint8_t segment_size;
	check(in.read_uint8(&segment_size));

	check(in.align(2 * 8));

	Log("len %d ver %d off %lld addr %d seg %d\n",
			unit_length, version, (long long)debug_info_offset, address_size, segment_size);
	for (;;) {
		uint64_t addr, length;
		if (!in.read_uint64(&addr))
			break;
		check(in.read_uint64(&length));
		Log("ofs %llx len %llx\n", (long long)addr, (long long)length);
		if (addr == 0 && length == 0)
			break;
	}
	return in.ofs_;
}



class DebugInfo {
	public:
		void load(Elf* elf);
};

void DebugInfo::load(Elf* elf) {
	Elf64_Shdr* shdr_debuginfo = elf->lookup_section(".debug_info");
	if (!shdr_debuginfo)
		fatal("couldn't find .debug_info");

	Stream in(elf->data() + shdr_debuginfo->sh_offset, shdr_debuginfo->sh_size);

	uint32_t unit_length;
	check(in.read_initial_length(&unit_length));
	uint16_t version;
	check(in.read_uint16(&version));
	uint32_t debug_abbrev_offset;
	check(in.read_uint32(&debug_abbrev_offset));
	uint8_t address_size;
	check(in.read_uint8(&address_size));
	Log("len %d ver %d ofs %d size %d\n", unit_length, version, debug_abbrev_offset, address_size);

	// 7.5.2 Debugging Information Entry

	uint64_t abbreviation_code;
	check(in.read_uleb128(&abbreviation_code));
	Log("code %d\n", (int)abbreviation_code);


	Elf64_Shdr* shdr_debugabbrev = elf->lookup_section(".debug_abbrev");
	if (!shdr_debugabbrev)
		fatal("couldn't find .debug_abbrev");
	Stream debugabbrev(elf->data() + shdr_debugabbrev->sh_offset,
			shdr_debugabbrev->sh_size);
	uint64_t code, tag;
	check(debugabbrev.read_uleb128(&code));
	check(debugabbrev.read_uleb128(&tag));
	Log("debugabbrev\n");
	Log("code 0x%x tag 0x%x (0x11 = compile_unit)\n", (int)code, (int)tag);

	enum {
		DW_CHILDREN_no = 0x0,
		DW_CHILDREN_yes = 0x1
	};

	uint8_t child_tag;
	check(debugabbrev.read_uint8(&child_tag));
	Log("child %d\n", (int)child_tag);
	if (child_tag == DW_CHILDREN_yes) {
	}

	for (;;) {
		uint64_t attr_name, attr_form;
		check(in.read_uleb128(&attr_name));
		check(in.read_uleb128(&attr_form));
		if (attr_name == 0 && attr_form == 0)
			break;
		Log("attr name 0x%x form 0x%x\n", attr_name, attr_form);
	}

}

void addr2line(uint64_t address, int* line, int* column, char* filename) {
    if (elf.data() == NULL) {
        printf("elf load failed\n");
        return;
    }

	Elf64_Shdr* shdr_lines = elf.lookup_section(".debug_line");
	if (!shdr_lines) {
        printf("couldn't find .debug_line\n");
        return;
    }

	AddressMap map;
	map.load(&data[shdr_lines->sh_offset], shdr_lines->sh_size);

    std::string file;
    int _line, _column;
    if (map.lookup(address, &file, &_line, &_column)) {
        *line = _line;
        *column = _column;
        strcpy(filename, file.c_str());
        // printf("%s:%d col: %d\n", file.c_str(), line, column);
    } else {
        printf("??:0\n");
    }
}

void prep_addr2line(int fd) {
    struct stat st;
	if (fstat(fd, &st) < 0)
		fatal("fstat(): %s", strerror(errno));

    data = (uint8_t*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!data)
		fatal("mmap(%d): %s", fd, strerror(errno));

	elf.load(data, st.st_size);
}
