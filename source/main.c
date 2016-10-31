/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <fat.h>

#ifdef WII
	#include <wiiuse/wpad.h>
#endif

#include "poke.h"
#include "me.h"

//from my tests 50us seems to be the lowest
//safe si transfer delay in between calls
#define SI_TRANS_DELAY 50

extern u8 gba_mb_gba[];
extern u32 gba_mb_gba_size;

void printmain()
{
	printf("\x1b[2J");
	printf("\x1b[37m");
	printf("Pokemon Generation 3 Event Distribution by suloku\n\n");
	printf("Based on GBA Link Cable Dumper v1.5 by FIX94\n");
	printf("Save Support based on SendSave by Chishm\n");
	printf("GBA BIOS Dumper by Dark Fader\n \n");
}

u8 *resbuf,*cmdbuf;

volatile u32 transval = 0;
void transcb(s32 chan, u32 ret)
{
	transval = 1;
}

volatile u32 resval = 0;
void acb(s32 res, u32 val)
{
	resval = val;
}

unsigned int docrc(u32 crc, u32 val)
{
	int i;
	for(i = 0; i < 0x20; i++)
	{
		if((crc^val)&1)
		{
			crc>>=1;
			crc^=0xa1c1;
		}
		else
			crc>>=1;
		val>>=1;
	}
	return crc;
}

