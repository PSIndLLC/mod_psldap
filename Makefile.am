OBJS= mod_psldap.o
SRC= mod_psldap.c
DEFSSAVE="-D USE_LIBXML2_LIBXSL"
DEFS=@DEFS@
INCSAVE="-I /usr/include/libxml2 -I /usr/include/libxslt"
INC=@CPPFLAGS@
LIBSAVE="-lldap -llber -lxml2 -lxslt"
LIBS=@LIBS@

clean:
	rm -f mod_psldap.o mod_psldap.lo mod_psldap.la mod_psldap.so

install: mod_psldap.la
	$(APXS) -i -A -n psldap mod_psldap.la

mod_psldap.la: $(SRC)
	$(APXS) $(INC) $(DEFS) -c $(SRC) $(LIBS)
	@echo "Edit your http config to remove the comment around the module config"

mod_psldap.so: $(SRC) Makefile
	$(APXS) -o $@ -c $(DEFS) $(INC) $(LIB) $(SRC)

mod_psldap.o: mod_psldap.c

