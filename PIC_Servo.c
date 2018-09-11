#include <18f26k22.h>
#fuses INTRC, PUT, NOBROWNOUT, NOWDT, NOPROTECT, NOLVP, NOMCLR
#use delay(CLOCK = 64M)
#use RS232(BAUD=19200,XMIT=PIN_C6,RCV=PIN_C7)
#use fast_io(a)
#use fast_io(b)
#use fast_io(c)

// �T�[�{�M�����荞�݃J�E���g��
// �v���X�P�[��16/(64MHz/4)=1us/�J�E���g
// 1us/�J�E���g*100�J�E���g=0.1ms
#define INTERVAL_T0 100
// �T�[�{�M�����荞�݃J�E���g��
// �v���X�P�[��8/(64MHz/4)=0.5us/�J�E���g
// 0.5us/�J�E���g*20000�J�E���g=10ms
#define INTERVAL_T1 20000
// �T�[�{�̌�
#define SERVO_NUM 2
// ���̓s���̐�
#define IN_NUM 2

// �T�[�{�M���o�̓s��
// {0,1}:���ʏ�
int servoOut[SERVO_NUM] = {PIN_B0, PIN_B1};
// �o�͂���T�[�{�̒l
int servoPulse[SERVO_NUM] = {0, 0};
// ���̓s��
// 0:���ʏ�
int iPin[IN_NUM] = {PIN_C4, PIN_C5};
// ���̓s���ω���̕b��[ds]
// �ő�6553.5s=109.225min
long inCount[IN_NUM] = {0xffff, 0xffff};

// pin_num�ɑΉ��������͂�Ԃ�
int in(int pin_num)
{
	return input(iPin[pin_num]);
}

void main(void)
{
	// int i;
	// �Z�b�g�g���X
	set_tris_a(0b10000000);
	set_tris_b(0b10000000);
	set_tris_c(0b00110000);
	// �T�[�{�M�����M�p
	// �v���X�P�[��16/(64MHz/4)=1us/�J�E���g
	setup_timer_0(T0_INTERNAL | T0_DIV_16);
	// ���̓s���̕ω����Ă��Ȃ����Ԃ��v��
	// �v���X�P�[��8/(64MHz/4)=0.5us/�J�E���g
	setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
	// ���荞�݋���
	enable_interrupts(INT_TIMER0);
	enable_interrupts(INT_TIMER1);
	enable_interrupts(GLOBAL);
	
	output_high(PIN_A0);
	output_high(PIN_A1);
	output_low(PIN_A0);
	output_low(PIN_A1);
	// printf("%5lu,%5lu\r\n", inCount[0], inCount[1]);
	
	while(1)
	{
		// ���ʏ�
		if(inCount[0] < 5)
		{
			servoPulse[0] = in(0)? 12: 15;
			servoPulse[1] = in(0)? 15: 18;
		}
		else if(10 <= inCount[0] && inCount[0] < 15)
		{
			servoPulse[0] = servoPulse[1] = 15;
		}
		else
		{
			servoPulse[0] = servoPulse[1] = 0;
		}
		
		printf("%5d,%5d\r\n", servoPulse[0], servoPulse[1]);
		
		// �ҋ@
		delay_ms(10);
	}
}

#INT_TIMER0
void intTimer0(void)
{
	int i;
	// ���荞�݉񐔃J�E���g
	static int count = 0;
	// servoPulse�̒l��ێ�����
	static int myServoPulse[SERVO_NUM] = {0};
	// �^�C�}�[�Đݒ�
	set_timer0(0xffff - INTERVAL_T0);
	// �T�[�{�M���o��
	for(i = 0; i < SERVO_NUM; i++)
	{
		// �J�E���g��r���ăp���X�o��
		if(count < myServoPulse[i])
			output_high(servoOut[i]);
		else
			output_low(servoOut[i]);
	}
	// �C���N�������g
	count++;
	// 1us/�J�E���g * 200 = 20ms�𒴂����烊�Z�b�g
	if(count > 200)
	{
		// count������
		count = 0;
		// �l�X�V
		for(i = 0; i < SERVO_NUM; i++)
			myServoPulse[i] = servoPulse[i];
	}
}

#INT_TIMER1
void intTimer1(void)
{
	int i;
	// ���荞�݉񐔃J�E���g
	static long count = 0;
	// �O�̊��荞�݂ł̓��͂̒l
	static int inOld[IN_NUM] = {0};
	// �^�C�}�[�Đݒ�
	set_timer1(0xffff - INTERVAL_T1);
	// 10ms * 10 = 0.1s���ƂɎ��s����
	if(count >= 10)
	{
		// count������
		count = 0;
		// �l�X�V
		for(i = 0; i < IN_NUM; i++)
		{
			// �ȑO�Ɠ����l��������C���N�������g
			// �������0�ɂ���
			if(in(i) == inOld[i])
			{
				// �I�[�o�[�t���[��������������������
				if(inCount[i] != 0xffff)
					inCount[i]++;
			}
			else
			{
				inCount[i] = 0;
				inOld[i] = !inOld[i];
			}
		}
	}
	// �C���N�������g
	count++;
}
