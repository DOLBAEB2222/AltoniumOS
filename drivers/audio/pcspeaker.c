#include "../../include/drivers/pcspeaker.h"
#include "../../include/drivers/console.h"
#include "../../include/drivers/keyboard.h"
#include "../../include/lib/string.h"

#define NULL ((void*)0)

#define PIT_CHANNEL2_DATA     0x42
#define PIT_COMMAND_REGISTER  0x43
#define SPEAKER_CONTROL       0x61

#define PIT_BASE_FREQUENCY    1193182
#define SPEAKER_ENABLE_BIT    0x03

static const struct {
    const char *note;
    uint16_t frequency;
} note_lut[] = {
    {"C3", 131}, {"C#3", 139}, {"D3", 147}, {"D#3", 156},
    {"E3", 165}, {"F3", 175}, {"F#3", 185}, {"G3", 196},
    {"G#3", 208}, {"A3", 220}, {"A#3", 233}, {"B3", 247},

    {"C4", 262}, {"C#4", 277}, {"D4", 294}, {"D#4", 311},
    {"E4", 330}, {"F4", 349}, {"F#4", 370}, {"G4", 392},
    {"G#4", 415}, {"A4", 440}, {"A#4", 466}, {"B4", 494},

    {"C5", 523}, {"C#5", 554}, {"D5", 587}, {"D#5", 622},
    {"E5", 659}, {"F5", 698}, {"F#5", 740}, {"G5", 784},
    {"G#5", 831}, {"A5", 880}, {"A#5", 932}, {"B5", 988},

    {"C6", 1047}, {"C#6", 1109}, {"D6", 1175}, {"D#6", 1245},
    {"E6", 1319}, {"F6", 1397}, {"F#6", 1480}, {"G6", 1568},
    {"G#6", 1661}, {"A6", 1760}, {"A#6", 1865}, {"B6", 1976},

    {"C7", 2093}, {"C#7", 2217}, {"D7", 2349}, {"D#7", 2489},
    {"E7", 2637}, {"F7", 2794}, {"F#7", 2960}, {"G7", 3136},
    {"G#7", 3322}, {"A7", 3520}, {"A#7", 3729}, {"B7", 3951},

    {NULL, 0}
};

static void pit_set_frequency(uint16_t frequency) {
    uint16_t divisor = PIT_BASE_FREQUENCY / frequency;
    
    outb(PIT_COMMAND_REGISTER, 0xB6);
    
    outb(PIT_CHANNEL2_DATA, divisor & 0xFF);
    outb(PIT_CHANNEL2_DATA, (divisor >> 8) & 0xFF);
}

static void speaker_enable(void) {
    uint8_t speaker = inb(SPEAKER_CONTROL);
    speaker |= SPEAKER_ENABLE_BIT;
    outb(SPEAKER_CONTROL, speaker);
}

static void speaker_disable(void) {
    uint8_t speaker = inb(SPEAKER_CONTROL);
    speaker &= ~SPEAKER_ENABLE_BIT;
    outb(SPEAKER_CONTROL, speaker);
}

static void sleep_ms(int ms) {
    for (volatile int i = 0; i < ms * 5000; i++) {
        __asm__ volatile("pause" ::: "memory");
    }
}

void pcspeaker_init(void) {
    speaker_disable();
}

void pcspeaker_beep(uint16_t frequency, uint16_t duration_ms) {
    if (frequency == 0 || duration_ms == 0) {
        speaker_disable();
        sleep_ms(duration_ms);
        return;
    }

    pit_set_frequency(frequency);
    speaker_enable();
    sleep_ms(duration_ms);
    speaker_disable();
}

void pcspeaker_play_melody(note_event_t *notes, int note_count) {
    for (int i = 0; i < note_count; i++) {
        pcspeaker_beep(notes[i].frequency, notes[i].duration_ms);
        sleep_ms(10);
    }
    pcspeaker_beep(0, 50);
}

void pcspeaker_note_to_frequency(const char *note_str, uint16_t *frequency_out) {
    *frequency_out = 0;
    
    if (!note_str || !*note_str) {
        return;
    }

    for (int i = 0; note_lut[i].note != NULL; i++) {
        if (strcmp_impl(note_str, note_lut[i].note) == 0) {
            *frequency_out = note_lut[i].frequency;
            return;
        }
    }
}

void pcspeaker_piano_mode(void) {
    console_print("Piano mode started. Press letter keys to play notes:\n");
    console_print("  A-W-S-E-D-F-T-G-Y-H-U-J-K-O-L-P  (like black/white keys)\n");
    console_print("  Press ESC to exit\n");
    
    int done = 0;
    while (!done) {
        if (keyboard_has_data()) {
            uint8_t scancode = keyboard_get_scancode();
            uint8_t key = keyboard_convert_scancode(scancode);
            
            if (key == KEY_ESC) {
                console_print("Exiting piano mode.\n");
                speaker_disable();
                done = 1;
            } else if (key != 0) {
                uint16_t frequency = 0;
                
                switch (key) {
                    case 'a': frequency = 262; break; 
                    case 'w': frequency = 277; break; 
                    case 's': frequency = 294; break; 
                    case 'e': frequency = 311; break; 
                    case 'd': frequency = 330; break; 
                    case 'f': frequency = 349; break; 
                    case 't': frequency = 370; break; 
                    case 'g': frequency = 392; break; 
                    case 'y': frequency = 415; break; 
                    case 'h': frequency = 440; break; 
                    case 'u': frequency = 466; break; 
                    case 'j': frequency = 523; break; 
                    case 'k': frequency = 587; break; 
                    case 'o': frequency = 622; break; 
                    case 'l': frequency = 659; break; 
                    case 'p': frequency = 698; break; 
                    default:  frequency = 0;  break;
                }
                
                if (frequency > 0) {
                    pcspeaker_beep(frequency, 150);
                }
            }
        }
        sleep_ms(10);
    }
}