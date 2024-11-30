/***********************************************************************************

*                  LCD_CS  LCD_CD  LCD_WR  LCD_RD  LCD_RST  SD_SS  SD_DI  SD_DO  SD_SCK 
*     Arduino Uno    A3      A2      A1      A0      A4      10     11     12      13                            
*Arduino Mega2560    A3      A2      A1      A0      A4      10     11     12      13                           

*                  LCD_D0  LCD_D1  LCD_D2  LCD_D3  LCD_D4  LCD_D5  LCD_D6  LCD_D7  
*     Arduino Uno    8       9       2       3       4       5       6       7
*Arduino Mega2560    8       9       2       3       4       5       6       7 

**********************************************************************************/

#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library

LCDWIKI_KBV mylcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset

// DEFINE COLOR VALUES
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY 0x9CD2
#define LIGHT_GREY 0xF7BE
#define AQUAMARINE 0x8F94
#define ORANGE 0xFD09

// declare global variables
int i = 0;
int display_width = 480;
int display_height = 320;
String displayed_speed = "00";
int current_lap = 0;
String displayed_laps;
bool reset = false;


//draw some oblique lines
void lines_test(void)
{
    mylcd.Fill_Screen(BLACK);
      mylcd.Set_Draw_color(GREEN);
    int i = 0;   
    for(i = 0; i< mylcd.Get_Display_Width();i+=5)
    {
       mylcd.Draw_Line(0, 0, i, mylcd.Get_Display_Height()-1);
     }
     for(i = mylcd.Get_Display_Height()-1; i>= 0;i-=5)
     {
       mylcd.Draw_Line(0, 0, mylcd.Get_Display_Width()-1, i);
     }
     
     mylcd.Fill_Screen(BLACK); 
       mylcd.Set_Draw_color(RED);
    for(i = mylcd.Get_Display_Width() -1; i>=0;i-=5)
    {
      mylcd.Draw_Line(mylcd.Get_Display_Width()-1, 0, i, mylcd.Get_Display_Height()-1);
     }
    for(i = mylcd.Get_Display_Height()-1; i>=0;i-=5)
    {
      mylcd.Draw_Line(mylcd.Get_Display_Width()-1, 0, 0, i);
     }
     
     mylcd.Fill_Screen(BLACK); 
      mylcd.Set_Draw_color(BLUE);
     for(i = 0; i < mylcd.Get_Display_Width();i+=5)
    {
      mylcd.Draw_Line(0, mylcd.Get_Display_Height()-1, i, 0);
     }
     for(i = 0; i < mylcd.Get_Display_Height();i+=5)
    {
      mylcd.Draw_Line(0, mylcd.Get_Display_Height()-1, mylcd.Get_Display_Width()-1, i);
     }

     mylcd.Fill_Screen(BLACK);
      mylcd.Set_Draw_color(YELLOW);
     for(i = mylcd.Get_Display_Width()-1; i >=0;i-=5)
    {
      mylcd.Draw_Line(mylcd.Get_Display_Width()-1, mylcd.Get_Display_Height()-1, i, 0);
     }
     for(i = 0; i<mylcd.Get_Display_Height();i+=5)
    {
      mylcd.Draw_Line(mylcd.Get_Display_Width()-1, mylcd.Get_Display_Height()-1, 0, i);
     }
}

//draw some vertical lines and horizontal lines
void h_l_lines_test(void)
{
    int i=0;
   mylcd.Fill_Screen(BLACK);
     mylcd.Set_Draw_color(GREEN);
    for(i =0;i<mylcd.Get_Display_Height();i+=5)
    {
      mylcd.Draw_Fast_HLine(0,i,mylcd.Get_Display_Width()); 
      delay(5);
    }
     mylcd.Set_Draw_color(BLUE);
     for(i =0;i<mylcd.Get_Display_Width();i+=5)
    {
      mylcd.Draw_Fast_VLine(i,0,mylcd.Get_Display_Height()); 
           delay(5);
    }
}

