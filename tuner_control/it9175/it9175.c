/* fsusb2i   (c) 2015-2016 trinity19683
  IT9175 USB interface, tuner, demod
  it9175.c
  2016-02-18
*/

#include <errno.h>
#include <string.h>
#include <malloc.h>

#include "usbops.h"
#include "osdepend.h"
#include "it9175.h"
#include "it9175_usb.h"
#include "it9175_priv.h"
#include "message.h"

#define TS_BulkSize  305

#define ARRAY_SIZE(x)  (sizeof(x)/(sizeof(x[0])))

/* write multiple registers */
static int writeRegs(struct state_st* const s, const uint32_t reg, const uint8_t* const val, const int len)
{
	if(6 + len > MAX_XFER_SIZE - REQ_HDR_LEN - CHECKSUM_LEN) {
		warn_info(0,"too long data");
		return -EINVAL;
	}

	s->buf[REQ_HDR_LEN   ] = len;
	s->buf[REQ_HDR_LEN +1] = 2;
	s->buf[REQ_HDR_LEN +2] = 0;
	s->buf[REQ_HDR_LEN +3] = 0;
	s->buf[REQ_HDR_LEN +4] = (reg >> 8) & 0xFF;
	s->buf[REQ_HDR_LEN +5] = reg & 0xFF;
	memcpy(&s->buf[REQ_HDR_LEN +6], val, len);
	return it9175_ctrl_msg(s, CMD_MEM_WR, (reg >> 16) & 0xFF, 6 + len, 0);
}

/* read multiple registers */
static int readRegs(struct state_st* const s, const uint32_t reg, uint8_t* const val, const int len)
{
	int r;
	s->buf[REQ_HDR_LEN   ] = len;
	s->buf[REQ_HDR_LEN +1] = 2;
	s->buf[REQ_HDR_LEN +2] = 0;
	s->buf[REQ_HDR_LEN +3] = 0;
	s->buf[REQ_HDR_LEN +4] = (reg >> 8) & 0xFF;
	s->buf[REQ_HDR_LEN +5] = reg & 0xFF;

	r = it9175_ctrl_msg(s, CMD_MEM_RD, (reg >> 16) & 0xFF, 6, len);
	if(0 == r && NULL != val) {
		memcpy(val, &s->buf[ACK_HDR_LEN], len);
	}
	return r;
}

/* write single register */
static int writeReg(struct state_st* const s, const uint32_t reg, const uint8_t val)
{
	return writeRegs(s, reg, &val, 1);
}

/* read single register */
static int readReg(struct state_st* const s, const uint32_t reg, uint8_t* const val)
{
	return readRegs(s, reg, val, 1);
}

/* write single register with mask */
static int writeRegMask(struct state_st* const s, const uint32_t reg, const uint8_t val, const uint8_t mask)
{
	int r;
	uint8_t tmp;

	if(0 == mask || 0xFF == mask) {
		return writeRegs(s, reg, &val, 1);
	}

	r = readRegs(s, reg, &tmp, 1);
	if(r)
		return r;
	tmp = (val & mask) | (tmp & ~mask);

	return writeRegs(s, reg, &tmp, 1);
}

/* obtain chip version, type, firmware status */
static int it9175_identify_state(struct state_st* const s)
{
	int ret;
	const uint8_t* const rbuf = &s->buf[ACK_HDR_LEN];
	uint16_t chip_type, chip_version;

	if((ret = readRegs(s, 0x1222, NULL, 3))) goto err1;
	chip_version = rbuf[0];  //# chip version
	chip_type = rbuf[2] << 8 | rbuf[1];

	if((ret = readReg(s, 0x384f, NULL))) goto err1;
	chip_version |= rbuf[0] << 8;  //# prechip version

	s->chip_id = chip_type << 16 | chip_version;
	if(0x91758301 == s->chip_id) {
		//# ITE IT9175FN, FSUSB2i
	}else{
		warn_msg(0,"unknown chip=%04X_%04X",chip_type,chip_version);
		return -1;
	}

	//# firmware info
	s->buf[REQ_HDR_LEN] = 1;
	if((ret = it9175_ctrl_msg(s, CMD_FW_QUERYINFO, 0, 1, 4))) goto err1;
	s->fw_info = (rbuf[3] << 24) | (rbuf[2] << 16) | (rbuf[1] << 8) | rbuf[0];

	dmsg("Chip= %04X_%04X, FWinfo= %08X", chip_type, chip_version, s->fw_info);

	return 0;
err1:
	return ret;
}

#define opr_div(A,B,C)  {C = A / B; A = A % B;}

