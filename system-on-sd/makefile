
all:
	mv   ~/aotu_test.c  ./  -f
	sync
	chmod 666 aotu_test.c
	arm-linux-gnueabihf-gcc   aotu_test.c    -o   aotu_test
	sync
	cp  aotu_test  ~/
	sync
	echo ====copy finish===
m:
	mv  ~/test_manage.c   ./  -f
	sync
	chmod 666 test_manage.c
	arm-linux-gnueabihf-gcc   test_manage.c    -o   manage_aotu
	cp    manage_aotu    ~/
	sync
	echo ====copy finish===

can:
	arm-linux-gnueabihf-gcc  cantest.c  -o  can_aotu
	cp   can_aotu  ~/
	sync
	echo  ====copy finish====
