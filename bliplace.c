#include <avr/interrupt.h>
#include <avr/pgmspace.h> 

#define bit(A)   (1 << A)
#define sbi(p,b) { p |= (unsigned char)bit(b); }
#define cbi(p,b) { p &= (unsigned char)~bit(b); }
#define tbi(p,b) { p ^= (unsigned char)bit(b); }
#define gbi(p,b) (p & (unsigned char)bit(b))

//#define BENCHMARK

//-----------------------------------------------------------------------------

#define TRIG1_STEP   1
#define TRIG1_CLAMP  70
#define BRIGHT1_UP   (65535 / 30)
#define BRIGHT1_DOWN (65535 / 300)
#define BRIGHT1_MAX  64

#define TRIG2_STEP   1
#define TRIG2_CLAMP  40
#define BRIGHT2_UP   (65535 / 40)
#define BRIGHT2_DOWN (65535 / 700)
#define BRIGHT2_MAX  64

#define DCFILTER    5
#define BASSFILTER  3

//-----------------------------------------------------------------------------

volatile register uint8_t  pdmA         asm("r2");
volatile register uint8_t  pdmC         asm("r3");

volatile register uint8_t  brightA      asm("r4");
volatile register uint8_t  brightC      asm("r5");

volatile register uint8_t  temp         asm("r6");
volatile register uint8_t  sregbackup   asm("r7");

volatile register int16_t  tmax1        asm("r8");
volatile register int16_t  tmax2        asm("r10");

int16_t accumN;
int16_t accumD;
int16_t accumB;

volatile register uint16_t ibright1     asm("r18");
volatile register uint16_t ibright2     asm("r20");

uint16_t brightaccum1 = 0;
uint16_t brightaccum2 = 0;

uint8_t tickcount = 0;

//-----------------------------------------------------------------------------

const uint8_t exptab[256] PROGMEM =
{
0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,5,5,
5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,8,8,8,8,8,8,9,9,9,9,9,10,10,10,
10,10,11,11,11,11,12,12,12,12,13,13,13,14,14,14,14,15,15,15,16,16,16,17,
17,18,18,18,19,19,20,20,21,21,21,22,22,23,23,24,24,25,25,26,27,27,28,28,
29,30,30,31,32,32,33,34,35,35,36,37,38,39,39,40,41,42,43,44,45,46,47,48,
49,50,51,52,53,55,56,57,58,59,61,62,63,65,66,68,69,71,72,74,76,77,79,81,
82,84,86,88,90,92,94,96,98,100,102,105,107,109,112,114,117,119,122,124,
127,130,133,136,139,142,145,148,151,155,158,162,165,169,172,176,180,184,
188,192,196,201,205,210,214,219,224,229,234,239,244,250,255,
};

// gamma-corrected (2.0) sin table

const uint8_t gammasin[256] PROGMEM = 
{
64,67,70,73,77,80,84,87,91,95,98,102,106,110,114,118,122,126,130,134,138,142,146,
150,154,158,162,166,170,174,178,182,186,190,193,197,200,204,207,211,214,217,220,
223,226,228,231,234,236,238,240,242,244,246,247,249,250,251,252,253,254,254,255,
255,255,255,255,254,254,253,252,251,250,249,247,246,244,242,240,238,236,234,231,
228,226,223,220,217,214,211,207,204,200,197,193,190,186,182,178,174,170,166,162,
158,154,150,146,142,138,134,130,126,122,118,114,110,106,102,98,95,91,87,84,80,77,
73,70,67,64,61,58,55,52,49,46,44,41,39,37,34,32,30,28,26,24,23,21,19,18,16,15,14,
13,11,10,9,9,8,7,6,5,5,4,4,3,3,2,2,2,2,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,4,4,5,5,6,7,8,9,9,10,
11,13,14,15,16,18,19,21,23,24,26,28,30,32,34,37,39,41,44,46,49,52,55,58,61
};

