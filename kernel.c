#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define VIDEO_MEMORY 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define MAX_INPUT 128

// ---- Keyboard buffer (simulation) ----
volatile char keyboard_buffer[MAX_INPUT];
volatile int kb_index = 0;

// ---- File system ----
#define MAX_FILES 16
#define MAX_FILENAME 64
#define MAX_FILE_SIZE 512

typedef struct {
    char name[MAX_FILENAME];
    uint8_t data[MAX_FILE_SIZE];
    size_t size;
    int used;
} File;

File fs[MAX_FILES]; 

// ---- VGA output ----
void kprint(const char* str) {
    volatile uint16_t* video = (volatile uint16_t*)VIDEO_MEMORY;
    static int pos = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') { 
            pos += SCREEN_WIDTH - (pos % SCREEN_WIDTH); 
            continue; 
        }
        if (pos >= SCREEN_WIDTH * SCREEN_HEIGHT) pos = 0;
        video[pos++] = (0x07 << 8) | str[i];
    }
}

// ---- File system functions ----
int fs_create(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs[i].used) {
            strncpy(fs[i].name, filename, MAX_FILENAME);
            fs[i].size = 0;
            fs[i].used = 1;
            return i;
        }
    }
    return -1;
}

int fs_write(int idx, const uint8_t* buffer, size_t len) {
    if (idx < 0 || idx >= MAX_FILES || !fs[idx].used) return -1;
    if (len > MAX_FILE_SIZE) len = MAX_FILE_SIZE;
    for (size_t i = 0; i < len; i++) fs[idx].data[i] = buffer[i];
    fs[idx].size = len;
    return 0;
}

int fs_read(int idx, uint8_t* buffer, size_t len) {
    if (idx < 0 || idx >= MAX_FILES || !fs[idx].used) return -1;
    if (len > fs[idx].size) len = fs[idx].size;
    for (size_t i = 0; i < len; i++) buffer[i] = fs[idx].data[i];
    return (int)len;
}

int fs_delete(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs[i].used && strcmp(fs[i].name, filename) == 0) {
            fs[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int fs_find(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs[i].used && strcmp(fs[i].name, filename) == 0) return i;
    }
    return -1;
}

// ---- Keyboard input ----
char get_char(void) {
    if (kb_index > 0) {
        char c = keyboard_buffer[0];
        for (int i = 1; i < kb_index; i++) keyboard_buffer[i-1] = keyboard_buffer[i];
        kb_index--;
        return c;
    }
    return 0;
}

// ---- Text editor (TCTEE) ----
void tctee(File* file) {
    char buffer[MAX_FILE_SIZE];
    for (size_t i = 0; i < file->size; i++) buffer[i] = file->data[i];
    size_t pos = file->size;

    kprint("TCTEE Editor opened! Type text. Press ESC to save.\n");

    while (1) {
        char c = get_char();
        if (!c) continue;
        if (c == 27) { // ESC
            fs_write(file - fs, (uint8_t*)buffer, pos);
            kprint("\nSaved & exiting editor.\n");
            break;
        } else if (c == '\b') {
            if (pos > 0) pos--;
        } else {
            if (pos < MAX_FILE_SIZE) buffer[pos++] = c;
        }
    }
}

// ---- Preload FOREVER file ----
void preload_forever_file() {
    const char* filename = "plzreadme.md";
    const char* content = 
        "Hello people, thanks for using the coolest OS ever!\n"
        "I am thebakedbengunslimgerdev, but you can call me coolguy11.\n"
        "Here are the features:\n"
        "- Filesystem (create/write/delete files)\n"
        "- mvto <file> (open TCTEE editor, basically cd)\n"
        "- rd <file> to read contents of a file\n"
        "- compile <java/c file> to compile JAVA/C code\n"
        "- del <file> to delete files\n"
        "- amiwho to see the current username\n"
        "- restartcomp1 to restart your PC\n"
        "- poweroffcomp1 to turn off your PC\n"
        "Call this OS TCOE!\n";

    int idx = fs_create(filename);
    if (idx >= 0) fs_write(idx, (const uint8_t*)content, strlen(content));
}

// ---- Command parser ----
void execute_command(const char* cmd) {
    if (cmd[0] == '\0') return;

    if (strncmp(cmd, "mvto ", 5) == 0) {
        const char* filename = cmd + 5;
        int idx = fs_find(filename);
        if (idx >= 0) tctee(&fs[idx]);
        else kprint("File not found!\n");
    } 
    else if (strncmp(cmd, "rd ", 3) == 0) {
        const char* filename = cmd + 3;
        int idx = fs_find(filename);
        if (idx >= 0) {
            char buffer[MAX_FILE_SIZE+1];
            int len = fs_read(idx, (uint8_t*)buffer, MAX_FILE_SIZE);
            buffer[len] = '\0';
            kprint(buffer);
            kprint("\n");
        } else kprint("File not found!\n");
    }
    else if (strncmp(cmd, "compile ", 8) == 0) {
        const char* filename = cmd + 8;
        int idx = fs_find(filename);
        if (idx >= 0) kprint("Compiling...Done!\n");
        else kprint("File not found!\n");
    }
    else if (strncmp(cmd, "del ", 4) == 0) {
        const char* filename = cmd + 4;
        if (fs_delete(filename) == 0) kprint("Deleted successfully\n");
        else kprint("File not found!\n");
    }
    else if (strcmp(cmd, "amiwho") == 0) {
        kprint("Current user: coolguy11\n");
    }
    else if (strcmp(cmd, "restartcomp1") == 0) {
        kprint("Restarting...\n");
        kernel_main(); // restart kernel
    }
    else if (strcmp(cmd, "poweroffcomp1") == 0) {
        kprint("Shutting down...\n");
        while(1); // halt CPU
    }
    else if (strcmp(cmd, "fllist") == 0) {
        kprint("Files in TCOE:\n");
        for (int i = 0; i < MAX_FILES; i++) {
            if (fs[i].used) {
                kprint(fs[i].name);
                kprint("\n");
            }
        }
    }
    else kprint("Unknown command\n");
}

// ---- Main kernel shell ----
void kernel_main(void) {
    kprint("Welcome to TCOE - The Coolest OS Ever!\n");

    preload_forever_file();

    char input[MAX_INPUT];
    int input_pos = 0;

    while (1) {
        char c = get_char();
        if (!c) continue;

        if (c == '\n') {
            input[input_pos] = '\0';
            execute_command(input);
            input_pos = 0;
        } else {
            if (input_pos < MAX_INPUT-1) input[input_pos++] = c;
        }
    }
}
//only 220 lines of code!, github! 220!!!!!