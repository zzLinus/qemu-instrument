#include "symbol.h"
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <limits>
#include "instrument/decoder/disasm.h"
#include "string.h"
#include "../util/error.h"
#include <fcntl.h>


struct compare_instruction_info {
    bool operator()(const INS lhs, const INS rhs) const
    {
        if (lhs->pc != rhs->pc) {
            return lhs->pc < rhs->pc;
        } else {
            return lhs->pc < rhs->pc;
        }
    };
};

// NOTE: RNT section (aka symbol)
struct pin_rtn {
	SEC *sec;
	std::string name;
	std::set<INS, compare_instruction_info> instructions;   /* symbols are ordered by addr and size */
	uint64_t addr;
	uint64_t size;

	pin_rtn(const char* name,uint64_t addr, uint64_t size) : name(name),addr(addr), size(size)
	{
	}
};

struct compare_symbol_info {
    bool operator()(const RTN *lhs, const RTN *rhs) const
    {
        if (lhs->addr != rhs->addr) {
            return lhs->addr < rhs->addr;
        } else {
            return lhs->size < rhs->size;
        }
    };
};

// NOTE: SEC section

struct sec {
    struct image *image;
	std::string name;
	uint64_t addr;
	uint64_t size;
	std::set<RTN*, compare_symbol_info> symbols;   /* symbols are ordered by addr and size */
    std::unordered_map<std::string, RTN *> symbol_name_map;

	sec(const char* name,uint64_t addr, uint64_t size) : name(name), addr(addr), size(size)
	{
	}
};

struct compare_section_info {
bool operator()(const sec *lhs, const sec *rhs) const
    {
        if (lhs->addr != rhs->addr) {
            return lhs->addr < rhs->addr;
        } else {
            return lhs->size < rhs->size;
        }
    };
};

// NOTE: IMG section
struct image {
    std::string path;
    uintptr_t load_base;
	std::set<SEC *,compare_section_info> sections; /* sections are ordered by addr and size */
    std::unordered_map<std::string, SEC*> section_name_map;

    image(const char *path, uintptr_t load_base) : path(path), load_base(load_base)
    {
    }
};


static std::vector<image *> images;
static std::set<SEC *,compare_section_info> all_sections;
static std::set<RTN*, compare_symbol_info> all_symbols;                        /* 所有的符号信息 */
static std::unordered_map<std::string, std::vector<RTN *>> debug_dup_symbols;   /* 名字重复的符号信息 */

bool is_symbol_name_dupcalited(const char *name)
{
    return (debug_dup_symbols.count(name) != 0);
}

image *image_alloc(const char *path, uintptr_t load_base)
{
    image *img = new image(path, load_base);
    images.push_back(img);
    return img;
}

SEC *sec_alloc(IMG img, const char* name, uint64_t addr, uint64_t size){
	sec * sec = new SEC(name, addr, size);
	sec->image = (image*)img;
	all_sections.insert(sec);
	((image*)img)->sections.insert(sec);
	((image*)img)->section_name_map.insert({sec->name,sec});

	return sec;
}

RTN *rtn_alloc(SEC* sec, const char* name, uint64_t addr, uint64_t size){
    RTN *symbol = new RTN(name, addr, size);
	symbol->sec = sec;
    all_symbols.insert(symbol);
	sec->symbols.insert(symbol);
	sec->symbol_name_map.insert({symbol->name, symbol});

	return symbol;
}

RTN *sec_get_symbol_by_name(SEC *sec, const char *name)
{
    if (sec->symbol_name_map.count(name) == 0) {
        return NULL;
    }
    return sec->symbol_name_map[name];
}

SEC *img_get_sec_by_name(IMG img, const char *name)
{
    if (((image*)img)->section_name_map.count(name) == 0) {
        return NULL;
    }
    return ((image*)img)->section_name_map[name];
}

