#include <psx.h>
#include <psxgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/pelota.h"

GsImage moneda_image;
unsigned int prim_list[0x4000];
unsigned char data_buffer[0x40000]; // 256 kilobytes

/* here we will save gamepad state */
unsigned short padbuf;

volatile int display_is_old = 1;
volatile int time_counter = 0;
int  dbuf=0;

int hx=0, hy=0;
int sample_pos[3];

typedef struct pelota {
    int vspeed;
    int hspeed;
    int image_index;
    int animation_time;
    GsSprite sprite;
    struct pelota *sig;
} pelota;

pelota *pelotaFirst;
pelota *pelotaLast;

void prog_vblank_handler();
int load_file_into_buffer(char *);
void leer_imagen(const char *, GsImage *);
pelota *newPelota();
void pelotaAdd(int, int);


int main() {
    PSX_Init();
    GsInit();
    GsSetList(prim_list);
    GsClearMem();
    GsSetVideoMode(320, 240, VMODE_NTSC);

    // Initialize sound
    SsInit();

    pelota *p, *p2;
    GsSprite fondo;
    GsSprite cursor;
    srand(time_counter);
    int c;
    int pressed_cross = 0;
    //char printf_time[45];

    
    leer_imagen("cdrom:TEXTURA1.TIM;1", &moneda_image);

    GsLoadFont(768, 0, 768, 256);
    while(GsIsDrawing());

    
    p = pelotaFirst;
    while(p != NULL) {
        p->sprite.tpage = gs_get_tpage_num(320, 0);
        p->sprite.w = 14;
        p->sprite.h = 17;
        p->sprite.attribute = COLORMODE(COLORMODE_16BPP);
        p->sprite.r = p->sprite.g = p->sprite.b = NORMAL_LUMINOSITY;
        p->sprite.attribute ^= (ENABLE_TRANS | TRANS_MODE(1));
        p = p->sig;
    }

    fondo.y = 0;
    fondo.h = 240;
    fondo.u = 0;
    fondo.v = 17;
    fondo.attribute = COLORMODE(COLORMODE_16BPP);
    fondo.r = fondo.g = fondo.b = NORMAL_LUMINOSITY;

	cursor.tpage = gs_get_tpage_num(320, 0);
	cursor.u = gs_get_tpage_u(56);
        cursor.w = 14;
        cursor.h = 17;
        cursor.attribute = COLORMODE(COLORMODE_16BPP);
        cursor.r = cursor.g = cursor.b = NORMAL_LUMINOSITY;
        cursor.attribute ^= (ENABLE_TRANS | TRANS_MODE(1));

    sample_pos[0] = SPU_DATA_BASE_ADDR;
    c = load_file_into_buffer("cdrom:COIN.RAW;1");
    SsUpload(data_buffer, c, sample_pos[0]);

    SsVoiceStartAddr(0, sample_pos[0]);
    SsVoiceVol(0, 0x3fff, 0x3fff);
    SsVoicePitch(0, 0x1000);

    SetVBlankHandler(prog_vblank_handler);

    // Event Creation
    int random;
    p = pelotaFirst;
    while(p != NULL) {
        p->sprite.x = rand() % (320-14);
        p->sprite.y = rand() % (240-17);
        p->image_index = 0;
        p->animation_time = 0;
        random = rand() % 2;
        switch(random) {
            case 0: p->hspeed = -2; break;
            case 1: p->hspeed = 2; break;
        }
        random = rand() % 2;
        switch(random) {
            case 0: p->vspeed = -2; break;
            case 1: p->vspeed = 2; break;
        }
        p = p->sig;
    }

    
    while(1) {
        if(display_is_old)  {
            dbuf=!dbuf;
            GsSetDispEnvSimple(0, dbuf ? 0 : 256);
            GsSetDrawEnvSimple(0, dbuf ? 256 : 0, 320, 240);

            GsSortCls(0,0,0);

            // get the gamepad state 
            PSX_ReadPad(&padbuf, NULL);


            // change coords according to which gamepad button was pressed 
            if(padbuf & PAD_LEFT)  if (hx - 1 > 0) hx--;
            if(padbuf & PAD_RIGHT) if (hx + 1 < 320) hx++;
            if(padbuf & PAD_UP)    if (hy - 1 > 0) hy--;
            if(padbuf & PAD_DOWN)  if (hy + 1 < 240) hy++;

            if (padbuf & PAD_CROSS) {
                if (pressed_cross) {
                    pressed_cross = 0;
                    srand(time_counter);

                    pelotaAdd(hx - 8, hy - 8);
                    pelotaLast->sprite.tpage = gs_get_tpage_num(320, 0);
                    pelotaLast->sprite.w = 14;
                    pelotaLast->sprite.h = 17;
                    pelotaLast->sprite.attribute = COLORMODE(COLORMODE_16BPP);
                    pelotaLast->sprite.r = pelotaLast->sprite.g = pelotaLast->sprite.b = NORMAL_LUMINOSITY;
                    pelotaLast->sprite.attribute ^= (ENABLE_TRANS | TRANS_MODE(1));
                    pelotaLast->image_index = 0;
                    pelotaLast->animation_time = 0;
                    random = rand() % 2;
                    switch(random) {
                        case 0: pelotaLast->hspeed = -2; break;
                        case 1: pelotaLast->hspeed = 2; break;
                    }
                    random = rand() % 2;
                    switch(random) {
                        case 0: pelotaLast->vspeed = -2; break;
                        case 1: pelotaLast->vspeed = 2; break;
                    }
                }
            }
            else pressed_cross = 1;
            
            // Event Step
            p = pelotaFirst;
            while(p != NULL) {
                p->animation_time++;
                if (p->animation_time == 6) {
                    p->animation_time = 0;
                    p->image_index++;
                    if (p->image_index == 4) {
                        p->image_index = 0;
                        p->sprite.u = 0;
                    }
                    else {
                        p->sprite.u = p->sprite.u + p->sprite.w;
                    }
                }

                if (p->sprite.x + p->sprite.w > 320) {
                    p->sprite.x = 320 - p->sprite.w ;
                    p->hspeed = -p->hspeed;
                }
                else if (p->sprite.x < 0) {
                    p->sprite.x = 0;
                    p->hspeed = -p->hspeed;
                }
                if (p->sprite.y + p->sprite.h > 240) {
                    p->sprite.y = 240 - p->sprite.h;
                    p->vspeed = -p->vspeed;
                }
                else if (p->sprite.y < 0) {
                    p->sprite.y = 0;
                    p->vspeed = -p->vspeed;
                }

		p2 = p->sig;
		while(p2 != NULL) {
			if (
			p->sprite.x + p->hspeed < 
			p2->sprite.x + p2->hspeed +p2->sprite.w &&
			p->sprite.x + p->hspeed + p->sprite.w > 
			p2->sprite.x +p2->hspeed  &&
			p->sprite.y < p2->sprite.y + p2->sprite.h &&
			p->sprite.y + p->sprite.h > p2->sprite.y) {
				SsKeyOn(0);
				p->sprite.x = p->sprite.x + p->hspeed;
				p2->sprite.x = p2->sprite.x + p2->hspeed;
				p->hspeed = -p->hspeed;
				p2->hspeed = -p2->hspeed;
			}
			else if (p->sprite.x < p2->sprite.x + p2->sprite.w &&
			p->sprite.x + p->sprite.w > p2->sprite.x &&
			p->sprite.y + p->vspeed < 
			p2->sprite.y +p2->vspeed + p2->sprite.h &&
			p->sprite.y + p->vspeed + p->sprite.h > 
			p2->sprite.y + p2->vspeed) {
				SsKeyOn(0);
				p->sprite.y = p->sprite.y + p->vspeed;
				p2->sprite.y = p2->sprite.y + p2->vspeed;
				p->vspeed = -p->vspeed;
				p2->vspeed = -p2->vspeed;
			}
			p2 = p2->sig;
		}

                p->sprite.x = p->sprite.x + p->hspeed;
                p->sprite.y = p->sprite.y + p->vspeed;
                p = p->sig;
            }

            // Event Draw
            // Dibujar fondo
            fondo.tpage = gs_get_tpage_num(320, 17);
            fondo.x = 0;
            fondo.w = 256;

            GsSortSimpleSprite(&fondo);

            fondo.tpage = gs_get_tpage_num(576, 17);
            fondo.x = suma(fondo.x, 256);
            fondo.w = 64;

            GsSortSimpleSprite(&fondo);
            
            // Dibujar pelotas
            p = pelotaFirst;
            while(p != NULL) {
                GsSortSimpleSprite(&p->sprite);
                p = p->sig;
            }

            /* display text with changes coords */
		cursor.x = hx;
		cursor.y = hy;
		GsSortSimpleSprite(&cursor);
            //GsPrintFont(hx, hy, ">");

            GsDrawList();
            while(GsIsDrawing());

            display_is_old=0;
        }
    }

    return 0;
}