/* read config, EEPROM address, demod clock */
static int it9175_read_config(struct state_st* const st)
{
	int ret;
	unsigned int ia, ib;
	uint8_t utmp;

	if((ret = readReg(st, 0x461c, &utmp))) goto err1;
	if(0 == utmp) {
		char msgbuf[56];
		const uint8_t* const prbuf = &st->buf[ACK_HDR_LEN];
		warn_info(0,"Invalid EEPROM");
		for(ia = 0; ia < 0x100; ia += 16) {
			st->buf[REQ_HDR_LEN   ] = 16;
			st->buf[REQ_HDR_LEN +1] = I2C_ADDR_EEPROM;
			st->buf[REQ_HDR_LEN +2] = 2;
			st->buf[REQ_HDR_LEN +3] = (ia >> 8) & 0xFF;
			st->buf[REQ_HDR_LEN +4] = ia & 0xFF;
			if((ret = it9175_ctrl_msg(st, CMD_CFG_RD, 0, 5, 16 ))) continue;
			dumpHex(msgbuf, sizeof(msgbuf), ia, prbuf, 16);
			warn_msg(0,msgbuf);
		}
		for(ia = 0; ia < 0x100; ia += 16) {
			if((ret = readRegs(st, EEPROM_BASE_IT9175 - 8 + ia, NULL, 16 ))) goto err1;
			dumpHex(msgbuf, sizeof(msgbuf), ia, prbuf, 16);
			warn_msg(0,msgbuf);
		}
		//# Assume the FSUSB2i device config
		st->tunerID = 0x70;

	}else{  //# read config from EEPROM
		if((ret = readReg(st, EEPROM_BASE_IT9175 + EEPROM_IR_MODE, &utmp))) goto err1;
		dmsgn("IR=%02X, ",utmp);
		if((ret = readReg(st, EEPROM_BASE_IT9175 + EEPROM_TS_MODE, &utmp))) goto err1;
		dmsgn("TS=%02X, ",utmp);
		if((ret = readReg(st, EEPROM_BASE_IT9175 + EEPROM_1_TUNER_ID, &utmp))) goto err1;
		st->tunerID = utmp;
		dmsgn("Tuner=%02X, ",utmp);
		if((ret = readReg(st, EEPROM_BASE_IT9175 + EEPROM_1_TUNER_IF, &utmp))) goto err1;
		dmsgn("TunerIF=%02X, ",utmp);
	}

	//# get device clock
	if((ret = readReg(st, 0xd800, &utmp))) goto err1;
	ia = clock_lut_dev[utmp & 0xF];
	opr_div(ia,1000,ib);
	dmsgn("DevCLK=%u.%03uMHz, ",ib,ia);

	return 0;
err1:
	return ret;
}

/* download firmware */
static int it9175_loadFW(struct state_st* const s)
{
#include "it9175_fw.h"
	const uint8_t *ptr = it9179_fw1;
	int ret;
	uint8_t* const p = &s->buf[REQ_HDR_LEN];
	const unsigned int max_datalen = 44;

	//# I2C bus clock 366 kHz
	if((ret = writeReg(s, 0xf103, 0x07))) goto err1;

	do {
		const uint8_t bankidx = *ptr;
		unsigned int bnum = 0, bfill = 0, addr = 0, blocksize = 0, idx = 0;
		const uint8_t *dptr[3];
		ptr++;
		do {
			if(0 == idx) {
				blocksize = (ptr[0] << 8) | ptr[1];
				addr = (ptr[2] << 8) | ptr[3];
				ptr += 4;
			}
			dptr[bnum] = &ptr[idx];
			{//#
				const unsigned int addr2 = addr + idx;
				const unsigned int remainBf = max_datalen - bfill;
				const unsigned int remainBl = blocksize - idx;
				p[(bnum *3) +4] = (addr2 >> 8) & 0xFF;
				p[(bnum *3) +5] = addr2 & 0xFF;
				if(remainBl + 3 <= remainBf) {
					p[(bnum *3) +6] = remainBl;
					bfill += remainBl + 3;
					idx = blocksize;
				}else{
					p[(bnum *3) +6] = remainBf - 3;
					bfill = max_datalen;
					idx += remainBf - 3;
				}
			}
			bnum++;
			if(idx >= blocksize) {
				idx = 0;
				ptr += blocksize;
				if(0 == ptr[0] && 0 == ptr[1]) {
					blocksize = 0;
				}
			}
			if(max_datalen <= bfill || 3 <= bnum || 0 == blocksize) {
				p[0] = 0x3;
				p[1] = bankidx;
				p[2] = 0;
				p[3] = bnum;
				bfill = (bnum *3) +4;
				do {
					const unsigned int bsize = p[(bnum *3) +3];
					bnum--;
					memcpy(&p[bfill], dptr[bnum], bsize);
					bfill += bsize;
				} while(0 != bnum);
				//# send
				if((ret = it9175_ctrl_msg(s, CMD_FW_SCATTER_WR, 0, bfill, 0))) goto err1;
				bfill = 0;
			}
		} while(0 != blocksize);
		ptr += 2;
	} while(0 != *ptr);

	//# firmware reboot
	if((ret = it9175_ctrl_msg(s, CMD_FW_BOOT, 0, 0, 0))) goto err1;
	if((ret = it9175_identify_state(s))) goto err1;
	else if(0 == s->fw_info) {
		return -1;
	}

	return 0;
err1:
	return ret;
}

