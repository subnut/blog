MAKEFLAGS	+= --no-print-directory
OG_MAKEFILE	= makefile
.DEFAULT_GOAL	= all

ifneq (,$(findstring clean,$(MAKECMDGOALS)))
ifneq (,$(findstring all,$(MAKECMDGOALS)))
.NOTPARALLEL:
endif
endif

.DEFAULT:
	@$(MAKE) $(MAKEFLAGS) -f $(OG_MAKEFILE) $@
