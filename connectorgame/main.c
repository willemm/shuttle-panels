/*
 * Shuttle spel: connector game
 *
 * Doet de hardware-aansturing van het shuttle paneel met de connectors
 *
 * Shuttlepaneel bevat: 4 led-ringen van 24 rgb-pizels
 *                      100 connectors verbonden aan 7 MCP23017 chips (GPIO)
 *                      Geluidskaartje met speakers
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include "mcp.h"
#include "leds.h"
#include "audio.h"
#include "main.h"

#include "connectors.h"

#define FRAMERATE 50
#define SLEEPTIME (1000000/FRAMERATE)

enum gamestates {
    GAME_BOOT,
    GAME_BOOTING,
    GAME_OK,
    GAME_OKING,
    GAME_BREAK,
    GAME_BREAKING,
    GAME_FIX,
    GAME_FIXING,
    GAME_FIXED,
    GAME_FIXEDING
};

static uint8_t running = 1;
static uint8_t debugging = 0;

void pdebug(const char *format, ...)
{
    va_list args;
    va_start (args, format);
    if (debugging) {
        fprintf(stdout, "DBG: ");
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    va_end(args);
}

static void ctrl_c_handler(int signum)
{
    (void)(signum);
    running = 0;
}

/* Ctrl-C afvangen */
static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Tijd ophalen in microseconden */
int64_t getutime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((((int64_t)tv.tv_sec) * 1000000) + (int64_t)tv.tv_usec);
}

void flash_spark(void)
{
    audio_play_file("spark.wav");
    led_set_flash(0, 5, 0, 2, 0xffffff, 3, 4, 0xccccff, 4, 3, 0xffcccc, 2, 10, 0x000000, 8, 3, 0x000000);
    led_set_flash(3, 5, 0, 3, 0xffccff, 3, 2, 0xffccff, 5, 2, 0xffffff, 2, 6, 0x000000, 12, 2, 0x000000);
    led_set_flash(1, 3, 0, 4, 0xff8888, 6, 2, 0xffffff, 6, 15, 0x000000);
    led_set_flash(2, 3, 0, 3, 0xff8888, 4, 4, 0xffffff, 5, 12, 0x000000);
}

int bootcount = 0;
int flashcount = 0;

int game_booting(clist_t *conns)
{
    if (--bootcount > 0) {
        return GAME_BOOTING;
    } else if (conns->on >= 10) {
        return GAME_OK;
    } else {
        return GAME_BREAK;
    }
}

int game_oking(clist_t *conns)
{
    if (conns->off > 0) {
        return GAME_BREAK;
    } else {
        return GAME_OKING;
    }
}

struct puzzle {
    int solution[NUM_ROWS];
} puzzle;

int randint(int from, int to)
{
    return from + ((random() % (to - from)));
}

void game_set_puzzle(clist_t *conns, int from, int to)
{
    pdebug("game_set_puzzle(%d/%d/%d, %d, %d)", conns->on, conns->newon, conns->off, from, to);
    for (int i = 0; i < NUM_ROWS; i++) {
        if (i < from || i >= to) {
            puzzle.solution[i] = -1;
        } else {
            puzzle.solution[i] = randint(i*5, (i+1)*5);
            pdebug("Suolution[%d] = %d", i, puzzle.solution[i]);
        }
    }
}

int game_fixing(clist_t *conns)
{
    if (conns->newon > 0) {
        audio_play_file("on.wav");
        for (int i = (conns->on - conns->newon); i < conns->on; i++) {
            pdebug("New connection: %d - %d", conns->pins[i].p1, conns->pins[i].p2);
        }
    } else if (conns->off > 0) {
        audio_play_file("off.wav");
    } else if (--flashcount <= 0) {
        flash_spark();
        flashcount = (int)(((double)(FRAMERATE/10 + (random() % (FRAMERATE * 4)))) * (1.0 + (((double)conns->on)/4)));
    }
    int colcnts[2] = {0,0};
    int poscnts[2] = {0,0};
    char seenpos[NUM_ROWS];
    for (int i = 0; i < NUM_ROWS; i++) {
        /* -1 is altijd goed */
        if (puzzle.solution[i] < 0) {
            seenpos[i] = 1;
            poscnts[(i >= 10)]++;
        } else {
            seenpos[i] = 0;
        }
    }
    /* Eerst kijken voor juiste posities */
    for (int i = 0; i < conns->on; i++) {
        int s[2] = { conns->pins[i].p1, conns->pins[i].p2 };
        for (int cc = 0; cc < 2; cc++) {
            int r = PIN_ROW(s[cc]);
            /* Kijken of de positie klopt */
            if (!seenpos[r]) {
                if (puzzle.solution[r] == s[cc]) {
                    seenpos[r] = 1;
                    poscnts[(r >= 10)]++;
                }
            }
        }
    }
    /* Dan kijken voor de resterende juiste kleuren */
    for (int i = 0; i < conns->on; i++) {
        int s[2] = { conns->pins[i].p1, conns->pins[i].p2 };
        for (int cc = 0; cc < 2; cc++) {
            int r = PIN_ROW(s[cc]);
            /* Kijken of de kleur klopt */
            int side = (r >= (NUM_ROWS/2)) ? (NUM_ROWS/2) : 0;
            for (int cr = side; cr < side + NUM_ROWS/2; cr++) {
                if (!seenpos[cr]) {
                    if (c_colors[puzzle.solution[cr]] == c_colors[s[cc]]) {
                        seenpos[cr] = 1;
                        colcnts[(r >= 10)]++;
                        break;
                    }
                }
            }
        }
    }
    ledshow_mastermind(0, colcnts[0], poscnts[0]);
    ledshow_mastermind(1, colcnts[1], poscnts[1]);
    if (poscnts[0] < 10 || poscnts[1] < 10) {
        return GAME_FIXING;
    } else {
        return GAME_OK;
    }
}