// y = sqrt(x/255) * 255

const uint8_t gammatab[256] PROGMEM = 
{
0,15,22,27,31,35,39,42,45,47,50,52,55,57,59,61,63,65,67,69,71,73,74,76,78,79,
81,82,84,85,87,88,90,91,93,94,95,97,98,99,100,102,103,104,105,107,108,109,110,
111,112,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,
131,132,133,134,135,136,137,138,139,140,141,141,142,143,144,145,146,147,148,
148,149,150,151,152,153,153,154,155,156,157,158,158,159,160,161,162,162,163,
164,165,165,166,167,168,168,169,170,171,171,172,173,174,174,175,176,177,177,
178,179,179,180,181,182,182,183,184,184,185,186,186,187,188,188,189,190,190,
191,192,192,193,194,194,195,196,196,197,198,198,199,200,200,201,201,202,203,
203,204,205,205,206,206,207,208,208,209,210,210,211,211,212,213,213,214,214,
215,216,216,217,217,218,218,219,220,220,221,221,222,222,223,224,224,225,225,
226,226,227,228,228,229,229,230,230,231,231,232,233,233,234,234,235,235,236,
236,237,237,238,238,239,240,240,241,241,242,242,243,243,244,244,245,245,246,
246,247,247,248,248,249,249,250,250,251,251,252,252,253,253,254,255,
};

//-----------------------------------------------------------------------------

void Update ( void )
{
	//----------
	// Wait for sample

	while(gbi(ADCSRA,ADIF) == 0) {}
	sbi(ADCSRA,ADIF);

	//----------

	int16_t sample = ADC;

	// remove rumble + residual DC bias

	sample -= accumD;
	accumD += (sample >> DCFILTER);

	// de-noise sample

	accumN = (accumN + sample) >> 1;

	sample = accumN;

	// split into bass & treble

	sample -= accumB;
	accumB += (sample >> BASSFILTER);

	int16_t bass = accumB;
	int16_t treb = sample;

	if(bass < 0) bass = -bass;
	if(treb < 0) treb = -treb;

	//----------
	// Every 64 samples, adapt to volume

	tickcount = (tickcount + 1) & 0x3F;

	if(tickcount == 0)
	{
		if(brightaccum1 > (BRIGHT1_MAX*64))
		{
			if(tmax1 < 32000)
			{
				tmax1 += TRIG1_STEP;
				tmax1 += tmax1 >> 10;
			}
		}
		else
		{
			if(tmax1 > TRIG1_CLAMP)
			{
				tmax1 -= tmax1 >> 10;
				tmax1 -= TRIG1_STEP;
			}
		}

		if(brightaccum2 > (BRIGHT2_MAX*64))
		{
			if(tmax2 < 32000)
			{
				tmax2 += TRIG2_STEP;
				tmax2 += tmax2 >> 10;
			}
		}
		else
		{
			if(tmax2 > TRIG2_CLAMP)
			{
				tmax2 -= tmax2 >> 10;
				tmax2 -= TRIG2_STEP;
			}
		}

		brightaccum1 = 0;
		brightaccum2 = 0;
	}

	//----------
	// Ramp our brightness up if we hit a trigger, down otherwise

	if(bass > tmax2)
	{
		if(ibright2 <= (65535-BRIGHT2_UP))
		{
			ibright2 += BRIGHT2_UP;
		}
	}
	else
	{
		if(ibright2 >= BRIGHT2_DOWN)
		{
			ibright2 -= BRIGHT2_DOWN;
		}
	}

	if(treb > tmax1)
	{
		if(ibright1 <= (65535-BRIGHT1_UP))
		{
			ibright1 += BRIGHT1_UP;
		}
	}
	else
	{
		if(ibright1 >= BRIGHT1_DOWN)
		{
			ibright1 -= BRIGHT1_DOWN;
		}
	}
}

//-----------------------------------------------------------------------------
// Software PDM interrupt

