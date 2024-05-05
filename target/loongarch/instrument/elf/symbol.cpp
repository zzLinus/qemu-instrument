#include "symbol.h"
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <limits>
#include "string.h"
#include "../util/error.h"


// NOTE: SEC section
struct compare_symbol_info {
    bool operator()(const symbol_info *lhs, const symbol_info *rhs) const
    {
        if (lhs->addr != rhs->addr) {
            return lhs->addr < rhs->addr;
        } else {
            return lhs->size < rhs->size;
        }
    };
};

struct image {
    std::string path;
    uintptr_t load_base;
    std::set<symbol_info *, compare_symbol_info> symbols;   /* symbols are ordered by addr and size */
    std::unordered_map<std::string, symbol_info *> symbol_name_map;

    image(const char *path, uintptr_t load_base) : path(path), load_base(load_base)
    {
    }
};

static std::vector<image *> images;
static std::set<symbol_info *, compare_symbol_info> all_symbols;                        /* 所有的符号信息 */
static std::unordered_map<std::string, std::vector<symbol_info *>> debug_dup_symbols;   /* 名字重复的符号信息 */

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

void is_sym_in_sec(image* img,uint64_t start,uint64_t size){
	symbol_info key = { .addr = start, .size = std::numeric_limits<uint64_t>::max() };
	auto iter = all_symbols.lower_bound(&key);
	if(iter == all_symbols.end())
		return;
	for(auto sym  = *iter;sym->addr <= start + size && iter != prev(all_symbols.end());sym = *(++iter)){
		if(sym->image == img){
			fprintf(stderr, "addr %p %s %d\n",sym->addr,sym->name,sym->size);
		}
	}
}

void image_add_symbol(image *img, const char * name, uint64_t addr, uint64_t size)
{
    /* alloc a symbol */
    symbol_info *symbol = (symbol_info *)malloc(sizeof(symbol_info));
    symbol->image = img;
    symbol->name = strdup(name);    /* FIXME: memory leak, free it somewhere */
    symbol->addr = addr;
    symbol->size = size;

    /* fprintf(stderr, "[debug] Find a symbol. image: %s, symbol: %s, addr: 0x%lx, size: 0x%lx\n", img->path.c_str(), name, addr, size); */
    if (addr == 0 || size == 0) {
        lsdebug("Find a symbol with invalid addr or size. name: %s, addr: 0x%lx, size: 0x%lx\n", name, addr, size);
    }

    /* check duplicate */
    if (img->symbol_name_map.count(name) != 0) {
        lsdebug("Duplicated symbol name in the same image. image: %s, symbol: %s, addr: %lu, size: %lu\n", img->path.c_str(), name, addr, size);
        /* 注：这个重复的列表是不完整的，同一个image内，相同name的最后一个符号没有被加入到该列表 */
        debug_dup_symbols[name].push_back(img->symbol_name_map[name]);
    }

    /* collect */
    img->symbols.insert(symbol);
    img->symbol_name_map[name] = symbol;
    all_symbols.insert(symbol);
}

symbol_info *image_get_symbol_by_name(image *img, const char *name)
{
    if (img->symbol_name_map.count(name) == 0) {
        return NULL;
    }
    return img->symbol_name_map[name];
}

/* pc -> symbol_info */
/* 假设：所有symbol不重叠，因此至多一个symbol满足 */
symbol_info *get_symbol_by_pc(uint64_t pc)
{
    symbol_info key = { .addr = pc, .size = std::numeric_limits<uint64_t>::max() };
    auto iter = all_symbols.upper_bound(&key);
    if (iter != all_symbols.begin()) {
        symbol_info *sym = *prev(iter);
        if (sym->addr <= pc && pc < sym->addr + sym->size) {
            return sym;
        }
    }
    return NULL;
}

const char *get_symbol_name_by_pc(uint64_t pc)
{
    const char *name = "";
    symbol_info *sym = get_symbol_by_pc(pc);
    if (sym) {
        name = sym->name;
    }
    return name;
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
    const symbol_info *last_symbol = NULL;

    printf("Print all symbols:\n");
    for (const image *img : images) {
        printf("Image: %s (%lu symbols)\n", img->path.c_str(), img->symbols.size());
        printf("ADDR    SIZE    NAME\n");
        for (const symbol_info *symbol : img->symbols) {
            printf("0x%-16lx    0x%-16lx    %s\n", symbol->addr, symbol->size, symbol->name);

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
            for (symbol_info *sym : p.second) {
                printf("0x%-16lx    0x%-16lx    %s\n", sym->addr, sym->size, sym->image->path.c_str());
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

RTN RTN_FindByName(IMG img, const CHAR *name)
{
    symbol_info *sym = image_get_symbol_by_name((image *)img, name);
    if (sym == NULL || sym->addr == 0 || sym->size == 0) {
        return {0, 0, 0};
    }
    if (is_symbol_name_dupcalited(name)) {
        lswarn("symbol %s is duplicated in image.\n", name);
    }
    return RTN_alloc(name, sym->addr, sym->size);
}

BOOL RTN_Valid(RTN x)
{
    return (x.name != NULL && x.addr != 0 && x.size != 0);
}

/* do nothing ... */
VOID RTN_Open(RTN rtn)
{
}

VOID RTN_Close(RTN rtn)
{
}

RTN RTN_alloc(const char *name, uint64_t addr, uint64_t size)
{
    return (RTN) {
        .name = name,
        .addr = addr,
        .size = size
    };
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
    lsassert(strcmp(lhs.name, rhs.name) == 0);
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

VOID RTN_add_entry_cb(RTN rtn, ANALYSIS_CALL *cb)
{
    rtn_entry_cbs[rtn.addr].emplace_back(*cb);
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

VOID RTN_add_exit_cb(RTN rtn, ANALYSIS_CALL *cb)
{
    rtn_exit_cbs[rtn].emplace_back(*cb);
}

ANALYSIS_CALL *RTN_get_exit_cbs(uintptr_t pc, int *cnt)
{
    /* find a rtn which contains the pc */
    /* FIXME 目前假设所有rtn不重叠，因此pc只出现在一个rnt的范围中 */
    RTN key = { .addr = pc, .size = std::numeric_limits<uint64_t>::max() };
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