/* device sleep, power save mode */
static int it9175_sleep(struct state_st* const st)
{
	int ret, i;
	uint8_t buf[15] = { 0 };

	if((ret = writeRegMask(st, 0x80fbb9, 0   , 0x20 ))) goto err1;
	if((ret = writeReg(st, 0xe00c, 0x1))) goto err1;
	if((ret = writeRegMask(st, 0x80fbb9, 0x20, 0x20 ))) goto err1;
	//# suspend demod
	if((ret = writeReg (st, 0x80004c, 0x1))) goto err1;
	if((ret = writeReg (st, 0x800000, 0))) goto err1;
	miliWait(30);
	for(i = 0; i < 20; i++) {
		if((ret = readReg(st, 0x80004c, buf))) goto err1;
		if(0 == buf[0]) break;
		miliWait(25);
	}
	if((ret = writeReg (st, 0x80fb24, 0x8))) goto err1;
	//# tuner clock off
	if((ret = writeReg (st, 0x80fba8, 0))) goto err1;
	//# power off tuner
	if((ret = writeReg (st, 0x80ec40, 0))) goto err1;
	buf[0] = 0;
	buf[1] = 0x0c;
	if((ret = writeRegs(st, 0x80ec02, buf, 15))) goto err1;
	buf[1] = 0;
	if((ret = writeRegs(st, 0x80ec12, buf, 4))) goto err1;
	if((ret = writeRegs(st, 0x80ec17, buf, 9))) goto err1;
	if((ret = writeRegs(st, 0x80ec22, buf, 10))) goto err1;
	if((ret = writeReg (st, 0x80ec20, 0))) goto err1;
	if((ret = writeReg (st, 0x80ec3f, 0x1))) goto err1;

	return 0;
err1:
	return ret;
}

static unsigned int it9175_div(unsigned int a, const unsigned int b, const int sh)
{
#if 0
	unsigned int c, r = 0, i = 0;
	if(a > b) {
		opr_div(a,b,c);
	}else{
		c = 0;
	}
	while(0 != a) {
		unsigned int j = __builtin_clz(a) - __builtin_clz(b);
		j = (sh - i > j) ? j : sh - i;
		a <<= j;
		r <<= j;
		if(a >= b) {
			a -= b;
			r |= 1;
		}
		i += j;
		if(i >= sh) break;
	}
	return (c << sh) + r;
#endif
	return ((uint64_t)a << sh) / b;
}

struct reg_val_mask {
	uint32_t reg;
	uint8_t val;
	uint8_t mask;
};

static int writeRegTable(struct state_st* const st, const struct reg_val_mask* const tab, const unsigned int len)
{
	int ret = 0;
	unsigned int i;
	for(i = 0; i < len; i++) {
		if((ret = writeRegMask(st, tab[i].reg, tab[i].val, tab[i].mask))) break;
	}
	return ret;
}

static int writeRegData(struct state_st* const st, const uint8_t* const init_tab)
{
	int ret = 0;
	const uint8_t *p = init_tab;
	while(0 != p[0]) {
		if((ret = writeRegs(st, 0x800000 | (p[2] << 8) | p[1], &p[3], p[0]))) break;
		p += p[0] + 3;
	}
	return ret;
}

/* tuning freq. boundaries */
static uint8_t getFreqNdiv(const uint32_t freq, uint8_t* const n_div)
{
	int i;
	static unsigned  freq_upp[] = { 78200, 117300, 156400, 234600, 312800, 469200, 625600, 950000};
	const uint8_t ndiv_tab[] = {48, 32, 24, 16, 12, 8, 6, 4, 2};
	if( n_div && *n_div ) {  //# set boundaries
		dmsgn("freq_bd= ");
		for(i = 0; i < *n_div; i++) {
			const unsigned  freq_bd = (freq / ndiv_tab[i + 1]) >> 2;
			freq_upp[i] = freq_bd;
			dmsgn("%u,",freq_bd);
		}
		dmsgn("kHz ");
		return 0;
	}
	for(i = 0; i < ARRAY_SIZE(freq_upp); i++) {
		if(freq_upp[i] >= freq)  break;
	}
	if( n_div ) {
		*n_div = ndiv_tab[i];
	}
	return i & 0xFF;
}