__attribute__((naked)) ISR(TIMER1_OVF_vect)
{
#ifndef BENCHMARK

	asm("in r7,0x3f");
	asm("clr r6");

	asm("add r2,r4"::);
	asm("rol r6"::);
	asm("rol r6"::);
	asm("add r3,r5"::);
	asm("rol r6"::);

	asm("out 0x18,r6"::);
	asm("out 0x3f,r7");

#else

	asm("nop");
	asm("nop");

	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");

	asm("nop");
	asm("nop");

#endif

	asm("reti");
}

//----------

void setbright (uint8_t b1, uint8_t b2, uint8_t b3)
{
	asm("mov r4,%0"::"r"(b1));
	asm("mov r5,%0"::"r"(b3));
	OCR0B = b2;
}

//-----------------------------------------------------------------------------
// Startup cylon pattern, which also covers up our initial adaptation phase ;)

void startup ( void )
{
	tmax1 = TRIG1_CLAMP;
	tmax2 = TRIG2_CLAMP;

	uint8_t a = 0;
	uint8_t b = 64;
	uint8_t c = 128;
	uint8_t b1 = 0;
	uint8_t b2 = 0;
	uint8_t b3 = 0;

	for(int j = 0; j < 1320; j++) Update();

	for(int i = 0; i < 51; i++)
	{
		b1 += 1;
		b2 += 5;
		b3 += 1;

		setbright(b1,b2,b3);

		for(int j = 0; j < 35; j++) Update();
	}

	b1 += 13;
	b3 += 13;

	for(int j = 0; j < 880; j++) Update();

	for(int i = 0; i < 512; i++)
	{
		a += 1;
		b += 2;
		c += 1;

		b1 = pgm_read_byte(gammasin+a);
		b2 = pgm_read_byte(gammasin+b);
		b3 = pgm_read_byte(gammasin+c);

		setbright(b1,b2,b3);

		for(int j = 0; j < 11; j++) Update();
	}

	for(int j = 0; j < 880; j++) Update();

	b1 -= 13;
	b3 -= 13;

	for(int i = 0; i < 51; i++)
	{
		b1 -= 1;
		b2 -= 5;
		b3 -= 1;

		setbright(b1,b2,b3);

		for(int j = 0; j < 35; j++) Update();
	}

	for(int j = 0; j < 44; j++) Update();
}

//-----------------------------------------------------------------------------

int main ( void )
{
	PRR |= bit(PRUSI);

	DDRB   = 0x7;
	PORTB  = 0x0;

	setbright(0,0,0);

	//----------
	// Turn on hardware PWM for central LED

	TCCR0A = bit(COM0B1) | bit(WGM00) | bit(WGM01);	
	TCCR0B = bit(CS00);

	//----------
	// Turn on PDM mode

	TIMSK = (1 << TOIE1);
	TCCR1 = (1 << CS10) | (1 << PWM1A) | (1 << CTC1);
	OCR1C = 72;
	
	sei();

	//----------
	// ADC config

	// We sample at 1000000 / (16*14) = ~4464 hz

	ADMUX  = bit(MUX2) | bit(MUX1) | bit(ADLAR);// | bit(REFS1);
	ADCSRB = bit(BIN);
	ADCSRA = bit(ADEN) | bit(ADPS2) | bit(ADATE);

	sbi(ADCSRA,ADSC);

	//----------
	// Play startup animation

	startup();

	//----------
	// Main loop

	while(1)
	{
		Update();

		uint8_t bright1 = pgm_read_byte(exptab+(ibright1 >> 8));
		uint8_t bright2 = pgm_read_byte(exptab+(ibright2 >> 8));

		brightaccum1 += pgm_read_byte(gammatab+bright1);
		brightaccum2 += pgm_read_byte(gammatab+bright2);

		setbright(bright1,bright2,bright1);

#ifdef BENCHMARK
		PORTB ^= 4;
#endif
	}
}

//-----------------------------------------------------------------------------