void endproc()
{
	printf("Start pressed, exit\n");
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	exit(0);
}
void fixFName(char *str)
{
	u8 i = 0;
	for(i = 0; i < strlen(str); ++i)
	{
		if(str[i] < 0x20 || str[i] > 0x7F)
			str[i] = '_';
		else switch(str[i])
		{
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '\"':
			case '<':
			case '>':
			case '|':
				str[i] = '_';
				break;
			default:
				break;
		}
	}
}
unsigned int calckey(unsigned int size)
{
	unsigned int ret = 0;
	size=(size-0x200) >> 3;
	int res1 = (size&0x3F80) << 1;
	res1 |= (size&0x4000) << 2;
	res1 |= (size&0x7F);
	res1 |= 0x380000;
	int res2 = res1;
	res1 = res2 >> 0x10;
	int res3 = res2 >> 8;
	res3 += res1;
	res3 += res2;
	res3 <<= 24;
	res3 |= res2;
	res3 |= 0x80808080;

	if((res3&0x200) == 0)
	{
		ret |= (((res3)&0xFF)^0x4B)<<24;
		ret |= (((res3>>8)&0xFF)^0x61)<<16;
		ret |= (((res3>>16)&0xFF)^0x77)<<8;
		ret |= (((res3>>24)&0xFF)^0x61);
	}
	else
	{
		ret |= (((res3)&0xFF)^0x73)<<24;
		ret |= (((res3>>8)&0xFF)^0x65)<<16;
		ret |= (((res3>>16)&0xFF)^0x64)<<8;
		ret |= (((res3>>24)&0xFF)^0x6F);
	}
	return ret;
}
void doreset()
{
	cmdbuf[0] = 0xFF; //reset
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,3,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}
void getstatus()
{
	cmdbuf[0] = 0; //status
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,3,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}
u32 recv()
{
	memset(resbuf,0,32);
	cmdbuf[0]=0x14; //read
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,5,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
	return *(vu32*)resbuf;
}
void send(u32 msg)
{
	cmdbuf[0]=0x15;cmdbuf[1]=(msg>>0)&0xFF;cmdbuf[2]=(msg>>8)&0xFF;
	cmdbuf[3]=(msg>>16)&0xFF;cmdbuf[4]=(msg>>24)&0xFF;
	transval = 0;
	resbuf[0] = 0;
	SI_Transfer(1,cmdbuf,5,resbuf,1,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}
bool dirExists(const char *path)
{
	DIR *dir;
	dir = opendir(path);
	if(dir)
	{
		closedir(dir);
		return true;
	}
	return false;
}
void createFile(const char *path, size_t size)
{
	int fd = open(path, O_WRONLY|O_CREAT);
	if(fd >= 0)
	{
		ftruncate(fd, size);
		close(fd);
	}
}
void warnError(char *msg)
{
	puts(msg);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	sleep(2);
}
void fatalError(char *msg)
{
	puts(msg);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	sleep(5);
	exit(0);
}

void displayPrintTickets( int cursor_position, int game, int language )
{
	printf("\x1b[14;2H");
	
	printf("Select your event:\n\n");
	printf("\x1b[16;0H");

	switch(language)
	{
		case 1: //JAP
			switch (game)
			{
				case 0: //RS
					printf("     Eon Ticket\n");
					break;
				case 1: //E
					printf("     Eon Ticket\n");
					printf("     Mystic Ticket 2005\n");
					printf("     Old Sea Map\n");
					break;
				case 2: //FRLG
					printf("     Aurora Ticket 2004\n");
					printf("     Mystic Ticket 2005\n");
					break;
			}
			break;
		case 2: //ENG
			switch (game)
			{
				case 0: //RS
					printf("     Eon Ticket (e-card)\n");
					printf("     Eon Ticket (nintendo Italy)\n");
					break;
				case 1: //E
					printf("     Aurora Ticket\n");
					printf("     Mystic Ticket\n");
					break;
				case 2: //FRLG
					printf("     Aurora Ticket\n");
					//printf("     Mystic Ticket\n");
					break;
			}
			break;
		default: //FRE-ITA-GER-ESP
			switch (game)
			{
				case 0: //RS
					printf("     Eon Ticket\n");
					break;
				case 1: //E
					printf("     Aurora Ticket\n");
					break;
				case 2: //FRLG
					printf("     Aurora Ticket\n");
					break;
			}
			break;
	}
	//Print cursor
	printf("\x1b[16;0H");
	int i = 0;
	for (i=0; i<cursor_position;i++)
	{printf("\n");}
	printf("  -->");
	
	printf("\x1b[20;0H");

}

int main(int argc, char *argv[]) 
{
	void *xfb = NULL;
	GXRModeObj *rmode = NULL;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	int x = 24, y = 32, w, h;
	w = rmode->fbWidth - (32);
	h = rmode->xfbHeight - (48);
	CON_InitEx(rmode, x, y, w, h);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
	PAD_Init();
#ifdef WII
	WPAD_Init();
#endif
	cmdbuf = memalign(32,32);
	resbuf = memalign(32,32);
	u8 *testdump = memalign(32,0x400000);
	if(!testdump) return 0;
	u8 *testdump2 = memalign(32,0x400000);
	if(!testdump2) return 0;
	int i;
	while(1)
	{
		printmain();
		printf("Waiting for a GBA in port 2...\n");
		resval = 0;

		SI_GetTypeAsync(1,acb);
		while(1)
		{
			if(resval)
			{
				if(resval == 0x80 || resval & 8)
				{
					resval = 0;
					SI_GetTypeAsync(1,acb);
				}
				else if(resval)
					break;
			}
			PAD_ScanPads();
#ifdef WII
			WPAD_ScanPads();
#endif
			VIDEO_WaitVSync();
			if(PAD_ButtonsHeld(0))
				endproc();
#ifdef WII
			if (WPAD_ButtonsHeld(0))
				endproc();
#endif
		}
		if(resval & SI_GBA)
		{
			printf("GBA Found! Waiting on BIOS\n");
			resbuf[2]=0;
			while(!(resbuf[2]&0x10))
			{
				doreset();
				getstatus();
			}
			printf("Ready, sending dumper\n");
			unsigned int sendsize = ((gba_mb_gba_size+7)&~7);
			unsigned int ourkey = calckey(sendsize);
			//printf("Our Key: %08x\n", ourkey);
			//get current sessionkey
			u32 sessionkeyraw = recv();
			u32 sessionkey = __builtin_bswap32(sessionkeyraw^0x7365646F);
			//send over our own key
			send(__builtin_bswap32(ourkey));
			unsigned int fcrc = 0x15a0;
			//send over gba header
			for(i = 0; i < 0xC0; i+=4)
				send(__builtin_bswap32(*(vu32*)(gba_mb_gba+i)));
			//printf("Header done! Sending ROM...\n");
			for(i = 0xC0; i < sendsize; i+=4)
			{
				u32 enc = ((gba_mb_gba[i+3]<<24)|(gba_mb_gba[i+2]<<16)|(gba_mb_gba[i+1]<<8)|(gba_mb_gba[i]));
				fcrc=docrc(fcrc,enc);
				sessionkey = (sessionkey*0x6177614B)+1;
				enc^=sessionkey;
				enc^=((~(i+(0x20<<20)))+1);
				enc^=0x20796220;
				send(enc);
			}
			fcrc |= (sendsize<<16);
			//printf("ROM done! CRC: %08x\n", fcrc);
			//send over CRC
			sessionkey = (sessionkey*0x6177614B)+1;
			fcrc^=sessionkey;
			fcrc^=((~(i+(0x20<<20)))+1);
			fcrc^=0x20796220;
			send(fcrc);
			//get crc back (unused)
			recv();
			printf("Done!\n");
			sleep(2);
			//hm
			while(1)
			{
				printmain();
				printf("Press A once you have a GBA Game inserted.\n");
				PAD_ScanPads();
#ifdef WII
				WPAD_ScanPads();
#endif
				VIDEO_WaitVSync();
				u32 btns = PAD_ButtonsDown(0);
#ifdef WII
				u32 btnsWii = WPAD_ButtonsDown(0);
#endif
#ifdef WII
				if(btns&PAD_BUTTON_START || btnsWii&WPAD_BUTTON_HOME)
#else
				if(btns&PAD_BUTTON_START)
#endif
					endproc();
#ifdef WII
				else if(btns&PAD_BUTTON_A || btnsWii&WPAD_BUTTON_A)
#else
				else if(btns&PAD_BUTTON_A)
#endif
				{
					if(recv() == 0) //ready
					{
						printf("Waiting for GBA\n");
						VIDEO_WaitVSync();
						int gbasize = 0;
						while(gbasize == 0)
							gbasize = __builtin_bswap32(recv());
						send(0); //got gbasize
						u32 savesize = __builtin_bswap32(recv());
						send(0); //got savesize
						if(gbasize == -1) 
						{
							warnError("ERROR: No (Valid) GBA Card inserted!\n");
							continue;
						}
						//get rom header
						for(i = 0; i < 0xC0; i+=4)
							*(vu32*)(testdump+i) = recv();
						//print out all the info from the  game
						printf("Game Name: %.12s\n",(char*)(testdump+0xA0));
						printf("Game ID: %.4s\n",(char*)(testdump+0xAC));
						printf("Company ID: %.2s\n",(char*)(testdump+0xB0));
						printf("ROM Size: %02.02f MB\n",((float)(gbasize/1024))/1024.f);
						
						//Get game/language from GAME ID
						int language = 0;
						int game = 0;
						int maxoptions = 0;
						
						char lang_[8];
						sprintf(lang_, "%.4s", (char*)(testdump+0xAC));
						
						switch(lang_[3])
						{
							case 0x4A: //JAP
								language = 1;
								break;
							case 0x45: //ENG
								language = 2;
								break;
							case 0x46: //FRE
								language = 3;
								break;
							case 0x49: //ITA
								language = 4;
								break;
							case 0x44: //GER
								language = 5;
								break;
							case 0x53: //ESP
								language = 7;
								break;
							default:
								language = 2; //English is compatible with most carts and holds more events than other EUR langs, but this point should never be reached.
								break;
						}

						if ( strncmp("AXV", (char*)(testdump+0xAC), 3) == 0 || strncmp("AXP", (char*)(testdump+0xAC), 3) == 0 ) // R S
						{
							game = 0;
						}
						else if ( strncmp("BPE", (char*)(testdump+0xAC), 3) == 0 ) //E
						{
							game = 1;
						}
						else if ( strncmp("BPR", (char*)(testdump+0xAC), 3) == 0 || strncmp("BPG", (char*)(testdump+0xAC), 3) == 0  ) //FR LG
						{
							game = 2;
						}
						else{
							game = 3; //Unknown
						}
						
						//Set cursor options
						if (game != 3)
						{
							switch(language)
							{
								case 1: //JAP
									switch (game)
									{
										case 0: //RS
											maxoptions = 0;
											break;
										case 1: //E
											maxoptions = 2;
											break;
										case 2: //FRLG
											maxoptions = 1;
											break;
									}
									break;
								case 2: //ENG
									switch (game)
									{
										case 0:
											maxoptions = 1;
											break;
										case 1:
											maxoptions = 1;
											break;
										case 2:
											maxoptions = 0;
											break;
									}
									break;
								default: //FRE-ITA-GER-ESP
									switch (game)
									{
										case 0:
											maxoptions = 0;
											break;
										case 1:
											maxoptions = 0;
											break;
										case 2:
											maxoptions = 0;
											break;
									}
									break;
								
							}
						}
						
						
						
						if(savesize > 0 && game != 3)
						{
							printf("Save Size: %02.02f KB\n \n",((float)(savesize))/1024.f);
							
							
							int command = 0;
							int cursor_position = 0;
							while(1)
							{
								PAD_ScanPads();
#ifdef WII
								WPAD_ScanPads();
#endif
								VIDEO_WaitVSync();
								u32 btns = PAD_ButtonsDown(0);
#ifdef WII
								u32 btnsWii = WPAD_ButtonsDown(0);
#endif
#ifdef WII
								if(btns&PAD_BUTTON_START || btnsWii&WPAD_BUTTON_HOME)
#else
								if(btns&PAD_BUTTON_START)
#endif
									endproc();
#ifdef WII
								else if(btns&PAD_BUTTON_A || btnsWii&WPAD_BUTTON_A)
#else
								if(btns&PAD_BUTTON_A)
#endif
								{
									command = 1;
									break;
								}
								if (btns & PAD_BUTTON_DOWN) { cursor_position++; }
								if (btns & PAD_BUTTON_UP) { cursor_position--; }

#ifdef WII
								if (btnsWii & WPAD_BUTTON_DOWN) { cursor_position++; }
								if (btnsWii & WPAD_BUTTON_UP) { cursor_position--; }
#endif
								if (cursor_position < 0) { cursor_position = maxoptions; }
								if (cursor_position > maxoptions) { cursor_position = 0; }
								
								displayPrintTickets( cursor_position, game, language );
								
							}
	
							if(command == 0)
								continue;
							if(command == 1)
							{
								send(2);
								//let gba prepare
								sleep(1);
								
								//create base file with size
								printf("Getting savegame...\n");
								printf("Waiting for GBA...");
								VIDEO_WaitVSync();
								u32 readval = 0;
								while(readval != savesize)
									readval = __builtin_bswap32(recv());
								send(0); //got savesize
								printf("Receiving...");
								for(i = 0; i < savesize; i+=4)
									*(vu32*)(testdump+i) = recv();
								
								printf("done.\n");
								printf("Distributing event...");
								
								//EVENT DISTRO CODE
								
								int ret = 0;
								
								switch(language)
								{
									case 1:
										switch (game)
										{
											case 0:
												ret = ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_jap, game, language);
												break;
											case 1:
												switch(cursor_position)
												{
													case 0:
														ret = ret = ret = ret = ret = me_inject ((char*)testdump,  NULL, game, language);
														break;
													case 1:
														ret = wc_inject ((char*)testdump, mystic_ticket_E_jap, game, language);
														break;
													case 2:
														ret = wc_inject((char*)testdump, old_map_jap, game, language);
														break;
												}
												break;
											case 2:
												switch(cursor_position)
												{
													case 0:
														ret = wc_inject((char*)testdump, aurora_ticket_FRLG_jap, game, language);
													case 1:
														ret = wc_inject((char*)testdump, mystic_ticket_FRLG_jap, game, language);
														break;
												}
												break;
										}
										break;
									case 2:
										switch (game)
										{
											case 0:
												switch(cursor_position)
												{
													case 0:
														ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_card_eng, game, language);
														break;
													case 1:
														ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_ninti_eng, game, language);
														break;
												}
												break;
											case 1:
												switch(cursor_position)
												{
													case 0:
														ret = wc_inject((char*)testdump, aurora_ticket_E_eng, game, language);
													case 1:
														ret = wc_inject((char*)testdump, mystic_ticket_E_eng, game, language);
														break;
												}
												break;
											case 2:
												switch(cursor_position)
												{
													case 0:
														ret = wc_inject((char*)testdump, aurora_ticket_FRLG_eng, game, language);
													case 1:
														ret = wc_inject((char*)testdump, mystic_ticket_FRLG_eng, game, language);
														break;
												}
												break;
										}
										break;
									case 3:
										switch (game)
										{
											case 0:
												ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_ninti_fre, game, language);
												break;
											case 1:
												ret = wc_inject((char*)testdump, aurora_ticket_E_ninti_fre, game, language);
												break;
											case 2:
												ret = wc_inject((char*)testdump, aurora_ticket_FRLG_ninti_fre, game, language);
												break;
										}
										break;
									case 4:
										switch (game)
										{
											case 0:
												ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_ninti_ita, game, language);
												break;
											case 1:
												ret = wc_inject((char*)testdump, aurora_ticket_E_ninti_ita, game, language);
												break;
											case 2:
												ret = wc_inject((char*)testdump, aurora_ticket_FRLG_ninti_ita, game, language);
												break;
										}
										break;
									case 5:
										switch (game)
										{
											case 0:
												ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_ninti_ger, game, language);
												break;
											case 1:
												ret = wc_inject((char*)testdump, aurora_ticket_E_ninti_ger, game, language);
												break;
											case 2:
												ret = wc_inject((char*)testdump, aurora_ticket_FRLG_ninti_ger, game, language);
												break;
										}
										break;
									case 7:
										switch (game)
										{
											case 0:
												ret = ret = ret = ret = me_inject ((char*)testdump, eon_ticket_ninti_esp, game, language);
												break;
											case 1:
												ret = wc_inject((char*)testdump, aurora_ticket_E_ninti_esp, game, language);
												break;
											case 2:
												ret = wc_inject((char*)testdump, aurora_ticket_FRLG_ninti_esp, game, language);
												break;
										}
										break;
								}
								
								
								if(!ret)
								{
									printf("\nCanceled.");
									sleep(5);
									continue;
								}
								
								sleep(5);
								
								//Now send the save! (testdump holds the save)

								if(recv() == 0) //ready
									//printf("GBA ready...");
								VIDEO_WaitVSync();
								int gbasize = 0;
								while(gbasize == 0)
									gbasize = __builtin_bswap32(recv());
								send(0); //got gbasize
								u32 savesize = __builtin_bswap32(recv());
								send(0); //got savesize
								if(gbasize == -1) 
								{
									warnError("\nERROR: No (Valid) GBA Card inserted!\n");
									continue;
								}
								//get rom header
								for(i = 0; i < 0xC0; i+=4)
									*(vu32*)(testdump2+i) = recv();

								send(3);
								//let gba prepare
								sleep(1);
								
								printf("Waiting for GBA...");
								//printf("Sending save\n");
								VIDEO_WaitVSync();
								readval = 0;
								while(readval != savesize)
									readval = __builtin_bswap32(recv());
								for(i = 0; i < savesize; i+=4)
									send(__builtin_bswap32(*(vu32*)(testdump+i)));
								//printf("Waiting for GBA\n");
								while(recv() != 0)
									VIDEO_WaitVSync();
								printf("done!\n");
								printf("Event distribution complete.\n");
								send(0);
								sleep(5);
							}
						}
						else
						{
							printf("No Save File. Press START to exit.\n \n");
						}
					}
				}
			}
		}
	}
	return 0;
}