/* initialize tuner, demod, USB interface */
static int it9175_tuner_init(struct state_st* const st)
{
	int ret;
	uint8_t rbuf[4];
	unsigned int iva, ivb;

	//# I2C bus clock 100 kHz
	if((ret = writeReg(st, 0xf103, 0x1a))) goto err1;
	//# set 2nd demod I2C address = NULL
	if((ret = writeReg(st, 0x4bfb, 0))) goto err1;
	//# disable clock out
	if((ret = writeReg(st, 0xcfff, 0))) goto err1;
	//# set tuner ID
	if((ret = writeReg(st, 0xf641, st->tunerID))) goto err1;

	{//#
		const struct reg_val_mask init1_mtab[] = {
			{0x80ec4c, 0xa8, 0},
			{0xd8c8, 0x01, 0x01},
			{0xd8c9, 0x01, 0x01},
			{0xd8bc, 0x01, 0x01},
			{0xd8bd, 0x01, 0x01},
			{0xe00c, 0   , 0},
			{0xd864, 0   , 0},
			{0x80f5ca, 0x01, 0x01},
			{0x80f715, 0x01, 0x01},
		};
		if((ret = writeRegTable(st, init1_mtab, ARRAY_SIZE(init1_mtab)))) goto err1;
	}

	//# program clock control = 12 MHz
	iva = it9175_div(12000000, 1000000, 19);
	rbuf[0] = iva & 0xFF;  rbuf[1] = (iva >> 8) & 0xFF;
	rbuf[2] = (iva >> 16) & 0xFF;  rbuf[3] = (iva >> 24) & 0xFF;
	if((ret = writeRegs(st, 0x800025, rbuf, 4))) goto err1;
	//# program ADC control = 20.25 MHz
	iva = it9175_div(20250000, 1000000, 19);
	rbuf[0] = iva & 0xFF;  rbuf[1] = (iva >> 8) & 0xFF;
	rbuf[2] = (iva >> 16) & 0xFF;  rbuf[3] = (iva >> 24) & 0xFF;
	if((ret = writeRegs(st, 0x80f1cd, rbuf, 3))) goto err1;

	//# init deMod step1
	if((ret = writeRegData(st, inittab_1))) goto err1;
	//# init deMod step2
	if((ret = writeRegData(st, inittab_2))) goto err1;

	if((ret = writeReg(st, 0x80004e, 0))) goto err1;
	if((ret = writeReg(st, 0x800000, 0x1))) goto err1;
	miliWait(30);
	if((ret = writeReg(st, 0xd827, 0))) goto err1;
	if((ret = writeReg(st, 0xd829, 0))) goto err1;

	//# get deMod clock
	if((ret = readReg(st, 0x80ec86, rbuf))) goto err1;
	st->clock_d = rbuf[0];
	dmsgn("demodCLK=mode%u, ", st->clock_d );
	switch( rbuf[0] ) {
	case 0: //# 12.000 MHz
		st->xtal = 2000;
		st->fdiv = 3;
		rbuf[3] = 16;
		break;
	case 1: //# 20.480 MHz
		st->xtal = 20480;
		st->fdiv = 18;
		rbuf[3] = 6;
		break;
	default: //# 20.480 MHz
		st->xtal = 640;
		st->fdiv = 1;
		rbuf[3] = 6;
		warn_info(0,"unknown demodCLK mode %02X",rbuf[0]);
		break;
	}


	if((ret = readReg(st, 0x80ed03, &rbuf[2] ))) goto err1;
	miliWait(10);
	for(iva = 0; iva < 15; iva++) {
		if((ret = readRegs(st, 0x80ed23, rbuf, 2))) goto err1;
		ivb = (rbuf[1] << 8) | rbuf[0];
		if(0 != ivb) break;
		miliWait(5);
	}
	//# set
	getFreqNdiv( ((ivb << 2) * st->xtal) / st->fdiv, &rbuf[2] );

	miliWait(20);
	for(iva = 0; iva < 10; iva++) {
		if((ret = readReg(st, 0x80ec82, rbuf))) goto err1;
		if(0 != rbuf[0]) break;
		miliWait(10);
	}
	//# iqik_m_cal
	if((ret = writeReg(st, 0x80ed81, rbuf[3]))) goto err1;

	if((ret = readReg(st, 0x8001dc, rbuf))) goto err1;
	if(1 != rbuf[0]) {
		warn_info(0,"deMod config error %02X",rbuf[0]);
		goto err2;
	}
	{//#
		const struct reg_val_mask init2_mtab[] = {
			{0xd8d0, 0x01, 0x01},
			{0xd8d1, 0x01, 0x01},
			{0xf41f, 0x04, 0x04},
			{0x80f990, 0x00, 0x01},   //# stream full speed = F
			{0xf41a, 0x01, 0x01},
			{0x80f985, 0x00, 0x01},   //# serial mode = F
			{0x80f986, 0x00, 0x01},   //# parallel mode = F
			{0xd91c, 0x00, 0x01},
			{0xd91b, 0x00, 0x01},
		};
		if((ret = writeRegTable(st, init2_mtab, ARRAY_SIZE(init2_mtab)))) goto err1;
	}
	//# bandwidth mode
	st->bw_mode = 0;
	if((ret = writeRegMask(st, 0x80cfff, st->bw_mode, 0x03))) goto err1;
	if((ret = writeRegMask(st, 0xcfff, st->bw_mode, 0x03))) goto err1;
	{//#
		const struct reg_val_mask init3_mtab[] = {
			{0x80f99d, 0x01, 0x01},   //# stream1 reset = T
			{0x80f9a4, 0x01, 0x01},   //# stream2 reset = T
			{0xdd11, 0x00, 0x60},   //# USB endpoint4,5 enable = F,F
			{0xdd13, 0x00, 0x60},   //# NACK = F,F
			{0xdd11, 0x20, 0x60},   //# enable = T,F
		};
		if((ret = writeRegTable(st, init3_mtab, ARRAY_SIZE(init3_mtab)))) goto err1;
	}
	{//# frame size, packet size= 512 /4 = 128
		const unsigned TS_FrameSize = TS_BulkSize * 188 / 4;
		rbuf[0] = TS_FrameSize & 0xFF;
		rbuf[1] = (TS_FrameSize >> 8) & 0xFF;
		if((ret = writeRegs(st, 0xdd88, rbuf, 2))) goto err1;
		if((ret = writeReg(st, 0xdd0c, 128))) goto err1;
	}
	{//#
		const struct reg_val_mask init4_mtab[] = {
			{0x80f985, 0x00, 0x01},   //# serial mode = F
			{0x80f986, 0x00, 0x01},   //# parallel mode = F
			{0x80f9a3, 0x00, 0x01},   //# stream2 disable
			{0x80f9cd, 0x00, 0x01},   //# TS disable
			{0x80f99d, 0x00, 0x01},   //# stream1 reset = F
			{0x80f9a4, 0x00, 0x01},   //# stream2 reset = F
			{0xd8fd, 0x01, 0},
			{0xd833, 0x01, 0},
			{0xd830, 0x00, 0},
			{0x80ec57, 0, 0},
			{0x80ec58, 0, 0},
			{0x80ec40, 0x01, 0},
			{0xd8b8, 0x01, 0},
			{0xd8b9, 0x01, 0},
			{0xd831, 0x00, 0},
			{0xd832, 0x00, 0},
			{0x80f992, 0x01, 0x01},   //# PID table reset
			{0x80f991, 0x01, 0x01},   //# PID filter = except
			{0x80f993, 0x01, 0x01},   //# PID filter = T
		};
		if((ret = writeRegTable(st, init4_mtab, ARRAY_SIZE(init4_mtab)))) goto err1;
	}
	{//# PID filter
		rbuf[0] = 0xFF;  rbuf[1] = 0x1F;
		if((ret = writeRegs(st, 0x80f996, rbuf, 2))) goto err1;
		if((ret = writeReg(st, 0x80f995, 0))) goto err1;
		if((ret = writeRegMask(st, 0x80f994, 0x1, 0x1))) goto err1;
	}

	return 0;
err2:
	return -1;
err1:
	return ret;
}

