#include <xc.h>
#include "adc.h" //access potentiometer
#include "clcd.h" //for lcd
#include "ds1307.h" //RTC
#include "eeprom.h" //store logs in eeprom
#include "i2c.h" //protocol used by RTC
#include "main.h" //
#include "matrix_keypad.h" //keypad
#include "uart.h" //used to send datato pc

#define __XTAL_FREQ 20000000 //clock frequency

#define DASHBOARD 0 //display showing time,speed,gear
#define MENU      1 //user menu
#define TOTAL_MENU_ITEMS 4 //no of menu items

#define LOG_SIZE 5 //hr,min,sec,gr,speed
#define MAX_LOGS 20 //max 20 logs can be stored, 100 bytes
#define VIEW_LOG 2 //system mode

#define EEPROM_START 0X00 //logs start storing from eeprom addr 0

unsigned char log_index=0; //index of logs
unsigned char prev_gear_logged=0XFF; //stores previous gear value, prevent storing dup logs
unsigned char view_index=0; //viewing logs, to navigate

void store_log(unsigned char hour,unsigned char min,unsigned char sec,unsigned char gear,unsigned char speed);//store vent data to eeprom
void view_log(unsigned char key); //display logs on lcd
void clear_log(void); //deletes all stored logs from eeprom
void download_log(void); //send logs to pc via uart
void set_log(void); //used for setting time


unsigned char mode = DASHBOARD; //which screen is active
unsigned char menu_start = 0; //stores current selected menu item

unsigned int speed = 0; //speed stored
unsigned char gear = 0; //gear stored
unsigned char previous_index=255; //avoid lcd updates

const char gears[][2] = {"N","1","2","3","4","5","R"}; //display gear

const char *menu[] = { //stores menu options
    "Download Log",
    "Set Log",
    "Clear Log",
    "View Log"
};

void display_dashboard(unsigned char *time); //dashboard display time,gr,spd
void display_menu(void); //menu display

void init_config() //initialise configuration
{
    init_adc(); //initialize adc
    init_clcd(); //initialize clcd
    init_matrix_keypad(); //keypad ip
    init_uart(); //serial comm
    init_i2c(); //i2c protocol
    init_ds1307(); //rtc initialization
}

void main(void)
{
    unsigned int adc_val; //to store adc value from 0-100
    unsigned char key; //stores key for switches
    unsigned char time[9]; //stores rtc time string
    unsigned char hour, min, sec; //rtc time values from ds1307

    init_config(); //initializes peripherals

    
    clcd_print("SPD GR    RTC", LINE1(0)); //display on clcd

    while(1) //infinite loop for continuous reading
    {
        key = read_matrix_keypad(); //reads pressed key

        
        if(key == MK_SW11) //for menu
        {
            if(mode == DASHBOARD) //from dashboard
            {
                /* Enter Menu */
                mode = MENU;
                menu_start = 0;//first item

                clcd_write(0x01, INSTRUCTION_COMMAND); //clear lcd
                __delay_ms(2); //delay

                display_menu(); //display menu
            }
            else if(mode == MENU) //
            {
                if(menu_start == 3)   // for View Log
                {
                    mode = VIEW_LOG; 
                    view_index = 0;//to navigate from 0 to 4
                    previous_index = 255; //prevents lcd updates
                    clcd_write(0x01, INSTRUCTION_COMMAND); //clears lcd
                    __delay_ms(2);
                }
                else if(menu_start == 2)   // Clear Log
                {
                    clear_log(); //clear log fn called - deletes logs from EEPROM
                    display_menu(); //return to menu display
                }
                else if(menu_start == 0)   // Download Log 
                {
                    download_log(); //fn called - UART and teraterm
                    display_menu(); //return to menu display
                }
                else if(menu_start == 1)   // Set Log (future)
                {
                    set_log(); //to set logs - hh:mm:ss
                    display_menu(); //back to menu display
                }
            }

            __delay_ms(200); //small delay for changing mode
        }
        
        if(mode == DASHBOARD) //dashboard mode
        {
            adc_val = read_adc(4); //reads adc from potentiometer for channel 4

            
            speed = (adc_val/10.23); //converts to 0-100 range

            
            if(key == MK_SW1 && gear < 6) //switch 1 for gear increase, max gear 6
            {
                gear++;
                __delay_ms(200);
            }
            else if(key == MK_SW2 && gear > 0) //switch 2 for gear decrease
            {
                gear--;
                __delay_ms(200);
            }

            //ds1307 rtc stores time values in binary coded decimal format
            
            hour = read_ds1307(HOUR_ADDR); //reads bcd formatted values(raw data)  eg: 0x23 to 23
            min  = read_ds1307(MIN_ADDR);
            sec  = read_ds1307(SEC_ADDR);

            hour = ((hour >> 4) * 10) + (hour & 0x0F); //convert bcd to decimal , >> removes lower 4 bits , & opr keeps lower 4 bits 
            min  = ((min >> 4) * 10) + (min & 0x0F); //eg: 2 * 10 + 3 = 23
            sec  = ((sec >> 4) * 10) + (sec & 0x0F); //23:45:23
            
            if(gear!=prev_gear_logged) //log stored when gear changes
            {
                store_log(hour,min,sec,gear,speed); //stored
                prev_gear_logged=gear; //update prev gear
            }

            time[0] = (hour / 10) + '0'; //converts no to ascii , 23 means 23/10=2 stored as '2'
            time[1] = (hour % 10) + '0'; //23%10=3 stored as ascii '3'
            time[2] = ':';
            time[3] = (min / 10) + '0';
            time[4] = (min % 10) + '0';
            time[5] = ':';
            time[6] = (sec / 10) + '0';
            time[7] = (sec % 10) + '0';
            time[8] = '\0'; //null char

            display_dashboard(time); //display dashboard
        }

        
        else if(mode == MENU)
        {
            if(key == MK_SW1) // to navigate down 
            {
                if(menu_start < TOTAL_MENU_ITEMS - 1) 
                    menu_start++; //0-3

                display_menu(); //back to display
                __delay_ms(200);
            }

            else if(key == MK_SW2) //to navigate up
            {
                if(menu_start > 0)
                    menu_start--;

                display_menu();
                __delay_ms(200);
            }
            else if(key == MK_SW12) //to exit from menu 
            {
                mode = DASHBOARD; //goes to dashboard

                clcd_write(0x01, INSTRUCTION_COMMAND); //clear lcd
                __delay_ms(2);

                clcd_print("SPD GR    RTC", LINE1(0)); //prints dashboard
                __delay_ms(200);
            }
     
        }
        else if(mode==VIEW_LOG) //mode
        {
            view_log(key); //scroll logs , display logs
        }
        
    }
}


