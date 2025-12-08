#ifndef PCSPEAKER_H
#define PCSPEAKER_H

typedef struct {
    char note[3];
    unsigned short frequency;
    unsigned short duration_ms;
} note_event_t;

void pcspeaker_init(void);
void pcspeaker_beep(unsigned short frequency, unsigned short duration_ms);
void pcspeaker_play_melody(note_event_t *notes, int note_count);
void pcspeaker_note_to_frequency(const char *note_str, unsigned short *frequency_out);

void pcspeaker_piano_mode(void);

#endif