static unsigned int getFreqValue(const uint32_t freq, const uint16_t f_multi, const uint16_t f_div, uint8_t* const lna_band)
{
	unsigned int i;
	const unsigned int freq_upp[] = { 444000, 484000, 533000, 587000, 645000, 710000, 782000, 860000 };
	for(i = 0; i < ARRAY_SIZE(freq_upp); i++) {
		if(freq_upp[i] >= freq) break;
	}
	if(i >= ARRAY_SIZE(freq_upp)) {
		i = ARRAY_SIZE(freq_upp) - 1;
	}
	if(NULL != lna_band) {
		*lna_band = i;
	}
	i = ((freq << 1) * f_multi) / f_div;
	return (i + 1) >> 1;
}

static unsigned  getSDRAM_clock(const uint32_t freq)
{
	int val = 0, j;
	const uint8_t SDRAM_CLK[] = {
//# 0 V1-3, C13-21
73, 78, 73, 77, 67, 72, 75, 60, 63, 61, 64, 61,
//# 12 C22, V4-7
65, 63, 70, 77, 73,
//# 17 V8-12, C23
75, 78, 80, 61, 62, 65,
//# 23 C24-63
68, 69, 71, 72, 78, 80, 79, 80, 61, 62, 64, 65, 67, 68, 60, 63,
69, 62, 80, 75, 66, 77, 61, 68, 76, 69, 80, 77, 75, 72, 60, 66,
65, 66, 80, 64, 67, 60, 62, 60,
//# 63 U13-62
64, 66, 66, 67, 67, 79, 80, 61, 62, 71, 62, 65, 66, 64, 67, 60,
68, 79, 80, 62, 63, 64, 65, 66, 66, 66, 67, 60, 74, 61, 79, 64,
65, 65, 66, 60, 73, 60, 68, 62, 69, 64, 80, 79, 79, 60, 60, 60,
63, 61,
//# 113
};
	const uint8_t idx_ch[] = { 0, 12, 17, 23, 63, 113 };
	const uint32_t fq_low[] = { 90143, 164143, 192143, 230143, 470143 };

	for(j = 4; j >= 0; j-- ) {
		const int num_ch = idx_ch[j +1] - idx_ch[j] - 1;
		val = (signed)freq - fq_low[j];
		if(0 > val)  continue;
		val /= 6000;
		if(num_ch < val)  val = num_ch;
		return SDRAM_CLK[val + idx_ch[j] ];
	}
	return SDRAM_CLK[0];
}

