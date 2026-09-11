#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <audsrv.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <sys/stat.h>

#define MAX_GAMES 10

typedef struct {
    char title[64];
    char game_dir[128];
    char save_dir[128];
} GameEntry;

GameEntry games[MAX_GAMES];
int total_games = 0;
int selected_game = 0;

static char padBuf[256] __attribute__((aligned(64)));
static u32 old_pad = 0;

void load_irx_modules() {
    SifInitRpc(0);

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:MCMAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    SifLoadModule("mass:/RPG2K/modules/usbd.irx", 0, NULL);
    SifLoadModule("mass:/RPG2K/modules/usbhdfsd.irx", 0, NULL);

    if (SifLoadModule("mass:/RPG2K/modules/audsrv.irx", 0, NULL) >= 0) {
        audsrv_init();
        audsrv_fmt_t format;
        format.bits = 16;
        format.freq = 44100;
        format.channels = 2;
        audsrv_set_format(&format);
    }
}

void init_pad() {
    padInit(0);
    padPortOpen(0, 0, padBuf);
}

u32 read_pad() {
    struct padButtonStatus buttons;
    u32 pact;
    u32 new_pad;

    int state = padGetState(0, 0);
    if (state != PAD_STATE_DISCONN && state != PAD_STATE_EXECCMD) {
        pact = padRead(0, 0, &buttons);
        if (pact != 0) {
            new_pad = 0xffff ^ buttons.btns;
            u32 pressed = new_pad & ~old_pad;
            old_pad = new_pad;
            return pressed;
        }
    }
    return 0;
}

void setup_game_list() {
    strncpy(games[0].title, "1. Ib (RPG Maker 2000)", 64);
    strncpy(games[0].game_dir, "mass:/RPG2K/Ib/", 128);
    strncpy(games[0].save_dir, "mc0:RPG2K_IB/", 128);

    strncpy(games[1].title, "2. Castelo Mogeko (RPG Maker 2000/2003)", 64);
    strncpy(games[1].game_dir, "mass:/RPG2K/MogekoCastle/", 128);
    strncpy(games[1].save_dir, "mc0:RPG2K_MOGEKO/", 128);

    total_games = 2;
}

void launch_game(int index) {
    mkdir(games[index].save_dir, 0777);
    printf("Iniciando %s...\n", games[index].title);
}

int main(int argc, char *argv[]) {
    load_irx_modules();
    init_pad();
    setup_game_list();

    GSGLOBAL *gsGlobal = gsKit_init_global();
    gsKit_init_screen(gsGlobal);

    // Inicialização do dmaKit atualizada para a nova versão da PS2SDK
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    u64 color_bg = GS_SETREG_RGBA(20, 24, 33, 0);
    u64 color_selected = GS_SETREG_RGBA(255, 200, 0, 0);
    u64 color_item = GS_SETREG_RGBA(80, 90, 110, 0);

    int running = 1;
    while (running) {
        u32 btn = read_pad();

        if (btn & PAD_DOWN) {
            selected_game = (selected_game + 1) % total_games;
        } else if (btn & PAD_UP) {
            selected_game = (selected_game - 1 + total_games) % total_games;
        } else if (btn & PAD_CROSS) {
            launch_game(selected_game);
        }

        gsKit_clear(gsGlobal, color_bg);

        // Renderização dos elementos usando a função correta (gsKit_prim_sprite_flat)
        for (int i = 0; i < total_games; i++) {
            u64 draw_color = (i == selected_game) ? color_selected : color_item;
            gsKit_prim_sprite_flat(gsGlobal, 80.0f, (float)(140 + (i * 45)), 560.0f, (float)(175 + (i * 45)), 1, draw_color);
        }

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
    }

    return 0;
}