int main(int argc, char *argv[])
{
    // int cycles = 0;
    // int timers[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    
    if (argc > 1 && argv[1][0] == 'd') {
        debugging = 1;
    }

    /* Alles initen */
    setup_handlers();
    init_mcps();
    init_leds();
    init_audio();

    /* Opstarten */
    running = 1;
    int gamestate = GAME_BOOT;
    while (running) {
        int64_t timertime = getutime(); // Om de framerate gelijk te houden
        clist_t *conns = find_connections(); // Altijd uitlezen
        switch (gamestate) {
            default: /* Fallthrough to boot */
            case GAME_BOOT:
                pdebug("GAME_BOOT");
                audio_play_file("booting.wav");
                bootcount = FRAMERATE*2;
            case GAME_BOOTING:
                gamestate = game_booting(conns);
                break;
            case GAME_OK:
                pdebug("GAME_OK");
                /* Wat leuke animaties */
                led_set_blobs(0, FRAMERATE/2, 3, 0x003300, 0x002211, 0x000033);
                led_set_blobs(3, FRAMERATE/2, 4, 0x000033, 0x003300, 0x001133, 0x003311);

                led_set_blobs(1, FRAMERATE/2, 3, 0x002222, 0x002200, 0x000022);
                led_set_blobs(2, FRAMERATE/2, 3, 0x002222, 0x000022, 0x002200);
                audio_play_file("ready.wav");
            case GAME_OKING:
                gamestate = game_oking(conns);
                break;
            case GAME_BREAK:
                pdebug("GAME_BREAK");
                flashcount = 0;
                led_set_blobs(0, 0, 3, 0x330000, 0x221100, 0x000011);
                led_set_blobs(3, 0, 4, 0x330000, 0x003300, 0x331100, 0x113300);
            case GAME_BREAKING:
            case GAME_FIX:
                pdebug("GAME_FIX");
                game_set_puzzle(conns,0,10);
                /* Rodere animaties */
                led_remove_animation(1);
                led_remove_animation(2);
            case GAME_FIXING:
                gamestate = game_fixing(conns);
                break;
        }
        free(conns);
        leds_mainloop();
        audio_mainloop();
        int64_t sleeptime = SLEEPTIME - (getutime() - timertime);
        if (sleeptime > 0) usleep(sleeptime);
    }
    /* Profiling */
    /*
    printf("Timer times (%d cycles):\n", cycles);
    printf(" find_connections(): %3.6f\n", (double)timers[0]/cycles/CLOCKS_PER_SEC);
    printf(" count:              %3.6f\n", (double)timers[1]/cycles/CLOCKS_PER_SEC);
    printf(" ledset(0):          %3.6f\n", (double)timers[2]/cycles/CLOCKS_PER_SEC);
    printf(" ledset(1):          %3.6f\n", (double)timers[3]/cycles/CLOCKS_PER_SEC);
    printf(" audio_play_file():  %3.6f\n", (double)timers[4]/cycles/CLOCKS_PER_SEC);
    printf(" free(conns):        %3.6f\n", (double)timers[5]/cycles/CLOCKS_PER_SEC);
    printf(" audio_mainloop():   %3.6f\n", (double)timers[6]/cycles/CLOCKS_PER_SEC);
    printf(" leds_mainloop():    %3.6f\n", (double)timers[7]/cycles/CLOCKS_PER_SEC);
    printf(" usleep():           %3.6f\n", (double)timers[8]/cycles/CLOCKS_PER_SEC);
    printf(" TOTAL:              %3.6f\n", (double)timers[9]/cycles/CLOCKS_PER_SEC);
    */
    fini_leds();
    fini_mcps();
    fini_audio();
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */