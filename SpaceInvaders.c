// SpaceInvaders.c
// Runs on LM4F120/TM4C123
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds
uint32_t ADCvalue[2];

uint8_t Level1=1;
uint8_t Level2=0;
uint8_t Level3=0;
uint8_t player1=0;
uint8_t player2=0;
uint8_t player1_score=0;
uint8_t player2_score=0;

typedef enum {dead,alive} status_t;
typedef enum {up_left, up_right, dow_left, down_right, right, left} direction_t;
struct sprite{
  uint32_t x;      // x coordinate
  uint32_t y;      // y coordinate
  const unsigned short *image; // ptr->image
	const unsigned short *imageClear; // ptr->Clear_image
	const unsigned short *dead_image;
	direction_t gun_direction[4];
	int gun_direction_index;
  status_t life;            // dead/alive
};   

struct ball{
  uint32_t x;      // x coordinate
  uint32_t y;      // y coordinate
  const unsigned short *image; // ptr->image
	const unsigned short *imageClear; // ptr->Clear_image
	direction_t direction;
  status_t life;            // dead/alive
};  
typedef struct ball ball_t;
typedef struct sprite sprite_t;
sprite_t Cowboy;
ball_t Ball;

void GameInit(void){
	Cowboy.x=1;
	Cowboy.y=28;
	Cowboy.image=Cowboy_straight;
	Cowboy.imageClear=qq;
	Cowboy.life=alive;
	Cowboy.dead_image=Cowboy_dead;
	Cowboy.gun_direction[0]=right;
	Cowboy.gun_direction[1]=down_right;
	Cowboy.gun_direction[2]=right;
	Cowboy.gun_direction[3]=up_right;
	Cowboy.gun_direction_index=0;
	Ball.life=dead;
	Ball.direction=right;
	Ball.image=ball_image;
	Ball.imageClear=ball_image_clear;
	Ball.x=0;
	Ball.y=0;
}

//ball direction only ever changes in these extreme cases
void static changeDirection_ball(direction_t dir){//called by other methods only
	if(Ball.life==dead){
		Ball.direction=dir;
	}
	else{
		if(Ball.direction==up_right){	//assuming that the ball is going up and hits the ceiling
			Ball.direction=down_right;
		}else if(Ball.direction==down_right){ //assuming that the ball is going up and hits the ceiling
			Ball.direction=up_right;
		}
	}
}
void changeDirection_cowboy(void){// change direction of the shooter and the ball
	Cowboy.gun_direction_index=(Cowboy.gun_direction_index+1)&0x03; //(%4)
	direction_t dir=Cowboy.gun_direction[Cowboy.gun_direction_index];
	if(Ball.life==dead){
		changeDirection_ball(dir);
	}
	if(dir==right){
		Cowboy.image=Cowboy_straight;
	}else if(dir==down_right){
		Cowboy.image=Cowbow_image_down;
	}else if(dir==up_right){
		Cowboy.image=Cowboy_image_up;
	}
}
void moveBall(void){//before you move the ball you have to clear the previous location of it	
	if(Ball.life==alive){
		if(Ball.direction==up_right){
			Ball.x++;Ball.y--;
		}else if(Ball.direction==right){
			Ball.x++;
		}else if(Ball.direction==down_right){
			Ball.x++;Ball.y++;
		}
	}
}
void checkBallSurroundings(void){// if the ball is too close to any of the boundaries, this will change it's direction
	if(Ball.life==alive){
		if((Ball.x>=63&&Ball.x<=75)&&(Ball.y>=62&&Ball.y<=94)){
			Ball.life=dead;
		}else if(((Ball.x>=57&&Ball.x<=65)&&(Ball.y>=9&&Ball.y<=40))&&Level2){
			Ball.life=dead;
		}else if(((Ball.x>=57&&Ball.x<=65)&&(Ball.y>=117&&Ball.y<=144))&&Level2){
			Ball.life=dead;
		}else if(Ball.y>=159){
			changeDirection_ball(Ball.direction);
		}else if(Ball.y<=1){
			changeDirection_ball(Ball.direction);
		}if(Ball.x>=120){//call ball display here to just create a black space where the ball originally was.
			Ball.life=dead;
			ST7735_DrawBitmap(Ball.x, Ball.y, Ball.imageClear, 7,5);//if you declare ball dead, you also have to erase it
		}
	}
}
//set starting coordinates for the ball
//make the ball alive
void ballShoot(void){
	if(Ball.life==dead){
		if(Cowboy.gun_direction[Cowboy.gun_direction_index]==up_right){
			Ball.x=Cowboy.x+14;
			Ball.y=Cowboy.y-17;
			Ball.direction=up_right;
		}else if(Cowboy.gun_direction[Cowboy.gun_direction_index]==down_right){
			Ball.x=Cowboy.x+14;
			Ball.y=Cowboy.y-7;
			Ball.direction=down_right;
		}else if(Cowboy.gun_direction[Cowboy.gun_direction_index]==right){
			Ball.x=Cowboy.x+15;
			Ball.y=Cowboy.y-12;
			Ball.direction=right;
		}
		Ball.life=alive;
	}
	
}
uint32_t convert(int ADCInput){
	return (130*ADCInput)/4096+22;
}
void UpdateCowboy(void){//also have to have something here that moves the cowboy's legs...
	int ADCvalue[2];
	int pe4=0;
	ADC_In89(ADCvalue);
	pe4=ADCvalue[1];// adc from pe5
	Cowboy.y=convert(pe4);
	
}
void displayCowboy(void){
	if(Cowboy.life==alive){
		ST7735_DrawBitmap(Cowboy.x, Cowboy.y, Cowboy.image, 20,26);
	}
}
void displayBall(void){
	ST7735_DrawBitmap(Ball.x, Ball.y, Ball.image, 9,7);
}

