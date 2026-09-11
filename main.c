/*
 * EasyRPG Player PS2 Launcher & Emulator Frontend
 * Categoria: Homebrew PS2 (RPG Maker 2000 / 2003 Engine)
 */

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

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

// Estrutura para entrada de jogo
typedef struct {
    char title[64];
    char game_dir[128];
    char save_dir[128];
    char icon_path[64];
} GameEntry;

#define MAX_GAMES 10
GameEntry games[MAX_GAMES];
int total_games = 0;
int current_selection = 0;

// Estado dos botões do DualShock 2
static char padBuf[256] __attribute__((aligned(64)));
static u32 old_pad = 0;

// Função para carregar módulos do sistema PS2 (I/O, USB, Áudio, Pad)
void load_ps2_modules(void) {
    SifInitRpc(0);

    // Módulos nativos ROM do PS2
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:MCMAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    // Módulos USB / Mass Storage
    SifLoadModule("host:usbd.irx", 0, NULL);
    SifLoadModule("host:usbhdfsd.irx", 0, NULL);

    // Módulo de Áudio (audsrv RPC)
    SifLoadModule("host:audsrv.irx", 0, NULL);
}

// Inicializa o sistema de áudio PCM / SPU2
int init_sound_system(void) {
    if (audsrv_init() != 0) {
        printf("Erro ao inicializar audsrv: %s\n", audsrv_get_error_string());
        return -1;
    }

    audsrv_fmt_t format;
    format.bits = 16;
    format.freq = 44100;
    format.channels = 2;

    if (audsrv_set_format(&format) != 0) {
        printf("Erro ao configurar formato de audio PCM\n");
        return -1;
    }

    audsrv_set_volume(MAX_VOLUME);
    printf("Audio SPU2 inicializado com sucesso (44.1kHz Stereo 16-bit)\n");
    return 0;
}

// Inicializa o controle DualShock 2 (Slot 0, Port 0)
void init_pad_system(void) {
    padInit(0);
    padPortOpen(0, 0, padBuf);
}

u32 read_pad_buttons(void) {
    struct padButtonStatus buttons;
    u32 new_pad = 0;

    int state = padGetState(0, 0);
    if (state == PAD_STATE_DISCONN) return 0;

    if (padRead(0, 0, &buttons) != 0) {
        u32 paddata = 0xffff ^ buttons.btns;
        new_pad = paddata & ~old_pad; // Borda de subida (apenas clique único)
        old_pad = paddata;
    }

    return new_pad;
}

// Garante que o diretório de save existe no Memory Card (mc0:)
void setup_save_directory(const char* save_path) {
    mkdir(save_path, 0777);
    printf("Diretorio de Save verificado/criado: %s\n", save_path);
}

// Renderiza a interface gráfica do Menu usando gsKit
void draw_ui_menu(GSGLOBAL *gsGlobal) {
    u64 color_bg       = GS_SETREG_RGBA(18, 22, 32, 0);
    u64 color_header   = GS_SETREG_RGBA(40, 50, 75, 0);
    u64 color_item     = GS_SETREG_RGBA(30, 38, 54, 0);
    u64 color_selected = GS_SETREG_RGBA(230, 160, 20, 0);

    gsKit_clear(gsGlobal, color_bg);

    // Painel Superior (Header)
    gsKit_prim_rect_flat(gsGlobal, 10, 10, SCREEN_WIDTH - 10, 60, 1, color_header);

    // Desenha Lista de Jogos
    int start_y = 90;
    int box_height = 45;
    int spacing = 12;

    for (int i = 0; i < total_games; i++) {
        int y1 = start_y + i * (box_height + spacing);
        int y2 = y1 + box_height;

        u64 current_color = (i == current_selection) ? color_selected : color_item;
        gsKit_prim_rect_flat(gsGlobal, 20, y1, SCREEN_WIDTH - 20, y2, 2, current_color);
    }

    gsKit_queue_exec(gsGlobal);
    gsKit_sync_flip(gsGlobal);
}

// Inicia a Engine EasyRPG Player com o jogo selecionado
void execute_easyrpg_game(GameEntry *game) {
    printf("========================================\n");
    printf("Iniciando RPG Maker Game: %s\n", game->title);
    printf("Pasta do Jogo: %s\n", game->game_dir);
    printf("Pasta de Saves: %s\n", game->save_dir);
    printf("========================================\n");

    // 1. Criar diretório de save no MC
    setup_save_directory(game->save_dir);

    // 2. Aqui a engine EasyRPG assume a execução:
    // EasyRPG_Main(game->game_dir, game->save_dir, "mass:/RPG2K/timidity.sf2");
}

int main(int argc, char *argv[]) {
    load_ps2_modules();

    // Inicializa subsistema gráfico via gsKit
    GSGLOBAL *gsGlobal = gsKit_init_global();
    gsKit_init_screen(gsGlobal);
    dmaKit_init();
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    // Inicializa Áudio e Controles
    init_sound_system();
    init_pad_system();

    // Popula Lista de Jogos RPG Maker 2000/2003 suportados
    strncpy(games[0].title, "Ib (RPG Maker 2000)", 64);
    strncpy(games[0].game_dir, "mass:/RPG2K/Ib/", 128);
    strncpy(games[0].save_dir, "mc0:RPG2K_IB/", 128);

    strncpy(games[1].title, "Castelo Mogeko (RPG Maker 2003)", 64);
    strncpy(games[1].game_dir, "mass:/RPG2K/MogekoCastle/", 128);
    strncpy(games[1].save_dir, "mc0:RPG2K_MOGEKO/", 128);

    total_games = 2;

    int running = 1;
    while (running) {
        draw_ui_menu(gsGlobal);

        u32 pad = read_pad_buttons();

        if (pad & PAD_DOWN) {
            current_selection = (current_selection + 1) % total_games;
        }
        if (pad & PAD_UP) {
            current_selection = (current_selection - 1 + total_games) % total_games;
        }
        if (pad & PAD_CROSS) {
            execute_easyrpg_game(&games[current_selection]);
        }
    }

    return 0;
}