void display_dashboard(unsigned char *time) //dash display , time to display rtc
{
    if(speed>=100) //to print speed if it is 100 or above
    {
        clcd_putch((speed/100)+'0', LINE2(0));
    }
    else // speed 100 means 100 printed, 45 means 45 printed
    {
        clcd_putch(' ', LINE2(0)); // to print below 100, make first place 0
    }
    clcd_putch(((speed/10)%10)+'0', LINE2(1)); //eg : 45 speed, to store '4'
    clcd_putch((speed%10)+'0', LINE2(2)); // to store '5'

    clcd_print(gears[gear], LINE2(4)); //to print gear
    clcd_print(time, LINE2(7)); // to print time from hh:mm:ss
}

void display_menu(void) //to print menu options
{
    clcd_write(0x01, INSTRUCTION_COMMAND);//clear lcd
    __delay_ms(2); //time to clear lcd

    clcd_print(menu[menu_start], LINE1(0)); //current menu item
    clcd_putch('<', LINE1(15)); //to know the item

    if(menu_start == TOTAL_MENU_ITEMS - 1) //last menu item reached
        clcd_print("                ", LINE2(0)); //set line2 blank , no more menu item
    else
        clcd_print(menu[menu_start + 1], LINE2(0)); //next menu item
}

void store_log(unsigned char hour,unsigned char min,unsigned char sec,unsigned char gear,unsigned char speed)
{                           //store logs to eeprom hr,min,sec,gr,spd, log uses 5 bytes
    unsigned int addr; //eeprom addr where log will be written
    if(log_index >= MAX_LOGS) //circular buffer check, where eeprom stores only fixed logs ie, 20, if more logs generated, overwrite old logs happens
    {
        log_index=0; //overwrite log 0
        
    }
    addr=EEPROM_START+(log_index*LOG_SIZE); //cal eeprom addr, eg:0 + (3*5)=15, log will be stored from addr 15
    
    write_internal_eeprom(addr++,hour); // write hr to eeprom, addr 15
    write_internal_eeprom(addr++,min); //min to eeprom, addr 16
    write_internal_eeprom(addr++,sec); //addr 17
    write_internal_eeprom(addr++,gear); //addr 18
    write_internal_eeprom(addr++,speed); //addr 19
    
    log_index++; //next log will be written at next mem block
}