////////////////////////////////////////////

void GPIOPortF_Handler(void){ // called on touch of either SW1 or SW2
  if(GPIO_PORTF_RIS_R&0x01){  // SW2 touch
    GPIO_PORTF_ICR_R = 0x01;  // acknowledge flag0
		changeDirection_cowboy();
	}
  if(GPIO_PORTF_RIS_R&0x10){  // SW1 touch
    GPIO_PORTF_ICR_R = 0x10;  // acknowledge flag4
		if(Ball.life==dead){
			ballShoot();
		}
	}
}
/////////////////////////////////////////////

/////////////////////////////////////////////
sprite_t Cowboy2;
ball_t Ball2;

void GameInit2(void){
	Cowboy2.x=110;
	Cowboy2.y=28;
	Cowboy2.image=Cowboy2_straight;
	Cowboy2.imageClear=qq;
	Cowboy2.life=alive;
	Cowboy2.gun_direction[0]=left;
	Cowboy2.gun_direction[1]=dow_left;
	Cowboy2.gun_direction[2]=left;
	Cowboy2.gun_direction[3]=up_left;
	Cowboy2.gun_direction_index=0;
	Ball2.life=dead;
	Ball2.direction=left;
	Ball2.image=ball_image;
	Ball2.imageClear=ball_image_clear;
	Ball2.x=120;
	Ball2.y=160;
}//ball direction only ever changes in these extreme cases
void static changeDirection_ball2(direction_t dir){//called by other methods only //converted
	if(Ball2.life==dead){
		Ball2.direction=dir;
	}
	else{
		if(Ball2.direction==up_left){	//assuming that the ball is going up and hits the ceiling
			Ball2.direction=dow_left;
		}else if(Ball2.direction==dow_left){ //assuming that the ball is going up and hits the ceiling
			Ball2.direction=up_left;
		}
	}
}
void changeDirection_cowboy2(void){// change direction of the shooter and the ball
	Cowboy2.gun_direction_index=(Cowboy2.gun_direction_index+1)&0x03; //(%4)
	direction_t dir=Cowboy2.gun_direction[Cowboy2.gun_direction_index];
	if(Ball2.life==dead){
		changeDirection_ball2(dir);
	}
	if(dir==left){
		Cowboy2.image=Cowboy2_straight;
	}else if(dir==dow_left){
		Cowboy2.image=Cowbow2_image_down;
	}else if(dir==up_left){
		Cowboy2.image=Cowboy2_image_up;
	}
}
void moveBall2(void){//before you move the ball you have to clear the previous location of it	//converted
	if(Ball2.life==alive){
		if(Ball2.direction==up_left){
			Ball2.x--;Ball2.y--;
		}else if(Ball2.direction==left){
			Ball2.x--;
		}else if(Ball2.direction==dow_left){
			Ball2.x--;Ball2.y++;
		}
	}
}
void checkBallSurroundings2(void){// if the ball is too close to any of the boundaries, this will change it's direction
	if(Ball2.life==alive){
		if((Ball2.x>=60&&Ball2.x<=64)&&(Ball2.y>=62&&Ball2.y<=94)){
			Ball2.life=dead;
		}else if(((Ball2.x>=60&&Ball2.x<=64)&&(Ball2.y>=9&&Ball2.y<=40))&&Level2){
			Ball2.life=dead;
		}else if(((Ball2.x>=60&&Ball2.x<=64)&&(Ball2.y>=110&&Ball2.y<=144))&&Level2){
			Ball2.life=dead;
		}if(Ball2.y>=159){
			changeDirection_ball2(Ball2.direction);
		}else if(Ball2.y<=20){
			changeDirection_ball2(Ball2.direction);
		}if(Ball2.x<=1){//call ball display here to just create a black space where the ball originally was.
			Ball2.life=dead;
			ST7735_DrawBitmap(Ball2.x, Ball2.y, Ball2.imageClear, 7,9);//if you declare ball dead, you also have to erase it
		}
	}
}
//set starting coordinates for the ball
//make the ball alive
void ballShoot2(void){
	if(Ball2.life==dead){
		if(Cowboy2.gun_direction[Cowboy2.gun_direction_index]==up_left){
			Ball2.y=Cowboy2.y-15;
			Ball2.x=Cowboy2.x;
			Ball2.direction=up_left;
		}else if(Cowboy2.gun_direction[Cowboy2.gun_direction_index]==left){
			Ball2.y=Cowboy2.y-11;
			Ball2.x=Cowboy2.x;
			Ball2.direction=left;
		}else if(Cowboy2.gun_direction[Cowboy2.gun_direction_index]==dow_left){
			Ball2.y=Cowboy2.y-8;
			Ball2.x=Cowboy2.x;
			Ball2.direction=dow_left;
		}
		Ball2.life=alive;
		
	}
	
}
void UpdateCowboy2(void){//also have to have something here that moves the cowboy's legs...
	int ADCvalue[2];
	int pe5=0;									
	ADC_In89(ADCvalue);
	pe5=ADCvalue[0];// adc from pe4
	Cowboy2.y=convert(pe5);
////	
////	int ADCvalue[2];
////	int pe4=0;						debugging here this is for debug
////	ADC_In89(ADCvalue);
////	pe4=ADCvalue[1];// adc from pe5
////	Cowboy2.y=convert(pe4);
	
}
void displayCowboy2(void){
	if(Cowboy2.life==alive){
		ST7735_DrawBitmap(Cowboy2.x, Cowboy2.y, Cowboy2.image, 20,26);
	}
}
void displayBall2(void){
	if(Ball2.life==alive){
	ST7735_DrawBitmap(Ball2.x, Ball2.y, Ball2.image, 9,7);
	}
}

