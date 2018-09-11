#include <18f26k22.h>
#fuses INTRC, PUT, NOBROWNOUT, NOWDT, NOPROTECT, NOLVP, NOMCLR
#use delay(CLOCK = 64M)
#use RS232(BAUD=19200,XMIT=PIN_C6,RCV=PIN_C7)
#use fast_io(a)
#use fast_io(b)
#use fast_io(c)

// サーボ信号割り込みカウント数
// プリスケーラ16/(64MHz/4)=1us/カウント
// 1us/カウント*100カウント=0.1ms
#define INTERVAL_T0 100
// サーボ信号割り込みカウント数
// プリスケーラ8/(64MHz/4)=0.5us/カウント
// 0.5us/カウント*20000カウント=10ms
#define INTERVAL_T1 20000
// サーボの個数
#define SERVO_NUM 2
// 入力ピンの数
#define IN_NUM 2

// サーボ信号出力ピン
// {0,1}:洗面所
int servoOut[SERVO_NUM] = {PIN_B0, PIN_B1};
// 出力するサーボの値
int servoPulse[SERVO_NUM] = {0, 0};
// 入力ピン
// 0:洗面所
int iPin[IN_NUM] = {PIN_C4, PIN_C5};
// 入力ピン変化後の秒数[ds]
// 最大6553.5s=109.225min
long inCount[IN_NUM] = {0xffff, 0xffff};

// pin_numに対応した入力を返す
int in(int pin_num)
{
	return input(iPin[pin_num]);
}

void main(void)
{
	// int i;
	// セットトリス
	set_tris_a(0b10000000);
	set_tris_b(0b10000000);
	set_tris_c(0b00110000);
	// サーボ信号送信用
	// プリスケーラ16/(64MHz/4)=1us/カウント
	setup_timer_0(T0_INTERNAL | T0_DIV_16);
	// 入力ピンの変化していない時間を計る
	// プリスケーラ8/(64MHz/4)=0.5us/カウント
	setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
	// 割り込み許可
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
		// 洗面所
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
		
		// 待機
		delay_ms(10);
	}
}

#INT_TIMER0
void intTimer0(void)
{
	int i;
	// 割り込み回数カウント
	static int count = 0;
	// servoPulseの値を保持する
	static int myServoPulse[SERVO_NUM] = {0};
	// タイマー再設定
	set_timer0(0xffff - INTERVAL_T0);
	// サーボ信号出力
	for(i = 0; i < SERVO_NUM; i++)
	{
		// カウント比較してパルス出力
		if(count < myServoPulse[i])
			output_high(servoOut[i]);
		else
			output_low(servoOut[i]);
	}
	// インクリメント
	count++;
	// 1us/カウント * 200 = 20msを超えたらリセット
	if(count > 200)
	{
		// count初期化
		count = 0;
		// 値更新
		for(i = 0; i < SERVO_NUM; i++)
			myServoPulse[i] = servoPulse[i];
	}
}

#INT_TIMER1
void intTimer1(void)
{
	int i;
	// 割り込み回数カウント
	static long count = 0;
	// 前の割り込みでの入力の値
	static int inOld[IN_NUM] = {0};
	// タイマー再設定
	set_timer1(0xffff - INTERVAL_T1);
	// 10ms * 10 = 0.1sごとに実行する
	if(count >= 10)
	{
		// count初期化
		count = 0;
		// 値更新
		for(i = 0; i < IN_NUM; i++)
		{
			// 以前と同じ値だったらインクリメント
			// 違ったら0にする
			if(in(i) == inOld[i])
			{
				// オーバーフローしそうだったら回避する
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
	// インクリメント
	count++;
}