/* pc -> RTN  */
/* 假设：所有symbol不重叠，因此至多一个symbol满足 */
RTN *get_symbol_by_pc(uint64_t pc)
{
    RTN key ("",pc,UINT64_MAX);
    auto iter = all_symbols.upper_bound(&key);
    if (iter != all_symbols.begin()) {
        RTN *sym = *prev(iter);
        if (sym->addr <= pc && pc < sym->addr + sym->size) {
            return sym;
        }
    }
    return NULL;
}

const char *get_symbol_name_by_pc(uint64_t pc)
{
    RTN *sym = get_symbol_by_pc(pc);
    if (sym) {
		return sym->name.c_str();
    }
    return NULL;
}

const char *get_img_name(image* img){
    for (const image *_img : images) {
		if (img ==_img)
			return img->path.c_str();
    }
	return "can't find img name!\n";
}

void print_collected_symbols(void)
{
    bool overlap = false;
    const RTN *last_symbol = NULL;

    printf("Print all symbols:\n");
    for (const SEC *sec : all_sections) {
        printf("Image: %s (%lu symbols)\n", sec->name.c_str(), sec->symbols.size());
        printf("ADDR    SIZE    NAME\n");
        for (const RTN *symbol : sec->symbols) {
            printf("0x%-16lx    0x%-16lx    %s\n", symbol->addr, symbol->size, symbol->name.c_str());

            /* debug: check overlap */
            if (last_symbol) {
                if (last_symbol->addr + last_symbol->size > symbol->addr) {
                    overlap = true;
                    printf("[debug] overlap with last symbol.\n");
                }
            }
            last_symbol = symbol;
        }
    }

    /* debug info */
    printf("\n");
    if (overlap) {
        printf("[debug] Some symbols are overlaped. :(\n");
    } else {
        printf("[debug] No symbols are overlaped. :)\n");
    }

    if (debug_dup_symbols.empty()) {
        printf("[debug] No duplicated symbols. :)\n");
    } else {
        printf("[debug] Duplicated symbols:\n");
        for (auto &p : debug_dup_symbols) {
            printf("%s:\n", p.first.c_str());
            for (RTN *sym : p.second) {
                printf("0x%-16lx    0x%-16lx  \n", sym->addr, sym->size);
            }
            printf("\n");
        }
    }
}


// NOTE: RTN section
const CHAR *RTN_FindNameByAddress(ADDRINT address)
{
    return get_symbol_name_by_pc(address);
}

RTN *RTN_FindByName(IMG img, const CHAR *name)
{
	for(auto & sec: ((image*)img)->sections) {
		RTN *sym = sec_get_symbol_by_name(sec, name);
		if (sym == NULL || sym->addr == 0 || sym->size == 0) {
			return NULL;
		}
		if (is_symbol_name_dupcalited(name)) {
			lswarn("symbol %s is duplicated in image.\n", name);
		}
		return sym;
	}
	return NULL;
}

/* === 下面为内部实现所需接口 === */

static auto compare_rtn = [](const RTN &lhs, const RTN &rhs)
{
    if (lhs.addr != rhs.addr) {
        return lhs.addr < rhs.addr;
    }
    if (lhs.size != rhs.size) {
        return lhs.size < rhs.size;
    }
    lsassert(lhs.name.compare(rhs.name)  == 0);
    return false;
};

/* 函数插桩
 * 1. RTN_FindByName 会创建一个 RTN
 * 2. RTN_InsertCall 向 RTN 注册 callbacks（rtn_entry_cbs 和 rtn_exit_cbs 分别维护函数入口和出口处插桩的 callbacks）
 * BUG: 以相同的image的name多次调用 RTN_FindByName 会创建多个相同的 RTN，导致 rtn_exit_cbs 里有多个相同的 rtn
 */
/* FIXME move this to pin_state */
static std::unordered_map<uintptr_t, std::vector<ANALYSIS_CALL>> rtn_entry_cbs;
static std::map<RTN, std::vector<ANALYSIS_CALL>, decltype(compare_rtn)> rtn_exit_cbs(compare_rtn);

