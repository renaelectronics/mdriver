#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include "6474.h"
#include "options.h"
#include "parport.h"

/* ctrl-c signal handler */
static int running = 1;

void signal_handler(int sig)
{
	running = 0;
}


void dump_motor_options(struct motor_options *p)
{
	printf("---------------------------------------\n");
	printf("             Motor Unit : %d\n", p->motor);
	printf("          Motor Current : %f A\n", p->current);
	printf("           PWM off Time : %f us\n", p->pwm_off);
	printf("        Fast Decay Time : %f us\n", p->t_fast);
	printf("         Fall Step Time : %f us\n", p->t_step);
	printf("        Minimum On Time : %f us\n", p->ton_min);
	printf("       Minimum Off Time : %f us\n", p->toff_min);
	printf(" Over Current Threshold : %f A\n", p->ocd_th);
	printf("              Step Mode : %d\n", p->step_mode);
	printf("---------------------------------------\n");
}

int options_check(int argc, char **argv, struct motor_options *p)
{
	int rc = 1;

	if ((p->motor < 0) || (p->motor > 3)){
		printf("Invalid motor unit %d, ",
			p->motor);
		rc = 0;
	}
	else if ((p->current <= 0.0) || (p->current > 4.0)){
		printf("Invalid motor current %fA, ",
			p->current);
		rc = 0;
	}
	else if ((p->ocd_th <= 0.0) || (p->ocd_th > 6.0)){
		printf("Invalid overcurrent detection %fA, ",
			p->ocd_th);
		rc = 0;
	}
	else if ((p->pwm_off < 4.0) || (p->pwm_off > 124.0)){
		printf("Invalid pwm_off value %fus, must be between 4 to 124, ",
			p->pwm_off);
		rc = 0;
	}
	else if ((p->t_fast < 2.0) || (p->t_fast > 32.0)){
		printf("Invalid t_fast value %fus, must be between 2 to 32, ",
			p->t_fast);
		rc = 0;
	}
	else if ((p->t_step < 2.0) || (p->t_step > 32.0)){
		printf("Invalid t_step value %fus, must be between 2 to 32, ",
			p->t_step);
		rc = 0;
	}
	else if ((p->ton_min < 0.5) || (p->ton_min > 64.0)){
		printf("Invalid ton_min value %fus, must be between 0.5 to 64, ",
			p->ton_min);
		rc = 0;
	}
	else if ((p->toff_min < 0.5) || (p->toff_min > 64.0)){
		printf("Invalid toff_min value %fus, must be between 2us to 32us, ",
			p->toff_min);
		rc = 0;
	}

	if (rc == 0){
		printf("use following command for help.\n\n");
		printf("\t%s --help\n\n", argv[0]);
	}

	/* current, resolution 31.25mA */
	p->current = (int)(p->current / 0.03125) * 0.03125;

	/* overcurrent, resolution 375mA */
	p->ocd_th = (int)(p->ocd_th / 0.375) * 0.375;

	return rc;
}

/*
 * prepare buffer
 */
