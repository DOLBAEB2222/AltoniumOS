void _start(void) {
    volatile unsigned short *video = (unsigned short *)0xB8000;
    const char *msg = "AltoniumOS 64-bit kernel - Not yet implemented";
    int i = 0;
    
    while (msg[i] != '\0') {
        video[i] = (unsigned short)((0x0C << 8) | msg[i]);
        i++;
    }
    
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
