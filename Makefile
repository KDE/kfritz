VERSION = $(shell grep 'static const char \*VERSION *=' KFritz.cpp | awk '{ print $$6 }' | sed -e 's/[";]//g')
WDIR    = $(shell pwd)
PODIR   = po
POTFILE = $(PODIR)/kfritz.pot
POFILES = $(wildcard $(PODIR)/*.po)


cmake:
	@if test ! -d build; \
	then mkdir -p build; \
	cd build; cmake ..; \
	fi

all: cmake $(POFILES) $(POTFILE)
	cd build; make 

clean:
	@-rm ../kfritz-${VERSION}.orig.tar.gz
	@-rm -rf build
	
dist: clean
	tar cvz --dereference \
	        --exclude-vcs --exclude="debian" --exclude=".settings" --exclude=".project" \
	        --exclude=".cproject" --exclude=".cdtproject" --exclude="test" \
	        -f ../kfritz_${VERSION}.orig.tar.gz ../kfritz

kde-install: all
	cd build; kdesudo make install

install:
	cd build; sudo make install

deb: dist
	debuild -i"(\.svn|\.settings|\.(c|cdt|)project|test)"

deb-src: dist
	debuild -S -i"(\.svn|\.settings|\.(c|cdt|)project|test)"
	
$(POTFILE): $(wildcard *.cpp) $(wildcard libfritz++/*.cpp) $(wildcard *.kcfg) $(wildcard *.rc) 
	xgettext --from-code=UTF-8 -C -kde -ci18n -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 -ktr2i18n:1\
	         -kI18N_NOOP:1 -kI18N_NOOP2:1c,2 -kaliasLocale -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3 \
	         --msgid-bugs-address="kfritz@joachim-wilke.de" --no-location -s \
	         -D ${WDIR} -o $(POTFILE) *.h *.cpp *.kcfg *.rc build/*.h libfritz++/*.cpp libfritz++/*.h
	grep -v POT-Creation $(POTFILE) > $(POTFILE)~
	mv $(POTFILE)~ $(POTFILE)
	         
po/%.po: $(POTFILE)
	msgmerge -U --no-wrap -s --backup=none -q $@ $<
	@touch $@
