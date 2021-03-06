/* Definities voor MCP23017 chip aansturing */

#define NUM_PINS 100
#define PINS_PER_MCP 15
#define PINS_PER_ROW 5
#define PIN_ROW(p) (((p) / PINS_PER_ROW) % 20)
#define NUM_ROWS ((NUM_PINS+PINS_PER_ROW-1)/PINS_PER_ROW)
#define NUM_MCPS ((NUM_PINS+PINS_PER_MCP-1)/PINS_PER_MCP)

#define PIN_DEBOUNCE 7
#define PIN_STATE (1 << PIN_DEBOUNCE)
/* 1000.. */
#define PIN_ON_OFF (1 << PIN_DEBOUNCE)
/* 0111.. */
#define PIN_OFF_ON (PIN_ON_OFF - 1)

#define MCP_BUTTONS 6
#define PIN_BUTTONS 12
#define NUM_BUTTONS 4

#define BUTTON_HOLDTIME ((FRAMERATE/SCANRATE)*1)
#define BUTTON_CLICKTIME 1
#define BUTTON_HOLD 0x100
#define BUTTON_ON 0x200
#define BUTTON_CHANGED 0x400
#define BUTTON_CLICKED 0x800
#define BUTTON_CLICKS 0xff

#define BUTTON_GREENSWITCH 0
#define BUTTON_REDSWITCH 1
#define BUTTON_BLUESWITCH 2
#define BUTTON_SL 3

typedef struct connection {
    unsigned char p[2];
} connection_t;

typedef struct button {
    int status;
} button_t;

typedef struct clist {
    int on, newon, off, event;
    button_t buttons[NUM_BUTTONS];
    connection_t pins[0];
} clist_t;

// extern unsigned char connections[NUM_PINS][NUM_PINS];
void init_mcps(void);
void fini_mcps(void);
clist_t *find_connections(void);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