/* set parameters of tuner, demod */
static int it9175_set_params(struct state_st* const st, const uint32_t freq)
{
	int ret;
	uint8_t rbuf[2], findex, n_div, lna_band;
	int iqik_m_cal;
	unsigned int freq_lo, iva;

	const uint8_t params_1[] = {
	0x5E,0x45,0x5B,0x7F,0x56,0x6A,0x4D,0xC5,0x2C,0xF9,0x2B,0xA7,0x29,0x3A,0x25,0x1B,
	0x17,0x01,0x16,0x53,0x15,0x16,0x12,0xFA };
	const uint8_t params_2[] = {
	0x03,0x35,0xED,0xD7,0x01,0x9A,0xF6,0xEB,0x00,0xCD,0x81,0xE2,0x00,0xCD,0x7B,0x76,
	0x00,0xCD,0x75,0x0A,0x00,0x66,0xBD,0xBB,0x01,0x9A,0xF6,0xEB,0x00,0xCD,0x7B,0x76,
	0x7E,0x02,0x9B,0x01 };

	if((ret = writeRegMask(st, 0x80cfff, st->bw_mode, 0x03))) goto err1;
	if((ret = writeRegs(st, 0x80011b, params_1, 24))) goto err1;
	if((ret = writeRegs(st, 0x800001, params_2, 36))) goto err1;
	if((ret = writeReg(st, 0x800040, 0))) goto err1;
	if((ret = writeReg(st, 0x800047, 0))) goto err1;
	if((ret = writeRegMask(st, 0x80f999, 0x00, 0x01))) goto err1;
	if((ret = writeReg(st, 0x80004b, (300000 < freq) ? 1 : 0))) goto err1;
	if((ret = readReg(st, 0x8001c6, rbuf))) goto err1;
	if(0 != rbuf[0]) {
		//# set SDRAM clock
		iva = getSDRAM_clock(freq) << 3;
		if((ret = writeReg(st, 0x80fb25, 0x65))) goto err1;
		if((ret = writeReg(st, 0x80fbb5, ((iva >> 8) & 0xFF) | 0x30))) goto err1;
		if((ret = writeReg(st, 0x80fbb6, iva & 0xFF))) goto err1;
		if((ret = writeReg(st, 0x80fbb7, 0x04))) goto err1;
		if((ret = writeReg(st, 0x80fbb9, 0x4a))) goto err1;
		if((ret = writeReg(st, 0x80fbb9, 0x48))) goto err1;
		if((ret = writeReg(st, 0x80fbba, 0x40))) goto err1;
	}
	if((ret = readReg(st, 0x8001dc, rbuf))) goto err1;
	if(1 != rbuf[0]) {
		warn_info(0,"deMod config error %02X",rbuf[0]);
		goto err2;
	}


	n_div = 0;
	findex = getFreqNdiv(freq, &n_div);
	freq_lo = getFreqValue(freq, (uint16_t)n_div * st->fdiv, st->xtal, &lna_band);

	if((ret = readReg(st, 0x80ec4c, &rbuf[1]))) goto err1;
	if((ret = readReg(st, 0x80ed81, rbuf))) goto err1;

	ret = ( (rbuf[0] & 0x1F) - (rbuf[0] & 0x20) ) * n_div;
	iqik_m_cal = (0 == st->clock_d) ? (ret * 9) >> 5 :  ret >> 1;

	if((ret = writeReg(st, 0x800160, lna_band))) goto err1;
	
	{//# bandwidth
		const uint8_t reg80ec56[4] = {2, 4, 6, 0};
		if((ret = writeReg(st, 0x80ec56, reg80ec56[st->bw_mode]))) goto err1;
	}
	//# l_band
	if((ret = writeReg(st, 0x80ec4c, (rbuf[1] & 0xe7) | ((312000 <= freq) ? 0x8 : 0 ) ))) goto err1;

	freq_lo |= findex << 13;
	iva = freq_lo + iqik_m_cal;
	if((ret = writeReg(st, 0x80ec4d, iva & 0xFF))) goto err1;
	if((ret = writeReg(st, 0x80ec4e, (iva >> 8) & 0xFF))) goto err1;
	if((ret = writeReg(st, 0x80015e, freq_lo & 0xFF))) goto err1;
	if((ret = writeReg(st, 0x80015f, (freq_lo >> 8) & 0xFF))) goto err1;

	if((ret = readReg(st, 0x8001dc, rbuf))) goto err1;
	if(1 != rbuf[0]) {
		warn_info(0,"deMod config error %02X",rbuf[0]);
		goto err2;
	}

	if((ret = writeRegMask(st, 0xd8cf, 0x01, 0x01))) goto err1;

	if((ret = writeReg(st, 0x8001e3, 0))) goto err1;
	findex -= 2;
	freq_lo = (findex << 13) | (freq_lo & 0x1FFF);
	if((ret = writeReg(st, 0x8001e1, freq_lo & 0xFF))) goto err1;
	if((ret = writeReg(st, 0x8001e2, (freq_lo >> 8) & 0xFF))) goto err1;

	if((ret = writeReg(st, 0x800000, 0))) goto err1;

	return 0;
err2:
	return -1;
err1:
	return ret;
}

