#include <u.h>
#include <libc.h>
#include <bio.h>

void updateled(void);
void procstatus(int, int, int);

int accset[4] = { 1, 2, 3, 4 };
uchar buf[16][8];

void
main(int argc, char *argv[]) {
	int kmode;
	int fd, cfd;
	int pid;
	char *path;

	kmode = 0;
	pid = 0;
	fd = -1;
	memset(buf, 0xff, 16*8);
	if(argc == 2 && strcmp(argv[1], "-k") == 0) {
		kmode = 1;
		fd = open("#L/ledmat", ORDWR);
		if(fd < 0) {
			fprint(2, "Kernel driver failure: %r\n");
			exits("kernel driver");
		}
		fprint(fd, "start\n");
	}
	else {
		pid = rfork(RFPROC|RFMEM);
		if(pid == 0) {
			updateled();
			exits(nil);
		}
		path = smprint("/proc/%d/ctl", pid);
		cfd = open(path, OWRITE);
		if(cfd < 0) {
			fprint(2, "child proc ctl: %r\n");
			exits(nil);
		}
		free(path);
		fprint(cfd, "period 1ms");
		fprint(cfd, "cost 1ms");
		if(fprint(cfd, "admit") < 0) {
			fprint(2, "real time failed: %r\n");
		}
	}
	print("ready\n");
	procstatus(kmode, fd, pid);
}

void
setpixel(int x, int y, int v) {
	int row, col, bit;

	row = y;
	col = x / 8;
	bit = 7 -(x % 8);
	if(v) {
		buf[row][col] &= ~(1 << bit);
	}
	else {
		buf[row][col] |= 1 << bit;
	}
}

int
isvis(int unit) {
	int i;

	for(i = 0; i < 4; i++) {
		if(unit == accset[i])
			return i;
	}
	return -1;
}

void
procstatus(int kmode, int fd, int pid) {
	Biobuf bbuf;
	int cfd;
	int unit, dig, val, pos;
	int i;
	char *fn, *updstr;
	char *toks[5];

	Binit(&bbuf, 0, OREAD);
	while(1) {
		updstr = Brdstr(&bbuf, '\n', 1);
		if(updstr == nil)
			break;
		tokenize(updstr, toks, 5);
		if(strcmp(toks[0], "ad") == 0) {
			unit = atoi(toks[1]);
			pos = isvis(unit);
			if(pos == -1)
				continue;
			dig = atoi(toks[2]);
			val = atoi(toks[3]);
			for(i = 0; i < 10; i++) {
				if(kmode) {
					fprint(fd, "s %d %d %d\n", pos*16+2+dig, 10 - i, val == i);
				}
				else {
					setpixel(pos*16+2+dig, 10 - i, val == i);
				}
			}
		}
		else if(strcmp(toks[0], "ac") == 0) {
			unit = atoi(toks[1]);
			pos = isvis(unit);
			if(pos == -1)
				continue;
			dig = atoi(toks[2]);
			val = atoi(toks[3]);
			if(kmode) {
				fprint(fd, "s %d 14 %d\n", pos*16+2+dig, val);
			}
			else {
				setpixel(pos*16+2+dig, 14, val);
			}
		}
		else if(strcmp(toks[0], "L") == 0) {
			accset[0] = atoi(toks[1]);
			accset[1] = atoi(toks[2]);
			accset[2] = atoi(toks[3]);
			accset[3] = atoi(toks[4]);
			memset(buf, 0xff, 16*8);
			print("refresh\n");
		}
		free(updstr);
	}
	if(kmode == 0) {
		fn = smprint("/proc/%d/ctl", pid);
		cfd = open(fn, OWRITE);
		fprint(cfd, "kill");
	}
	else {
		fprint(fd, "stop\n");
	}
	exits(nil);
}

/*
 * current wiring
 * GPIO	LED
 * 17		LA
 * 27		LB
 * 22		LC
 * 18		LD
 * 24		LAT
 * 25		EN
 *
 * SPI
 * SCLK	CLK
 * MOSI	R1/G1
 */
void
updateled() {
	int fdg, spictl, spidat;
	int row;

	fdg = open("#G/gpio", OWRITE);
	if(fdg < 0) {
		perror("gpio open");
		exits(nil);
	}
	/* Just to be safe in case they got overridden */
	fprint(fdg, "function 10 alt0\n");
	fprint(fdg, "function 11 alt0\n");
	spictl = open("#π/spictl", ORDWR);
	spidat = open("#π/spi0", ORDWR);
	if(spictl < 0 || spidat < 0) {
		perror("spi open");
		exits(nil);
	}
	fprint(fdg, "function 17 out\n");
	fprint(fdg, "function 27 out\n");
	fprint(fdg, "function 22 out\n");
	fprint(fdg, "function 18 out\n");
	fprint(fdg, "function 24 out\n");
	fprint(fdg, "function 25 out\n");
	row = 0;
	while(1) {
		pwrite(spidat, buf[row], 8, 0);
		fprint(fdg, "set 25 1\n");
		fprint(fdg, "set 24 1\n");
		fprint(fdg, "set 17 %d", row & 01);
		fprint(fdg, "set 27 %d", (row >> 1) & 01);
		fprint(fdg, "set 22 %d", (row >> 2) & 01);
		fprint(fdg, "set 18 %d", (row >> 3) & 01);
		fprint(fdg, "set 25 0\n");
		fprint(fdg, "set 24 0\n");
		row = (row + 1) & 0xf;
		sleep(0);
	}
}