//draw some rectangles
void rectangle_test(void)
{
  int i = 0;
   mylcd.Fill_Screen(BLACK);
     mylcd.Set_Draw_color(GREEN);
   for(i = 0;i<mylcd.Get_Display_Width()/2;i+=4)
   {
      mylcd.Draw_Rectangle(i,(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2+i,mylcd.Get_Display_Width()-1-i,mylcd.Get_Display_Height()-(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2-i);  
        delay(5);
   }
}

//draw some filled rectangles
void fill_rectangle_test(void)
{
  int i = 0;
   mylcd.Fill_Screen(BLACK);
     mylcd.Set_Draw_color(YELLOW);
   mylcd.Fill_Rectangle(0,(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2,mylcd.Get_Display_Width()-1,mylcd.Get_Display_Height()-(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2);
    mylcd.Set_Draw_color(MAGENTA);
   for(i = 0;i<mylcd.Get_Display_Width()/2;i+=4)
   {
      mylcd.Draw_Rectangle(i,(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2+i,mylcd.Get_Display_Width()-1-i,mylcd.Get_Display_Height()-(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2-i);  
        delay(5);
   }
   for(i = 0;i<mylcd.Get_Display_Width()/2;i+=4)
   {
       mylcd.Set_Draw_color(random(255), random(255), random(255));
      mylcd.Fill_Rectangle(i,(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2+i,mylcd.Get_Display_Width()-1-i,mylcd.Get_Display_Height()-(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2-i);  
        delay(5);
   }
}

//draw some filled circles
void fill_circles_test(void)
{
  int r=10,i=0,j=0;
  mylcd.Fill_Screen(BLACK);
   mylcd.Set_Draw_color(MAGENTA);
  for(i=r;i<mylcd.Get_Display_Width();i+=2*r)
  {
    for(j=r;j<mylcd.Get_Display_Height();j+=2*r)
    {
      mylcd.Fill_Circle(i, j, r);
    }
  }
}

//draw some circles
void circles_test(void)
{
  int r=10,i=0,j=0;
   mylcd.Set_Draw_color(GREEN);
  for(i=0;i<mylcd.Get_Display_Width()+r;i+=2*r)
  {
    for(j=0;j<mylcd.Get_Display_Height()+r;j+=2*r)
    {
      mylcd.Draw_Circle(i, j, r);
    }
  }  
}

//draw some triangles
void triangles_test(void)
{
   int i = 0;
   mylcd.Fill_Screen(BLACK);
   for(i=0;i<mylcd.Get_Display_Width()/2;i+=5)
   {
      mylcd.Set_Draw_color(0,i+64,i+64);
      mylcd.Draw_Triangle(mylcd.Get_Display_Width()/2-1,mylcd.Get_Display_Height()/2-1-i,
                    mylcd.Get_Display_Width()/2-1-i,mylcd.Get_Display_Height()/2-1+i,
                    mylcd.Get_Display_Width()/2-1+i,mylcd.Get_Display_Height()/2-1+i);                   
   }
}

//draw some filled triangles
void fill_triangles_test(void)
{
   int i = 0;
   mylcd.Fill_Screen(BLACK);
    for(i=mylcd.Get_Display_Width()/2-1;i>0;i-=5)
   {
    
      mylcd.Set_Draw_color(0,i+64,i+64);
      mylcd.Fill_Triangle(mylcd.Get_Display_Width()/2-1,mylcd.Get_Display_Height()/2-1-i,
                    mylcd.Get_Display_Width()/2-1-i,mylcd.Get_Display_Height()/2-1+i,
                    mylcd.Get_Display_Width()/2-1+i,mylcd.Get_Display_Height()/2-1+i);                   
      mylcd.Set_Draw_color(i,0,i);
      mylcd.Draw_Triangle(mylcd.Get_Display_Width()/2-1,mylcd.Get_Display_Height()/2-1-i,
                    mylcd.Get_Display_Width()/2-1-i,mylcd.Get_Display_Height()/2-1+i,
                    mylcd.Get_Display_Width()/2-1+i,mylcd.Get_Display_Height()/2-1+i);                   
   }
}

//draw some round rectangles
void round_rectangle(void)
{
   int i = 0;
   mylcd.Fill_Screen(BLACK);
     for(i = 0;i<mylcd.Get_Display_Width()/2;i+=4)
   {
       mylcd.Set_Draw_color(255-i,0,160-i);
      mylcd.Draw_Round_Rectangle(i,(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2+i,mylcd.Get_Display_Width()-1-i,mylcd.Get_Display_Height()-(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2-i,8);
        delay(5);
   } 
}

//draw some filled round rectangles
void fill_round_rectangle(void)
{
     int i = 0;
   mylcd.Fill_Screen(BLACK);
     for(i = 0;i<mylcd.Get_Display_Width()/2;i+=4)
   {
       mylcd.Set_Draw_color(255-i,160-i,0);
      mylcd.Fill_Round_Rectangle(i,(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2+i,mylcd.Get_Display_Width()-1-i,mylcd.Get_Display_Height()-(mylcd.Get_Display_Height()-mylcd.Get_Display_Width())/2-i,8);
        delay(5);
   } 
}


bool draw_low_fuel(){
  mylcd.Set_Text_Back_colour(RED);
  mylcd.Set_Text_colour(BLACK);
  mylcd.Set_Text_Size(4);

  mylcd.Fill_Screen(RED);
  mylcd.Print_String("LOW FUEL!", 250, 260);

  return true;
}

void draw_safe_background(){
  
  mylcd.Fill_Screen(AQUAMARINE);

  // Draw off-white rounded rectangles onto a aquamarine background
  // mylcd.Fill_round_Rectangle(x1, y1, x2, y2, radius of curvature (for corners))
  mylcd.Set_Draw_color(LIGHT_GREY);
  mylcd.Fill_Round_Rectangle(15, 15, display_width/2 - 20, display_height - 15, 5);

  mylcd.Set_Draw_color(LIGHT_GREY);
  mylcd.Fill_Round_Rectangle(display_width/2, 15, display_width - 15, display_height - 15, 5);
  
  
}

void draw_spedometer_laps(float speed, int laps, bool fuel_low){
  
  // leading zero for speed:
  if (round(speed) < 10){
      displayed_speed = "0" + String(round(speed)); 
  }
  
  else{
    displayed_speed = String(round(speed));
  }

  // The following code draws the speed
  mylcd.Set_Text_Back_colour(LIGHT_GREY);
  mylcd.Set_Text_colour(BLACK);
  mylcd.Set_Text_Size(10);
  mylcd.Print_String(displayed_speed, 60, 100); 

  // Draw the "KM/H" label below speed
  mylcd.Set_Text_colour(GREY);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("KM/H", 80, 195);
  
  if (current_lap < 10){
      displayed_laps = "0" + String(laps);
  }
  
  else{
    displayed_laps = String(laps);
  }

  mylcd.Set_Text_colour(BLACK);
  mylcd.Set_Text_Size(10);
  mylcd.Print_String(displayed_laps, display_width/2 + 70, 100);

  Serial.println(current_lap);

  mylcd.Set_Text_colour(GREY);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("LAP(s)", display_width/2 + 80, 195);

}

bool check_if_fuel_low(){
  if (i > 80){
    return true;
  }

  return false;
}




bool fuel_low_chg_flag = false; // a flag to see if fuel_low_state has changed 

void setup() 
{
  Serial.begin(9600);
  mylcd.Init_LCD();
  mylcd.Set_Rotation(3); // orientes display horizontally

  draw_safe_background();
}


                              
void loop() 
{
  i++;

  if (i > 99){ // resets the development lap counter
    i = 1;
  }

  current_lap = round(i/5); // five i to a lap

  bool fuel_low_state = check_if_fuel_low(); // check to see if the fuel low, in the future this may be implemented mechanically
  bool fuel_low_drawn = false; //

  if (mylcd.Read_Pixel(1, 1) == 63488){
    Serial.println("Red");
  };
  
  Serial.println(mylcd.Read_Pixel(1, 1));

  if(fuel_low_state && fuel_low_drawn == false){
    draw_low_fuel(); 
    fuel_low_chg_flag = true; 
    fuel_low_drawn = true;
    
    draw_spedometer_laps(i, current_lap, fuel_low_state);
  }

  if(fuel_low_state == false && fuel_low_chg_flag == true){
    draw_safe_background();
    fuel_low_chg_flag = false;
    fuel_low_drawn = false;

    draw_spedometer_laps(i, current_lap, fuel_low_state);
  }

  else{
    draw_spedometer_laps(i, current_lap, fuel_low_state);
  }
  
  draw_spedometer_laps(i, current_lap, fuel_low_state);

  delay(120);

}
// current behaviour 
// at 80 laps low fuel displays, but draws each time, we need to fix this