void options_to_buf(struct motor_options *p, char *pbuf)
{
	int n,m;
	int checksum;

	/* EEPROM_MOTOR_NUM */
	pbuf[EEPROM_MOTOR_NUM] = p->motor;

	/* EEPROM_VERSION */
	pbuf[EEPROM_VERSION] = 0x00;

	/* EEPROM_ABS_POS */
	pbuf[EEPROM_ABS_POS + 0] = 0x00;
	pbuf[EEPROM_ABS_POS + 1] = 0x00;
	pbuf[EEPROM_ABS_POS + 2] = 0x00;

	/* EEPROM_EL_POS */
	pbuf[EEPROM_EL_POS + 0] = 0x00;
	pbuf[EEPROM_EL_POS + 1] = 0x00;

	/* EEPROM_MARK */
	pbuf[EEPROM_MARK + 0] = 0x00;
	pbuf[EEPROM_MARK + 1] = 0x00;
	pbuf[EEPROM_MARK + 2] = 0x00;

	/* EEPROM_TVAL */
	n = p->current / 0.03125;
	pbuf[EEPROM_TVAL] = (n & 0xff);

	/* EEPROM_T_FAST */
	n = p->t_fast / 2.0;
	n = (n - 1) & 0xf;
	m = p->t_step / 2.0;
	m = (m - 1) & 0xf;
	pbuf[EEPROM_T_FAST] = (n<<4) | m;

	/* EEPROM_TON_MIN */
	n = p->ton_min / 0.5;
	n = (n - 1) & 0x7f;
	pbuf[EEPROM_TON_MIN] = n;

	/* EEPROM_TOFF_MIN */
	n = p->toff_min / 0.5;
	n = (n - 1) & 0x7f;
	pbuf[EEPROM_TOFF_MIN] = n;

	/* EEPROM_ADC_OUT */
	pbuf[EEPROM_ADC_OUT] = 0x00;

	/* EEPROM_OCD_TH */
	n = p->ocd_th / 0.375;
	pbuf[EEPROM_OCD_TH] =  n & 0xff;

	/* EEPROM_STEP_MODE, bit7,3 must be 1 */
	pbuf[EEPROM_STEP_MODE] = 0x88 | p->step_mode;

	/* EEPROM_ALARM_EN */
	pbuf[EEPROM_ALARM_EN] = 0xff;

	/* EEPROM_CONFIG: TOFF[14:10]=pwm_off POW_SR[9:8]=0x02 */
	n = p->pwm_off / 4.0;
	n = n & 0x1f;
	pbuf[EEPROM_CONFIG + 0] = (n << 2) | 0x02;
	pbuf[EEPROM_CONFIG + 1] = 0x88;

	/* EEPROM_STATUS*/
	pbuf[EEPROM_STATUS + 0] = 0x00;
	pbuf[EEPROM_STATUS + 1] = 0x00;

	/* EEPROM_CHECK_SUM */
	pbuf[EEPROM_CHECK_SUM] = 0x00;
	for (n=0, checksum=0; n<EEPROM_CHECK_SUM; n++){
		checksum += pbuf[n];
	}
	checksum = ~checksum;
	checksum += 1;
	pbuf[EEPROM_CHECK_SUM] = checksum & 0xff;
}


/*
 * main entry
 */
int main(int argc, char **argv)
{
	char data[EEPROM_MAX_BYTE];
	struct motor_options p;
	int fd;

	/* set default and parse options,
	 * refer to L6474 datasheet,
	 * Doc ID 022529 Rev 3
	 */
	memset(&p, 0, sizeof(struct motor_options));
	p.motor = 0;
	p.version = 0;
	p.current = 0.03125;
	p.pwm_off = 44.0;
	p.t_fast = 4.0;
	p.t_step = 20.0;
	p.ton_min = 21.0;
	p.toff_min = 21.0;
	p.ocd_th = 8 * 0.375;
	p.step_mode = 0;/* default to full step */
	p.readinfo = 0;
	p.console = 0;

	if (!get_motor_options(argc, argv, &p)){
		exit(0);
	}
	
	if (!p.console){
		/* options check */
		if (!options_check(argc, argv, &p)){
			exit(0);
		}
	}

	/* initialize parallel port */
	fd = parport_init("/dev/parport0");
	if (fd <0){
		printf("failed to initialize parallel port\n");
		exit (0);
	}

	/* debug console */
	if (p.console){
		for(;;){
			pulse_HOST_CS(fd);
			while (get_HOST_SDI(fd) == 0){
				if (!running)
					goto out;
			}
			receive_packet_raw(data, 1, fd);
			printf("%c", data[0]);
			fflush(stdout);
			if (!running)
				goto out;
		}
	}

	/* read info */
	if (p.readinfo){
		memset(data, 0, sizeof(data));
		receive_packet(data, EEPROM_MAX_BYTE, fd);
		dump_data(data, EEPROM_MAX_BYTE);
		goto out;
	}

	/* dump_motor_options */
	dump_motor_options(&p);

	/* prepare data */
	memset(data, 0, EEPROM_MAX_BYTE);
	options_to_buf(&p, data);

	/* write motor data */
	printf("Programming ... ");
	send_packet(data, EEPROM_MAX_BYTE, fd);
	fflush(stdout);

out:
	/* close and exit */
	parport_exit(fd);
	exit(0);

}
