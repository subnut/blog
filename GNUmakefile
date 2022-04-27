MAKEFLAGS	+= --jobs
MAKEFLAGS	+= --no-print-directory
.DEFAULT_GOAL	= all

ifneq (,$(findstring clean,$(MAKECMDGOALS)))
ifneq (,$(findstring all,$(MAKECMDGOALS)))
.NOTPARALLEL:
endif
endif

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
MAKEFLAGS	+= --jobs 1
endif

ifeq (,$(wildcard makefile))
include Makefile
.NOTPARALLEL:
endif

.DEFAULT:
	@$(MAKE) -f makefile $(MAKEFLAGS) $@
