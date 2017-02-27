DEST = ../bin/atomic_test
SRC = *.cpp
INCLUDE = -I ../..
LIBS = ../bin/comm.a
STDLIBS = -lpthread -ldl
IS_LIB = 0
IS_SHARED_LIB = 0


SRC2 = $(shell ls $(SRC))
OBJS = $(SRC2:.cpp=.o)


#IS_DEBUG = $(shell test -f ".debug" && echo 1)
ifeq ($(IS_DEBUG), 1)
	CC = g++ -Wall $(CPPFLAGS) -march=nocona -mno-sse3 -g -fmessage-length=0
else
	CC = g++ -Wall $(CPPFLAGS) -march=nocona -mno-sse3 -DNDEBUG -O3
endif
ifeq ($(IS_SHARED_LIB), 1)
	CC += -fPIC
endif


all: DELETE_DEPEND2 $(DEST) SUCCESS


.cpp.o:
	@echo $(<F)
	$(CC) -c $(INCLUDE) $(@D)/$(<F) -o $(@D)/$(@F)


$(DEST): $(LIBS) $(OBJS)
	@echo Linking ...
  ifeq ($(IS_LIB), 1)
		@rm -f $(DEST)
		@$(AR) $(ARFLAGS) $(DEST) $(OBJS) > /dev/null
  else
    ifeq ($(IS_SHARED_LIB), 1)
		@$(CC) -shared -o $(DEST) $(OBJS) $(LIBS) $(STDLIBS)
    else
		$(CC) -o $(DEST) $(OBJS) $(LIBS) $(STDLIBS)
    endif
    ifneq ($(IS_DEBUG), 1)
		@strip $(DEST)
    endif
  endif


$(LIBS): FORCE
	@make -C $(@D)

clean: DELETE_DEPEND2
	@rm $(OBJS) $(DEST) 2> /dev/null || true

cleanall: clean
	@rm .depend .depen2 2> /dev/null || true
	@for i in $(LIBS) ; do TMP="$${i%/*}"; make -C $$TMP clean || true; done
	@echo Ok

.DEFAULT: DELETE_DEPEND2
	@echo No rule to make target $@

DELETE_DEPEND2:
	@rm .depend2 2>/dev/null || true

FORCE:

SUCCESS:
	@echo Ok

dep:
	@if ! [ -f ".depend2" ]; then \
		echo Building dependencies for $(DEST)  ...; \
		$(CC) -MM -c $(INCLUDE) $(SRC2) | sed "s@^\(\(.*\).o: \(.*\)\2\.cpp\)@\3\1@" > .depend; \
		echo Ok; \
	else \
		rm .depend2; \
	fi

.depend:
	@echo Building dependencies for $(DEST)  ...
	@$(CC) -MM -c $(INCLUDE) $(SRC2) | sed "s@^\(\(.*\).o: \(.*\)\2\.cpp\)@\3\1@" > .depend
	@touch .depend2
	@echo Ok

-include .depend