/* public function */

int it9175_create(it9175_state* const  state, struct usb_endpoint_st * const pusbep)
{
	int ret;
	struct state_st* st;
	if(NULL == *state) {
		st = malloc(sizeof(struct state_st));
		if(NULL == st) {
			warn_info(errno,"failed to allocate");
			return -1;
		}
		*state = st;
		st->pmutex = NULL;
	}else{
		st = *state;
	}
	if((ret = uthread_mutex_init(&st->pmutex)) != 0) {
		warn_info(ret,"failed");
		return -2;
	}
	pusbep->endpoint = EP_TS1;
	pusbep->startstopFunc = NULL;
	pusbep->xfer_size = TS_BulkSize * 188;
	st->chip_id = 0;
	st->fd = pusbep->fd;

#if 0
	if((ret = usb_reset(st->fd)) != 0) {
		warn_info(ret,"failed");
		return -3;
	}
#endif
#if 0
	if((ret = usb_setconfiguration(st->fd, 1)) != 0) {
		warn_info(ret,"failed");
		return -4;
	}
#endif
	if((ret = usb_claim(st->fd, 0)) != 0) {
		warn_info(ret,"failed");
		return -5;
	}
#if 0
	if((ret = usb_setinterface(st->fd, 0, 0)) != 0) {
		warn_info(ret,"failed");
		return -5;
	}
#endif
	if((ret = it9175_usbSetTimeout(st)) != 0) {
		warn_info(ret,"failed");
		return -5;
	}
	if((ret = it9175_identify_state(st)) != 0) {
		warn_info(ret,"failed");
		return -6;
	}
	if((ret = it9175_read_config(st)) != 0) {
		warn_info(ret,"failed");
		return -7;
	}
	//# check firmware code
	if(0 == st->fw_info) {
		//# download firmware
		dmsgn("Loading FW. ");
		if((ret = it9175_loadFW(st)) != 0) {
			warn_msg(ret,"failed to load FW");
			return -8;
		}
	}
	if((ret = it9175_tuner_init(st)) != 0) {
		warn_info(ret,"failed");
		return -10;
	}
	dmsg(" Init done!");

	return 0;
}

int it9175_destroy(const it9175_state state)
{
	int ret;
	struct state_st* const s = state;
	if(NULL == s) {
		warn_info(0,"null pointer");
		return -1;
	}

	if((ret = it9175_sleep(s))) {
		warn_info(ret,"failed");
		return -7;
	}
	if((ret = usb_release(s->fd, 0))) {
		warn_info(ret,"failed");
		return -3;
	}

	if((ret = uthread_mutex_destroy(s->pmutex))) {
		warn_info(ret,"mutex_destroy failed");
		return -2;
	}
	free(s);
	return 0;
}

int it9175_setFreq(const it9175_state state, const unsigned int freq)
{
	int ret = 0;
	struct state_st* const s = state;
	if(NULL == s) {
		warn_info(0,"null pointer");
		return -EINVAL;
	}
	if(53000 > freq || 860000 <= freq) {
		warn_msg(0,"freq %u kHz is out of range.", freq);
		return -EINVAL;
	}
	if((ret = it9175_set_params(s, freq))) goto err1;

	return 0;
err1:
	return ret;
}