void view_log(unsigned char key) //reads logs eeprom and allows user to scroll, displays stored logs on clcd
{
    unsigned int addr;
    unsigned char h,m,s,g,sp;

    if(log_index == 0) //if no logs present
    {
        clcd_write(0x01, INSTRUCTION_COMMAND); //clear display
        __delay_ms(2);
        clcd_print("No Logs", LINE1(0)); //print mssg
        __delay_ms(1000);

        mode = MENU;//returns to menu
        display_menu();
        return;
    }

    if(previous_index != view_index) //display updates only when log changes
    {
        previous_index = view_index;

        addr = EEPROM_START + (view_index * LOG_SIZE); //eeprom addr calculation

        h  = read_internal_eeprom(addr++); //read eeprom data
        m  = read_internal_eeprom(addr++);
        s  = read_internal_eeprom(addr++);
        g  = read_internal_eeprom(addr++);
        sp = read_internal_eeprom(addr++);

        clcd_write(0x01, INSTRUCTION_COMMAND); //clear display
        __delay_ms(2);

        clcd_print("SPD GR    RTC", LINE1(0)); //heading

        /* Speed */
        if(sp >= 100)
            clcd_putch((sp/100)+'0', LINE2(0)); //for proper alignment
        else
            clcd_putch(' ', LINE2(0));

        clcd_putch(((sp/10)%10)+'0', LINE2(1));
        clcd_putch((sp%10)+'0', LINE2(2));

        /* Gear */
        clcd_print(gears[g], LINE2(4));

        /* Time */
        clcd_putch((h/10)+'0', LINE2(7));
        clcd_putch((h%10)+'0', LINE2(8));
        clcd_putch(':', LINE2(9));
        clcd_putch((m/10)+'0', LINE2(10));
        clcd_putch((m%10)+'0', LINE2(11));
        clcd_putch(':', LINE2(12));
        clcd_putch((s/10)+'0', LINE2(13));
        clcd_putch((s%10)+'0', LINE2(14));
    }

    
    if(key == MK_SW1) //moves forward
    {
        if(view_index < log_index - 1)
            view_index++;

        __delay_ms(200);
    }
    else if(key == MK_SW2) //moves backward
    {
        if(view_index > 0)
            view_index--;

        __delay_ms(200);
    }
    else if(key == MK_SW12) //exit view log
    {
        view_index = 0; //view index to 0
        previous_index = 255; //prevents any lcd updates
        mode = MENU; //mode menu
        clcd_write(0x01, INSTRUCTION_COMMAND); //clear lcd , go to menu
        __delay_ms(2);
        display_menu(); //goes to display menu
    }
}

void clear_log(void) //erases all stored logs from eeprom
{
    unsigned int i; //loop counter to iterate thru eeprom addres

    for(i = 0; i < (MAX_LOGS * LOG_SIZE); i++)
    {
        write_internal_eeprom(EEPROM_START + i, 0xFF); //each eeprom loc is written with 0xff which means it is cleared
    }

    log_index = 0; //resets write ptr for new logs
    view_index = 0; //resets log viewing pos
    previous_index = 255;      // important reset , next log display refresh properly, prevents displaying old cached data
    prev_gear_logged = 0xFF; //stores last gear logged

    clcd_write(0x01, INSTRUCTION_COMMAND); //clear display
    __delay_ms(2);
    clcd_print("Logs Cleared", LINE1(0)); //show confirmation mssg
    __delay_ms(1000);

    mode = MENU;               //return to menu mode, explicit state change
    display_menu();            // menu screen displayed, refresh screen
}

