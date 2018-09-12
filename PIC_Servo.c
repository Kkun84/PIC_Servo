#include <18f26k22.h>
#fuses INTRC, PUT, NOBROWNOUT, NOWDT, NOPROTECT, NOLVP, NOMCLR
#use delay(CLOCK = 64M)
#use RS232(BAUD=19200,XMIT=PIN_C6,RCV=PIN_C7)
#use fast_io(a)
#use fast_io(b)
#use fast_io(c)

// サーボ信号出力用割り込み
// T0の周期[us]
#define T0_PERIOD 100
// プリスケーラ16/(64[MHz]/4)=1[us/カウント]
// T0_INTERVAL[カウント] = T0_PERIOD[us] / 1[us/カウント]
#define T0_INTERVAL T0_PERIOD
// T0_20ms[ms] = (20[ms] / 1[us/カウント]) / T0_INTERVAL[カウント]
#define T0_20ms (20000 / T0_INTERVAL)

// 入力状態時間計測用割り込み
// T1の周期[us]
#define T1_PERIOD 10000
// プリスケーラ8/(64MHz/4)=0.5us/カウント
// T1_INTERVAL[カウント] = T1_PERIOD[us] / 0.5[us/カウント] = T1_PERIOD * 2
#define T1_INTERVAL (T1_PERIOD * 2)

/*****************************変更ポイント*****************************/
// サーボの個数
#define SERVO_NUM 2
// 入力ピンの数
#define IN_NUM 1

/*****************************変更ポイント*****************************/
// サーボ信号出力ピン
// 出力するサーボの値
int servoPulse[SERVO_NUM] = {0, 0};
int servoOut[SERVO_NUM] = {PIN_C2, PIN_C3};
// 入力ピン
int inPin[IN_NUM] = {PIN_C4};

// 入力ピン変化後の秒数[ds]
// 最大6553.5s=109.225min
long inCount[IN_NUM] = {0};


// pin_numに対応した入力を返す
int in(int pin_num)
{
	return input(inPin[pin_num]);
}

void main(void)
{
	// 変数宣言
	int i;
	// セットトリス
	set_tris_a(0b10000000);
	set_tris_b(0b10000000);
	set_tris_c(0b00110000);
	// サーボ信号送信用タイマ
	// プリスケーラ16/(64MHz/4)=1us/カウント
	setup_timer_0(T0_INTERNAL | T0_DIV_16);
	// 入力ピンの変化していない時間を計るタイマ
	// プリスケーラ8/(64MHz/4)=0.5us/カウント
	setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
	// 割り込み許可
	enable_interrupts(INT_TIMER0);
	enable_interrupts(INT_TIMER1);
	
	enable_interrupts(GLOBAL);
	while(1)
	{
		/*****************************変更ポイント*****************************/
		/*
			in(0)が0→1 : servoOut[0]が動作
			in(0)が1→0 : servoOut[1]が動作
		*/
		if(inCount[0] < 5)
		{
			servoPulse[0] = in(0)? 1.8: 1.5;
			
			servoPulse[1] = in(0)? 1.4: 1.7;
		}
		else if(5 <= inCount[0] && inCount[0] < 10)
		{
			servoPulse[0] = 1.5;
			servoPulse[1] = 1.4;
		}
		else
		{
			servoPulse[0] = servoPulse[1] = 0;
		}
		
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
	set_timer0(0xffff - T0_INTERVAL);
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
	// 20msを超えたらリセット
	if(count > T0_20ms)
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
	set_timer1(0xffff - T1_INTERVAL);
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
				if(inCount[i] < 0xffff)
					inCount[i]++;
			}
			else
			{
				inCount[i] = 0;
				inOld[i] = in(i);
			}
		}
	}
	// インクリメント
	count++;
}