int it9175_waitTuning(const it9175_state state, const int timeout)
{
	int ret, i;
	struct state_st* const s = state;
	uint8_t rbuf[2], info;
	const int sleep_ms = 40;

	info = 0;
	rbuf[1] = 2;
	for(i = 0; i < timeout; i+= sleep_ms) {
		if((ret = readReg(s, 0x800047, rbuf))) goto err1;
		if(1 == rbuf[0] && 1 == rbuf[1]) {
			//# found channel
			info = 0x1;
			break;
		}
		if(2 == rbuf[0] && 2 == rbuf[1]) {
			//# empty channel
			info = 0x2;
			break;
		}
		rbuf[1] = rbuf[0];
		miliWait(sleep_ms);
	}

	return (i << 2) | info;
err1:
	return ret;
}

int it9175_waitStream(const it9175_state state, const int timeout)
{
	int ret, i;
	struct state_st* const s = state;
	uint8_t utmp, info;
	const int sleep_ms = 32;

	info = 0;
	for(i = 0; i < timeout; i+= sleep_ms) {
		if((ret = readReg(s, 0x80f999, &utmp))) goto err1;
		if(utmp & 0x1) {
			//# MPEG stream SYNC byte locked
			info = 0x1;
			break;
		}
		miliWait(sleep_ms);
	}
	if((ret = readReg(s, 0x80f980, &utmp))) goto err1;
	if(utmp & 0x1) {
		//# buffer overflow
		if((ret = writeReg(s, 0x80f980, 0))) goto err1;
		info |= 0x2;
	}

	return (i << 2) | info;
err1:
	return ret;
}

/* Transmission and Multiplexing Configuration and Control (See ARIB STD-B31) */
int it9175_readTMCC(const it9175_state state, void* const pData)
{
	int ret, j;
	struct state_st* const s = state;
	uint8_t rbuf[4], val, *ptr = pData, txmode;
	const uint8_t n_txmod[4] = {1,3,2,0};

	if((ret = readRegs(s, 0x80f900, rbuf, 3))) goto err1;
	txmode = n_txmod[rbuf[0] & 0x3];
	ptr[0] = txmode;
	val = rbuf[1] & 0x3;
	ptr[1] = 32 >> val;
	ptr[2] = rbuf[2] & 0x1;
	for(j = 0; j < 3; j++) {
		ptr += 4;
		if((ret = readRegs(s, 0x80f903 +(j * 4), rbuf, 4))) goto err1;
		val = rbuf[0] & 0x7;
		if(0x7 == val) {
			ptr[0] = 0;
			continue;
		}
		ptr[0] = rbuf[3] & 0xF;
		ptr[1] = (3 < val) ? 4 : val;
		val = rbuf[1] & 0x7;
		ptr[2] = (4 < val) ? 5 : val;
		val = rbuf[2] & 0x7;
		val = (3 < val) ? 0 : 4 >> (3 - val);
		ptr[3] = val << (3 - txmode);
	}

	return 0;
err1:
	return ret;
}

int it9175_readStatistic(const it9175_state state, uint8_t* const data)
{
	int ret = 0;
	struct state_st* const st = state;
	uint8_t rbuf[7];
	uint32_t* const pval = (uint32_t*)(data + 8);

	//# TPSD_LOCK MPEG_LOCK (flag)
	if((ret = readRegs(st, 0x80003c, rbuf, 2))) goto err1;
	data[0] = (rbuf[0] & 1) | ((rbuf[1] & 1) << 1);
	//# Signal Quality (%)
	if((ret = readReg(st, 0x800049, rbuf))) goto err1;
	data[1] = *rbuf;
	//# Signal Strength (%)
	if((ret = readRegs(st, 0x80013e, rbuf, 2))) goto err1;
	data[2] = *rbuf;
	data[4] = (int32_t)rbuf[1] -100;
	//# S/N Ratio (dB)
	if((ret = readRegs(st, 0x8001c9, rbuf, 1))) goto err1;
	data[3] = *rbuf;
	//# Bit error, Uncorrectable packet
	if((ret = readRegs(st, 0x800032, rbuf, 7))) goto err1;
	pval[0] = (rbuf[4] << 16) | (rbuf[3] << 8) | rbuf[2];
	pval[1] = ( (rbuf[6] << 8) | rbuf[5]) * 204 * 8;
	pval[2] = (rbuf[1] << 8) | rbuf[0];
	if((ret = readRegs(st, 0x8000f3, rbuf, 7))) goto err1;
	pval[3] = (rbuf[4] << 16) | (rbuf[3] << 8) | rbuf[2];
	pval[4] = ( (rbuf[6] << 8) | rbuf[5]) * 204 * 8;
	pval[5] = (rbuf[1] << 8) | rbuf[0];
	if((ret = readRegs(st, 0x8000fc, rbuf, 7))) goto err1;
	pval[6] = (rbuf[4] << 16) | (rbuf[3] << 8) | rbuf[2];
	pval[7] = ( (rbuf[6] << 8) | rbuf[5]) * 204 * 8;
	pval[8] = (rbuf[1] << 8) | rbuf[0];

	return 0;
err1:
	return ret;
}

/*EOF*/