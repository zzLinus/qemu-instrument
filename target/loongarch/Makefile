SSH="ssh"
PORT = "2225"
SERV_USERNAME="root"
SERV_IP="www.yongxiang.work"
#SOT="/opt/devel/pintest"
#SOT="/opt/devel/thread"
#SOT="/opt/devel/pintest-c"
SOT="/opt/devel/fork_app"
#SOT="/opt/devel/coremark/coremark.exe"
#SOT="/usr/bin/ls"
RES_FILE=".res"
SHELL=bash

SERV_QEMU="/opt/devel/qemu-instrument/build/qemu-loongarch64"
SERV_PIN="/opt/devel/qemu-instrument/build/target/loongarch/pin/pintools/"

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
END='\033[0m'

NAMES :=
#NAMES += template
#NAMES += ins_count           # (x)
#NAMES += mem_trace           # (x)
#NAMES += isampling           # (x)
#NAMES += bbl_count           # (x)
#NAMES += hotblock
#NAMES += inline_add
#NAMES += strace              # (x)
#NAMES += routine             # (x)
#NAMES += bcc
#NAMES += reg_frequency
#NAMES += time_in_code_cache
#NAMES += branch_count
#NAMES += branch_offset
#NAMES += champsim
#NAMES += champsim_full
#NAMES += champsim_slice
#NAMES += champsim_multi
#NAMES += staticicount
#NAMES += buffer_linux
#NAMES += imageload
#NAMES += inscount_tls
#NAMES += nonstatica
NAMES += fork_jit_tool

SONAMES := $(addsuffix .so,$(addprefix lib,$(NAMES)))

.PHONY: all

all: res $(SONAMES) clean

res:
	@touch $(RES_FILE)

mkall: mkqemu mkpin

mkqemu:
	@echo -e +$(GREEN)CC$(END) BUILD qemu
	 #@$(SSH) -p $(PORT) $(SERV_USERNAME)@$(SERV_IP) make -C /opt/devel/qemu-instrument/build clean
	@$(SSH) -p $(PORT) $(SERV_USERNAME)@$(SERV_IP) make -C /opt/devel/qemu-instrument/build -j

mkpin:
	@echo -e +$(GREEN)CC$(END) BUILD pin-tools
	#@$(SSH) -p $(PORT) $(SERV_USERNAME)@$(SERV_IP) make -C /opt/devel/qemu-instrument/build/target/loongarch/pin/pintools/ clean
	@$(SSH) -p $(PORT) $(SERV_USERNAME)@$(SERV_IP) make -C /opt/devel/qemu-instrument/build/target/loongarch/pin/pintools/ -j 

mvsrc:
	@echo -e + $(RED)RM$(END) REMOTE LOONGARCH DIR
	@$(SSH) -p $(PORT) $(SERV_USERNAME)@$(SERV_IP) rm -rf /opt/devel/qemu-instrument/target/loongarch
	@echo -e + $(BLUE)MV$(END) LOCAL LOONGARCH DIR TO REMOTE
	@scp -P $(PORT) -r qemu-instrument/target/loongarch $(SERV_USERNAME)@$(SERV_IP):/opt/devel/qemu-instrument/target/

lib%.so:
	@echo -e +EXEC PINTOOLS : $(BLUE)$@$(END)
	@$(SSH) -p $(PORT) $(SERV_USERNAME)@$(SERV_IP) $(SERV_QEMU) -t $(SERV_PIN)$@ $(SOT) \
		&&  echo -e $(GREEN)success!$(END) $(BLUE)$@$(END) >> $(RES_FILE) \
		|| echo -e $(RED)failure!$(END) $(BLUE)$@$(END) >> $(RES_FILE)

clean:
	@cat $(RES_FILE)
	@rm $(RES_FILE)