void download_log(void) //reads logs from eeprom and prints thru UART
{
    unsigned char i;
    unsigned int addr;
    unsigned char h,m,s,g,sp;

    if(log_index == 0) //no logs stored
    {
        clcd_write(0x01, INSTRUCTION_COMMAND); //clear display
        __delay_ms(2);
        clcd_print("No Logs", LINE1(0)); //print no logs
        __delay_ms(2000);
        return;
    }

    
    puts("\r\n#  TIME      EVENT   SPEED\r\n"); //print header on uart , teraterm
    puts("--------------------------------\r\n");

    for(i = 0; i < log_index; i++) //reads each log from eeprom
    {
        addr = EEPROM_START + (i * LOG_SIZE); //eeprom addr calculation

        h  = read_internal_eeprom(addr++); //hr
        m  = read_internal_eeprom(addr++); //min
        s  = read_internal_eeprom(addr++); //sec
        g  = read_internal_eeprom(addr++); //gear
        sp = read_internal_eeprom(addr++); //speed

        /* Print log number */
        if(i+1 >= 10)
        {
            putch(((i+1)/10) + '0'); 
            putch(((i+1)%10) + '0'); //10-...
        }
        else
        {
            putch(' '); //0-9
            putch((i+1) + '0');
        }
        puts("  ");

        /* Print time */
        putch((h/10)+'0'); //stores hr tens place
        putch((h%10)+'0'); //hr units place
        putch(':');
        putch((m/10)+'0'); //min tens place
        putch((m%10)+'0'); //min units place
        putch(':');
        putch((s/10)+'0'); //sec tens place
        putch((s%10)+'0'); //sec units place

        puts("   G"); //stores as G1....

        /* Gear */
        putch(g + '0'); //1-6

        puts("      ");

        /* Speed */
        if(sp >= 100)
            putch((sp/100)+'0'); //100
        putch(((sp/10)%10)+'0'); //045
        putch((sp%10)+'0');

        puts("\r\n"); //used to print new line
    }

    puts("--------------------------------\r\n");

    clcd_write(0x01, INSTRUCTION_COMMAND); //clear display
    __delay_ms(2);
    clcd_print("Downloaded", LINE1(0)); //print mssg
    clcd_print("Successfully", LINE2(0));
    __delay_ms(2500); //delay for showing mssg
    mode = MENU; //return to menu
    display_menu(); //display menu 
}

void set_log(void) //time setting fn for rtc ds1307
{
    unsigned char hour = 0;
    unsigned char min  = 0;
    unsigned char sec  = 0;
    unsigned char field = 2;  //filed to edit 0=hour, 1=min, 2=sec
    unsigned char key;
    unsigned char blink = 0; //for blinking

    clcd_write(0x01, INSTRUCTION_COMMAND); //clear display
    __delay_ms(2);

    while(1)
    {
        clcd_print("HH:MM:SS", LINE1(0)); //display

        /* ---- Display Hours ---- */
        if(field == 0 && blink) //editing hr, hr digits blink
            clcd_print("  ", LINE2(0));
        else
        {
            clcd_putch((hour/10)+'0', LINE2(0));
            clcd_putch((hour%10)+'0', LINE2(1));
        }

        clcd_putch(':', LINE2(2));

        /* ---- Display Minutes ---- */
        if(field == 1 && blink) // min digits blink, editing min
            clcd_print("  ", LINE2(3));
        else
        {
            clcd_putch((min/10)+'0', LINE2(3));
            clcd_putch((min%10)+'0', LINE2(4));
        }

        clcd_putch(':', LINE2(5));

        /* ---- Display Seconds ---- */
        if(field == 2 && blink) //sec digits blink, editing sec
            clcd_print("  ", LINE2(6));
        else
        {
            clcd_putch((sec/10)+'0', LINE2(6));
            clcd_putch((sec%10)+'0', LINE2(7));
        }

        blink = !blink;
        __delay_ms(300); //every 300ms, blinking toggles

        key = read_matrix_keypad(); //read keypad

        /* ---- Increment ---- */
        if(key == MK_SW1) //for incrementing
        {
            if(field == 0) //hr field
            {
                hour++; //increment hr
                if(hour >= 24) hour = 0; //if hr>=0, set hr to 0
            }
            else if(field == 1) //min field
            {
                min++;
                if(min >= 60) min = 0; //if min>=60, set min to 0
            }
            else //sec field
            {
                sec++;
                if(sec >= 60) sec = 0; //if sec>=60, set sec to 0
            }

            __delay_ms(200);
        }

        /* ---- Change Field ---- */
        else if(key == MK_SW2) //change field
        {
            if(field == 0) //2-1-0-2
                field = 2;
            else
                field--;

            __delay_ms(200);
        }

        /* ---- Save ---- */
        else if(key == MK_SW11) //to save and writes time to ds1307
        {
            write_ds1307(SEC_ADDR,  ((sec/10)<<4)  | (sec%10)); //actual to bcd is written
            write_ds1307(MIN_ADDR,  ((min/10)<<4)  | (min%10));
            write_ds1307(HOUR_ADDR, ((hour/10)<<4) | (hour%10));

            __delay_ms(200);
            return;
        }

        /* ---- Cancel ---- */
        else if(key == MK_SW12) //not save and move to dashboard
        {
            __delay_ms(200);
            return;
        }
    }
}


    