void checkAlive1(void){
	if(Ball2.life==alive){
		if((Ball2.x<=25) && (Ball2.y<=Cowboy.y && Ball2.y>=(Cowboy.y-26))){
			Cowboy.life=dead;
			player2=1;
			ST7735_DrawBitmap(Cowboy.x, Cowboy.y, Cowboy.dead_image, 57,34);
		}
		
	}
}
void checkAlive2(void){
	if(Ball.life==alive){
		if(Ball.x>=115&& (Ball.y<=Cowboy2.y && Ball.y>=(Cowboy2.y-26))){
			Cowboy2.life=dead;
			player1=1;
			ST7735_DrawBitmap(Cowboy2.x-35, Cowboy2.y, Cowboy2.dead_image, 57,34);
		}
		
	}
}
void updateGame(void){
	UpdateCowboy();
	displayCowboy();
	if(Ball.life==alive){
			displayBall();
			if(1%1==0){
				moveBall();
			}
			
			checkBallSurroundings();	
			
		}
	checkAlive1();
}
void updateGame2(void){
	UpdateCowboy2();
	displayCowboy2();
	if(Ball2.life==alive){
			displayBall2();
		if(1%1==0){
			moveBall2();
		}
			checkBallSurroundings2();	
			Sound_Shoot();
		}
	checkAlive2();
}
//idea: have 2 global score variables that initially start at 20. whenever the person's score goes below