VOID RTN_add_entry_cb(RTN *rtn, ANALYSIS_CALL *cb)
{
	rtn_entry_cbs[rtn->addr].emplace_back(*cb);
}

ANALYSIS_CALL *RTN_get_entry_cbs(uintptr_t pc, int *cnt)
{
    if (rtn_entry_cbs.count(pc) == 0) {
        *cnt = 0;
        return NULL;
    }
    *cnt = rtn_entry_cbs[pc].size();
    return rtn_entry_cbs[pc].data();
}

VOID RTN_add_exit_cb(RTN* rtn, ANALYSIS_CALL *cb)
{
    rtn_exit_cbs[*rtn].emplace_back(*cb);
}

ANALYSIS_CALL *RTN_get_exit_cbs(uintptr_t pc, int *cnt)
{
    /* find a rtn which contains the pc */
    /* FIXME 目前假设所有rtn不重叠，因此pc只出现在一个rnt的范围中 */
    RTN key ("",pc,UINT64_MAX);
    auto iter = rtn_exit_cbs.upper_bound(key);
    if (iter != rtn_exit_cbs.begin()) {
        RTN rtn = prev(iter)->first;
        if (rtn.addr <= pc && pc < (rtn.addr + rtn.size)) {
            *cnt = rtn_exit_cbs[rtn].size();
            return rtn_exit_cbs[rtn].data();
        }
    }

    *cnt = 0;
    return NULL;
}

SEC* IMG_SecHead(IMG img)
{
	return *((image*)img)->sections.begin();
}

SEC* SEC_Next(SEC* sec)
{
	image * img = sec->image;
	if(img->sections.upper_bound(sec) != img->sections.end()){
		return *img->sections.upper_bound(sec);
	}

	return NULL;
}

bool SEC_Valid(SEC* sec)
{
	return (sec != NULL);
}

const char* SEC_Name(SEC* sec)
{
	return sec->name.c_str();
}

RTN* SEC_RtnHead(SEC* sec)
{
	return *sec->symbols.begin();
}

RTN* RTN_Next(RTN* rtn)
{
	SEC * sec= rtn->sec;
	if(sec->symbols.upper_bound(rtn) != sec->symbols.end()){
		return *sec->symbols.upper_bound(rtn);
	}

	return NULL;
}

bool RTN_Valid(RTN* rtn)
{
	return (rtn != NULL);
}

const char* RTN_Name(RTN* rtn)
{
	fprintf(stderr,"addr %p size %d\n", rtn->addr, rtn->size);
	return rtn->name.c_str();
}

/* do nothing ... */
VOID RTN_Open(RTN* rtn)
{
    int fd = open(rtn->sec->image->path.c_str(), O_RDONLY, 0);
	INS prev_ins = NULL;
	for(uint64_t i = 0; i < rtn->size; i += 4) {
		uint32_t opcode;
		uint64_t ins_addr = rtn->addr + i - rtn->sec->image->load_base;
		pread(fd, &opcode, sizeof(opcode), ins_addr);
		//fprintf(stderr,"ins addr %p opcode %x\n",ins_addr,opcode);
		Ins * origin_ins = new Ins;
		//la_disasm(opcode, origla_disasmin_ins);
		INS INS = new pin_ins;
		INS->next = NULL;
		INS->prev = NULL;
		INS->pc = ins_addr;
		INS->opcode = opcode;
		INS->origin_ins = origin_ins;
		INS->first_ins = NULL;
		INS->last_ins = NULL;
		INS->len = 0;
		INS->ibefore_next_cb = NULL;
		INS->iafter_next_cb = NULL;
		if(i == 0) {
			INS->prev = NULL;
		} else {
			prev_ins->next = INS;
			INS->prev = prev_ins;
		}
		prev_ins = INS;
		rtn->instructions.insert(INS);
	}
}

VOID RTN_Close(RTN* rtn)
{
	rtn->instructions.clear();
}

INS RTN_InsHead(RTN* rtn)
{
	return *rtn->instructions.begin();
}