void prog_vblank_handler() {
    display_is_old = 1;
    time_counter++;
}

void leer_imagen(const char *nombre, GsImage *image) {
    FILE *archivo;
    int size;

    
    archivo = fopen(nombre, "rb");
    fseek(archivo, 0, SEEK_END);
    size = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);
    printf("%s (%d)\n", nombre, size);
    fread(data_buffer, sizeof(char), size, archivo);
    fclose(archivo);
    GsImageFromTim(image, data_buffer);

    GsUploadImage(image);
    while(GsIsDrawing());
}

int load_file_into_buffer(char *fname)
{
	FILE *f;
	int sz;
	f = fopen(fname, "rb");
	
	fseek(f, 0, SEEK_END);	

	sz = ftell(f);

	fseek(f, 0, SEEK_SET);

	printf("%s (%d)\n", fname, sz);
	
	fread(data_buffer, sizeof(char), sz, f);
	
	fclose(f);
	
	return sz;
}

pelota *newPelota() {
    pelota *p;
    p = (pelota *) malloc(sizeof(pelota));
    if (p == NULL) {
        printf("ERROR: No se puede asignar mas espacio de memoria.\n");
        exit(0);
    }
    return p;
}

void pelotaAdd(int x, int y) {
    pelota *p = newPelota();
    p->sprite.x = x;
    p->sprite.y = y;
    p->sig = NULL;
    if (pelotaFirst == NULL) {
        pelotaFirst = p;
        pelotaLast = p;
    }
    else {
        pelotaLast->sig = p;
        pelotaLast = p;
    }
}