int main(void){
	PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
	Random_Init(1);
	ADC_Init89();
	Output_Init();
	VectorButon_Init();
	VectorButtons2_Init();
	ST7735_FillScreen(0x0000);
	Delay100ms(1);              // delay 5 sec at 80 MHz
	GameInit2();
	GameInit();
	Sound_Init();
	Cowboy2.dead_image=Cowboy_dead;
	ST7735_DrawBitmap(20, 80, level1, 50,15);	///PUT THE PLAYER1 WINS HERE
	ST7735_FillScreen(0x0000);
	Delay100ms(1);
	while(Level1){
		updateGame();
		updateGame2();
		//ST7735_DrawBitmap(60, 40, cactus, 13,31);	
		ST7735_DrawBitmap(60, 95, cactus, 13,31);	
		//ST7735_DrawBitmap(60, 145, cactus, 13,31);	
		if(player1){
			Delay100ms(2);
			ST7735_FillScreen(0x0000);
			Level1=0;
			Level2=1;
			player1_score++;
			player1=0;
			Sound_Explosion();
		}else if(player2){
			Delay100ms(2);
			ST7735_FillScreen(0x0000);
			Level1=0;
			Level2=1;
			player2=0;
			player2_score++;
			Sound_Explosion();
		}
		}
	
	ST7735_DrawBitmap(12, 80, level2, 50,13);	///PUT THE PLAYER1 WINS HERE	Delay100ms(1);              // delay 5 sec at 80 MHz
	Delay100ms(7);
	GameInit2();
	GameInit();
	ST7735_FillScreen(0x0000);
	Cowboy2.dead_image=Cowboy_dead;
	while(Level2){
		updateGame2();
		updateGame();
		ST7735_DrawBitmap(60, 40, cactus, 13,31);	
		ST7735_DrawBitmap(60, 95, cactus, 13,31);	
		ST7735_DrawBitmap(60, 145, cactus, 13,31);	
		if(player1){
			ST7735_FillScreen(0x0000);
//			ST7735_DrawBitmap(12, 80, play1, 74,44);	///PUT THE PLAYER1 WINS HERE
			Level1=0;
			Level2=0;
			Level3=1;
			player1_score++;
			player1=0;
			Sound_Fastinvader3();
		}else if(player2){
			ST7735_FillScreen(0x0000);
//			ST7735_DrawBitmap(12, 80, , 74,44);	///PUT THE PLAYER1 WINS HERE
			Level1=0;
			Level2=0;
			Level3=1;
			player2_score++;
			player2=0;
			Sound_Invaderkilled();
		}
		
	}
	if(player2_score==2){
		ST7735_DrawBitmap(12, 80, play2, 93,53);	///PUT THE PLAYER1 WINS HERE
		ST7735_SetCursor(50, 100);
		ST7735_OutString("Player 2 score = 2");
		Level3=0;
	}else if(player1_score==2){
			ST7735_DrawBitmap(12, 80, play1, 74,44);	///PUT THE PLAYER1 WINS HERE
			ST7735_SetCursor(50, 100);
			ST7735_OutString("Player 1 score =  2");
			Level3=0;

	}
	while(Level3){
		
	}
}




//////////////////////////////////
void GPIOPortC_Handler(void){
	if(GPIO_PORTC_RIS_R&0x10|| GPIO_PORTC_DATA_R&0x10){  // SW4 touch
		GPIO_PORTC_ICR_R = 0x10;    // acknowledge flag4
		changeDirection_cowboy2();
		GPIO_PORTF_DATA_R=0X02;
	}else if(GPIO_PORTC_RIS_R&0x20|| GPIO_PORTC_DATA_R&0x20){  // SW5 touch
		GPIO_PORTC_ICR_R = 0x20;    // acknowledge flag4
		ballShoot2();
		GPIO_PORTF_DATA_R=0X02;
}

}

void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
