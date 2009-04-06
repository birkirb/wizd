
all:
	(cd source; make all)
	-rm -f wizd && ln -s source/wizd

inetd:
	(cd source; make wizd_inetd)
	-rm -f wizd_inetd && ln -s source/wizd_inetd

clean:
	(cd source; make clean)
